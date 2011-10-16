/*
 *  apds9900.c - Linux kernel modules for ambient light + proximity sensor
 *
 *  Copyright (C) 2010 Lee Kai Koon <kai-koon.lee@avagotech.com>
 *  Copyright (C) 2010 Avago Technologies
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/input.h>

#include <linux/ioctl.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>

/* sungmin.shin@lge.com 10.08.06
	sensor interrupt handler
*/
#ifdef CONFIG_MACH_LGE_BRYCE
#include <mach/board-bryce.h>
#include <linux/interrupt.h>
#include <mach/vreg.h>
#include <mach/gpio.h>	
#endif 

#define APDS9900_DRV_NAME	"apds9900"
#define DRIVER_VERSION		"1.0.3"
#define APDS9900_ALS_THRESHOLD_HSYTERESIS	20	/* 20 = 20% */

/*
 * Defines
 */

#define APDS9900_ENABLE_REG	0x00
#define APDS9900_ATIME_REG	0x01
#define APDS9900_PTIME_REG	0x02
#define APDS9900_WTIME_REG	0x03
#define APDS9900_AILTL_REG	0x04
#define APDS9900_AILTH_REG	0x05
#define APDS9900_AIHTL_REG	0x06
#define APDS9900_AIHTH_REG	0x07
#define APDS9900_PILTL_REG	0x08
#define APDS9900_PILTH_REG	0x09
#define APDS9900_PIHTL_REG	0x0A
#define APDS9900_PIHTH_REG	0x0B
#define APDS9900_PERS_REG	0x0C
#define APDS9900_CONFIG_REG	0x0D
#define APDS9900_PPCOUNT_REG	0x0E
#define APDS9900_CONTROL_REG	0x0F
#define APDS9900_REV_REG	0x11
#define APDS9900_ID_REG		0x12
#define APDS9900_STATUS_REG	0x13
#define APDS9900_CDATAL_REG	0x14
#define APDS9900_CDATAH_REG	0x15
#define APDS9900_IRDATAL_REG	0x16
#define APDS9900_IRDATAH_REG	0x17
#define APDS9900_PDATAL_REG	0x18
#define APDS9900_PDATAH_REG	0x19

#define CMD_BYTE	0x80
#define CMD_WORD	0xA0
#define CMD_SPECIAL	0xE0

#define CMD_CLR_PS_INT	0xE5
#define CMD_CLR_ALS_INT	0xE6
#define CMD_CLR_PS_ALS_INT	0xE7

#define ALS_SENSOR_NAME "apds9900"
#define PROX_INT_HIGH_TH	(0x70) // near/far interrupt threahold
#define PROX_INT_LOW_TH	(0x0) // 

/*
 * Structs
 */

struct apds9900_data {
	struct i2c_client *client;
	struct mutex update_lock;

	unsigned int enable;
	unsigned int atime;
	unsigned int ptime;
	unsigned int wtime;
	unsigned int ailt;
	unsigned int aiht;
	unsigned int pilt;
	unsigned int piht;
	unsigned int pers;
	unsigned int config;
	unsigned int ppcount;
	unsigned int control;
/* kwangdo.yi@lge.com S 10.09.15 */
	/* PS parameters */
	unsigned int ps_threshold;
	unsigned int ps_detection;		/* 0 = near-to-far; 1 = far-to-near */
	unsigned int ps_data;			/* to store PS data */

	/* ALS parameters */
	unsigned int als_threshold_l;	/* low threshold */
	unsigned int als_threshold_h;	/* high threshold */
	unsigned int als_data;			/* to store ALS data */
	unsigned int cdata_l;
	unsigned int cdata_h;
	unsigned int irdata_l;
	unsigned int irdata_h;

/* kwangdo.yi@lge.com E 10.09.15 */
	
	/* sungmin.shin@lge.com 10.08.06
		sensor interrupt handler
	*/
#ifdef CONFIG_MACH_LGE_BRYCE	
	struct delayed_work poswork;
	int irq;
#endif
/* kwangdo.yi@lge.com 10.09.17
    added input event
    */
#ifdef CONFIG_MACH_LGE_BRYCE    
	struct input_dev *input_dev;
#endif
/* kwangdo.yi@lge.com E */
};
/* kwangdo.yi@lge.com 10.09.30 S 
  * 0009611: change light sensor to report event of light
*/
#ifdef CONFIG_MACH_LGE_BRYCE
#define ABS_LIGHT		0x29
#endif
/* kwangdo.yi@lge.com 10.09.30 E */


/* kwangdo.yi@lge.com S 10.09.15 */
#ifdef CONFIG_MACH_LGE_BRYCE
struct apds9900_data *papds9900_data;

/*
 * Global data
 */

/*
 * Management functions
 */

static void apds9900_change_ps_threshold(struct i2c_client *client)
{
	struct apds9900_data *data = i2c_get_clientdata(client);

	data->ps_data =	i2c_smbus_read_word_data(client, CMD_WORD|APDS9900_PDATAL_REG);

	if ( (data->ps_data > data->pilt) && (data->ps_data >= data->piht) ) {
		/* far-to-near detected */
		data->ps_detection = 1;
		
/* kwangdo.yi@lge.com 10.09.17 S
  * add input event
  */
		input_report_abs(data->input_dev, ABS_DISTANCE, 3); // fix to 3cm when near 
		input_sync(data->input_dev);
       
/* kwangdo.yi@lge.com E */

		i2c_smbus_write_word_data(client, CMD_WORD|APDS9900_PILTL_REG, data->ps_threshold);
		i2c_smbus_write_word_data(client, CMD_WORD|APDS9900_PIHTL_REG, 1023);

		data->pilt = data->ps_threshold;
		data->piht = 1023;

		set_irq_wake(data->irq, 1);
		printk("### far-to-near detected\n");
	}
	else if ( (data->ps_data <= data->pilt) && (data->ps_data < data->piht) ) {
		/* near-to-far detected */
		data->ps_detection = 0;

/* kwangdo.yi@lge.com 10.09.17 S
  * add input event
  */
		input_report_abs(data->input_dev, ABS_DISTANCE, 10); // fix to 10cm when far
		input_sync(data->input_dev);			
/* kwangdo.yi@lge.com E */

		i2c_smbus_write_word_data(client, CMD_WORD|APDS9900_PILTL_REG, 0);
		i2c_smbus_write_word_data(client, CMD_WORD|APDS9900_PIHTL_REG, data->ps_threshold);

		data->pilt = 0;
		data->piht = data->ps_threshold;
		set_irq_wake(data->irq, 0);

		printk("### near-to-far detected\n");
	}
}

