/*
 * drivers/input/touchscreen/t1310.c - Touch driver
 *
 * Copyright (C) 2010 LGE, Inc.
 * Author: Cho, EunYoung [ey.cho@lge.com]
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <asm/uaccess.h>
#include <linux/module.h>
#include <linux/input.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <asm/io.h>
#include <linux/delay.h>
#include <asm/gpio.h>
#include <mach/vreg.h>
#include <linux/wakelock.h>
#include <mach/msm_i2ckbd.h>
#include <linux/spinlock.h>
#include <linux/slab.h>

#include <linux/jiffies.h>

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>

static struct early_suspend ts_early_suspend;
static void touch_early_suspend(struct early_suspend *h);
static void touch_late_resume(struct early_suspend *h);
#endif

extern int FirmwareUpgrade_main(int version);
extern int touch_power(int enable);
static int touch_device_initialise(void);


// START:jaekyung83.lee@lge.com 2011.07.07
// ESD Detection Feature Add
#define ESD_DETECTION
// END:jaekyung83.lee@lge.com 2011.07.07

// START:jaekyung83.lee@lge.com 2011.07.07
// ESD TOUCH Reset Function Add
#ifdef ESD_DETECTION
static int esd_touch_power(int enable);
static void esd_touch_reset();
#endif
// END:jaekyung83.lee@lge.com 2011.07.07

//#define SINGLE_TOUCH (0)
//#define TOUCHEY_DEBUG_PRINT	(1)
#define TOUCHEY_ERROR_PRINT	(1)

#if defined(TOUCHEY_DEBUG_PRINT)
#define TOUCHD(fmt, args...) \
			printk(KERN_INFO "D[%-18s:%5d]" \
				fmt, __FUNCTION__, __LINE__, ##args);
#else
#define TOUCHD(fmt, args...)	{};
#endif

#if defined(TOUCHEY_ERROR_PRINT)
#define TOUCHE(fmt, args...) \
			printk(KERN_ERR "E[%-18s:%5d]" \
				fmt, __FUNCTION__, __LINE__, ##args);
#else
#define TOUCHE(fmt, args...)	{};
#endif

#if 0
#define BTN1	KEY_5
#else
#define BTN1	KEY_MENU
#endif 
#define BTN2	KEY_HOME
#define BTN3	KEY_BACK
#define BTN4	KEY_SEARCH
#define MAX_BTN 4

#define PRESS 	1
#define RELEASE 0
#define MAX_FINGER 5
//#define DEAD_ZONE 5

#define NOT_BOOTING 	0
#define WORKING 		1
#define SUSPEND			2
#define FRIMWARE_DOWNLOAD 3
#define I2C_ERROR		4

#define FIRMWARE_VERSION	13

s8 t1310_state = NOT_BOOTING;

int dead = 5;
s16 grip_state = 0;
bool btn_state = RELEASE;
s8 touch_state = 2;
s8 finger_num = 0;
unsigned char keycode_old;
s16 x_old = 0;
s16 y_old = 0;
s16 x2_old = 0;
s16 y2_old = 0;
bool old_finger[5]= {RELEASE};
bool fd_state = 0;
s16 old_x[5] = {0};
s16 old_y[5] = {0};

#define TOUCH_X_MAX (1123)
#define TOUCH_Y_MAX (1872)
#define I2C_NO_REG (0xFFFF)

#define GPIO_TOUCH_ATTN				107

#define I2C_RETRY_DELAY		5
#define I2C_RETRIES		5
struct t1310_device {
	struct i2c_client		*client;		/* i2c client for adapter */
	struct input_dev		*input_dev;		/* input device for android */
	struct delayed_work		dwork;			/* delayed work for bh */	
	spinlock_t			lock;			/* protect resources */
	int	irq;			/* Terminal out irq number */
};

static struct t1310_device *touch_pdev = NULL;
static struct workqueue_struct *touch_wq;


static void large_mdelay(unsigned int msec)
{
	unsigned long endtime = 0;
	unsigned long msec_to_jiffies = 0;

	msec_to_jiffies = msecs_to_jiffies(msec);
	endtime = jiffies + msec_to_jiffies + 1;

	while(jiffies < endtime);
	
}


// BEGIN: 0009214 sehyuny.kim@lge.com 2010-09-03
// MOD 0009214: [DIAG] LG Diag feature added in side of android
void Send_Touch( unsigned int x, unsigned int y)
{
			input_report_abs(touch_pdev->input_dev, ABS_MT_TOUCH_MAJOR, 1);
			input_report_abs(touch_pdev->input_dev, ABS_MT_POSITION_X, x);
			input_report_abs(touch_pdev->input_dev, ABS_MT_POSITION_Y, y);
			input_mt_sync(touch_pdev->input_dev);
			input_sync(touch_pdev->input_dev);

			input_report_abs(touch_pdev->input_dev, ABS_MT_TOUCH_MAJOR, 0);
			input_report_abs(touch_pdev->input_dev, ABS_MT_POSITION_X, x);
			input_report_abs(touch_pdev->input_dev, ABS_MT_POSITION_Y, y);
			input_mt_sync(touch_pdev->input_dev);

			input_sync(touch_pdev->input_dev);
}
EXPORT_SYMBOL(Send_Touch); 
// END: 0009214 sehyuny.kim@lge.com 2010-09-03


