/*
 * drivers/media/video/msm/lm3559_flash.c
 *
 * Flash (LM3559) driver
 *
 * Copyright (C) 2010 LGE, Inc.
 *
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
 */
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/hrtimer.h>
#include <../../../drivers/staging/android/timed_output.h>
#include <linux/leds.h>
#include <linux/gpio.h>
#include <mach/board-bryce.h>
#include <mach/camera.h>
#include <mach/lg_comdef.h>


/* Register Descriptions */
#define LM3559_REG_ENABLE				0x10
#define LM3559_REG_GPIO					0x20
#define LM3559_REG_VLED_MONITOR			0x30
#define LM3559_REG_ADC_DELAY			0x31
#define LM3559_REG_VIN_MONITOR			0x80
#define LM3559_REG_LAST_FLASH			0x81
#define LM3559_REG_TORCH_BRIGHTNESS		0xA0
#define LM3559_REG_FLASH_BRIGHTNESS		0xB0
#define LM3559_REG_FLASH_DURATION		0xC0
#define LM3559_REG_FLAGS				0xD0
#define LM3559_REG_CONFIGURATION1		0xE0
#define LM3559_REG_CONFIGURATION2		0xF0
#define LM3559_REG_PRIVACY				0x11
#define LM3559_REG_MESSAGE_INDICATOR	0x12
#define LM3559_REG_INDICATOR_BLINKING	0x13
#define LM3559_REG_PRIVACY_PWM			0x14

#define LM3559_I2C_NAME  "lm3559"

#define LM3559_POWER_OFF				0
#define LM3559_POWER_ON					1

enum{
   LM3559_LED_OFF,
   LM3559_LED_LOW,
   LM3559_LED_NORMAL,
   LM3559_LED_HIGH,
   LM3559_LED_MAX
}; 

enum {
  LM3559_LED_MODE_AUTO,
  LM3559_LED_MODE_OFF,
  LM3559_LED_MODE_ON,
  LM3559_LED_MODE_MAX
};

enum{
   LM3559_STATE_OFF,
   LM3559_STATE_ON_STROBE,
   LM3559_STATE_ON_TORCH,
   LM3559_STATE_ON_TORCH_PREFLASH, 
};

#ifdef CONFIG_MACH_LGE_BRYCE

enum{
   FLASH_EN_GPIO_NUMBER_REV_D = 84,
   FLASH_EN_GPIO_NUMBER	= 31,	
};
#if 1 // [MS910_GB] sungmin.cho@lge.com
extern byte CheckHWRev();
#endif
#endif

struct led_flash_platform_data *lm3559_pdata = NULL;
struct i2c_client *lm3559_client = NULL;
static int lm3559_onoff_state = 0;

static int32_t lm3559_write_reg(struct i2c_client *client, unsigned char* buf, int length)
{
	int32_t err = 0;
	
	struct i2c_msg	msg[] = {
		{
			.addr  = client->addr, 
			.flags = 0, 
			.len   = length, 
			.buf   = buf, 
		},
	};
	
	if ((err = i2c_transfer(client->adapter, &msg[0], 1)) < 0) {
		dev_err(&client->dev, "i2c write error [%d]\n",err);
	}
	
	return err;
}

static int32_t lm3559_i2c_write(struct i2c_client *client,unsigned char addr, unsigned char data)
{
	unsigned char buf[2] ={0,};
	int32_t rc = -EIO;

	if(client == NULL)
		return rc;


	buf[0] = addr;
	buf[1] = data;

	rc = lm3559_write_reg(client,&buf[0],2);

	return rc;
}

static int32_t lm3559_read_reg(struct i2c_client *client, unsigned char* buf, int length)
{
	int32_t err = 0;
	
	struct i2c_msg	msgs[] = {	
		{ 
			.addr  = client->addr, 
			.flags = 0, 
			.len   = 2,
			.buf   = buf, 
		},
		{ 
			.addr  = client->addr, 
			.flags = I2C_M_RD, 
			.len   = length,
			.buf   = buf, 
		},
	};
	
	if ((err = i2c_transfer(client->adapter, msgs, 1)) < 0) {
		dev_err(&client->dev, "i2c write error [%d]\n",err);
	}
	
	return err;
	
}