static void apds9900_change_als_threshold(struct i2c_client *client)
{
	struct apds9900_data *data = i2c_get_clientdata(client);
    	
        /*  LGE_S   jihye.ahn   2010-12-18  fixed LUX level */
         int ch0data, ch1data, iac1, iac2, iac;
         u16 cdata1, irdata1;
       
         mutex_lock(&data->update_lock);         
        data->cdata_l = i2c_smbus_read_word_data(client, CMD_WORD|APDS9900_CDATAL_REG);
        data->irdata_l = i2c_smbus_read_word_data(client, CMD_WORD|APDS9900_IRDATAL_REG);
         mutex_unlock(&data->update_lock);
   
        cdata1 = ((data->cdata_l)&0xff00) >> 8;
        irdata1 =((data->irdata_l)&0xff00) >> 8;

        ch0data = 256*cdata1 + ((data->cdata_l)&0x00ff);
        ch1data = 256*irdata1 + ((data->irdata_l)&0x00ff);
     
         iac1 = ch0data -((197*ch1data)/100);
         iac2 = (ch0data*9)/10 - (ch1data*175)/100;

         if((iac1 < 0)&&(iac2<0)) iac = 0;
         else if(iac1 > iac2) iac = iac1;
		 else iac = iac2;

        data->als_data =(unsigned int)((iac*2)/3);
     
          //printk("[LUX] als_data = %d\n",data->als_data);
          
	/* kwangdo.yi@lge.com 10.09.30 S
	  * 0009611: change light sensor to report event of light
	  */
		printk("### apds9900  data->als_data = %d\n", data->als_data); 
		input_report_abs(data->input_dev, ABS_LIGHT, data->als_data); //report als data
		input_sync(data->input_dev);			
	/* kwangdo.yi@lge.com E */
      
		data->als_threshold_l = (ch0data * (100-15) ) /100;
		data->als_threshold_h = (ch0data * (100+15) ) /100;
        /*  LGE_E   jihye.ahn   2010-12-18  fixed LUX level */ 
        
        i2c_smbus_write_word_data(client, CMD_WORD|APDS9900_AILTL_REG, data->als_threshold_l);
        i2c_smbus_write_word_data(client, CMD_WORD|APDS9900_AIHTL_REG, data->als_threshold_h);

}

static void apds9900_reschedule_work(struct apds9900_data *data,
					  unsigned long delay)
{
	unsigned long flags;

	spin_lock_irqsave(&data->update_lock, flags);

	/*
	 * If work is already scheduled then subsequent schedules will not
	 * change the scheduled time that's why we have to cancel it first.
	 */
	__cancel_delayed_work(&data->poswork);
	schedule_delayed_work(&data->poswork, delay);

	spin_unlock_irqrestore(&data->update_lock, flags);
}


