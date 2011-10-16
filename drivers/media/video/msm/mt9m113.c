/* drivers/media/video/msm/mt9m113.c 
*
* This software is for APTINA 1.3M sensor 
*  
* Copyright (C) 2010-2011 LGE Inc.  
*
* This software is licensed under the terms of the GNU General Public  
* License version 2, as published by the Free Software Foundation, and  
* may be copied, distributed, and modified under those terms.  
*  
* This program is distributed in the hope that it will be useful,  
* but WITHOUT ANY WARRANTY; without even the implied warranty of  
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the  
* GNU General Public License for more details. 
*/

#include <linux/delay.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <media/msm_camera.h>
#include <mach/gpio.h>
#include <mach/vreg.h>
#include <linux/byteorder/little_endian.h>
#include <linux/slab.h>
#include <mach/board-bryce.h>

#include "mt9m113.h"


/* SYSCTL Registers*/
#define MT9M113_REG_K24A_CHIP_ID		0x0000
#define MT9M113_REG_PLL_DIVIDERS		0x0010
#define MT9M113_REG_PLL_P_DIVIDERS		0x0012
#define MT9M113_REG_PLL_CTRL			0x0014
#define MT9M113_REG_CLK_CTRL			0x0016
#define MT9M113_REG_STANDBY_CTRL_STATUS	0x0018
#define MT9M113_REG_RESET_MISC_CTRL		0x001A
#define MT9M113_REG_MCU_BOOT_MODE		0x001C
#define MT9M113_REG_PAD_SLEW			0x001E
#define MT9M113_REG_VDD_DIS_COUNTER		0x0022
#define MT9M113_REG_GPI_STATUS			0x0024
#define MT9M113_REG_HARD_STANDBY_SEL	0x0028

/* XMDA Registers*/
#define MT9M113_REG_MCU_VAR_ADDR		0x098C
#define MT9M113_REG_MCU_VAR_DATA0		0x0990
#define MT9M113_REG_MCU_VAR_DATA1		0x0992
#define MT9M113_REG_MCU_VAR_DATA2		0x0994
#define MT9M113_REG_MCU_VAR_DATA3		0x0996
#define MT9M113_REG_MCU_VAR_DATA4		0x0998
#define MT9M113_REG_MCU_VAR_DATA5		0x099A
#define MT9M113_REG_MCU_VAR_DATA6		0x099C
#define MT9M113_REG_MCU_VAR_DATA7		0x099E

/* chanhee.park@lge.com 
   temp : we define the delay time on MSM as 0xFFFE address 
*/
#define MT9M113_REG_REGFLAG_DELAY  		0xFFFE

DEFINE_MUTEX(mt9m113_mutex);

enum{
  MT9M113_FPS_NORMAL = 0,
  MT9M113_FPS_5 = 5,
  MT9M113_FPS_7 = 7,
  MT9M113_FPS_10 = 10,
  MT9M113_FPS_12 = 12,
  MT9M113_FPS_15 = 15,
  MT9M113_FPS_20 = 20,
  MT9M113_FPS_24 = 24,
  MT9M113_FPS_25 = 25,
  MT9M113_FPS_30 = 30
};

enum{
  MT9M113_FPS_RANGE_5000 = 5000,
  MT9M113_FPS_RANGE_6000 = 6000,	
  MT9M113_FPS_RANGE_7000 = 7000,
  MT9M113_FPS_RANGE_7500 = 7500,
  MT9M113_FPS_RANGE_10000 = 10000,
  MT9M113_FPS_RANGE_15000 = 15000,
  MT9M113_FPS_RANGE_18000 = 18000,
  MT9M113_FPS_RANGE_19000 = 19000,
  MT9M113_FPS_RANGE_20000 = 20000,
  MT9M113_FPS_RANGE_22000 = 22000,
  MT9M113_FPS_RANGE_25000 = 25000,
  MT9M113_FPS_RANGE_26000 = 26000,
  MT9M113_FPS_RANGE_28000 = 28000,
  MT9M113_FPS_RANGE_29000 = 29000,
  MT9M113_FPS_RANGE_30000 = 30000,
};


struct mt9m113_ctrl_t {
	const struct msm_camera_sensor_info *sensordata;
	int8_t  previous_mode;

   /* for Video Camera */
	int8_t effect;
	int8_t wb;
	unsigned char brightness;
	int8_t video_mode_fps;

   /* for register write */
	int16_t write_byte;
	int16_t write_word;
};

static struct i2c_client  *mt9m113_client;

static struct mt9m113_ctrl_t *mt9m113_ctrl = NULL;
static atomic_t init_reg_mode;

static int mt9m113_set_fps_range(int fps);

/*=============================================================
	EXTERNAL DECLARATIONS
==============================================================*/
extern struct mt9m113_reg mt9m113_regs;

static int32_t mt9m113_i2c_txdata(unsigned short saddr,
	unsigned char *txdata, int length)
{
	struct i2c_msg msg[] = {
		{
			.addr = saddr,
			.flags = 0,
			.len = length,
			.buf = txdata,
		},
	};

	if (i2c_transfer(mt9m113_client->adapter, msg, 1) < 0 ) {
		CAM_ERR("mt9m113_i2c_txdata failed\n");
		return -EIO;
	}

	return 0;
}
static int32_t mt9m113_i2c_write(unsigned short saddr,
	unsigned short waddr, unsigned short wdata, enum mt9m113_width width)
{
	int32_t rc = -EIO;
	unsigned char buf[4];

	memset(buf, 0, sizeof(buf));
	switch (width) {
	case WORD_LEN: {
		buf[0] = (waddr & 0xFF00)>>8;
		buf[1] = (waddr & 0x00FF);
		buf[2] = (wdata & 0xFF00)>>8;   
		buf[3] = (wdata & 0x00FF);

		rc = mt9m113_i2c_txdata(saddr, buf, 4);
	}
		break;

	case BYTE_LEN: {
		buf[0] = (waddr & 0xFF00)>>8;
		buf[1] = (waddr & 0x00FF);
		buf[2] =  wdata;
		rc = mt9m113_i2c_txdata(saddr, buf, 3);
	}
		break;
	}

	if (rc < 0){
		CAM_ERR("i2c_write failed, addr = 0x%x, val = 0x%x!\n",waddr, wdata);
	}
	
	return rc;
	
}