static int32_t lm3559_i2c_read(struct i2c_client *client,unsigned char addr, unsigned char *data)
{
	unsigned char buf[2] ={0,};
	int32_t rc = -EIO;

	if((client == NULL)||(data == NULL))
		return rc;


	buf[0] = addr;

	rc = lm3559_read_reg(client,&buf[0],1);
    if(rc < 0)
		return rc;

	*data = buf[0];
	
	return rc;

}

void lm3559_led_shutdown(void)
{	
	lm3559_i2c_write(lm3559_client,LM3559_REG_ENABLE,0x18);
}

/*	Torch Current
	 000 : 28.125 mA		100 : 140.625 mA	 
	 001 : 56.25 mA 		101 : 168.75 mA
	 010 : 84.375 mA 		110 : 196.875 mA
	 011 : 112.5mA  		111 : 225 mA
*/
void lm3559_enable_torch_mode(int state)
{
	unsigned char data = 0;

	
    if(state == LM3559_LED_LOW){
		/* 001 : 56.25  mA  */
		lm3559_i2c_write(lm3559_client,LM3559_REG_TORCH_BRIGHTNESS,0x09);
	}
	if(state == LM3559_LED_NORMAL){
		/* 100 : 140.625 mA  */
		lm3559_i2c_write(lm3559_client,LM3559_REG_TORCH_BRIGHTNESS,0x24);
	}
	else{
		/* 111 : 225 mA  */ 
		lm3559_i2c_write(lm3559_client,LM3559_REG_TORCH_BRIGHTNESS,0x3F);
	}

	lm3559_i2c_write(lm3559_client,LM3559_REG_ENABLE,0x0A);

}

/*	 Flash Current
	 0000 : 56.25 mA		1000 : 506.25 mA	 
	 0001 : 112.5 mA 		1001 : 562.5 mA
	 0010 : 168.75 mA 	1010 : 618.75 mA
	 0011 : 225 mA  		1011 : 675 mA
	 0100 : 281.25 mA		1100 : 731.25 mA
	 0101 : 337.5 mA		1101 : 787.5 mA
	 0110 : 393.75 mA		1110 : 843.75 mA
	 0111 : 450 mA		1111 : 900 mA
*/
void lm3559_enable_flash_mode(int state)
{
	unsigned char data = 0;

	lm3559_i2c_read(lm3559_client,LM3559_REG_FLASH_DURATION,&data);	

	printk("%s: before - LM3559_REG_FLASH_DURATION[0x%x]\n",__func__,data);

	data = ((data & 0x1F) | 0x1F);

	printk("%s: LM3559_REG_FLASH_DURATION[0x%x]\n",__func__,data);
	
	lm3559_i2c_write(lm3559_client,LM3559_REG_FLASH_DURATION,data);
			 
	if(state == LM3559_LED_LOW){ 		
		/* 0001 : 112.5 mA*/
		lm3559_i2c_write(lm3559_client,LM3559_REG_FLASH_BRIGHTNESS,0x11);
	}
	else{
		/* 1001 : 562.5 mA*/	 
		lm3559_i2c_write(lm3559_client,LM3559_REG_FLASH_BRIGHTNESS,0x99);
	}

	lm3559_i2c_write(lm3559_client,LM3559_REG_ENABLE,0x1B);
	

}

void lm3559_power_onoff(int onoff){
	
	if(onoff == LM3559_POWER_OFF)
		gpio_set_value(lm3559_pdata->gpio_flen, 0);		
	else
		gpio_set_value(lm3559_pdata->gpio_flen, 1);		
	
}