static void apds9900_event_work(struct work_struct *work)
{
	struct apds9900_data *data = container_of(work, struct apds9900_data, poswork.work);
	struct i2c_client *client=data->client;
	int	status, pdataL, pdataH;
	int pdata, cdata;

	status = i2c_smbus_read_byte_data(client, CMD_BYTE|APDS9900_STATUS_REG);

	i2c_smbus_write_byte_data(client, CMD_BYTE|APDS9900_ENABLE_REG, 1);	/* disable 9900's ADC first */

/*	printk("### status = %x\n", status);
	pdataL = i2c_smbus_read_word_data(data->client, CMD_WORD|APDS9900_PDATAL_REG);
	pdataH = i2c_smbus_read_word_data(data->client, CMD_WORD|APDS9900_PDATAH_REG);
	printk("### pdataL = %d, pdataH = %d \n", pdataL, pdataH);*/

	if ((status & data->enable & 0x30) == 0x30) {
		/* both PS and ALS are interrupted */
		cdata = i2c_smbus_read_word_data(client, CMD_WORD|APDS9900_CDATAL_REG);

		printk(KERN_INFO "%s, [APDS9900_STATUS_PINT_AINT] status : %d,   cdata : %d, isNear : %d\n",__func__, status, cdata, data->ps_detection);

		// Non Strong Light, Normal Operation
		if(cdata < (75*(1024*(256-data->atime)))/100)
		{
			apds9900_change_als_threshold(client);
			apds9900_change_ps_threshold(client);
		}
		else if((data->ps_detection == 1) && (cdata >= (75*(1024*(256-data->atime)))/100) )
		{
			// Report Far
			data->ps_detection = 0;

			input_report_abs(data->input_dev, ABS_DISTANCE, 10); // fix to 10cm when far
			input_sync(data->input_dev);


			i2c_smbus_write_word_data(client, CMD_WORD|APDS9900_PILTL_REG, 0);
			i2c_smbus_write_word_data(client, CMD_WORD|APDS9900_PIHTL_REG, data->ps_threshold);

			data->pilt = 0;
			data->piht = data->ps_threshold;
			apds9900_change_als_threshold(client);
		}
		else if((data->ps_detection == 0) && (cdata >= (75*(1024*(256-data->atime)))/100) )
		{
			// Only Operate ALS because of strong light
			apds9900_change_als_threshold(client);
		}
		else
		{
			apds9900_change_als_threshold(client);
			apds9900_change_ps_threshold(client);  	
		}
		
                  
		i2c_smbus_write_byte(client, CMD_CLR_PS_ALS_INT);
	}
	else if ((status & data->enable & 0x20) == 0x20) {
		/* only PS is interrupted */
		cdata = i2c_smbus_read_word_data(client, CMD_WORD|APDS9900_CDATAL_REG);

		// Normal state
		if(cdata < (75*(1024*(256-data->atime)))/100)
		{
			apds9900_change_ps_threshold(client);
		}
		//When state of proximity is near. change state to Far from Near.
		else if((data->ps_detection == 1) && (cdata >= (75*(1024*(256-data->atime)))/100) )
		{
			// Report Far
			data->ps_detection = 0;

			input_report_abs(data->input_dev, ABS_DISTANCE, 10); // fix to 10cm when far
			input_sync(data->input_dev);


			i2c_smbus_write_word_data(client, CMD_WORD|APDS9900_PILTL_REG, 0);
			i2c_smbus_write_word_data(client, CMD_WORD|APDS9900_PIHTL_REG, data->ps_threshold);

			data->pilt = 0;
			data->piht = data->ps_threshold;

		}
		// Strong light, Not Report Proximity State.
		else if((data->ps_detection == 0) && (cdata >= (75*(1024*(256-data->atime)))/100) )
		{
			pdata = i2c_smbus_read_word_data(client, CMD_WORD|APDS9900_PDATAL_REG);
			printk(KERN_INFO "%s, [APDS9900_STATUS_PINT] cdata : %d, pdata : %d\n", __func__, cdata, pdata);
		}
		else
		{
			apds9900_change_ps_threshold(client);
		}

		i2c_smbus_write_byte(client, CMD_CLR_PS_INT);

	}
	else if ((status & data->enable & 0x10) == 0x10) {
		/* only ALS is interrupted */	
		apds9900_change_als_threshold(client);
		i2c_smbus_write_byte(client, CMD_CLR_ALS_INT);
	}

	i2c_smbus_write_byte_data(client, CMD_BYTE|APDS9900_ENABLE_REG, data->enable);	
	    
}
#else
 /* sungmin.shin@lge.com 10.08.06
		sensor interrupt handler
	*/
#ifdef CONFIG_MACH_LGE_BRYCE	
#define PROX_INT_LOW_TH	(0x6F) // far interrupt threshold
#define PROX_INT_HIGH_TH	(0x70) // near interrupt threahold
#endif

/* sungmin.shin@lge.com 10.08.06
	sensor interrupt handler
	pdata is inversely proportional to light strength. if the room is brigher, the pdata value is smaller.
	Note! Interrupt will be generated every about 400ms when it is out of pilt and piht boundary.
*/
#define EVENT_SAMPLING_RATE	(HZ/30) // HZ/30-> Real interrupt rate: 18HZ (max: HZ/60)
static void apds9900_event_work(struct work_struct *work)
{
	struct apds9900_data *data = container_of(work, struct apds9900_data, poswork.work);
	int pdata, cdata, irdata;	
	
	mutex_lock(&data->update_lock);
	pdata = i2c_smbus_read_word_data(data->client, CMD_WORD|APDS9900_PDATAL_REG);
	cdata = i2c_smbus_read_word_data(data->client, CMD_WORD|APDS9900_CDATAL_REG);
	irdata = i2c_smbus_read_word_data(data->client, CMD_WORD|APDS9900_IRDATAL_REG);
	i2c_smbus_write_byte(data->client, CMD_CLR_PS_INT);
	mutex_unlock(&data->update_lock);
	/* kwangdo.yi@lge.com 10.08.31
		added to inform LgHiddenMenu of Near/Far
		*/
#ifdef CONFIG_MACH_LGE_BRYCE   
	if(pdata > PROX_INT_HIGH_TH) // near intr
		data->IsNear = 1; 
	else data->IsNear = 0; // far intr,  pdata<= PROX_INT_LOW_TH
#endif
	printk("### APDS interrupt! -> pdata : %x, cdata:%x, irdata:%x\n", pdata, cdata, irdata);

	return;
}
#endif 
/* kwangdo.yi@lge.com E 10.09.15 */

static irqreturn_t apds9900_isr(int irq, void *dev_id)
{
	struct apds9900_data *data = dev_id;
	printk("### apds9900_interrupt\n");
	apds9900_reschedule_work(data, 0);	

	return IRQ_HANDLED;
}


/*
 * Global data
 */

/*
 * Management functions
 */
 
static int apds9900_set_command(struct i2c_client *client, int command)
{
	struct apds9900_data *data = i2c_get_clientdata(client);
	int ret;
	int clearInt;

	if (command == 0)
		clearInt = CMD_CLR_PS_INT;
	else if (command == 1)
		clearInt = CMD_CLR_ALS_INT;
	else
		clearInt = CMD_CLR_PS_ALS_INT;
		
	mutex_lock(&data->update_lock);
	ret = i2c_smbus_write_byte(client, clearInt);
	mutex_unlock(&data->update_lock);

	return ret;
}

static int apds9900_set_enable(struct i2c_client *client, int enable)
{
	struct apds9900_data *data = i2c_get_clientdata(client);
	int ret;

	mutex_lock(&data->update_lock);
	ret = i2c_smbus_write_byte_data(client, CMD_BYTE|APDS9900_ENABLE_REG, enable);
	mutex_unlock(&data->update_lock);

	data->enable = enable;

	return ret;
}

static int apds9900_set_atime(struct i2c_client *client, int atime)
{
	struct apds9900_data *data = i2c_get_clientdata(client);
	int ret;
	
	mutex_lock(&data->update_lock);
	ret = i2c_smbus_write_byte_data(client, CMD_BYTE|APDS9900_ATIME_REG, atime);
	mutex_unlock(&data->update_lock);

	data->atime = atime;

	return ret;
}