static int32_t mt9m113_i2c_write_table(
	struct register_address_value_pair const *reg_conf_tbl,
	int num_of_items_in_table)
{
	int32_t retry;
	int32_t i;
	int32_t rc = 0;
	
	for (i = 0; i < num_of_items_in_table; i++) {

		if(reg_conf_tbl->register_address == MT9M113_REG_REGFLAG_DELAY){
				mdelay(reg_conf_tbl->register_value);
		}
		else{
			rc = mt9m113_i2c_write(mt9m113_client->addr,
			reg_conf_tbl->register_address, reg_conf_tbl->register_value,
			WORD_LEN);
		}
	         
		if(rc < 0){
    		for(retry = 0; retry < 3; retry++){
   				rc = mt9m113_i2c_write(mt9m113_client->addr,
		    		 reg_conf_tbl->register_address, reg_conf_tbl->register_value,
		    		 WORD_LEN);
           
            	if(rc >= 0)
               		retry = 3;        
	
         	}
         	reg_conf_tbl++;
			
		}else
         	reg_conf_tbl++;

	}
	
	return rc;
	
}

static int mt9m113_i2c_rxdata(unsigned short saddr,
	unsigned char *rxdata, int length)
{
	struct i2c_msg msgs[] = {
	{
		.addr   = saddr,
		.flags = 0,
		.len   = 2,
		.buf   = rxdata,
	},
	{
		.addr   = saddr,
		.flags = I2C_M_RD,
		.len   = length,
		.buf   = rxdata,
	},
	};

	if (i2c_transfer(mt9m113_client->adapter, msgs, 2) < 0) {
		CAM_ERR("mt9m113_i2c_rxdata failed!\n");
		return -EIO;
	}

	return 0;
	
}
static int32_t mt9m113_i2c_read(unsigned short saddr,unsigned short raddr, 
	                               unsigned short *rdata,enum mt9m113_width width)
{
	int32_t rc = 0;
	unsigned char buf[4];

	if (!rdata)
		return -EIO;

	memset(buf, 0, sizeof(buf));

	switch (width) {
	case WORD_LEN: {
		buf[0] = (raddr & 0xFF00)>>8;
		buf[1] = (raddr & 0x00FF);

		rc = mt9m113_i2c_rxdata(saddr, buf, 2);
		if (rc < 0)
			return rc;

		*rdata = buf[0] << 8 | buf[1];
		
		}
		break;
   	
   case BYTE_LEN:{
		buf[0] = (raddr & 0xFF00)>>8;
		buf[1] = (raddr & 0x00FF);

		rc = mt9m113_i2c_rxdata(saddr, buf, 2);
		if (rc < 0)
			return rc;
		
		*rdata = buf[0];
		}
		break;           
	}

	if (rc < 0)
		CAM_ERR("mt9m113_i2c_read failed!\n");
   
	return rc;
	
}

static long mt9m113_set_effect(int effect)
{
	int32_t rc = 0;

   switch (effect) {
   	case CAMERA_EFFECT_OFF: 
		CAM_MSG("mt9m113_set_effect: effect is OFF\n");
		rc = mt9m113_i2c_write_table(&mt9m113_regs.effect_off_reg_settings[0],
                mt9m113_regs.effect_off_reg_settings_size);
		if (rc < 0)
			return rc;
		break;
		
	case CAMERA_EFFECT_MONO: 
		CAM_MSG("mt9m113_set_effect: effect is MONO\n");
		rc = mt9m113_i2c_write_table(&mt9m113_regs.effect_mono_reg_settings[0],
                mt9m113_regs.effect_mono_reg_settings_size);
		if (rc < 0)
			return rc;
		break;

   case CAMERA_EFFECT_SEPIA: 
		CAM_MSG("mt9m113_set_effect: effect is SEPIA\n");
		rc = mt9m113_i2c_write_table(&mt9m113_regs.effect_sepia_reg_settings[0],
                mt9m113_regs.effect_sepia_reg_settings_size);
		if (rc < 0)			
			return rc;
		break;

   case CAMERA_EFFECT_NEGATIVE: 
		CAM_MSG("mt9m113_set_effect: effect is NAGATIVE\n");
		rc = mt9m113_i2c_write_table(&mt9m113_regs.effect_negative_reg_settings[0],
                mt9m113_regs.effect_negative_reg_settings_size);
		if (rc < 0)
			return rc;

		break;

   /* This effect is not supported in mt9m113 sensor */
   case CAMERA_EFFECT_NEGATIVE_SEPIA:
	    CAM_MSG("mt9m113_set_effect: effect is OFF\n");
	    rc = mt9m113_i2c_write_table(&mt9m113_regs.effect_off_reg_settings[0],
			    mt9m113_regs.effect_off_reg_settings_size);
	    if (rc < 0)
		    return rc;

   		break;

   case CAMERA_EFFECT_SOLARIZE: 
		CAM_MSG("mt9m113_set_effect: effect is SOLARIZE\n");
		rc = mt9m113_i2c_write_table(&mt9m113_regs.effect_solarize_reg_settings[0],
                mt9m113_regs.effect_solarize_reg_settings_size);
		if (rc < 0)
			return rc;

		break;
		
   case CAMERA_EFFECT_BLUE: 
		 CAM_MSG("mt9m113_set_effect: effect is SOLARIZE\n");
		 rc = mt9m113_i2c_write_table(&mt9m113_regs.effect_blue_reg_settings[0],
				 mt9m113_regs.effect_blue_reg_settings_size);
		 if (rc < 0)
			 return rc;

		 break;

   default: 
		CAM_ERR("mt9m113_set_effect: wrong effect mode\n");
		return -EINVAL;	
	}
	
	return 0;
}