static s8
touch_i2c_write(u8 addr, u8 value)
{
	int ret;
	int i;
	
// START:jaekyung83.lee@lge.com 2011.07.07
// When I2C Fail, Retry I2C
	for(i = 0;i < 10;i++)
	{
		ret = i2c_smbus_write_byte_data(touch_pdev->client, addr, value);

		if (ret < 0) {
			TOUCHE("%d, E: caddr(0x%x),addr(%u),ret(%d)\n",i, touch_pdev->client->addr, addr, ret);

			if(t1310_state == NOT_BOOTING)
				return ret;
			
			t1310_state = I2C_ERROR;
			continue;
		}
		else
		{
			TOUCHD("addr(0x%x),val(0x%x)\n", addr, value);
			if(t1310_state == I2C_ERROR)
				t1310_state = WORKING;
			return ret;
		}
	}

	if(t1310_state == I2C_ERROR)
	{
		esd_touch_reset();
		return 0;
	}
// END:jaekyung83.lee@lge.com 2011.07.07
	return ret;
}

static int
touch_i2c_read(u8 addr)
{
	int ret =0;
	int i;
	
// START:jaekyung83.lee@lge.com 2011.07.07
// When I2C Fail, Retry I2C
	for(i = 0;i < 10; i++)
	{
		ret = i2c_smbus_read_byte_data(touch_pdev->client, addr);

		if (ret < 0) {
			TOUCHE("%d, E: addr(0x%x), ret(0x%d)\n", i, addr, ret);

			if(t1310_state == NOT_BOOTING)
				return ret;
			
			t1310_state = I2C_ERROR;
			continue;
		}
		else
		{
			TOUCHD("addr(0x%x),val(0x%08x)\n", addr, ret);
			if(t1310_state == I2C_ERROR)
				t1310_state = WORKING;
			return ret;
		}
	}

	if(t1310_state == I2C_ERROR)
	{
		esd_touch_reset();
		return 0;
	}
// END:jaekyung83.lee@lge.com 2011.07.07
	return ret;
}

unsigned int SynaWriteRegister(u8 uRmiAddress, u8 buf[], unsigned int len)
{
	unsigned char err;
	int tries = 0;
	int i;

	for(i = len ; i > 0 ; i--)
	{
		buf[i] = buf[i-1];
	}
	buf[0] = uRmiAddress;

	struct i2c_msg msgs[] = {
		{
		 .addr = touch_pdev->client->addr,
		 .flags = touch_pdev->client->flags & I2C_M_TEN,
		 .len = len + 1,
		 .buf = buf,
		 },
	};

	do {
		err = i2c_transfer(touch_pdev->client->adapter, msgs, 1);
		if (err != 1)
			msleep_interruptible(I2C_RETRY_DELAY);
	} while ((err != 1) && (++tries < I2C_RETRIES));

	if (err != 1) {
		dev_err(&touch_pdev->client->dev, "write transfer error\n");
		t1310_state = I2C_ERROR;
		err = -EIO;
	} else {
		err = 0;
	}

	return err;
}
EXPORT_SYMBOL(SynaWriteRegister); 

unsigned int SynaReadRegister(u8 uRmiAddress, u8 buf[], unsigned int len)
{
	unsigned char err;
	int tries = 0;

	buf[0] = uRmiAddress;

	struct i2c_msg msgs[] = {
		{
		 .addr = touch_pdev->client->addr,
		 .flags = touch_pdev->client->flags & I2C_M_TEN,
		 .len = 1,
		 .buf = buf,
		 },
		{
		 .addr = touch_pdev->client->addr,
		 .flags = (touch_pdev->client->flags & I2C_M_TEN) | I2C_M_RD,
		 .len = len,
		 .buf = buf,
		 },
	};

	do {
		err = i2c_transfer(touch_pdev->client->adapter, msgs, 2);
		if (err != 2)
			msleep_interruptible(I2C_RETRY_DELAY);
	} while ((err != 2) && (++tries < I2C_RETRIES));

	if (err != 2) {
		dev_err(&touch_pdev->client->dev, "read transfer error\n");
		t1310_state = I2C_ERROR;
		err = -EIO;
	} else {
		err = 0;
	}

	return err;
}
EXPORT_SYMBOL(SynaReadRegister); 

bool SynaWaitForATTN(int dwMilliseconds)
{
	int trialMs=0;
	int state;

	state = gpio_get_value(GPIO_TOUCH_ATTN);
	while( ( (state == 1) && (trialMs<dwMilliseconds)) )
	{
		mdelay(1);
		trialMs++;
		state = gpio_get_value(GPIO_TOUCH_ATTN);
	}

	state = gpio_get_value(GPIO_TOUCH_ATTN);
	if(state == 0)						
		return 0;

	return -1;			
}
EXPORT_SYMBOL(SynaWaitForATTN); 
static int
touch_device_initialise(void)
{
	int ret = 0;
	int version = 0;
	u8 data[28] = {0};
	int reg = 0;

	TOUCHD("entry\n");

	ret = touch_i2c_read(0x14);
	//ret = SynaReadRegister(0x14, &data, 28);
	TOUCHD("ret(0x0x14): 0x%x\n", ret);

// START:jaekyung83.lee@lge.com 2011.07.26 
// ABS Position Filter Register Setting, Jittering Set
#if 0	
	reg = 0x35;

	data[0] = 0x00;
	data[1] = 0x00;
	data[2] = 0x01;
	data[3] = 0x01;

	ret = SynaWriteRegister(reg, &data, 4);
#endif	
	ret = touch_i2c_write(0x35, 0x08);
// END:jaekyung83.lee@lge.com 2011.07.26

// START:jaekyung83.lee@lge.com 2011.07.06 
// Palm Touch Detection Register Setting	
	ret = touch_i2c_write(0x40, 0x0);
	ret = touch_i2c_write(0x3F, 0x0);
// END:jaekyung83.lee@lge.com 2011.07.06


// START:jaekyung83.lee@lge.com 2011.08.12
// NoSleep Bit enable 
	ret = touch_i2c_write(0x33, 0x04);
// END:jaekyung83.lee@lge.com 2011.08.12

	btn_state = RELEASE;

 	return ret;

}