static int apds9900_set_ptime(struct i2c_client *client, int ptime)
{
	struct apds9900_data *data = i2c_get_clientdata(client);
	int ret;
	
	mutex_lock(&data->update_lock);
	ret = i2c_smbus_write_byte_data(client, CMD_BYTE|APDS9900_PTIME_REG, ptime);
	mutex_unlock(&data->update_lock);

	data->ptime = ptime;

	return ret;
}

static int apds9900_set_wtime(struct i2c_client *client, int wtime)
{
	struct apds9900_data *data = i2c_get_clientdata(client);
	int ret;
	
	mutex_lock(&data->update_lock);
	ret = i2c_smbus_write_byte_data(client, CMD_BYTE|APDS9900_WTIME_REG, wtime);
	mutex_unlock(&data->update_lock);

	data->wtime = wtime;

	return ret;
}

static int apds9900_set_ailt(struct i2c_client *client, int threshold)
{
	struct apds9900_data *data = i2c_get_clientdata(client);
	int ret;
	
	mutex_lock(&data->update_lock);
	ret = i2c_smbus_write_word_data(client, CMD_WORD|APDS9900_AILTL_REG, threshold);
	mutex_unlock(&data->update_lock);
	
	data->ailt = threshold;

	return ret;
}

static int apds9900_set_aiht(struct i2c_client *client, int threshold)
{
	struct apds9900_data *data = i2c_get_clientdata(client);
	int ret;
	
	mutex_lock(&data->update_lock);
	ret = i2c_smbus_write_word_data(client, CMD_WORD|APDS9900_AIHTL_REG, threshold);
	mutex_unlock(&data->update_lock);
	
	data->aiht = threshold;

	return ret;
}

static int apds9900_set_pilt(struct i2c_client *client, int threshold)
{
	struct apds9900_data *data = i2c_get_clientdata(client);
	int ret;
	
	mutex_lock(&data->update_lock);
	ret = i2c_smbus_write_word_data(client, CMD_WORD|APDS9900_PILTL_REG, threshold);
	mutex_unlock(&data->update_lock);
	
	data->pilt = threshold;

	return ret;
}

static int apds9900_set_piht(struct i2c_client *client, int threshold)
{
	struct apds9900_data *data = i2c_get_clientdata(client);
	int ret;
	
	mutex_lock(&data->update_lock);
	ret = i2c_smbus_write_word_data(client, CMD_WORD|APDS9900_PIHTL_REG, threshold);
	mutex_unlock(&data->update_lock);
	
	data->piht = threshold;
/* kwnagdo.yi@lge.com 10.09.16
   set the threshold by sys
*/
#ifdef CONFIG_MACH_LGE_BRYCE
	data->ps_threshold = threshold;
#endif
	return ret;
}

static int apds9900_set_pers(struct i2c_client *client, int pers)
{
	struct apds9900_data *data = i2c_get_clientdata(client);
	int ret;
	
	mutex_lock(&data->update_lock);
	ret = i2c_smbus_write_byte_data(client, CMD_BYTE|APDS9900_PERS_REG, pers);
	mutex_unlock(&data->update_lock);

	data->pers = pers;

	return ret;
}

static int apds9900_set_config(struct i2c_client *client, int config)
{
	struct apds9900_data *data = i2c_get_clientdata(client);
	int ret;
	
	mutex_lock(&data->update_lock);
	ret = i2c_smbus_write_byte_data(client, CMD_BYTE|APDS9900_CONFIG_REG, config);
	mutex_unlock(&data->update_lock);

	data->config = config;

	return ret;
}

static int apds9900_set_ppcount(struct i2c_client *client, int ppcount)
{
	struct apds9900_data *data = i2c_get_clientdata(client);
	int ret;
	
	mutex_lock(&data->update_lock);
	ret = i2c_smbus_write_byte_data(client, CMD_BYTE|APDS9900_PPCOUNT_REG, ppcount);
	mutex_unlock(&data->update_lock);

	data->ppcount = ppcount;

	return ret;
}

static int apds9900_set_control(struct i2c_client *client, int control)
{
	struct apds9900_data *data = i2c_get_clientdata(client);
	int ret;
	
	mutex_lock(&data->update_lock);
	ret = i2c_smbus_write_byte_data(client, CMD_BYTE|APDS9900_CONTROL_REG, control);
	mutex_unlock(&data->update_lock);

	data->control = control;

	return ret;
}

/*
 * SysFS support
 */

static ssize_t apds9900_store_command(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	unsigned long val = simple_strtoul(buf, NULL, 10);
	int ret;

	if (val < 0 || val > 2)
		return -EINVAL;

	ret = apds9900_set_command(client, val);

	if (ret < 0)
		return ret;

	return count;
}

static DEVICE_ATTR(command, S_IWUSR,
		   NULL, apds9900_store_command);

static ssize_t apds9900_show_enable(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct apds9900_data *data = i2c_get_clientdata(client);

	return sprintf(buf, "%x\n", data->enable);
}

static ssize_t apds9900_store_enable(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	unsigned long val = simple_strtoul(buf, NULL, 10);
	int ret;

	ret = apds9900_set_enable(client, val);
      
	if (ret < 0)
		return ret;

	return count;
}

static DEVICE_ATTR(enable, S_IWUSR | S_IRUGO|S_IWGRP |S_IRGRP,
		   apds9900_show_enable, apds9900_store_enable);

static ssize_t apds9900_show_atime(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct apds9900_data *data = i2c_get_clientdata(client);

	return sprintf(buf, "%x\n", data->atime);
}

static ssize_t apds9900_store_atime(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	unsigned long val = simple_strtoul(buf, NULL, 10);
	int ret;

	ret = apds9900_set_atime(client, val);

	if (ret < 0)
		return ret;

	return count;
}