static long mt9m113_set_wb(int8_t wb)
{
	int32_t rc;
   
	CAM_MSG("mt9m113_set_wb : called, new wb: %d\n", wb);

	switch (wb) {
	case CAMERA_WB_AUTO:
		CAM_MSG("mt9m113_set_wb: wb is AUTO\n");
		rc = mt9m113_i2c_write_table(&mt9m113_regs.wb_auto_reg_settings[0],
					mt9m113_regs.wb_auto_reg_settings_size);
		
		if (rc < 0)
			return rc;
		break;

	case CAMERA_WB_INCANDESCENT:
		CAM_MSG("mt9m113_set_wb: wb is INCANDESCENT\n");
		rc = mt9m113_i2c_write_table(&mt9m113_regs.wb_incandescent_reg_settings[0],
                mt9m113_regs.wb_incandescent_reg_settings_size);

		if (rc < 0)
			return rc;
		break;

	case CAMERA_WB_FLUORESCENT:
		CAM_MSG("mt9m113_set_wb: wb is FLUORESCENT\n");
		rc = mt9m113_i2c_write_table(&mt9m113_regs.wb_fluorescent_reg_settings[0],
                mt9m113_regs.wb_fluorescent_reg_settings_size);

		if (rc < 0)
			return rc;
		break;


	case CAMERA_WB_DAYLIGHT:
		CAM_MSG("mt9m113_set_wb: wb is DAYLIGHT\n");
		rc = mt9m113_i2c_write_table(&mt9m113_regs.wb_sunny_reg_settings[0],
                mt9m113_regs.wb_sunny_reg_settings_size);

		if (rc < 0)
			return rc;
		break;


	case CAMERA_WB_CLOUDY_DAYLIGHT:
		CAM_MSG("mt9m113_set_wb: wb is CLOUDY_DAYLIGHT\n");
		rc = mt9m113_i2c_write_table(&mt9m113_regs.wb_cloudy_reg_settings[0],
                mt9m113_regs.wb_cloudy_reg_settings_size);

		if (rc < 0)
			return rc;
		break;

	default:
		CAM_ERR("mt9m113: wrong white balance value\n");
		return -EFAULT;
	}

	mt9m113_ctrl->wb = wb;

	return 0;
	
}

