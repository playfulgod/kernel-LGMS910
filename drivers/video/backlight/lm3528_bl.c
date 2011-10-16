/* drivers/video/backlight/lm3528_bl.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* CONFIG_MACH_LGE	jihye.ahn	10.07.27
	LCD backlight driver	
*/

#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/backlight.h>
#include <linux/fb.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <mach/board-bryce.h>
#include <mach/vreg.h>

#define MODULE_NAME	"lm3528"
#define CONFIG_BACKLIGHT_LEDS_CLASS

#ifdef CONFIG_BACKLIGHT_LEDS_CLASS
#include <linux/leds.h>
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

// BEGIN : munho.lee@lge.com 2010-10-26
// MOD :0010205: [Display] To change resolution for removing display noise. 
atomic_t lcd_event_handled;
atomic_t lcd_bootup_handle;
// END : munho.lee@lge.com 2010-10-26

/********************************************
 * Definition
 ********************************************/
#define LCD_LED_MAX			0x7C		         /* 17mA */
#define LCD_LED_NOR			0X7A		/* 15mA */
#define LCD_LED_MIN			0x0			/* 2mA */
#define DEFAULT_BRIGHTNESS	LCD_LED_MAX

#ifdef CONFIG_BACKLIGHT_LEDS_CLASS
#define LEDS_BACKLIGHT_NAME "lcd-backlight"
#endif

enum {
	UNINIT_STATE=-1,
	POWERON_STATE,
	NORMAL_STATE,
	SLEEP_STATE,
	POWEROFF_STATE
} LM3528_STATE;