#if defined(LG_HW_REV5)
static void
touch_report_key_event(u16 state)
{
	int keycode;
	TOUCHD("state= 0x%x \n", keycode);

	if (state == 0x04 || state == 0x0) {
		btn_state = RELEASE;
		keycode = keycode_old;
	} else if(state == 0x6 || state == 0x2) {
		btn_state = PRESS;
		keycode = BTN2;
	} else if(state == 0x5 || state == 0x1) {
		btn_state = PRESS;
		keycode = BTN3;
	} else {
		TOUCHD("Unknown key type(0x%x)\n", state);
	}

	keycode_old = keycode;

	if(state == 0x0 || state == 0x1 || state == 0x2 || state == 0x04 || state == 0x6 || state == 0x5 ) {
		input_report_key(touch_pdev->input_dev, keycode, btn_state);
		input_sync(touch_pdev->input_dev);
		TOUCHD("state= %x, TOUCH KEY->%d\n", state, keycode);
	}
}
#else
static void
touch_report_key_event(u16 state)
{
	int keycode;

	if (state == 0) {
		btn_state = RELEASE;
		keycode = keycode_old;
	} else if(state == 0x1) {
		if(PRESS == btn_state)
		{
			btn_state = RELEASE;
			input_report_key(touch_pdev->input_dev, keycode_old, btn_state);
			input_sync(touch_pdev->input_dev);
			TOUCHD("TOUCH KEY->%d\n", keycode_old);
			/* neo.kang@lge.com 11-01-22 
			 * 0014608 : add the key log */
			printk("ts key : %x, %x\n", keycode_old, btn_state);			
		}

		btn_state = PRESS;
		keycode = BTN1;
	} else if(state == 0x2) {
		if(PRESS == btn_state)
		{
			btn_state = RELEASE;
			input_report_key(touch_pdev->input_dev, keycode_old, btn_state);
			input_sync(touch_pdev->input_dev);
			TOUCHD("TOUCH KEY->%d\n", keycode_old);
			/* neo.kang@lge.com 11-01-22 
			 * 0014608 : add the key log */
			printk("ts key : %x, %x\n", keycode_old, btn_state);			
		}

		btn_state = PRESS;
		keycode = BTN2;
	} else if(state == 0x4) {
		if(PRESS == btn_state)
		{
			btn_state = RELEASE;			
			input_report_key(touch_pdev->input_dev, keycode_old, btn_state);
			input_sync(touch_pdev->input_dev);
			TOUCHD("TOUCH KEY->%d\n", keycode_old);
			/* neo.kang@lge.com 11-01-22 
			 * 0014608 : add the key log */
			printk("ts key : %x, %x\n", keycode_old, btn_state);			
		}	

		btn_state = PRESS;
		keycode = BTN3;
	} else if(state == 0x8) {
		if(PRESS == btn_state)
		{
			btn_state = RELEASE;
			input_report_key(touch_pdev->input_dev, keycode_old, btn_state);
			input_sync(touch_pdev->input_dev);
			TOUCHD("TOUCH KEY->%d\n", keycode_old);
			/* neo.kang@lge.com 11-01-22 
			 * 0014608 : add the key log */
			printk("ts key : %x, %x\n", keycode_old, btn_state);			
		}

		btn_state = PRESS;
		keycode = BTN4;
	} else {
		TOUCHD("Unknown key type(0x%x)\n", state);
		return;
	}

	keycode_old = keycode;

	if(state == 0 || state == 0x1 || state == 0x2 || state == 0x4 ||state == 0x8) {
		input_report_key(touch_pdev->input_dev, keycode, btn_state);
		input_sync(touch_pdev->input_dev);
		TOUCHD("TOUCH KEY->%d\n", keycode);
		/* neo.kang@lge.com 11-01-22 
		 * 0014608 : add the key log */
		printk("ts key : %x, %x\n", keycode, btn_state);
	}
}
#endif


static void multitouch_event(s8 state, s16 x, s16 y) 
{
	if (x >= grip_state && x <= TOUCH_X_MAX - grip_state)
	{
		input_report_abs(touch_pdev->input_dev, ABS_MT_TOUCH_MAJOR, state);
		input_report_abs(touch_pdev->input_dev, ABS_MT_POSITION_X, x);
		input_report_abs(touch_pdev->input_dev, ABS_MT_POSITION_Y, y);
		input_mt_sync(touch_pdev->input_dev);
	}
}

void Touch_Check(void)
{
	int i;

  if(btn_state != RELEASE)
		touch_report_key_event(0);

	for(i = 0 ; i < MAX_FINGER; i++)
	{
		if(old_finger[i] == 1) {
			multitouch_event(RELEASE, 0, 0);
			input_sync(touch_pdev->input_dev);
			old_finger[i] = RELEASE;
			TOUCHD("finger=%d RELEASE\n", i);
		}
	}
}
EXPORT_SYMBOL(Touch_Check); 