/* =====================================================================================*/
/* mt9m113_set_brightness                                                                        								    */
/* =====================================================================================*/
static long mt9m113_set_brightness(int8_t brightness)
{

	long rc = 0;

	CAM_MSG("mt9m113_set_brightness: %d\n", brightness);	
// START : [MS910] sungmin.cho@lge.com 2011.05.18 android original parameter. change from luma-adaptation to exposure-compensation.
#if 1
	switch (brightness) {
	case -5:
		rc = mt9m113_i2c_write(mt9m113_client->addr,
				0x098C, 0xA24F, WORD_LEN);		// MCU_ADDRESS [AE_BASETARGET]
		if (rc < 0)
			return rc;
		
		rc = mt9m113_i2c_write(mt9m113_client->addr,
				0x0990, 0x0017, WORD_LEN);		// MCU_DATA_0
		if (rc < 0)
			return rc;
		break;

	case -4:
		rc = mt9m113_i2c_write(mt9m113_client->addr,
				0x098C, 0xA24F, WORD_LEN);		// MCU_ADDRESS [AE_BASETARGET]
		if (rc < 0)
			return rc;
		
		rc = mt9m113_i2c_write(mt9m113_client->addr,
				0x0990, 0x0022, WORD_LEN);		// MCU_DATA_0
		if (rc < 0)
			return rc;
		break;


	case -3:
		rc = mt9m113_i2c_write(mt9m113_client->addr,
				0x098C, 0xA24F, WORD_LEN);		// MCU_ADDRESS [AE_BASETARGET]
		if (rc < 0)
			return rc;
		
		rc = mt9m113_i2c_write(mt9m113_client->addr,
				0x0990, 0x002A, WORD_LEN);		// MCU_DATA_0
		if (rc < 0)
			return rc;
		break;


	case -2:
		rc = mt9m113_i2c_write(mt9m113_client->addr,
				0x098C, 0xA24F, WORD_LEN);		// MCU_ADDRESS [AE_BASETARGET]
		if (rc < 0)
			return rc;
		
		rc = mt9m113_i2c_write(mt9m113_client->addr,
				0x0990, 0x0032, WORD_LEN);		// MCU_DATA_0
		if (rc < 0)
			return rc;
		break;


	case -1:
		rc = mt9m113_i2c_write(mt9m113_client->addr,
				0x098C, 0xA24F, WORD_LEN);		// MCU_ADDRESS [AE_BASETARGET]
		if (rc < 0)
			return rc;
		
		rc = mt9m113_i2c_write(mt9m113_client->addr,
				0x0990, 0x003A, WORD_LEN);		// MCU_DATA_0
		if (rc < 0)
			return rc;
		break;

	case 0:
		rc = mt9m113_i2c_write(mt9m113_client->addr,
				0x098C, 0xA24F, WORD_LEN);		// MCU_ADDRESS [AE_BASETARGET]
		if (rc < 0)
			return rc;
		
		rc = mt9m113_i2c_write(mt9m113_client->addr,
				0x0990, 0x0040, WORD_LEN);		// MCU_DATA_0
		if (rc < 0)
			return rc;
		break;

	case 1:
		rc = mt9m113_i2c_write(mt9m113_client->addr,
				0x098C, 0xA24F, WORD_LEN);		// MCU_ADDRESS [AE_BASETARGET]
		if (rc < 0)
			return rc;
		
		rc = mt9m113_i2c_write(mt9m113_client->addr,
				0x0990, 0x005C, WORD_LEN);		// MCU_DATA_0
		if (rc < 0)
			return rc;
		break;

	case 2:
		rc = mt9m113_i2c_write(mt9m113_client->addr,
				0x098C, 0xA24F, WORD_LEN);		// MCU_ADDRESS [AE_BASETARGET]
		if (rc < 0)
			return rc;
		
		rc = mt9m113_i2c_write(mt9m113_client->addr,
				0x0990, 0x0067, WORD_LEN);		// MCU_DATA_0
		if (rc < 0)
			return rc;
		break;

	case 3:
		rc = mt9m113_i2c_write(mt9m113_client->addr,
				0x098C, 0xA24F, WORD_LEN);		// MCU_ADDRESS [AE_BASETARGET]
		if (rc < 0)
			return rc;
		
		rc = mt9m113_i2c_write(mt9m113_client->addr,
				0x0990, 0x0070, WORD_LEN);		// MCU_DATA_0
		if (rc < 0)
			return rc;
		break;

	case 4:
		rc = mt9m113_i2c_write(mt9m113_client->addr,
				0x098C, 0xA24F, WORD_LEN);		// MCU_ADDRESS [AE_BASETARGET]
		if (rc < 0)
			return rc;
		
		rc = mt9m113_i2c_write(mt9m113_client->addr,
				0x0990, 0x007E, WORD_LEN);		// MCU_DATA_0
		if (rc < 0)
			return rc;
		break;

	case 5:
		rc = mt9m113_i2c_write(mt9m113_client->addr,
				0x098C, 0xA24F, WORD_LEN);		// MCU_ADDRESS [AE_BASETARGET]
		if (rc < 0)
			return rc;
		
		rc = mt9m113_i2c_write(mt9m113_client->addr,
				0x0990, 0x0088, WORD_LEN);		// MCU_DATA_0
		if (rc < 0)
			return rc;
		break;
	
	default:
		CAM_ERR("mt9m113: wrong ev value, set to the default\n");
	}

#else

	switch (brightness) {
	case 0:
		rc = mt9m113_i2c_write(mt9m113_client->addr,
				0x098C, 0xA24F, WORD_LEN);		// MCU_ADDRESS [AE_BASETARGET]
		if (rc < 0)
			return rc;
		
		rc = mt9m113_i2c_write(mt9m113_client->addr,
				0x0990, 0x0017, WORD_LEN);		// MCU_DATA_0
		if (rc < 0)
			return rc;
		break;

	case 1:
		rc = mt9m113_i2c_write(mt9m113_client->addr,
				0x098C, 0xA24F, WORD_LEN);		// MCU_ADDRESS [AE_BASETARGET]
		if (rc < 0)
			return rc;
		
		rc = mt9m113_i2c_write(mt9m113_client->addr,
				0x0990, 0x0022, WORD_LEN);		// MCU_DATA_0
		if (rc < 0)
			return rc;
		break;


	case 2:
		rc = mt9m113_i2c_write(mt9m113_client->addr,
				0x098C, 0xA24F, WORD_LEN);		// MCU_ADDRESS [AE_BASETARGET]
		if (rc < 0)
			return rc;
		
		rc = mt9m113_i2c_write(mt9m113_client->addr,
				0x0990, 0x002A, WORD_LEN);		// MCU_DATA_0
		if (rc < 0)
			return rc;
		break;


	case 3:
		rc = mt9m113_i2c_write(mt9m113_client->addr,
				0x098C, 0xA24F, WORD_LEN);		// MCU_ADDRESS [AE_BASETARGET]
		if (rc < 0)
			return rc;
		
		rc = mt9m113_i2c_write(mt9m113_client->addr,
				0x0990, 0x0032, WORD_LEN);		// MCU_DATA_0
		if (rc < 0)
			return rc;
		break;


	case 4:
		rc = mt9m113_i2c_write(mt9m113_client->addr,
				0x098C, 0xA24F, WORD_LEN);		// MCU_ADDRESS [AE_BASETARGET]
		if (rc < 0)
			return rc;
		
		rc = mt9m113_i2c_write(mt9m113_client->addr,
				0x0990, 0x003A, WORD_LEN);		// MCU_DATA_0
		if (rc < 0)
			return rc;
		break;

	case 5:
		rc = mt9m113_i2c_write(mt9m113_client->addr,
				0x098C, 0xA24F, WORD_LEN);		// MCU_ADDRESS [AE_BASETARGET]
		if (rc < 0)
			return rc;
		
		rc = mt9m113_i2c_write(mt9m113_client->addr,
				0x0990, 0x0040, WORD_LEN);		// MCU_DATA_0
		if (rc < 0)
			return rc;
		break;

	case 6:
		rc = mt9m113_i2c_write(mt9m113_client->addr,
				0x098C, 0xA24F, WORD_LEN);		// MCU_ADDRESS [AE_BASETARGET]
		if (rc < 0)
			return rc;
		
		rc = mt9m113_i2c_write(mt9m113_client->addr,
				0x0990, 0x005C, WORD_LEN);		// MCU_DATA_0
		if (rc < 0)
			return rc;
		break;

	case 7:
		rc = mt9m113_i2c_write(mt9m113_client->addr,
				0x098C, 0xA24F, WORD_LEN);		// MCU_ADDRESS [AE_BASETARGET]
		if (rc < 0)
			return rc;
		
		rc = mt9m113_i2c_write(mt9m113_client->addr,
				0x0990, 0x0067, WORD_LEN);		// MCU_DATA_0
		if (rc < 0)
			return rc;
		break;

	case 8:
		rc = mt9m113_i2c_write(mt9m113_client->addr,
				0x098C, 0xA24F, WORD_LEN);		// MCU_ADDRESS [AE_BASETARGET]
		if (rc < 0)
			return rc;
		
		rc = mt9m113_i2c_write(mt9m113_client->addr,
				0x0990, 0x0070, WORD_LEN);		// MCU_DATA_0
		if (rc < 0)
			return rc;
		break;

	case 9:
		rc = mt9m113_i2c_write(mt9m113_client->addr,
				0x098C, 0xA24F, WORD_LEN);		// MCU_ADDRESS [AE_BASETARGET]
		if (rc < 0)
			return rc;
		
		rc = mt9m113_i2c_write(mt9m113_client->addr,
				0x0990, 0x007E, WORD_LEN);		// MCU_DATA_0
		if (rc < 0)
			return rc;
		break;

	case 10:
		rc = mt9m113_i2c_write(mt9m113_client->addr,
				0x098C, 0xA24F, WORD_LEN);		// MCU_ADDRESS [AE_BASETARGET]
		if (rc < 0)
			return rc;
		
		rc = mt9m113_i2c_write(mt9m113_client->addr,
				0x0990, 0x0088, WORD_LEN);		// MCU_DATA_0
		if (rc < 0)
			return rc;
		break;
	
	default:
		CAM_ERR("mt9m113: wrong ev value, set to the default\n");
	}
#endif
// END : [MS910] sungmin.cho@lge.com 2011.05.18 android original parameter. change from luma-adaptation to exposure-compensation.

	if(rc<0)
		return rc;
	
	mt9m113_ctrl->brightness = brightness;

	return rc;
}