int lm3559_flash_set_led_state(int state)
{	
	int rc = 0;
	
	switch (state) {
	case LM3559_STATE_OFF:
		printk("[LM3559]LM3559_STATE_OFF\n");
		lm3559_power_onoff(LM3559_POWER_OFF);
		lm3559_onoff_state = LM3559_POWER_OFF;
		break;
	case LM3559_STATE_ON_STROBE:
		printk("[LM3559] LM3559_STATE_ON_STROBE\n");
		if(lm3559_onoff_state == LM3559_POWER_OFF){	
			lm3559_power_onoff(LM3559_POWER_ON);
			lm3559_enable_flash_mode(LM3559_LED_HIGH);
			lm3559_onoff_state = LM3559_POWER_ON;
		}
		break;
	case LM3559_STATE_ON_TORCH:	 // video
		printk("[LM3559]LM3559_STATE_ON_TORCH\n");
		if(lm3559_onoff_state == LM3559_POWER_OFF){	
			lm3559_power_onoff(LM3559_POWER_ON);
			lm3559_enable_torch_mode(LM3559_LED_NORMAL);
			lm3559_onoff_state = LM3559_POWER_ON;
		}
		break;

	/* [MS910] sungmin.cho@lge.com 2011.03.31 
	 * It would be used for only snapshot mode.
	 */
	case LM3559_STATE_ON_TORCH_PREFLASH:
		printk("[LM3559]LM3559_STATE_ON_TORCH_PREFLASH\n");
		if(lm3559_onoff_state == LM3559_POWER_OFF){	
			lm3559_power_onoff(LM3559_POWER_ON);
			lm3559_enable_torch_mode(LM3559_LED_HIGH);
			lm3559_onoff_state = LM3559_POWER_ON;
		}
		break;
	default:
		lm3559_onoff_state = LM3559_POWER_OFF;
		rc = -EFAULT;
		break;
	}
	
	

	return rc;
	
}

EXPORT_SYMBOL(lm3559_flash_set_led_state);


static void lm3559_flash_led_set(struct led_classdev *led_cdev,
	enum led_brightness value)
{
	led_cdev->brightness = value;

    printk("%s:led_cdev->brightness[%d]\n",value);
	
    if(value)
    {
		lm3559_power_onoff(LM3559_POWER_ON);
		lm3559_enable_torch_mode(LM3559_LED_HIGH);
    }
    else
	lm3559_power_onoff(LM3559_POWER_OFF);
		
}

static struct led_classdev lm3559_flash_led = {
	.name			= "spotlight",
	.brightness_set	= lm3559_flash_led_set,
};

static int __devinit lm3559_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	byte check_rev = 0;
	int rc = 0;
	
	if (i2c_get_clientdata(client))
		return -EBUSY;
	
	lm3559_client = client;
	lm3559_pdata = client->dev.platform_data;	

	led_classdev_register(&client->dev, &lm3559_flash_led);
	

#ifdef CONFIG_MACH_LGE_BRYCE
	lm3559_pdata->gpio_flen = FLASH_EN_GPIO_NUMBER;
#endif

	printk("%s: check_rev[0x%x] gpio_flen[%d]\n",__func__,check_rev,lm3559_pdata->gpio_flen);

#ifdef CONFIG_MACH_LGE_BRYCE
	gpio_request(lm3559_pdata->gpio_flen, "cam_flash_en");
#endif

	gpio_tlmm_config(GPIO_CFG(lm3559_pdata->gpio_flen, 0, GPIO_CFG_OUTPUT,
				GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_set_value(lm3559_pdata->gpio_flen, 0);
	
	return rc;
	
}

static int lm3559_remove(struct i2c_client *client)
{
	return 0;
}	

static const struct i2c_device_id lm3559_ids[] = {
	{ LM3559_I2C_NAME, 0 },	/* lm3559 */
	{ /* end of list */ },
};

static struct i2c_driver lm3559_driver = {
	.probe 	  = lm3559_probe,
	.remove   = lm3559_remove,
	.id_table = lm3559_ids,
	.driver   = {
		.name =  LM3559_I2C_NAME,
		.owner= THIS_MODULE,
    },
};
static int __init lm3559_init(void)
{
    return i2c_add_driver(&lm3559_driver);
}

static void __exit lm3559_exit(void)
{
	i2c_del_driver(&lm3559_driver);
}

module_init(lm3559_init);
module_exit(lm3559_exit);

MODULE_AUTHOR("LG Electronics");
MODULE_DESCRIPTION("LM3559 Flash Driver");
MODULE_LICENSE("GPL");