/*
 * interrupt service routine
 */
static int touch_irq_handler(int irq, void *dev_id)
{
#if 0
	int state;

	state = gpio_get_value(GPIO_TOUCH_ATTN);
	TOUCHD("PIN STATE: %d\n", state);
#endif
	if(t1310_state == WORKING){
		spin_lock(&touch_pdev->lock);
		queue_work(touch_wq, &touch_pdev->dwork);
		spin_unlock(&touch_pdev->lock);
	}

	return IRQ_HANDLED;
}

static void
touch_work_func(struct work_struct *work)
{
	int ret = 0;
	s16 x = 0;
	s16 y = 0;
	s16 lsb = 0;
	s16 but= 0;
	int i = 0;
	s8 interrupt_state =0;
	u8 data[28] = {0};
	int but_state, tou_state;
	bool finger[5];
	int tem;
	int status;
	bool check_finger[6] = {false};

	ret = touch_i2c_read(0x30); // Tap Detect Gesture Read
	ret = touch_i2c_read(0x31); // Palm Detect Gesture Read
	
	ret = touch_i2c_read(0x14);
	
	status = touch_i2c_read(0x13);
	TOUCHD("status : 0x%x\n", status);
#ifdef ESD_DETECTION
	if(ret & 0x02)
	{
		if(0x03 == (status & 0x03))
		{
			esd_touch_reset();
			return;
		}
	}
#endif	
	if(ret < 0 && t1310_state != SUSPEND) {
		ret = touch_power(0);
		ret = touch_power(1);
		mdelay(400);
		//	ret = touch_device_initialise();
		ret = touch_i2c_read(0x14);
		if (ret < 0) {
			TOUCHE("failed to init\n");
			t1310_state = I2C_ERROR;
		} else {
			t1310_state = WORKING;
		}
	}

	interrupt_state = ret;

	but_state = (interrupt_state & 0x08) >> 3;
	tou_state = (interrupt_state & 0x04) >> 2;

	ret = SynaReadRegister(0x15, &data, 28);
	but = touch_i2c_read(0x32);

	TOUCHD("state(7): 0x%x, 0x%x, 0x%x\n", interrupt_state, but_state, tou_state);

	if(1 == but_state)
	{
	     if((1 == tou_state) && (btn_state == RELEASE))
	     {
			TOUCHD("Do nothing at button interrupt\n");
	     }
		 else
		 {
		 	touch_report_key_event(but);	
			check_finger[5] = false;
		 }
	}
	
	if(1 == tou_state)
	{
		/*Touch*/	
		finger[0] = (data[0] & 0x01);
		finger[1] = (data[0] & 0x04) >> 2;
		finger[2] = (data[0] & 0x10) >> 4;
		finger[3] = (data[0] & 0x40) >> 6;
		finger[4] = (data[1] & 0x01);

		TOUCHD("|%d|%d|%d|%d|%d|\n", finger[0], finger[1], finger[2], finger[3], finger[4]);

		for(i = 0 ; i < MAX_FINGER; i++)
		{
			if(finger[i] == 1 || (finger[i] == 0 && old_finger[i] == 1))
			{
				tem = (i*5)+2;

				x = data[tem]; 
				y = data[tem+1]; 
				lsb = data[tem+2]; 

				x = (x << 4) | (0x0F & lsb);
				y = (y << 4) | ((0xF0 & lsb)>>4);

// START:jaekyung83.lee@lge.com 2011.07.26 
// ABS Position Filter Register Setting, Jittering Set
#if 0
				if (i == 0 && finger[0] == 1 && data[0] == 0x01)
				{
				
					if( x>(old_x[0]+dead)|| x<(old_x[0]-dead)|| y>(old_y[0]+dead) || y<(old_y[0]-dead))
					{
						multitouch_event(finger[i], x, y);
						TOUCHD("%d %d(%x,%x)\n",i, finger[i], x, y);
						TOUCHD("%d %d old (%x,%x)\n", i, finger[i], old_x[i], old_x[i]);
						old_x[i] = x;
						old_y[i] = y;
						old_finger[i] = finger[i];
					}
					else
					{
						TOUCHD("Jittering finger[0] old_x:0x%x, old_y:0x%x, x:0x%x, y:0x%x\n",old_x[i], old_y[i], x, y);
					}
				} 
				else 
				{
					multitouch_event(finger[i], x, y);
					old_finger[i] = finger[i];
					TOUCHD("%d(%x,%x)\n", finger[i], x, y);
				}
#endif				
// END:jaekyung83.lee@lge.com 2011.07.26 

				multitouch_event(finger[i], x, y);
				old_finger[i] = finger[i];
				TOUCHD("%d(%x,%x)\n", finger[i], x, y);
			}
// START:jaekyung83.lee@lge.com 2011.07.18
// interrupt occur during nontouch, Touch Device try to reset 
			else
			{
				check_finger[i] = true;
			}
// END:jaekyung83.lee@lge.com 2011.07.18 
			
		}


// START:jaekyung83.lee@lge.com 2011.07.18
// interrupt occur during nontouch, Touch Device try to reset(Power Off -> Power On -> Initailize)
		if(check_finger[0] && check_finger[1] && check_finger[2] && check_finger[3] && check_finger[4] && check_finger[5])
		{
			printk(KERN_INFO "%s, Error touch, Touch Device Reset\n",__func__);
			esd_touch_reset();
			return;
		}
// END:jaekyung83.lee@lge.com 2011.07.18 		
		input_sync(touch_pdev->input_dev);
	}
	
}