/* START : [MS910] sungmin.cho@lge.com 2011.06.24 add focus distances for GB */
/*---------------------------------------------------------------------------
    mt9m113_get_focus_distances
   ---------------------------------------------------------------------------*/
int32_t mt9m113_get_focus_distances(struct focus_distances_type *distances)
{
	int32_t rc = 0;
	
	CAM_MSG("mt9m113_get_focus_distances: called: \n");
	// maximum distance
	distances->near_focus = 0.1; 
	distances->current_focus = 0.6;
	distances->far_focus = 0.6;
	
	return rc;
}
/* END : [MS910] sungmin.cho@lge.com 2011.06.24 add focus distances for GB */


/* BUG FIX : msm is changed preview mode but sensor is not change preview mode. */
/* so there is case that vfe get capture image on preview mode */
void mt9m113_check_sensor_mode(void)
{
	unsigned short mcu_address_sts =0, mcu_data =0;
	int i, rc;
	
	for(i=0; i<50; i++){

		/* MCU_ADDRESS : check mcu_address */
		rc = mt9m113_i2c_read(mt9m113_client->addr,
				0x098C, &mcu_address_sts, WORD_LEN);
		if (rc < 0){
			CAM_ERR("mt9m113: reading mcu_address_sts fail\n");
			return rc;
		}

		/* MCU_DATA_0 : check mcu_data */
		rc = mt9m113_i2c_read(mt9m113_client->addr,
				0x0990, &mcu_data, WORD_LEN);
		if (rc < 0){
			CAM_ERR("mt9m113: reading mcu_data fail\n");
			return rc;
		}
  
	
		if( ((mcu_address_sts & 0xA103) == 0xA103) && (mcu_data == 0x00)){  
			break;
		}
		msleep(10); 
	}
}

long mt9m113_set_preview_fps(uint16_t fps)
{
	uint16_t reg_value = 0;  /* MODE_SENSOR_FRAME_LENGTH_A */
	uint16_t mode = 0;
	uint32_t fps_range = 0;
	long rc = 0;	

	printk("mt9m113_set_preview_fps : fps = %d, current fps = %d", fps, mt9m113_ctrl->video_mode_fps); 

	if(fps > MT9M113_FPS_30)
		fps = MT9M113_FPS_30;

	if(mt9m113_ctrl->video_mode_fps==fps){
		return rc;
	}		
	mode = fps;

	fps_range = (((uint32_t)fps) << 16) | fps;
	rc = mt9m113_set_fps_range(fps_range);
	
#if 0	
	switch(fps){
	case MT9M113_FPS_5:
		reg_value = 0x0CD5;
		break;
	case MT9M113_FPS_7:
		reg_value = 0x092B;
		break;
	case MT9M113_FPS_10:		
		reg_value = 0x066A;
		break;
	case MT9M113_FPS_12:		
		reg_value = 0x0559;
		break;
	case MT9M113_FPS_15:		
		reg_value = 0x0447;
		break;	
	case MT9M113_FPS_20:
		reg_value = 0x0335;
		break;
	case MT9M113_FPS_24:
		reg_value = 0x02AC;
		break;
	case MT9M113_FPS_25:
		reg_value = 0x0291;
		break;
	case MT9M113_FPS_30:
		reg_value = 0x0239;
		break;
	default: /* Normal */
		mode = MT9M113_FPS_NORMAL;
		break;
	}

	CAM_MSG("[mt9m113] fps[%d] MODE_SENSOR_FRAME_LENGTH_A:%d \n",fps,reg_value);

	if(mode != MT9M113_FPS_NORMAL){

		rc = mt9m113_i2c_write(mt9m113_client->addr,
				MT9M113_REG_MCU_VAR_ADDR, 0x271F, WORD_LEN);	/* [MODE_SENSOR_FRAME_LENGTH_A]*/

		rc = mt9m113_i2c_write(mt9m113_client->addr,
				MT9M113_REG_MCU_VAR_DATA0, reg_value, WORD_LEN);

		rc = mt9m113_i2c_write(mt9m113_client->addr,
				MT9M113_REG_MCU_VAR_ADDR, 0xA20C, WORD_LEN); /* [AE_MAX_INDEX] */

		rc = mt9m113_i2c_write(mt9m113_client->addr,
				MT9M113_REG_MCU_VAR_DATA0, 0x0004, WORD_LEN);

		rc = mt9m113_i2c_write(mt9m113_client->addr,
				MT9M113_REG_MCU_VAR_ADDR, 0xA21B, WORD_LEN); /* [AE_INDEX] */
	
		rc = mt9m113_i2c_write(mt9m113_client->addr,
				MT9M113_REG_MCU_VAR_DATA0, 0x0004, WORD_LEN);

		rc = mt9m113_i2c_write(mt9m113_client->addr,
				MT9M113_REG_MCU_VAR_ADDR, 0xA103, WORD_LEN); /* [SEQ_CMD]*/

		rc = mt9m113_i2c_write(mt9m113_client->addr,
				MT9M113_REG_MCU_VAR_DATA0, 0x0006, WORD_LEN);

		msleep(300);
	
		rc = mt9m113_i2c_write(mt9m113_client->addr,
				MT9M113_REG_MCU_VAR_ADDR, 0xA103, WORD_LEN); /* [SEQ_CMD]*/
	
		rc = mt9m113_i2c_write(mt9m113_client->addr,
				MT9M113_REG_MCU_VAR_DATA0, 0x0005, WORD_LEN);

		msleep(300);

	}
	else{  /* Normal Mode ......*/
		rc = mt9m113_i2c_write(mt9m113_client->addr,
				MT9M113_REG_MCU_VAR_ADDR, 0x271F, WORD_LEN);	/* [MODE_SENSOR_FRAME_LENGTH_A]*/
		
		rc = mt9m113_i2c_write(mt9m113_client->addr,
				MT9M113_REG_MCU_VAR_DATA0,0x024A, WORD_LEN);
		
		rc = mt9m113_i2c_write(mt9m113_client->addr,
				MT9M113_REG_MCU_VAR_ADDR, 0xA20C, WORD_LEN); /* [AE_MAX_INDEX] */
		
		rc = mt9m113_i2c_write(mt9m113_client->addr,
				MT9M113_REG_MCU_VAR_DATA0,0x0011, WORD_LEN);

		rc = mt9m113_i2c_write(mt9m113_client->addr,
				MT9M113_REG_MCU_VAR_ADDR, 0xA103, WORD_LEN); /* [AE_MAX_INDEX] */
	
		rc = mt9m113_i2c_write(mt9m113_client->addr,
				MT9M113_REG_MCU_VAR_DATA0,0x0006, WORD_LEN);
		
	}

#endif	
	mt9m113_ctrl->video_mode_fps = mode; 	

	return rc;
	
}

