#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/backlight.h>
#include <linux/fb.h>
#include <linux/i2c.h>
#include <linux/string.h>
#include <asm/gpio.h>
#include <asm/system.h>
#include <mach/board-bryce.h>
//#include "bh6172_pm.h"

#define BH6172_I2C_NAME  "bh6172"
#define BH6172_I2C_ADDR	 0x9E >> 1


static u8 bh6172_init_data[9] ={
    //0x00, 0x3F, 0x08, 0xC8, 0xCE, 0x06, 0x00, 0x3F, 0x0F //default on  
    // 0h       0h      1h      2h      3h       4h     5h      6h     7h
 #if defined(LG_HW_REV2)
    0x00, 0x00, 0x08, 0xCC, 0xCE, 0x06, 0x00, 0x3F, 0x0F //default off - EVB2
 #else
    0x00, 0x00, 0x08, 0xC8, 0xCE, 0x06, 0x00, 0x3F, 0x0F //default off - EVB1,REV.A
 #endif          
};

static unsigned char bh6172_output_status = 0x00; //default on 0x3F
struct i2c_client *bh6172_client = NULL;

static void bh6172_write_reg(struct i2c_client *client, u8* buf, u16 num)
{
	int err;
	
	struct i2c_msg	msg = {	
		client->addr, 0, num, buf 
	};
	
	if ((err = i2c_transfer(client->adapter, &msg, 1)) < 0) {
		dev_err(&client->dev, "i2c write error [%d]\n",err);
	}
	
	return;
}

void bh6172_set_output(int outnum, int onoff)
{
    if(onoff == 0)
	    bh6172_output_status &= ~(1<<outnum);
    else
	    bh6172_output_status |= (1<<outnum);
	
}

EXPORT_SYMBOL(bh6172_set_output);

void bh6172_output_enable(void)
{
    u8 buf[2];

    if(bh6172_client == NULL)
		return;
	
	buf[0] = 0x00 | 0x80; //auto increment off
	buf[1] = bh6172_output_status & 0x3F;
	
    bh6172_write_reg(bh6172_client, &buf[0], 2);
}

EXPORT_SYMBOL(bh6172_output_enable);

static void bh6172_init(struct i2c_client *client)
{
	unsigned char val = 0;

    bh6172_write_reg(client, &bh6172_init_data[0], sizeof(bh6172_init_data));

	return;
}

//static int __init bh6172_probe(struct i2c_client *client, const struct i2c_device_id *id)
static int __devinit bh6172_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct bh6172_platform_data *pdata;

	if (i2c_get_clientdata(client))
		return -EBUSY;
	
	bh6172_client = client;

	pdata = client->dev.platform_data;

    pdata->subpm_reset(pdata->gpio_reset);

	udelay(10);

	bh6172_init(client);
     

    return 0;
	
}

static int bh6172_remove(struct i2c_client *client)
{
	return 0;
}	

static const struct i2c_device_id bh6172_ids[] = {
	{ BH6172_I2C_NAME, 0 },	/*bh6172*/
	{ /* end of list */ },
};

static struct i2c_driver subpm_bh6172_driver = {
	.probe 	  = bh6172_probe,
	.remove   = bh6172_remove,
	.id_table = bh6172_ids,
	.driver   = {
		.name =  BH6172_I2C_NAME,
		.owner= THIS_MODULE,
    },
};
static int __init subpm_bh6172_init(void)
{
	printk("..........subpm_bh6172_init\n");

 	return i2c_add_driver(&subpm_bh6172_driver);
}

static void __exit subpm_bh6172_exit(void)
{
	i2c_del_driver(&subpm_bh6172_driver);
}

module_init(subpm_bh6172_init);
module_exit(subpm_bh6172_exit);

MODULE_AUTHOR("LG Electronics");
MODULE_DESCRIPTION("BH6172 sub pmic Driver");
MODULE_LICENSE("GPL");