static int touch_suspend(struct i2c_client *i2c_dev, pm_message_t state);
static int touch_resume(struct i2c_client *i2c_dev);

// START:jaekyung83.lee@lge.com 2011.07.07
// ESD Touch Rest Add.
#ifdef ESD_DETECTION
static void esd_touch_reset()
{
	int ret;
	
	printk(KERN_INFO "%s, ESD Detected\n",__func__);

	gpio_tlmm_config(GPIO_CFG(GPIO_TOUCH_ATTN, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_DISABLE);			
	disable_irq(gpio_to_irq(GPIO_TOUCH_ATTN));
	
	ret = esd_touch_power(0);
//	mdelay(1000);
	large_mdelay(1000);

	ret = esd_touch_power(1);
//	mdelay(400);
	large_mdelay(400);

	ret = touch_device_initialise();

	gpio_tlmm_config(GPIO_CFG(GPIO_TOUCH_ATTN, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	enable_irq(gpio_to_irq(GPIO_TOUCH_ATTN));
	
// Change t1310_state from I2C_ERROR to WORKING when I2C Fail after ESD Detection
	if(t1310_state == I2C_ERROR)
	{
		t1310_state = WORKING;
	}
}
// Add esd touch power
static int esd_touch_power(int enable)
{
	struct vreg* vreg_touch_power;
	struct vreg* vreg_touch_power_io;
	int err = -1;

	vreg_touch_power = vreg_get(0, "wlan");
	vreg_touch_power_io = vreg_get(0, "gp9");

	if(enable)
	{
		vreg_enable(vreg_touch_power);
		err = vreg_set_level(vreg_touch_power, 3000);
		if(err != 0)
		{
			TOUCHE("vreg_touch_power failed\n");
			return -1;
		}
		mdelay(2);
		vreg_enable(vreg_touch_power_io);
		err = vreg_set_level(vreg_touch_power_io, 2800);
		if(err != 0)
		{
			TOUCHE("vreg_touch_power_io failed\n");
			return -1;
		}

		TOUCHD("touch_power On OK\n");
//		mdelay(40);
		large_mdelay(40);

	}
	else
	{
		err = vreg_set_level(vreg_touch_power_io, 0);
		if(err != 0)
		{
			TOUCHE("vreg_touch_power_io failed\n");
			return -1;
		}
		err = vreg_set_level(vreg_touch_power, 0);
		if(err != 0)
		{
			TOUCHE("vreg_touch_power failed.\n");
			return -1;
		}

		vreg_disable(vreg_touch_power_io);
		vreg_disable(vreg_touch_power);

		TOUCHD("touch_power Off OK\n");		
	}

	return err;
}
#endif
// END:jaekyung83.lee@lge.com 2011.07.07

#if 1
int touch_power(int enable)
{
	struct vreg *vreg_touch_power;
	struct vreg *vreg_touch_power_io;
	int err;

	vreg_touch_power = vreg_get(0, "wlan");
	vreg_touch_power_io = vreg_get(0, "gp9");

	if(t1310_state != WORKING) {
		if (enable) {
			vreg_enable(vreg_touch_power);
			err = vreg_set_level(vreg_touch_power, 3000);
			if (err != 0) {
				TOUCHE("vreg_touch_power failed.\n");
				return -1;
			}

			mdelay(2);
			vreg_enable(vreg_touch_power_io);
			err = vreg_set_level(vreg_touch_power_io, 2800);
			if (err != 0) {
				TOUCHE("vreg_touch_power_io failed.\n");
				return -1;
			}
			TOUCHD("touch_touch_power OK\n");
			mdelay(40);
		} 
		else {
			err = vreg_set_level(vreg_touch_power_io, 0);
			if (err != 0) {
				TOUCHE("vreg_touch_power_io failed.\n");
				return -1;
			}
			err = vreg_set_level(vreg_touch_power, 0);
			if (err != 0) {
				TOUCHE("vreg_touch_power failed.\n");
				return -1;
			}
			vreg_disable(vreg_touch_power_io);
			vreg_disable(vreg_touch_power);
		}
	} else {
		  TOUCHD("Touch Power WORKING enable:%d\n", enable);
		  if(enable) {
				err = touch_device_initialise();
				if (err < 0) {
					t1310_state = I2C_ERROR;
					TOUCHE("failed to init- restart\n");
					touch_power(0);
					touch_power(1);
					mdelay(400);
					err = touch_device_initialise();
					if (err < 0) {
						TOUCHE("failed to init\n");
						t1310_state = I2C_ERROR;
					} 
					else {
						t1310_state = WORKING;
					}
				} 
				else {
					t1310_state = WORKING;
				}
		  }
	}


	return 0;
}
EXPORT_SYMBOL(touch_power); 
#endif

/*  ------------------------------------------------------------------------ */
/*  --------------------    SYSFS DEVICE FIEL    --------------------------- */
/*  ------------------------------------------------------------------------ */
static ssize_t
touch_mode_show(struct device *dev, struct device_attribute *attr, char *buf)
{

	printk("|NOT_BOTING(0)|WORKING(1)|SUSPEND(2)|FD(3)|I2C_ERROR(4): %d\n", t1310_state);
	printk("t1310_state: %d\n", t1310_state);

	return 0;
}

static ssize_t
touch_mode_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	sscanf(buf, "%d", &t1310_state);

	printk("|NOT_BOTING(0)|WORKING(1)|SUSPEND(2)|FD(3)|I2C_ERROR(4): %d\n", t1310_state);

	return count;
}
static ssize_t
touch_dead_show(struct device *dev, struct device_attribute *attr, char *buf)
{

	printk("dead: %d\n", dead);

	return 0;
}

static ssize_t
touch_dead_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	sscanf(buf, "%d", &dead);

	printk("dead: %d\n", dead);

	return count;
}

static ssize_t
touch_grip_show(struct device *dev, struct device_attribute *attr, char *buf)
{

    printk("grip suppression: %x \n ", grip_state);
	return 0;
}

static ssize_t
touch_grip_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	sscanf(buf, "%x", &grip_state);

	return count;
}