// START : ADD : sungmin.cho@lge.com 2011.06.20 for CTS
static int mt9m113_set_fps_range(int fps)
{
	printk(KERN_ERR " >> %s START \n", __func__);
	uint16_t reg_value = 0;  
	int rc = 0;
	uint16_t min_fps = (((uint32_t) fps) & 0xFFFF0000) >> 16;
	uint16_t max_fps = (fps & 0x0000FFFF);

	printk("mt9m113 : mt9m113_set_fps_range : min_fps = %d, max_fps = %d", min_fps, max_fps); 

	if(!atomic_read(&init_reg_mode)== CAMERA_VIDEO_MODE_ON) // snapshot mode
	{
		if((min_fps == MT9M113_FPS_RANGE_7500) && (max_fps == MT9M113_FPS_RANGE_22000))
		{
			rc = mt9m113_i2c_write_table(&mt9m113_regs.fps_range_7p5_22_reg_settings[0],
					mt9m113_regs.fps_range_7p5_22_reg_settings_size);				
		}
		else if((min_fps == MT9M113_FPS_RANGE_15000) && (max_fps == MT9M113_FPS_RANGE_15000))
		{
			rc = mt9m113_i2c_write_table(&mt9m113_regs.fps_range_15_reg_settings[0],
					mt9m113_regs.fps_range_15_reg_settings_size);				
		}
		else if(((min_fps == MT9M113_FPS_RANGE_18000) && (max_fps == MT9M113_FPS_RANGE_20000))
			|| ((min_fps == MT9M113_FPS_RANGE_20000) && (max_fps == MT9M113_FPS_RANGE_20000)))
		{
			rc = mt9m113_i2c_write_table(&mt9m113_regs.fps_range_20_reg_settings[0],
					mt9m113_regs.fps_range_20_reg_settings_size);				
		}
		/* 30fps */
		else if((min_fps == MT9M113_FPS_RANGE_30000) && (max_fps == MT9M113_FPS_RANGE_30000))
		{
			rc = mt9m113_i2c_write_table(&mt9m113_regs.fps_range_30_reg_settings[0],
					mt9m113_regs.fps_range_30_reg_settings_size);				
		}	
	}
	else // video mode
	{		
		if((min_fps == MT9M113_FPS_RANGE_15000) && (max_fps == MT9M113_FPS_RANGE_15000))
		{
			rc = mt9m113_i2c_write_table(&mt9m113_regs.fps_range_15_video_reg_settings[0],
					mt9m113_regs.fps_range_15_video_reg_settings_size);				
		}
		else if((min_fps == MT9M113_FPS_RANGE_20000) && (max_fps == MT9M113_FPS_RANGE_20000))
		{
			rc = mt9m113_i2c_write_table(&mt9m113_regs.fps_range_20_video_reg_settings[0],
					mt9m113_regs.fps_range_20_video_reg_settings_size);				
		}
		else if((min_fps == MT9M113_FPS_RANGE_30000) && (max_fps == MT9M113_FPS_RANGE_30000))
		{
			rc = mt9m113_i2c_write_table(&mt9m113_regs.fps_range_30_video_reg_settings[0],
					mt9m113_regs.fps_range_30_video_reg_settings_size);				
		}
	}

	printk(KERN_ERR " << %s END (rc = %d)\n", __func__, rc);
	return rc;
}
// END : sungmin.cho@lge.com 2011.06.20 for CTS

static long mt9m113_set_sensor_mode(int mode)
{
	int32_t rc = 0;

	switch (mode) {
	case SENSOR_PREVIEW_MODE:
		CAM_MSG("mt9m113_set_sensor_mode: sensor mode is PREVIEW\n");
		
		if(mt9m113_ctrl->previous_mode == SENSOR_SNAPSHOT_MODE){
			rc = mt9m113_i2c_write(mt9m113_client->addr,
						MT9M113_REG_MCU_VAR_ADDR,0xA115,WORD_LEN);
			rc = mt9m113_i2c_write(mt9m113_client->addr,
						MT9M113_REG_MCU_VAR_DATA0,0x0000,WORD_LEN);	
			rc = mt9m113_i2c_write(mt9m113_client->addr,
						MT9M113_REG_MCU_VAR_ADDR,0xA103,WORD_LEN);
			rc = mt9m113_i2c_write(mt9m113_client->addr,
						MT9M113_REG_MCU_VAR_DATA0,0x0001,WORD_LEN);	
			if(rc < 0)
				return rc;
			
			mt9m113_check_sensor_mode();
		}
		
	    break;
 	case SENSOR_SNAPSHOT_MODE:
		
		CAM_MSG("mt9m113_set_sensor_mode: SENSOR_SNAPSHOT_MODE: 1280x960 \n");
			
		rc = mt9m113_i2c_write(mt9m113_client->addr,
					MT9M113_REG_MCU_VAR_ADDR,0xA115,WORD_LEN);
		rc = mt9m113_i2c_write(mt9m113_client->addr,
					MT9M113_REG_MCU_VAR_DATA0,0x0002,WORD_LEN);
		rc = mt9m113_i2c_write(mt9m113_client->addr,
					MT9M113_REG_MCU_VAR_ADDR,0xA103,WORD_LEN);
		rc = mt9m113_i2c_write(mt9m113_client->addr,
					MT9M113_REG_MCU_VAR_DATA0,0x0002,WORD_LEN);

		if(rc < 0)
			return rc;

		mt9m113_check_sensor_mode();		
		break;
	default:
		return -EINVAL;
	}

	mt9m113_ctrl->previous_mode = mode;

   
	return 0;
	
}