static DEVICE_ATTR(atime,  S_IWUSR | S_IRUGO|S_IWGRP |S_IRGRP,
		   apds9900_show_atime, apds9900_store_atime);

static ssize_t apds9900_show_ptime(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct apds9900_data *data = i2c_get_clientdata(client);

	return sprintf(buf, "%x\n", data->ptime);
}

static ssize_t apds9900_store_ptime(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	unsigned long val = simple_strtoul(buf, NULL, 10);
	int ret;

	ret = apds9900_set_ptime(client, val);

	if (ret < 0)
		return ret;

	return count;
}

static DEVICE_ATTR(ptime,  S_IWUSR | S_IRUGO|S_IWGRP |S_IRGRP,
		   apds9900_show_ptime, apds9900_store_ptime);

static ssize_t apds9900_show_wtime(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct apds9900_data *data = i2c_get_clientdata(client);

	return sprintf(buf, "%x\n", data->wtime);
}

static ssize_t apds9900_store_wtime(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	unsigned long val = simple_strtoul(buf, NULL, 10);
	int ret;

	ret = apds9900_set_wtime(client, val);

	if (ret < 0)
		return ret;

	return count;
}

static DEVICE_ATTR(wtime,  S_IWUSR | S_IRUGO|S_IWGRP |S_IRGRP ,
		   apds9900_show_wtime, apds9900_store_wtime);

static ssize_t apds9900_show_ailt(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct apds9900_data *data = i2c_get_clientdata(client);

	return sprintf(buf, "%x\n", data->ailt);
}

static ssize_t apds9900_store_ailt(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	unsigned long val = simple_strtoul(buf, NULL, 10);
	int ret;

	ret = apds9900_set_ailt(client, val);

	if (ret < 0)
		return ret;

	return count;
}

static DEVICE_ATTR(ailt,  S_IWUSR | S_IRUGO|S_IWGRP |S_IRGRP,
		   apds9900_show_ailt, apds9900_store_ailt);

static ssize_t apds9900_show_aiht(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct apds9900_data *data = i2c_get_clientdata(client);

	return sprintf(buf, "%x\n", data->aiht);
}

static ssize_t apds9900_store_aiht(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	unsigned long val = simple_strtoul(buf, NULL, 10);
	int ret;

	ret = apds9900_set_aiht(client, val);

	if (ret < 0)
		return ret;

	return count;
}

static DEVICE_ATTR(aiht,  S_IWUSR | S_IRUGO|S_IWGRP |S_IRGRP,
		   apds9900_show_aiht, apds9900_store_aiht);

static ssize_t apds9900_show_pilt(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct apds9900_data *data = i2c_get_clientdata(client);

	return sprintf(buf, "%x\n", data->pilt);
}

static ssize_t apds9900_store_pilt(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	unsigned long val = simple_strtoul(buf, NULL, 10);
	int ret;

	ret = apds9900_set_pilt(client, val);

	if (ret < 0)
		return ret;

	return count;
}

static DEVICE_ATTR(pilt,  S_IWUSR | S_IRUGO|S_IWGRP |S_IRGRP,
		   apds9900_show_pilt, apds9900_store_pilt);

static ssize_t apds9900_show_piht(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct apds9900_data *data = i2c_get_clientdata(client);

	return sprintf(buf, "%x\n", data->piht);
}

static ssize_t apds9900_store_piht(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	unsigned long val = simple_strtoul(buf, NULL, 10);
	int ret;

	ret = apds9900_set_piht(client, val);

	if (ret < 0)
		return ret;

	return count;
}

static DEVICE_ATTR(piht,  S_IWUSR | S_IRUGO|S_IWGRP |S_IRGRP,
		   apds9900_show_piht, apds9900_store_piht);

static ssize_t apds9900_show_pers(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct apds9900_data *data = i2c_get_clientdata(client);

	return sprintf(buf, "%x\n", data->pers);
}

static ssize_t apds9900_store_pers(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	unsigned long val = simple_strtoul(buf, NULL, 10);
	int ret;

	ret = apds9900_set_pers(client, val);

	if (ret < 0)
		return ret;

	return count;
}

static DEVICE_ATTR(pers,  S_IWUSR | S_IRUGO|S_IWGRP |S_IRGRP,
		   apds9900_show_pers, apds9900_store_pers);

static ssize_t apds9900_show_config(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct apds9900_data *data = i2c_get_clientdata(client);

	return sprintf(buf, "%x\n", data->config);
}

static ssize_t apds9900_store_config(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	unsigned long val = simple_strtoul(buf, NULL, 10);
	int ret;

	ret = apds9900_set_config(client, val);

	if (ret < 0)
		return ret;

	return count;
}

static DEVICE_ATTR(config,  S_IWUSR | S_IRUGO|S_IWGRP |S_IRGRP,
		   apds9900_show_config, apds9900_store_config);

static ssize_t apds9900_show_ppcount(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct apds9900_data *data = i2c_get_clientdata(client);

	return sprintf(buf, "%x\n", data->ppcount);
}

static ssize_t apds9900_store_ppcount(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	unsigned long val = simple_strtoul(buf, NULL, 10);
	int ret;

	ret = apds9900_set_ppcount(client, val);

	if (ret < 0)
		return ret;

	return count;
}

static DEVICE_ATTR(ppcount,  S_IWUSR | S_IRUGO|S_IWGRP |S_IRGRP,
		   apds9900_show_ppcount, apds9900_store_ppcount);

static ssize_t apds9900_show_control(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct apds9900_data *data = i2c_get_clientdata(client);

	return sprintf(buf, "%x\n", data->control);
}

static ssize_t apds9900_store_control(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	unsigned long val = simple_strtoul(buf, NULL, 10);
	int ret;

	ret = apds9900_set_control(client, val);

	if (ret < 0)
		return ret;

	return count;
}