static ssize_t
touch_rw_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int ret;
	int i;
	u8 data[4]={0};

	ret = touch_i2c_read(0x9d);
	printk("ret(0x9d): 0x%x\n", ret);
	ret = SynaReadRegister(0x35, &data, 4);
	for(i = 0 ; i < 4 ; i++){
		printk("--->data(0x%x): 0x%x\n", i, data[i]);
	}

	return 0;
}

static ssize_t
touch_rw_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	u8 data[5]={0};
	u8 reg;
	int ret;
	int i;

	sscanf(buf, "%x", &reg);

	printk("reg0x%x\n", reg);
	ret = SynaReadRegister(reg, &data, 5);
	for(i = 0 ; i < 5 ; i++){
		printk("--->data(0x%x): 0x%x\n", i, data[i]);
	}

	return count;
}
static ssize_t
touch_fd_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int ret;

	printk("frimware upgrade start \n");
	t1310_state = FRIMWARE_DOWNLOAD;
	FirmwareUpgrade_main(FIRMWARE_VERSION);
	printk("frimware upgrade end \n");
	ret = touch_device_initialise();
	if (ret < 0) {
		TOUCHE("failed to init\n");
	} else { 
		t1310_state = WORKING;
	}
	
	return 0;
}

static ssize_t
touch_fd_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int version;
	int ret;

	sscanf(buf, "%d", &version);
	printk("Firmware upgrade start - version:%d\n", version);

	t1310_state = FRIMWARE_DOWNLOAD;
	FirmwareUpgrade_main(version);
	printk("Firmware upgrade end \n");
	ret = touch_device_initialise();
	if (ret < 0) {
		TOUCHE("failed to init\n");
	} else { 
		t1310_state = WORKING;
	}
	return count;
}

static ssize_t
touch_attn_show(struct device *dev, struct device_attribute *attr, char *buf)
{

	int state, ret, i;
	u8 data[4] ={0};

	state = gpio_get_value(GPIO_TOUCH_ATTN);
	printk("GPIO_TOUCH_ATTN state : %d\n", state);

	ret = touch_i2c_read(0x14);
	printk("ret(0x14): 0x%x\n", ret);

	ret = touch_i2c_read(0x9d);
	printk("ret(0x9d): 0x%x\n", ret);
	ret = SynaReadRegister(0x35, &data, 4);
	for(i = 0 ; i < 4 ; i++){
		printk("--->data(0x%x): 0x%x\n", i, data[i]);
	}
	return 0;
}