/* =====================================================================================*/
/*  mt9m113_sensor_config                                                                        								    */
/* =====================================================================================*/
int mt9m113_sensor_config(void __user *argp)
{
	struct sensor_cfg_data cfg_data;
	long   rc = 0;

	CAM_MSG("mt9m113_sensor_config...................\n");	
	 
	if (copy_from_user(&cfg_data,(void *)argp,sizeof(struct sensor_cfg_data)))
		return -EFAULT;

	CAM_MSG("mt9m113_ioctl, cfgtype = %d, mode = %d\n", cfg_data.cfgtype, cfg_data.mode);

	mutex_lock(&mt9m113_mutex);

	switch (cfg_data.cfgtype) {
	 case CFG_SET_MODE:
		CAM_MSG("mt9m113_sensor_config: command is CFG_SET_MODE\n");
			
		rc = mt9m113_set_sensor_mode(cfg_data.mode); 
		break;

	 case CFG_SET_EFFECT:
		 CAM_MSG("mt9m113_sensor_config: command is CFG_SET_EFFECT\n");
			 
		 rc = mt9m113_set_effect(cfg_data.cfg.effect);
		 break;
		 
	 case CFG_SET_WB:
		 CAM_MSG("mt9m113_sensor_config: command is CFG_SET_WB\n");
			 
		 rc = mt9m113_set_wb(cfg_data.wb);
		 break;
		 
	 case CFG_SET_BRIGHTNESS:
		 CAM_MSG("mt9m113_sensor_config: command is CFG_SET_BRIGHTNESS\n");

		 rc = mt9m113_set_brightness(cfg_data.brightness);
		 break;
	  case CFG_SET_FPS:
	  	 if(atomic_read(&init_reg_mode)== CAMERA_VIDEO_MODE_ON){ /* variable frame rate */
	  	 	CAM_MSG("%s:CFG_SET_FPS : user fps[%d]\n",__func__, cfg_data.cfg.fps.fps_div);
		 	rc = mt9m113_set_preview_fps(cfg_data.cfg.fps.fps_div);
	  	 }
		 break;
		 
 /* START : [MS910] sungmin.cho@lge.com 2011.06.24 add focus distances for GB */
	 case CFG_GET_FOCUS_DISTANCES:			 
		 rc = mt9m113_get_focus_distances(&cfg_data.focus_distances);
		 if (copy_to_user((void *)argp, &cfg_data,sizeof(struct sensor_cfg_data)))
			 rc = -EFAULT;
		 break;
 /* END : [MS910] sungmin.cho@lge.com 2011.06.24 add focus distances for GB */

// START : ADD : sungmin.cho@lge.com 2011.06.20 for CTS
	case CFG_SET_FPS_RANGE:
		rc = mt9m113_set_fps_range(cfg_data.mode);
		break;
// END : sungmin.cho@lge.com 2011.06.20 for CTS
	  default:	 	
		CAM_MSG("mt9m113_sensor_config:cfg_data.cfgtype[%d]\n",cfg_data.cfgtype);
		rc = 0;//-EINVAL; 
		break;
	}

	mutex_unlock(&mt9m113_mutex);
	
	if (rc < 0){
		CAM_ERR("mt9m113: ERROR in sensor_config, %ld\n", rc);
	}
	
	
	return rc;
 	
}

int mt9m113_init_sensor_reg(void)
{
	int rc = 0;

	printk("%s\n",__func__);

	if(atomic_read(&init_reg_mode)== CAMERA_VIDEO_MODE_ON){
			printk("[mt9m113_sensor_init] init_reg_mode 30FPS..... \n");
			rc = mt9m113_i2c_write_table(&mt9m113_regs.init_reg_vt_settings[0],
				 mt9m113_regs.init_reg_vt_settings_size);	
	
	}else{
			printk("[mt9m113_sensor_init] init_reg_mode NORMARL.....\n");
			rc = mt9m113_i2c_write_table(&mt9m113_regs.init_reg_settings[0],
				 mt9m113_regs.init_reg_settings_size);
	}

	return rc;
}

/* =====================================================================================*/
/* 	mt9m113 sysfs                                                                          									   */
/* =====================================================================================*/
static ssize_t mt9m113_mclk_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	int value;

	sscanf(buf, "%d", &value);
	
	//vt_cam_mclk_rate = value;

	printk("[mt9m113] mclk_rate = %d\n", value);

	return size;
}
//2011.07.28 - CTS FAIL android.permission.cts.FileSystemPermissionTest#testAllBlockDevicesAreNotReadableWritable
static DEVICE_ATTR(mclk, 0775 , NULL, mt9m113_mclk_store);
// static DEVICE_ATTR(mclk, S_IRWXUGO, NULL, mt9m113_mclk_store);

static ssize_t mt9m113_init_code_store(struct device* dev, struct device_attribute* attr, const char* buf, size_t n)
{
	int val;

	if (mt9m113_ctrl == NULL)
		return 0;

	sscanf(buf,"%x",&val);

	CAM_ERR("mt9m113: init code type [0x%x] \n",val);

	/* 		0 : 1.3M YUV snapshot
    	    1 : support the preview 30fps
	*/
	if(val < 0 || val >= CAMERA_VIDEO_MODE_MAX ) {
		CAM_ERR("mt9m113: invalid init. type[%d] \n",val);
		val = CAMERA_VIDEO_MODE_OFF;
	}

	atomic_set(&init_reg_mode,val);
	
	return n;
	
}

//2011.07.28 - CTS FAIL android.permission.cts.FileSystemPermissionTest#testAllBlockDevicesAreNotReadableWritable
static DEVICE_ATTR(init_code, 0664 , NULL, mt9m113_init_code_store);
//static DEVICE_ATTR(init_code, S_IRUGO|S_IWUGO, NULL, mt9m113_init_code_store);

static struct attribute* mt9m113_sysfs_attrs[] = {
	&dev_attr_mclk.attr,
	&dev_attr_init_code.attr,
	NULL
};