static DEVICE_ATTR(control,  S_IWUSR | S_IRUGO|S_IWGRP |S_IRGRP,
		   apds9900_show_control, apds9900_store_control);

static ssize_t apds9900_show_rev(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct apds9900_data *data = i2c_get_clientdata(client);
	int rev;

	mutex_lock(&data->update_lock);
	rev = i2c_smbus_read_byte_data(client, CMD_BYTE|APDS9900_REV_REG);
	mutex_unlock(&data->update_lock);

	return sprintf(buf, "%x\n", rev);
}

static DEVICE_ATTR(rev, S_IRUGO |S_IRGRP | S_IROTH,
		   apds9900_show_rev, NULL);

static ssize_t apds9900_show_id(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct apds9900_data *data = i2c_get_clientdata(client);
	int id;

	mutex_lock(&data->update_lock);
	id = i2c_smbus_read_byte_data(client, CMD_BYTE|APDS9900_ID_REG);
	mutex_unlock(&data->update_lock);

	return sprintf(buf, "%x\n", id);
}

static DEVICE_ATTR(id, S_IRUGO |S_IRGRP | S_IROTH,
		   apds9900_show_id, NULL);

static ssize_t apds9900_show_status(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct apds9900_data *data = i2c_get_clientdata(client);
	int status;

	mutex_lock(&data->update_lock);
	status = i2c_smbus_read_byte_data(client, CMD_BYTE|APDS9900_STATUS_REG);
	mutex_unlock(&data->update_lock);

	return sprintf(buf, "%x\n", status);
}

static DEVICE_ATTR(status, S_IRUGO |S_IRGRP | S_IROTH,
		   apds9900_show_status, NULL);

static ssize_t apds9900_show_cdata(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct apds9900_data *data = i2c_get_clientdata(client);
	int cdata;

	mutex_lock(&data->update_lock);
	cdata = i2c_smbus_read_word_data(client, CMD_WORD|APDS9900_CDATAL_REG);
	mutex_unlock(&data->update_lock);
	
	return sprintf(buf, "%x\n", cdata);
}

static DEVICE_ATTR(cdata, S_IRUGO |S_IRGRP | S_IROTH,
		   apds9900_show_cdata, NULL);

static ssize_t apds9900_show_irdata(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct apds9900_data *data = i2c_get_clientdata(client);
	int irdata;

	mutex_lock(&data->update_lock);
	irdata = i2c_smbus_read_word_data(client, CMD_WORD|APDS9900_IRDATAL_REG);
	mutex_unlock(&data->update_lock);
	
	return sprintf(buf, "%x\n", irdata);
}

static DEVICE_ATTR(irdata, S_IRUGO |S_IRGRP | S_IROTH,
		   apds9900_show_irdata, NULL);

static ssize_t apds9900_show_pdata(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct apds9900_data *data = i2c_get_clientdata(client);
	int pdata;

	mutex_lock(&data->update_lock);
	pdata = i2c_smbus_read_word_data(client, CMD_WORD|APDS9900_PDATAL_REG);
	mutex_unlock(&data->update_lock);
	
	return sprintf(buf, "%x\n", pdata);
}

static DEVICE_ATTR(pdata, S_IRUGO |S_IRGRP | S_IROTH,
		   apds9900_show_pdata, NULL);

/* kwangdo.yi@lge.com 10.08.31
    added to inform LgHiddenMenu of Near/Far
    */
#ifdef CONFIG_MACH_LGE_BRYCE   
static ssize_t apds9900_show_prox(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct apds9900_data *data = i2c_get_clientdata(client);
	int pdata;
    
	pdata = data->ps_detection;
      
	return sprintf(buf, "%x\n", pdata);
}

static DEVICE_ATTR(prox, S_IRUGO |S_IRGRP | S_IROTH,
		   apds9900_show_prox, NULL);
#endif

/* LGE_S jihye.ahn    2010-11-26  for CDMA Test Mode */
static ssize_t apds9900_show_enable1(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct apds9900_data *data = i2c_get_clientdata(client);

	return sprintf(buf, "%x\n", data->enable);
}

static ssize_t apds9900_store_enable1(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	unsigned long val = simple_strtoul(buf, NULL, 10);
	int ret;
         unsigned int value;

         sscanf(buf, "%d\n", &value);

        if (value == 0)
            ret = apds9900_set_enable(client, 0); 
        
        else if (value ==1)
             ret = apds9900_set_enable(client, 0x2F); 

        else
             printk("0 : off, 1 : on \n");

	if (ret < 0)
		return ret;

	return count;
}

static DEVICE_ATTR(enable1, S_IWUSR | S_IRUGO|S_IWGRP |S_IRGRP,
		   apds9900_show_enable1, apds9900_store_enable1);

static ssize_t apds9900_show_proximity(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct apds9900_data *data = i2c_get_clientdata(client);
	int pdata;
    
	pdata = data->ps_detection;
    
        printk("jihye.ahn [%s]  pdata = %d \n",__func__, pdata);
        
	return sprintf(buf, "%x\n", pdata);
}

static DEVICE_ATTR(proximity, S_IRUGO |S_IRGRP | S_IROTH,
		   apds9900_show_proximity, NULL);
/* LGE_E jihye.ahn    2010-11-26  for CDMA Test Mode */


