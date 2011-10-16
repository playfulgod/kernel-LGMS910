/* drivers/media/video/msm/isx006.h
*
* This software is for SONY 5M sensor 
*  
* Copyright (C) 2010-2011 LGE Inc.  
*
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


#ifndef ISX006_H
#define ISX006_H

#include <linux/types.h>
#include <mach/camera.h>


#if 1//def CONFIG_MSM_CAMERA_DEBUG
#define CAM_MSG(fmt, args...)	printk(KERN_ERR "CAM_MSG [%-18s:%5d] " fmt, __FUNCTION__, __LINE__, ## args)
#define CAM_ERR(fmt, args...)	printk(KERN_ERR "CAM_ERR [%-18s:%5d] " fmt, __FUNCTION__, __LINE__, ## args)
#else
#define CAM_MSG(fmt, args...)
#define CAM_ERR(fmt, args...)
#endif

//#define INPUT_CLK_CHANGE_MODE

extern struct isx006_reg isx006_regs;

enum isx006_width {
	WORD_LEN,
	BYTE_LEN,
	ADDRESS_TUNE
};

struct isx006_i2c_reg_conf {
	unsigned short waddr;
	unsigned short wdata;
	enum isx006_width width;
	unsigned short mdelay_time;	
};

struct isx006_register_address_value_pair {
	uint16_t register_address;
	uint16_t register_value;
	enum isx006_width register_length;
};

struct isx006_reg {
	const struct isx006_i2c_reg_conf *pll;
	uint16_t pll_size;

	const struct isx006_i2c_reg_conf *pll_vt_mode;
	uint16_t pll_vt_mode_size;
	
	const struct isx006_i2c_reg_conf *init;
	uint16_t init_size;

	const struct isx005_i2c_reg_conf *af_driver_reg_init;
	uint16_t af_driver_reg_init_size;
	
	/*register for scene  -- Normal : for flicker preview 15 fps */
	const struct isx006_i2c_reg_conf *scene_normal_reg_settings;
	uint16_t scene_normal_reg_settings_size;	
	const struct isx006_i2c_reg_conf *scene_portrait_reg_settings;
	uint16_t scene_portrait_reg_settings_size;	
	const struct isx006_i2c_reg_conf *scene_landscape_reg_settings;
	uint16_t scene_landscape_reg_settings_size;
	const struct isx006_i2c_reg_conf *scene_sport_reg_settings;
	uint16_t scene_sport_reg_settings_size;
	const struct isx006_i2c_reg_conf *scene_sunset_reg_settings;
	uint16_t scene_sunset_reg_settings_size;
	const struct isx006_i2c_reg_conf *scene_night_reg_settings;
	uint16_t scene_night_reg_settings_size; 

	
	/*register for scene  -- VT mode : for flicker preview 30 fps */
	const struct isx006_i2c_reg_conf *scene_normal_vt_reg_settings;
	uint16_t scene_normal_vt_reg_settings_size;	
	const struct isx006_i2c_reg_conf *scene_portrait_vt_reg_settings;
	uint16_t scene_portrait_vt_reg_settings_size;	
	const struct isx006_i2c_reg_conf *scene_landscape_vt_reg_settings;
	uint16_t scene_landscape_vt_reg_settings_size;
	const struct isx006_i2c_reg_conf *scene_sport_vt_reg_settings;
	uint16_t scene_sport_vt_reg_settings_size;
	const struct isx006_i2c_reg_conf *scene_sunset_vt_reg_settings;
	uint16_t scene_sunset_vt_reg_settings_size;
	const struct isx006_i2c_reg_conf *scene_night_vt_reg_settings;
	uint16_t scene_night_vt_reg_settings_size;


	/*register for AF*/
	const struct isx006_i2c_reg_conf *AF_reg_settings;
	uint16_t AF_reg_settings_size;
	const struct isx006_i2c_reg_conf *AF_driver_reg_settings;
	uint16_t AF_driver_reg_settings_size;
	const struct isx006_i2c_reg_conf *AF_nomal_reg_settings;
	uint16_t AF_nomal_reg_settings_size;
	const struct isx006_i2c_reg_conf *AF_macro_reg_settings;
	uint16_t AF_macro_reg_settings_size;
	const struct isx006_i2c_reg_conf *manual_focus_reg_settings;
	uint16_t manual_focus_reg_settings_size;
	const struct isx006_i2c_reg_conf *manual_focus_normal_reg_settings;
	uint16_t manual_focus_normal_reg_settings_size;
	
	/*register for iso*/
	const struct isx006_i2c_reg_conf *iso_auto_reg_settings;
	uint16_t iso_auto_reg_settings_size;
	const struct isx006_i2c_reg_conf *iso_100_reg_settings;
	uint16_t iso_100_reg_settings_size;
	const struct isx006_i2c_reg_conf *iso_200_reg_settings;
	uint16_t iso_200_reg_settings_size;
	const struct isx006_i2c_reg_conf *iso_400_reg_settings;
	uint16_t iso_400_reg_settings_size;

	/* register for led flash */
	const struct isx006_i2c_reg_conf *led_flash_on_reg_settings;
	uint16_t led_flash_on_reg_settings_size;
	const struct isx006_i2c_reg_conf *led_flash_off_reg_settings;
	uint16_t led_flash_off_reg_settings_size;

};






int32_t isx006_i2c_write_table(struct isx006_i2c_reg_conf const *reg_conf_tbl,
									int num_of_items_in_table);

int32_t isx006_i2c_write_ext(unsigned short waddr, unsigned short wdata, 
								 enum isx006_width width);

int32_t isx006_i2c_read_ext(unsigned short raddr,unsigned short *rdata,
								 enum isx006_width width);


#endif 