static int mt9m113_sysfs_add(struct kobject* kobj)
{
	int i, n, ret;
	
	n = ARRAY_SIZE(mt9m113_sysfs_attrs);
	for(i = 0; i < n; i++){
		if(mt9m113_sysfs_attrs[i]){
			ret = sysfs_create_file(kobj, mt9m113_sysfs_attrs[i]);
			if(ret < 0){
				CAM_ERR("mt9m113 sysfs is not created\n");
			}
		}
	}

	return 0;
}
/*======================================================================================*/
/*  end :  sysfs                                                                         										    */
/*======================================================================================*/


/* =====================================================================================*/
/*  mt9m113_sensor_init                                                                        								    */
/* =====================================================================================*/
int mt9m113_sensor_init(const struct msm_camera_sensor_info *data)
{
	int rc =0;
	int num = 0;
	
	if (data)
		mt9m113_ctrl->sensordata = data;
	else
		goto init_fail;

	data->pdata->camera_power_on();

	rc = mt9m113_init_sensor_reg();
	if(rc < 0){
		for(num = 0 ; num < 5 ; num++){
			msleep(10);
			rc = mt9m113_init_sensor_reg();
			if(rc < 0){
				printk(KERN_ERR"%s: initial code setting fail\n",__func__);
			}else{
				printk(KERN_DEBUG"%s: initial code setting success\n",__func__);
				break;
			}
		}
	}
	
	if(rc < 0 ){
	   CAM_ERR("mt9m113: mt9m113 writing fail\n");
	   goto init_fail; 
	}
#if 0
	{
		unsigned short sensor_read_mode_A, sensor_read_mode_B;

		mt9m113_i2c_write(mt9m113_client->addr, 0x098C, 0x2717, WORD_LEN);
		rc = mt9m113_i2c_read(mt9m113_client->addr,
				0x0990, &sensor_read_mode_A, WORD_LEN);
		if (rc < 0){
			CAM_ERR("mt9m113: reading sensor_read_mode_A fail\n");
			return rc;
		}

		mt9m113_i2c_write(mt9m113_client->addr, 0x098C, 0x272D, WORD_LEN);
		rc = mt9m113_i2c_read(mt9m113_client->addr,
				0x0990, &sensor_read_mode_B, WORD_LEN);
		if (rc < 0){
			CAM_ERR("mt9m113: reading sensor_read_mode_B fail\n");
			return rc;
		}
		printk("[mt9m113_sensor_init] sensor_read_mode_A = 0x%x, sensor_read_mode_B = 0x%x\n", sensor_read_mode_A, sensor_read_mode_B); 	
	}
#endif

	printk("[mt9m113_sensor_init] init register successful. rc = %d\n", rc); 
	
	mt9m113_ctrl->previous_mode = SENSOR_PREVIEW_MODE;
	mt9m113_ctrl->video_mode_fps = MT9M113_FPS_30;

	return rc;

init_fail:
	CAM_ERR("mt9m113:mt9m113_sensor_init failed\n");
	return rc;

}
/* =====================================================================================*/
/*  mt9m113_sensor_release                                                                        								    */
/* =====================================================================================*/
int mt9m113_sensor_release(void)
{
    mt9m113_ctrl->sensordata->pdata->camera_power_off();
    mt9m113_ctrl->video_mode_fps = MT9M113_FPS_30;	

	atomic_set(&init_reg_mode,CAMERA_VIDEO_MODE_OFF);

	return 0;	
}
static int mt9m113_i2c_probe(struct i2c_client *client,const struct i2c_device_id *id)
{
	int rc = 0;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		rc = -ENOTSUPP;
		goto probe_failure;
	}

	mt9m113_client = client;

	rc = mt9m113_sysfs_add(&client->dev.kobj);

	return rc;

probe_failure:	
	CAM_ERR("mt9m113_i2c_probe.......rc[%d]....................\n",rc);
	return rc;		
	
}

static int mt9m113_i2c_remove(struct i2c_client *client)
{
	return 0;
}

static const struct i2c_device_id mt9m113_i2c_ids[] = {
	{"mt9m113", 0},
	{ /* end of list */},
};

static struct i2c_driver mt9m113_i2c_driver = {
	.probe  = mt9m113_i2c_probe,
	.remove = mt9m113_i2c_remove, 
	.id_table = mt9m113_i2c_ids,	
	.driver   = {
		.name =  "mt9m113",
		.owner= THIS_MODULE,	
    },
};

static int mt9m113_sensor_probe(const struct msm_camera_sensor_info *info,
	                                 	struct msm_sensor_ctrl *s)
{
	int rc = 0;

	CAM_MSG("mt9m113_sensor_probe...................\n");

	rc = i2c_add_driver(&mt9m113_i2c_driver);
	if (rc < 0) {
		rc = -ENOTSUPP;
		CAM_ERR("[mt9m113_sensor_probe] return value :ENOTSUPP\n");
		goto probe_done;
	}
	
	mt9m113_ctrl = kzalloc(sizeof(struct mt9m113_ctrl_t), GFP_KERNEL);  
	if (!mt9m113_ctrl) {
		CAM_ERR("mt9m113_init failed!\n");
		rc = -ENOMEM;
		goto probe_done;
	}

	atomic_set(&init_reg_mode,CAMERA_VIDEO_MODE_OFF);

	s->s_init    = mt9m113_sensor_init;
	s->s_release = mt9m113_sensor_release;
	s->s_config  = mt9m113_sensor_config;
	s->s_camera_type = FRONT_CAMERA_2D;
	s->s_mount_angle = 180; // 0;

    return 0;

probe_done:
	return rc;
	
}
static int __mt9m113_probe(struct platform_device *pdev)
{	
	return msm_camera_drv_start(pdev,mt9m113_sensor_probe);	
}

static struct platform_driver msm_camera_driver = {
	.probe = __mt9m113_probe,
	.driver = {
		.name = "msm_camera_mt9m113",
		.owner = THIS_MODULE,
	},
};
static int __init mt9m113_init(void)
{
	CAM_MSG("mt9m113_init...................\n");
		
	return platform_driver_register(&msm_camera_driver);  
}

module_init(mt9m113_init);