static struct attribute *apds9900_attributes[] = {
#ifdef CONFIG_MACH_LGE_BRYCE
	&dev_attr_enable.attr,
	&dev_attr_cdata.attr,
	&dev_attr_irdata.attr,
	&dev_attr_prox.attr,
#else
	&dev_attr_command.attr,
	&dev_attr_enable.attr,
	&dev_attr_atime.attr,
	&dev_attr_ptime.attr,
	&dev_attr_wtime.attr,
	&dev_attr_ailt.attr,
	&dev_attr_aiht.attr,
	&dev_attr_pilt.attr,
	&dev_attr_piht.attr,
	&dev_attr_pers.attr,
	&dev_attr_config.attr,
	&dev_attr_ppcount.attr,
	&dev_attr_control.attr,
	&dev_attr_rev.attr,
	&dev_attr_id.attr,
	&dev_attr_status.attr,
	&dev_attr_cdata.attr,
	&dev_attr_irdata.attr,
	&dev_attr_pdata.attr,
	&dev_attr_prox.attr,
    &dev_attr_enable1.attr,
    &dev_attr_proximity.attr,
#endif
    NULL
};

static const struct attribute_group apds9900_attr_group = {
	.attrs = apds9900_attributes,
};

/*
 * Initialization function
 */
static int apds9900_init_client(struct i2c_client *client)
{
	struct apds9900_data *data = i2c_get_clientdata(client);
	int err;

	err = apds9900_set_enable(client, 0);

	if (err < 0)
		return err;

	mdelay(1);

	mutex_lock(&data->update_lock);
	err = i2c_smbus_read_byte_data(client, CMD_BYTE|APDS9900_ENABLE_REG);
	mutex_unlock(&data->update_lock);

	if (err != 0)
		return -ENODEV;

	data->enable = 0;

	return 0;
}

/* sungmin.shin@lge.com	10.08.06
	sensor initialization function
	TODO. tuning should be done for initialization values.
*/
#ifdef CONFIG_MACH_LGE_BRYCE
static int apds9900_device_init(struct i2c_client *client)
{

	apds9900_set_enable(client, 0);

	apds9900_set_wtime(client, 0xFF); /*0xF6 : WAIT=27.2ms */
	apds9900_set_control(client, 0x20); /* 100mA, IR-diode, 1X PGAIN,  1X AGAIN */
	apds9900_set_config(client, 0x00); /* unless they need to use wait time more than > 700ms */

	/* ALS tuning */
	apds9900_set_atime(client, 0xDE); /* ALS = 100ms */

	/* proximity tuning */
	apds9900_set_ptime(client, 0xFF); /* recommended value. don't change it unless there's proper reason */
	apds9900_set_ppcount(client, 0x8); /* use 8-pulse should be enough for evaluation */

	/* interrupt tuning */
	apds9900_set_pers(client, 0x33);  /* interrupt persistence */
	apds9900_set_pilt(client, PROX_INT_LOW_TH);
	apds9900_set_piht(client, PROX_INT_HIGH_TH); 

	/* kwangdo.yi@lge.com 2010.08.31
		 leave out to UI scenario, turn apds on/off from app, telephony etc.
	  */
#if 0
	apds9900_set_enable(client, 0x2F); /* enable PROX int */
	mdelay(12);
#endif
	printk("apds9900 device (re)init finished.\n");
	
	return 0;
}
#endif
/* kwangdo.yi@lge.com 10.09.17 S
	add to support ioctl to sensor HAL 
*/
#ifdef CONFIG_MACH_LGE_BRYCE

static int apds_open(struct inode *inode, struct file *file)
{
	printk("### apds open \n");
	return 0;
}
static int apds_release(struct inode *inode, struct file *file)
{
	printk("### apds release \n");
	return 0;
}
static struct file_operations apds_fops = {
	.owner = THIS_MODULE,
	.open = apds_open,
	.release = apds_release,
};

static struct miscdevice apds_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "apds9900",
	.fops = &apds_fops,
};

#endif
/* kwangdo.yi@lge.com 10.09.17 E */

/*
 * I2C init/probing/exit functions
 */

static struct i2c_driver apds9900_driver;
static int __devinit apds9900_probe(struct i2c_client *client,
				   const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	//struct apds9900_data *data;
	int err = 0;

/* sungmin.shin@lge.com 10.08.06
	sensor interrupt handler
*/
#ifdef CONFIG_MACH_LGE_BRYCE
	struct apds9900_platform_data* pdata = client->dev.platform_data;
	int irq;
#endif 

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE)) {
		err = -EIO;
		goto exit;
	}

	papds9900_data = kzalloc(sizeof(struct apds9900_data), GFP_KERNEL);
	if (!papds9900_data) {
		err = -ENOMEM;
		goto exit;
	}
	papds9900_data->client = client;
	i2c_set_clientdata(client, papds9900_data);

	papds9900_data->enable = 0;	/* default mode is standard */
/* kwangdo.yi@lge.com 10.09.16 S
   init threshold value
*/	
#ifdef CONFIG_MACH_LGE_BRYCE
	papds9900_data->ps_threshold = PROX_INT_HIGH_TH;
	papds9900_data->ps_detection = 0;	/* default to no detection == far */
#endif
/* kwangdo.yi@lge.com  E */	
	dev_info(&client->dev, "enable = %s\n",
			papds9900_data->enable ? "1" : "0");

	mutex_init(&papds9900_data->update_lock);
	/* kwangdo.yi@lge.com 10.08.31
		added to inform LgHiddenMenu of Near/Far
		*/
#ifdef CONFIG_MACH_LGE_BRYCE   
	papds9900_data->ps_detection = 0;
#endif

	/* kwangdo.yi 	100811
		 power on apds9900
	*/
#ifdef CONFIG_MACH_LGE_BRYCE
	pdata->power(1);
#endif 

	/* Initialize the APDS9900 chip */
		/* kwangdo.yi@lge.com 2010.08.31
		 leave out to UI scenario, turn apds on/off from app, telephony etc.
	  */
#ifndef CONFIG_MACH_LGE_BRYCE
	err = apds9900_init_client(client);
	if (err)
		goto exit_kfree;