#define dprintk(fmt, args...) \
	do { \
		if(debug) \
			printk("%s:%s: " fmt, MODULE_NAME, __func__, ## args); \
	}while(0);


struct lm3528_driver_data {
	struct i2c_client *client;
	struct backlight_device *bd;
	struct led_classdev *led;
	int gpio;
	int intensity;
	int max_intensity;
	int state;
	int init_on_boot;
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
#endif
};

/********************************************
 * Global variable
 ********************************************/
static unsigned int debug = 0; /* jihye.ahn	   debug message disable : 0 , enable : 1 */
module_param(debug, uint, 0644);


/********************************************
 * Functions
 ********************************************/
static int lm3528_read(struct i2c_client *client, u8 reg, u8 *pval)
{
	int ret;
	int status;

	if (client == NULL) { 	/* No global client pointer? */
		dprintk("client is null\n");
		return -1;
	}

	if ((ret = i2c_smbus_read_byte_data(client, reg)) >= 0) {
		*pval = ret;
		status = 0;
	} else {
		status = -EIO;
		dprintk("fail to read(reg=0x%x,val=0x%x)\n", reg,*pval);
	}

	return status;
}

static int lm3528_write(struct i2c_client *client, u8 reg, u8 val)
{
	int ret;
	int status;

	if (client == NULL) {		/* No global client pointer? */
		dprintk("client is null\n");
		return -1;
	}

	ret = i2c_smbus_write_byte_data(client, reg, val);
	if (ret == 0) {
		status = 0;
	} else {
		status = -EIO;
		dprintk("fail to write(reg=0x%x,val=0x%x)\n", reg, val);
	}
	dprintk("TTT write(reg=0x%x,val=0x%x)\n", reg, val);

	return status;
}


static void lm3528_hw_reset(struct lm3528_driver_data *drvdata)
{
	if(drvdata->client && gpio_is_valid(drvdata->gpio)) {
/* kwangdo.yi@lge.com S 2010.09.04
	replaced with gpio_direction_output to fix build err
*/
#if 0		
		gpio_configure(drvdata->gpio, GPIOF_DRIVE_OUTPUT);
#else
		gpio_direction_output(drvdata->gpio, 1);
#endif
		gpio_set_value(drvdata->gpio, 1);
		udelay(5);
		gpio_set_value(drvdata->gpio, 0);
		udelay(5);
		gpio_set_value(drvdata->gpio, 1);
		udelay(5);
	}
}


static void lm3528_go_opmode(struct lm3528_driver_data *drvdata)
{
	
		//	lm3528_write(drvdata->client, 0x80, 0x01);   // Power on Reset
			lm3528_write(drvdata->client, 0x80, 0x00);  // HW_EN
		
	/* jihye.ahn	2010-09-02		modified Rate of Change Bits 11-->00 */
			//lm3528_write(drvdata->client, 0x10, 0xDF);  // GP : 1101 1111 
			lm3528_write(drvdata->client, 0x10, 0xC7);  // GP : 1100 0111 
			lm3528_write(drvdata->client, 0xA0, LCD_LED_MAX);

			drvdata->state = NORMAL_STATE;
}

static void lm3528_init(struct lm3528_driver_data *drvdata)
{
	if(drvdata->init_on_boot || system_state != SYSTEM_BOOTING) {
		lm3528_hw_reset(drvdata);
		lm3528_go_opmode(drvdata);
	}
	drvdata->state = NORMAL_STATE;
}


/* This function provide LM3528 sleep enter routine for power management. */
static void lm3528_sleep(struct lm3528_driver_data *drvdata)
{
	if (!drvdata || drvdata->state == SLEEP_STATE)
		return;

	dprintk("operation mode is %s\n");
	
			drvdata->state = SLEEP_STATE;
		//	bd6091gu_set_table(drvdata, &N_5_2[0]);
}

static void lm3528_wakeup(struct lm3528_driver_data *drvdata)
{
	if (!drvdata || drvdata->state == NORMAL_STATE)
		return;

	dprintk("operation mode is %s\n");

	if (drvdata->state == POWEROFF_STATE) {
		lm3528_go_opmode(drvdata);
	}
	else if(drvdata->state == SLEEP_STATE) {
		//lm3528_write(drvdata->client, 0x03, drvdata->intensity);
		drvdata->state = NORMAL_STATE;
		}
}


static void lm3528_poweron(struct lm3528_driver_data *drvdata)
{
	if (!drvdata || drvdata->state != POWEROFF_STATE)
		return;
	
	dprintk("POWER ON \n");

	lm3528_init(drvdata);
	//lm3528_write(drvdata->client, 0x03, drvdata->intensity);
}

static void lm3528_poweroff(struct lm3528_driver_data *drvdata)
{
	if (!drvdata || drvdata->state == POWEROFF_STATE)
		return;

	dprintk("POWER OFF \n");

	if (drvdata->state == SLEEP_STATE) {
		gpio_direction_output(drvdata->gpio, 0);
		msleep(6);
		return;
	}
		
	//bd6091gu_write(drvdata->client, 0x00, 0x01);	// sofrware reset
	gpio_tlmm_config(GPIO_CFG(drvdata->gpio, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),GPIO_CFG_ENABLE);
	gpio_direction_output(drvdata->gpio, 0);
	mdelay(6);

	drvdata->state = POWEROFF_STATE;
}

static int lm3528_send_intensity(struct lm3528_driver_data *drvdata, int next)
{
	if(next > drvdata->max_intensity)
		next = drvdata->max_intensity;
	
	if(next < LCD_LED_MIN)
		next = LCD_LED_MIN;
	dprintk("next current is %d\n", next);

	if(drvdata->state == NORMAL_STATE && drvdata->intensity != next)
		lm3528_write(drvdata->client, 0xA0, next);
	
	/* 	jaeseong.gim 	10.08.27
	 * 	for preventing backlight turn off issue
	 */
#ifdef CONFIG_MACH_LGE_BRYCE
	drvdata->bd->props.brightness =
#endif
	drvdata->intensity = next;

	return 0;
}

static int lm3528_get_intensity(struct lm3528_driver_data *drvdata)
{
	return drvdata->intensity;
}


#ifdef CONFIG_PM
#ifdef CONFIG_HAS_EARLYSUSPEND
static void lm3528_early_suspend(struct early_suspend * h)
{	
	struct lm3528_driver_data *drvdata = container_of(h, struct lm3528_driver_data,
						    early_suspend);

	dprintk("start[%s]\n",__func__);
   lm3528_write(drvdata->client, 0x10, 0x0);  // LM3528 Disabled

	return;
}

static void lm3528_late_resume(struct early_suspend * h)
{	
	struct lm3528_driver_data *drvdata = container_of(h, struct lm3528_driver_data,
						    early_suspend);

	dprintk("start[%s]\n",__func__);
	lm3528_write(drvdata->client, 0x10, 0xC7);  // LM3528 Enabled
	/* jihye.ahn  2010-11-15 to remove max brightness when backlight resume*/
	//lm3528_init(drvdata);

	return;
}
#else
static int lm3528_suspend(struct i2c_client *i2c_dev, pm_message_t state)
{
	struct lm3528_driver_data *drvdata = i2c_get_clientdata(i2c_dev);
        dprintk("start\n");
	lm3528_write(drvdata->client, 0x80, 0x01);  /* HW block disable */
	return 0;
}

static int lm3528_resume(struct i2c_client *i2c_dev)
{
	struct lm3528_driver_data *drvdata = i2c_get_clientdata(i2c_dev);
         dprintk("start\n");
	/* jihye.ahn  2010-11-15 to remove max brightness when backlight resume*/
        //lm3528_init(drvdata);

	return 0;
}
#endif	/* CONFIG_HAS_EARLYSUSPEND */
#else
#define lm3528_suspend	NULL
#define lm3528_resume	NULL
#endif	/* CONFIG_PM */


static int lm3528_set_brightness(struct backlight_device *bd)
{
	struct lm3528_driver_data *drvdata = dev_get_drvdata(bd->dev.parent);
	
	return lm3528_send_intensity(drvdata, bd->props.brightness);
}

static int lm3528_get_brightness(struct backlight_device *bd)
{
	struct lm3528_driver_data *drvdata = dev_get_drvdata(bd->dev.parent);
	return lm3528_get_intensity(drvdata);
}

static struct backlight_ops lm3528_ops = {
	.get_brightness = lm3528_get_brightness,
	.update_status  = lm3528_set_brightness,
};


#ifdef CONFIG_BACKLIGHT_LEDS_CLASS
static void leds_brightness_set(struct led_classdev *led_cdev, enum led_brightness value)
{
	struct lm3528_driver_data *drvdata = dev_get_drvdata(led_cdev->dev->parent);
	int brightness;
	int next = 0;    /*jihye.ahn 10-10-12 WBT fix TD157992 */

	if (!drvdata) {
		dprintk("Error getting drvier data\n");
		return;
	}

	brightness = lm3528_get_intensity(drvdata);
	/* jihye.ahn	2010.08.23	mapping  backlight brightness  range */


    
    if(value >=30)
        next = 70+(value*(drvdata->max_intensity-77)/ (LED_FULL- 30));

    else
	next = 4*value * drvdata->max_intensity / LED_FULL;

	
	dprintk("android input brightness =%d]\n", value);

	if (brightness != next) {
		dprintk("brightness[current=%d, next=%d]\n", brightness, next);
		lm3528_send_intensity(drvdata, next);
	}
}

static struct led_classdev lm3528_led_dev = {
	.name = LEDS_BACKLIGHT_NAME,
	.brightness_set = leds_brightness_set,
};
#endif

static int lm3528_probe(struct i2c_client *i2c_dev, const struct i2c_device_id *i2c_dev_id)
{
	dprintk("+++++++Jihye++++\n");

	struct backlight_platform_data *pdata;
	struct lm3528_driver_data *drvdata;
	struct backlight_device *bd;
	struct backlight_properties props;
	int err;


	printk("%s start, client addr=0x%x\n", __func__, i2c_dev->addr);

	drvdata = kzalloc(sizeof(struct lm3528_driver_data), GFP_KERNEL);
	if (!drvdata) {
		dev_err(&i2c_dev->dev, "failed to allocate memory\n");
		return -ENOMEM;
	}

	pdata = i2c_dev->dev.platform_data;
	/* jihye.ahn 10-10-12  WBT fix TD157993
	if(pdata && pdata->platform_init)
		pdata->platform_init();
	*/
	drvdata->client = i2c_dev;
	drvdata->gpio = pdata->gpio;
	drvdata->init_on_boot = pdata->init_on_boot;
	drvdata->max_intensity = LCD_LED_MAX;
	if(pdata->max_current > 0)
		drvdata->max_intensity = pdata->max_current;
	drvdata->intensity = LCD_LED_MIN;
	drvdata->state = UNINIT_STATE;

	if(drvdata->gpio && gpio_request(drvdata->gpio, "lm3528 reset") != 0) {
		dprintk("Error while requesting gpio %d\n", drvdata->gpio);
		kfree(drvdata);
		return -ENODEV;
	}

	bd = backlight_device_register("lm3528-bl", &i2c_dev->dev, NULL, &lm3528_ops,&props);
	if (bd == NULL) {
		dprintk("entering lm3528_bl probe function error \n");
		if(gpio_is_valid(drvdata->gpio))
			gpio_free(drvdata->gpio);
		kfree(drvdata);
		return -1;
	}
	bd->props.power = FB_BLANK_UNBLANK;
	bd->props.brightness = drvdata->intensity;
	bd->props.max_brightness = drvdata->max_intensity;
	drvdata->bd = bd;
/* jihye.ahn 10.08.11
	Register backlight driver to LED class
*/
// BEGIN : munho.lee@lge.com 2010-10-26
// MOD :0010205: [Display] To change resolution for removing display noise. 
atomic_set(&lcd_event_handled,1);
atomic_set(&lcd_bootup_handle,1);
// END : munho.lee@lge.com 2010-10-26

#ifdef CONFIG_BACKLIGHT_LEDS_CLASS
        if(led_classdev_register(&i2c_dev->dev, &lm3528_led_dev) == 0) {
                dprintk("Registering led class dev successfully.\n");
                drvdata->led = &lm3528_led_dev;
        }
#endif

	i2c_set_clientdata(i2c_dev, drvdata);
	/* jihye.ahn  2010-12-10 fix backlight on after power off */
	i2c_set_adapdata(i2c_dev->adapter, i2c_dev);

	lm3528_init(drvdata);
	lm3528_send_intensity(drvdata, drvdata->max_intensity);

#ifdef CONFIG_HAS_EARLYSUSPEND
	drvdata->early_suspend.suspend = lm3528_early_suspend;
	drvdata->early_suspend.resume = lm3528_late_resume;
//	drvdata->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN - 40;
	drvdata->early_suspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB-3;
	register_early_suspend(&drvdata->early_suspend);
#endif

	dprintk("+++++++LM3528 done\n");
	return 0;

probe_free_exit:
	kfree(drvdata);
	i2c_set_clientdata(i2c_dev, NULL);	
	return err;	
}

static int __devexit lm3528_remove(struct i2c_client *i2c_dev)
{
	struct lm3528_driver_data *drvdata = i2c_get_clientdata(i2c_dev);

	lm3528_send_intensity(drvdata, 0);

	backlight_device_unregister(drvdata->bd);
	led_classdev_unregister(drvdata->led);
	i2c_set_clientdata(i2c_dev, NULL);
	if (gpio_is_valid(drvdata->gpio))
		gpio_free(drvdata->gpio);
	kfree(drvdata);

	return 0;
}
/* jihye.ahn    2010-12-16  add shutdown I/F */
static int lm3528_shutdown(struct i2c_client *i2c_dev)
{
           struct lm3528_driver_data *drvdata = i2c_get_clientdata(i2c_dev);

           printk("shutdown lm3528\n");
           lm3528_write(drvdata->client, 0x10, 0x0);  // LM3528 Disabled

           return 0;
}

static const struct i2c_device_id lm3528_ids[] = {
	{ MODULE_NAME, 0 },
	{ },		
};
MODULE_DEVICE_TABLE(i2c, lm3528_ids); /*jihye.ahn  10.08.11 fixed typo*/

static struct i2c_driver lm3528_driver = {
	.driver.name	= MODULE_NAME,
	.id_table	= lm3528_ids,		
	.probe 	= lm3528_probe,
	.remove 	= lm3528_remove,	
         .shutdown = lm3528_shutdown,   
#ifndef CONFIG_HAS_EARLYSUSPEND
	.suspend 	= lm3528_suspend,
	.resume 	= lm3528_resume,
#endif
};

static int __init lm3528_drv_init(void)
{
	int rc = i2c_add_driver(&lm3528_driver);
	pr_notice("%s: i2c_add_driver: rc = %d\n", __func__, rc);
	return rc;
}

static void __exit lm3528_drv_exit(void)
{
	i2c_del_driver(&lm3528_driver);
}
module_init(lm3528_drv_init);
module_exit(lm3528_drv_exit);


MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("LM3528 driver");
MODULE_VERSION("1.0");
MODULE_ALIAS("platform:LM3528");