static ssize_t
touch_attn_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int enable;
	int ret;
	sscanf(buf, "%d", &enable);

	if(enable == 1 ) {
		touch_power(0);
		mdelay(4);
		touch_power(enable);
		mdelay(4);
		gpio_tlmm_config(GPIO_CFG(GPIO_TOUCH_ATTN, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		enable_irq(gpio_to_irq(GPIO_TOUCH_ATTN));

		ret = touch_device_initialise();
		if (ret < 0) {
			TOUCHE("failed to init\n");
		} else { 
			t1310_state = WORKING;
		}


	} else {
		gpio_tlmm_config(GPIO_CFG(GPIO_TOUCH_ATTN, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_DISABLE);
		touch_power(enable);
		disable_irq(gpio_to_irq(GPIO_TOUCH_ATTN));
		t1310_state = SUSPEND;
	}
	printk("touch_power enable: %d\n", enable);

	return count;
}

static struct device_attribute touch_device_attrs[] = {
	__ATTR(rw, S_IRUGO | S_IWUSR, touch_rw_show, touch_rw_store),	
	__ATTR(fd, S_IRUGO | S_IWUSR, touch_fd_show, touch_fd_store),	
	__ATTR(attn, S_IRUGO | S_IWUSR, touch_attn_show, touch_attn_store),
	__ATTR(grip, S_IRUGO | S_IWUSR, touch_grip_show, touch_grip_store),
	__ATTR(dead, S_IRUGO | S_IWUSR, touch_dead_show, touch_dead_store),
	__ATTR(mode, S_IRUGO | S_IWUSR, touch_mode_show, touch_mode_store),

};

/*  ------------------------------------------------------------------------ */
/*  --------------------        I2C DRIVER       --------------------------- */
/*  ------------------------------------------------------------------------ */

#ifdef CONFIG_HAS_EARLYSUSPEND
static void 
touch_early_suspend(struct early_suspend *h)
{
	int ret;

	TOUCHD("entry\n");

	t1310_state = SUSPEND;

	cancel_delayed_work_sync(&touch_pdev->dwork);
	flush_workqueue(touch_wq);

	ret = touch_power(0);
	if (ret < 0) {
		TOUCHE("failed to touch_early_suspend Power off\n");
	}	

	gpio_tlmm_config(GPIO_CFG(GPIO_TOUCH_ATTN, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_DISABLE);
	disable_irq(gpio_to_irq(GPIO_TOUCH_ATTN));
	
	//race.min 
	Touch_Check();

	TOUCHD("exit\n");

}


static void 
touch_late_resume(struct early_suspend *h)
{
	int ret;

	TOUCHD("entry\n");

	ret = touch_power(1);
	gpio_tlmm_config(GPIO_CFG(GPIO_TOUCH_ATTN, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	enable_irq(gpio_to_irq(GPIO_TOUCH_ATTN));

	ret = touch_device_initialise();
	if (ret < 0) {
		TOUCHE("failed to init\n");
	} else { 
		t1310_state = WORKING;
	}

	TOUCHD("exit\n");

}
#endif

static int
touch_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret = 0;
	int i;
	unsigned keycode = KEY_UNKNOWN;

	TOUCHD("t1310-entry\n");

	ret = touch_power(1);

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)){
		TOUCHE("it is not support I2C_FUNC_I2C.\n");
		return -ENODEV;
	}

	touch_pdev = kzalloc(sizeof(struct t1310_device), GFP_KERNEL);
		if (touch_pdev == NULL) {
			TOUCHE("failed to allocation\n");
			return -ENOMEM;
		}
	

	INIT_DELAYED_WORK(&touch_pdev->dwork, touch_work_func);

	touch_pdev->client = client;

	i2c_set_clientdata(touch_pdev->client, touch_pdev);

	/* allocate input device for transfer proximity event */
	touch_pdev->input_dev = input_allocate_device();
	if (NULL == touch_pdev->input_dev) {
			dev_err(&client->dev, "failed to allocation\n");
			goto err_input_allocate_device;
	}

	/* initialise input device for touch00200F */
	touch_pdev->input_dev->name = "touch_keypad";
	touch_pdev->input_dev->phys = "touch_keypad/input3";

/*touch key*/
	touch_pdev->input_dev->evbit[0] = BIT_MASK(EV_KEY);
	touch_pdev->input_dev->keycode = BTN1, BTN2, BTN3, BTN4;
	touch_pdev->input_dev->keycodesize = sizeof(unsigned short);
	touch_pdev->input_dev->keycodemax = MAX_BTN;

	keycode = BTN1;
	set_bit(keycode, touch_pdev->input_dev->keybit);
	keycode = BTN2;
	set_bit(keycode, touch_pdev->input_dev->keybit);
	keycode = BTN3;
	set_bit(keycode, touch_pdev->input_dev->keybit);
	keycode = BTN4;
	set_bit(keycode, touch_pdev->input_dev->keybit);
	
/*touch*/
    touch_pdev->input_dev->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);
	touch_pdev->input_dev->absbit[0] = BIT(ABS_X) | BIT(ABS_Y) | BIT(ABS_PRESSURE);
	touch_pdev->input_dev->absbit[BIT_WORD(ABS_MISC)] = BIT_MASK(ABS_MISC);
	touch_pdev->input_dev->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);

#ifdef SINGLE_TOUCH
	input_set_abs_params(touch_pdev->input_dev, ABS_X, 0, TOUCH_X_MAX, 0, 0);
	input_set_abs_params(touch_pdev->input_dev, ABS_Y, 0, TOUCH_Y_MAX, 0, 0);
#endif

/*dual touch */
	set_bit(ABS_MT_TOUCH_MAJOR, 	 touch_pdev->input_dev->absbit);
	input_set_abs_params(touch_pdev->input_dev, ABS_MT_POSITION_X, 0, TOUCH_X_MAX, 0, 0);
	input_set_abs_params(touch_pdev->input_dev, ABS_MT_POSITION_Y, 0, TOUCH_Y_MAX, 0, 0);

	set_bit(EV_SYN, 	 touch_pdev->input_dev->evbit);
	set_bit(EV_KEY, 	 touch_pdev->input_dev->evbit);
	set_bit(EV_ABS, 	 touch_pdev->input_dev->evbit);

	/* register input device for touch */
	ret = input_register_device(touch_pdev->input_dev);
	if (ret < 0) {
		TOUCHE("failed to register input\n");
		goto err_input_register_device;
	}
#if 0
	pdata = touch_pdev->client->dev.platform_data;
	if (pdata == NULL) {
			TOUCHE("failed to get platform data\n");
			goto err_touch_initialise;
	}
#endif
	TOUCHD("input_register_device\n");
	spin_lock_init(&touch_pdev->lock);

	gpio_tlmm_config(GPIO_CFG(GPIO_TOUCH_ATTN, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE); 
	touch_pdev->irq = gpio_to_irq(GPIO_TOUCH_ATTN);

/*Auto-Firmware Upgrade*/
	mdelay(400);
	ret = touch_i2c_read(0x9d);
	printk(KERN_INFO "%s, F/W:0x%x Touch F/W Version : 0x%x\n",__func__,FIRMWARE_VERSION,ret);
	if(ret > 0 && ret != FIRMWARE_VERSION){
		FirmwareUpgrade_main(FIRMWARE_VERSION);
		printk("Touch Firmware Upgrade OK!");		
 	}
	
	ret = touch_device_initialise();
	if (ret < 0) {
		TOUCHE("failed to init\n");
		goto err_touch_initialise;
	} else {
		t1310_state = WORKING;
	}

	/* register interrupt handler */
	ret = request_irq(touch_pdev->irq, touch_irq_handler, IRQF_TRIGGER_FALLING, "t1310_irq", touch_pdev);
	if (ret < 0) {
		TOUCHE("failed to register irq\n");
		goto err_irq_request;
	}

	TOUCHD("i2c client addr(0x%x)\n", touch_pdev->client->addr);
	TOUCHD("ATTN STATE : %d\n", gpio_get_value(GPIO_TOUCH_ATTN));

	for (i = 0; i < ARRAY_SIZE(touch_device_attrs); i++) {
		ret = device_create_file(&client->dev, &touch_device_attrs[i]);
		if (ret) {
			goto err_device_create_file;
		}
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
	TOUCHD("ey.cho earysuspend touch\n");
	ts_early_suspend.suspend = touch_early_suspend;
	ts_early_suspend.resume = touch_late_resume;
	ts_early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	register_early_suspend(&ts_early_suspend);
#endif

	TOUCHD("exit\n");


	return 0;

err_device_create_file:
	while(--i >= 0)
		device_remove_file(&client->dev, &touch_device_attrs[i]);
err_irq_request:
	input_unregister_device(touch_pdev->input_dev);
err_touch_initialise:
err_input_register_device:
	input_free_device(touch_pdev->input_dev);
err_input_allocate_device:
	kfree(touch_pdev);

	return ret;
}

static int
touch_i2c_remove(struct i2c_client *client)
{
	struct t1310_device *pdev = i2c_get_clientdata(client);
	int i;

	TOUCHD("entry\n");
	//free_irq(pdev->irq, NULL);
	input_unregister_device(pdev->input_dev);
	input_free_device(pdev->input_dev);
	kfree(touch_pdev);

	for (i = 0; i < ARRAY_SIZE(touch_device_attrs); i++)
		device_remove_file(&client->dev, &touch_device_attrs[i]);

#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&ts_early_suspend);
#endif
	TOUCHD("exit\n");

	return 0;
}


static int
touch_i2c_suspend(struct i2c_client *i2c_dev, pm_message_t state)
{
	struct t1310_device *pdev = i2c_get_clientdata(i2c_dev);
	int ret;

	TOUCHD("entry\n");

	t1310_state = SUSPEND;
	ret = touch_power(0);
	if (ret < 0) {
		TOUCHE("failed to touch_i2c_suspend\n");
	}
	
	disable_irq(gpio_to_irq(GPIO_TOUCH_ATTN));

	cancel_delayed_work_sync(&pdev->dwork);
	flush_workqueue(touch_wq);

	//race.min 
	Touch_Check();

	TOUCHD("exit\n");

	return 0;
}


static int
touch_i2c_resume(struct i2c_client *i2c_dev)
{
	int ret;

	TOUCHD("entry\n");

//	ret = touch_power(1);
	gpio_tlmm_config(GPIO_CFG(GPIO_TOUCH_ATTN, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	enable_irq(gpio_to_irq(GPIO_TOUCH_ATTN));

	ret = touch_device_initialise();
	if (ret < 0) {
		TOUCHE("failed to init\n");
	} else {
		t1310_state = WORKING;
	}

	TOUCHD("exit\n");

	return 0;
}
// BEGIN: 0010583 alan.park@lge.com 2010.11.06
// ADD 0010583: [ETA/MTC] ETA Capture, Key, Touch, Logging / MTC Key, Logging  
int touch_get_x_max (void)
{
	return TOUCH_X_MAX;
}

int touch_get_y_max(void)
{
	return TOUCH_Y_MAX;
}
// END: 0010583 alan.park@lge.com 2010.11.06
static const struct i2c_device_id touch_i2c_ids[] = {
		{"t1310", 0 },
		{ },
};

MODULE_DEVICE_TABLE(i2c, touch_i2c_ids);

static struct i2c_driver touch_i2c_driver = {
	.probe		= touch_i2c_probe,
	.remove		= touch_i2c_remove,

// START:jaekyung83.lee@lge.com 2011.06.10 
// Remove suspend, resume if define ealry_suspend	
#ifndef CONFIG_HAS_EARLYSUSPEND
	.suspend	= touch_i2c_suspend,
	.resume		= touch_i2c_resume,
#endif	
// END:jaekyung83.lee@lge.com 2011.06.10 
	.id_table	= touch_i2c_ids,
	.driver = {
		.name	= "t1310",
		.owner	= THIS_MODULE,
	},
};

static void __exit touch_i2c_exit(void)
{
	i2c_del_driver(&touch_i2c_driver);
	if (touch_wq)
		destroy_workqueue(touch_wq);
}

static int __init touch_i2c_init(void)
{
	int ret;

	TOUCHD("entry\n");

	touch_wq = create_singlethread_workqueue("touch_wq");
	if (!touch_wq) {
		TOUCHE("failed to create singlethread workqueue\n");
		return -ENOMEM;
	}

	ret = i2c_add_driver(&touch_i2c_driver);
	if (ret < 0) {
		TOUCHE("failed to i2c_add_driver \n");
		destroy_workqueue(touch_wq);
		return ret;
	}
		TOUCHD("entry\n");

	return 0;
}

module_init(touch_i2c_init);
module_exit(touch_i2c_exit);
	
MODULE_LICENSE("GPL");