#endif		

	/* Register sysfs hooks */
	err = sysfs_create_group(&client->dev.kobj, &apds9900_attr_group);
	if (err)
		goto exit_kfree;

	dev_info(&client->dev, "support ver. %s enabled\n", DRIVER_VERSION);

	/* sungmin.shin@lge.com 10.08.06
		sensor interrupt handler
	*/
#ifdef CONFIG_MACH_LGE_BRYCE
	INIT_DELAYED_WORK(&papds9900_data->poswork, apds9900_event_work);

	err = gpio_request(pdata->irq_num, "apds_irq");
	if (err) {
		printk(KERN_ERR"Unable to request GPIO.\n");
		goto exit_kfree;
	}

	gpio_tlmm_config(GPIO_CFG(pdata->irq_num, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE); 
	gpio_direction_input(pdata->irq_num);
	irq = gpio_to_irq(pdata->irq_num);
	//printk("request irq(%d) <- gpio%d \n", irq, pdata->irq_num);
	
	if (irq < 0) {
		err = irq;
		printk(KERN_ERR"Unable to request gpio irq. err=%d\n", err);
		gpio_free(pdata->irq_num);
		goto exit_kfree;
	}
	
	papds9900_data->irq = irq;
	err = request_irq(papds9900_data->irq, apds9900_isr, IRQF_TRIGGER_FALLING, ALS_SENSOR_NAME, papds9900_data);
	if (err) {
		printk(KERN_ERR"Unable to request irq.\n");
		goto exit_kfree;
	}
	/* kwangdo.yi@lge.com 10.09.17 S
	 * add input event 
	 */
	papds9900_data->input_dev = input_allocate_device();
	if (!papds9900_data->input_dev) {
		err = -ENOMEM;
		printk("### apds9900_probe: Failed to allocate input device\n");
	}
	set_bit(EV_ABS, papds9900_data->input_dev->evbit);
	/* distance data  */
	input_set_abs_params(papds9900_data->input_dev, ABS_DISTANCE, 0, 10, 0, 0);
	/* kwangdo.yi@lge.com 10.09.30 S
	 * 0009611: change light sensor to report event of light
	 */
	input_set_abs_params(papds9900_data->input_dev, ABS_LIGHT, 0, ABS_MAX, 0, 0);
	/* kwangdo.yi@lge.com 10.09.30 E */
/* add light input event */	
	papds9900_data->input_dev->name = "apds9900";

	err = input_register_device(papds9900_data->input_dev);

	if (err) 
		printk("### apds9900_probe: Unable to register input device: %s\n",papds9900_data->input_dev->name);
	
	/* kwangdo.yi@lge.com E */

	err = misc_register(&apds_device);
	if (err) {
		printk("### apds_probe: apds_device register failed\n");
	}

	apds9900_device_init(client);
#endif 

	return 0;

exit_kfree:
	kfree(papds9900_data);
exit:
	return err;
}

static int __devexit apds9900_remove(struct i2c_client *client)
{
/* kwangdo.yi@lge.com S
*/
#ifdef CONFIG_MACH_LGE_BRYCE
	struct apds9900_data *data = i2c_get_clientdata(client);
	free_irq(data->irq, client);
#endif
/* kwangdo.yi@lge.com E */

	sysfs_remove_group(&client->dev.kobj, &apds9900_attr_group);

	/* Power down the device */
	apds9900_set_enable(client, 0);

	kfree(i2c_get_clientdata(client));

	return 0;
}

#ifdef CONFIG_PM

static int apds9900_suspend(struct i2c_client *client, pm_message_t mesg)
{
		/* kwangdo.yi@lge.com 2010.08.31
			 leave out to UI scenario, turn apds on/off from app, telephony etc.
		  */
	      printk("jihye.ahn [%s]",__func__);
	      return 0;
#if 0

/* sungmin.shin@lge.com 10.08.06
	power down for saving energy.
*/
#ifdef CONFIG_MACH_LGE_BRYCE
	struct apds9900_platform_data* pdata = client->dev.platform_data;
	apds9900_set_enable(client, 0);

	pdata->power(0);
	printk("### apds9900_suspend \n");
	return 0;
#else
	return apds9900_set_enable(client, 0);
#endif
#endif
}

static int apds9900_resume(struct i2c_client *client)
{
		/* kwangdo.yi@lge.com 2010.08.31
			 leave out to UI scenario, turn apds on/off from app, telephony etc.
		  */		  
        printk("jihye.ahn [%s]",__func__);
		return 0;
#if 0

/* sungmin.shin@lge.com 10.08.06
	power on 
*/
#ifdef CONFIG_MACH_LGE_BRYCE
	struct apds9900_platform_data* pdata = client->dev.platform_data;
	pdata->power(1);

	apds9900_device_init(client);

	printk("### apds9900_resume \n");	
	return 0;
#else
	return apds9900_set_enable(client, 0);
#endif 
#endif
}

#else

#define apds9900_suspend	NULL
#define apds9900_resume		NULL

#endif /* CONFIG_PM */

static const struct i2c_device_id apds9900_id[] = {
	{ "apds9900", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, apds9900_id);

static struct i2c_driver apds9900_driver = {
	.driver = {
		.name	= APDS9900_DRV_NAME,
		.owner	= THIS_MODULE,
	},
	.suspend = apds9900_suspend,
	.resume	= apds9900_resume,
	.probe	= apds9900_probe,
	.remove	= __devexit_p(apds9900_remove),
	.id_table = apds9900_id,
};

static int __init apds9900_init(void)
{
	return i2c_add_driver(&apds9900_driver);
}

static void __exit apds9900_exit(void)
{
	i2c_del_driver(&apds9900_driver);
}

MODULE_AUTHOR("Lee Kai Koon <kai-koon.lee@avagotech.com>");
MODULE_DESCRIPTION("APDS9900 ambient light + proximity sensor driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRIVER_VERSION);

module_init(apds9900_init);
module_exit(apds9900_exit);
