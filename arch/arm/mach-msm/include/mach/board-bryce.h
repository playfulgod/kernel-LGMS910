/* arch/arm/mach-msm/include/mach/board_lge.h
 * Copyright (C) 2010 LGE Corporation.
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
#ifndef __ASM_ARCH_MSM_BOARD_BRYCE_H
#define __ASM_ARCH_MSM_BOARD_BRYCE_H

/* LGE_CHANGE [sungmin.shin@lge.com] 2010-04-19 LGE common define */
#include <linux/types.h>
#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/i2c-gpio.h>
#include <linux/rfkill.h>
#include <linux/platform_device.h>
#include <asm/setup.h>
/* BEGIN:0010882        ehgrace.kim@lge.com     2010.11.15*/
/* MOD: add the call mode */
#define LGE_AUDIO_PATH 1
/* END:0010882        ehgrace.kim@lge.com     2010.11.15*/

#if __GNUC__
#define __WEAK __attribute__((weak))
#endif

/* define PMEM address size */
//#define MSM_PMEM_MDP_SIZE      0x1C91000
//#define MSM_PMEM_ADSP_SIZE     0xAE4000
//#define MSM_PMEM_AUDIO_SIZE    0x121000
//#define MSM_FB_SIZE            0x177000
//#define MSM_GPU_PHYS_SIZE      SZ_2M
//#define PMEM_KERNEL_EBI1_SIZE  0x64000

/* board revision information */
enum {
	EVB         = 0,
	LGE_REV_A,
	LGE_REV_B,
	LGE_REV_C,
	LGE_REV_D,
	LGE_REV_E,
	LGE_REV_TOT_NUM,
};

/* define gpio pin number of i2c-gpio */
struct gpio_i2c_pin {
	unsigned int sda_pin;
	unsigned int scl_pin;
	unsigned int reset_pin;
	unsigned int irq_pin;
};

/* touch screen platform data */
struct touch_platform_data {
	int ts_x_min;
	int ts_x_max;
	int ts_y_min;
	int ts_y_max;
	int (*power)(unsigned char onoff);
	int irq;
	int scl;
	int sda;
};

/* pp2106 qwerty platform data */
struct pp2106_platform_data {
	unsigned int reset_pin;
	unsigned int irq_pin;
	unsigned int sda_pin;
	unsigned int scl_pin;
	unsigned int keypad_row;
	unsigned int keypad_col;
	unsigned char *keycode;
};

/* bu52031 hall ic platform data */
struct bu52031_platform_data {
	unsigned int irq_pin;
	unsigned int prohibit_time;
};

/* gpio switch platform data */
struct lge_gpio_switch_platform_data {
	const char *name;
	unsigned *gpios;
	size_t num_gpios;
	unsigned long irqflags;
	unsigned int wakeup_flag;
	int (*work_func)(void);
	char *(*print_state)(int state);
	int (*sysfs_store)(const char *buf, size_t size);
};
/* kwangdo.yi 20100819
   proxi sensor changed from LG_HW_REV3 and new data structure used
   */
#if defined(LG_HW_REV1) || defined(LG_HW_REV2)
/* proximity platform data */
struct proximity_platform_data {
	int irq_num;
	int (*power)(unsigned char onoff);
};

/* acceleration platform data */
struct acceleration_platform_data {
	int irq_num;
	int (*power)(unsigned char onoff);
};
#endif

/* kr3dh acceleration platform data */
struct kr3dh_platform_data {
	int poll_interval;
	int min_interval;

	u8 g_range;

	u8 axis_map_x;
	u8 axis_map_y;
	u8 axis_map_z;

	u8 negate_x;
	u8 negate_y;
	u8 negate_z;

	int (*kr_init)(void);
	void (*kr_exit)(void);
	int (*power_on)(void);
	int (*power_off)(void);
	int (*gpio_config)(int config);
};

/* ecompass platform data */
struct ecom_platform_data {
	int pin_int;
	int pin_rst;
	int (*power)(unsigned char onoff);
	char accelerator_name[20];
	int fdata_sign_x;
        int fdata_sign_y;
        int fdata_sign_z;
	int fdata_order0;
	int fdata_order1;
	int fdata_order2;
	int sensitivity1g;
	s16 *h_layout;
	s16 *a_layout;
};

/* gyro(ami602) platform data */
struct gyro_platform_data {
        int pin_rst;
        int pin_busy;
        int pin_trg;
        int (*power)(unsigned char onoff);
};

/* msm pmic leds platform data */
struct msm_pmic_leds_pdata {
	struct led_classdev *custom_leds;
	int (*register_custom_leds)(struct platform_device *pdev);
	void (*unregister_custom_leds)(void);
	void (*suspend_custom_leds)(void);
	void (*resume_custom_leds)(void);
	int (*msm_keypad_led_set)(unsigned char value);
};


/* LED flash platform data */
struct led_flash_platform_data {
	int gpio_flen;
	int gpio_en_set;
	int gpio_inh;
};

/* android vibrator platform data */
struct android_vibrator_platform_data {
	int enable_status;
	int (*power_set)(int enable); 		/* LDO Power Set Function */
	int (*pwn_set)(int enable, int gain); 		/* PWM Set Function */
	int (*ic_enable_set)(int enable); 	/* Motor IC Set Function */
};

/* bd6084gu backight, pmic */
#define BACKLIGHT_NORMAL_MODE  0
#define BACKLIGHT_ALC_MODE     1
struct backlight_platform_data {
	void (*platform_init)(void);
	int gpio;
	unsigned int mode;		     /* initial mode */
	int max_current;			 /* led max current(0-7F) */
	int init_on_boot;			 /* flag which initialize on system boot */
};
int bd6084gu_ldo_enable(struct device *dev, unsigned num, unsigned enable);
int bd6084gu_ldo_set_level(struct device *dev, unsigned num, unsigned vol);

/* rt9393 backlight */
struct rt9393_platform_data {
	int gpio_en;
};

/* LCD panel */
struct msm_panel_lgit_pdata {
	int gpio;
	int (*backlight_level)(int level, int max, int min);
	int (*pmic_backlight)(int level);
	int (*panel_num)(void);
	void (*panel_config_gpio)(int);
	int *gpio_num;
	int initialized;
};

// CONFIG_MACH_LGE_BRYCE chanha.park@lge.com    10.08.26
// START : For Bluetooth
/* Bluetooth Platform Data : chanha.park */
struct bluetooth_platform_data {
	int (*bluetooth_power)(int on);
	int (*bluetooth_toggle_radio)(void *data, bool blocked);
};
// END : For Bluetooth

struct bluesleep_platform_data {
	int bluetooth_port_num;
};

struct gpio_h2w_platform_data {
	int gpio_detect;
	int gpio_button_detect;
};

/* bh6172 pm platform data */
enum 
{
  BH6172_SWREG =0,
  BH6172_LDO1,
  BH6172_LDO2,
  BH6172_LDO3,
  BH6172_LDO4,
  BH6172_LDO5
};

struct bh6172_platform_data {
	int  gpio_reset;
	void (*subpm_reset)(int reset_pin);
};

void bh6172_set_output(int outnum, int onoff);
void bh6172_output_enable(void);

/*sungmin.shin	2010.07.28
	ALS platform data
*/
#if defined (CONFIG_SENSOR_APDS9900)
struct apds9900_platform_data {
	int irq_num;
	int (*power)(unsigned char onoff);
};
#endif

/*ey.cho	2010.06.25
	for touch keypad in EVB2 
*/
struct tskey_platform_data {
	int attn;
};

typedef void (gpio_i2c_init_func_t)(int bus_num);
int __init init_gpio_i2c_pin(struct i2c_gpio_platform_data *i2c_adap_pdata,
		struct gpio_i2c_pin gpio_i2c_pin,
		struct i2c_board_info *i2c_board_info_data);
/* kwangdo. yi 10.07.20
   deprecated 
   */
#if 0   
void __init lge_add_input_devices(void);
#endif
void __init lge_add_lcd_devices(void);
void __init lge_add_mmc_devices(void);
void __init lge_add_usb_devices(void); //added by sungwoo.cho@lge.com
void __init lge_add_misc_devices(void);
void __init lge_add_gpio_i2c_device(gpio_i2c_init_func_t *init_func, int bus_num);
void __init lge_add_gpio_i2c_devices(void);
void __init lge_add_camera_devices(void); //added by jisun.shin@lge.com

/* CONFIG_MACH_LGE_BRYCE	ehgrace.kim 10.05.03
	add amp and mic_sel
*/
#if defined (CONFIG_MACH_LGE_BRYCE)
void __init lge_add_amp_devices(void);
//Baikal Id:0009963. Rx voice volume change support from testmode. kiran.kanneganti@lge.com 
struct platform_device** lge_get_audio_device_data_address(int* Num_of_Dev);
void set_amp_gain(int num);
void lge_snddev_spk_amp_on(void);
void lge_snddev_hs_amp_on(void);
//For sonification stream need to turn on both headset & speaker.  
// ++ kiran.kanneganti@lge.com
void lge_snddev_spk_hs_amp_on(void);
// -- kiran.kanneganti@lge.com
#if LGE_AUDIO_PATH
/* BEGIN:0010882        ehgrace.kim@lge.com     2010.11.15*/
/* MOD: add the call mode */
void lge_snddev_spk_phone_amp_on(void);
void lge_snddev_hs_phone_amp_on(void);
/* END:0010882        ehgrace.kim@lge.com     2010.11.15*/
#endif
void lge_snddev_amp_off(void);

/* daniel.kang@lge.com ++ Nov 17 // 0011018: Remove Audience Code */
/* revB, revC has mic switch, revD doesn't have */
#if defined(LG_HW_REV4) || defined(LG_HW_REV5)
void lge_snddev_MSM_mic_route_config(void);
#endif
/* daniel.kang@lge.com ++ Nov 17 // 0011018: Remove Audience Code */

// CONFIG_MACH_LGE_BRYCE chanha.park@lge.com    10.08.26
// START : For Bluetooth
/* Bluetooth Init */
void __init lge_add_btpower_devices(void);
// END : For Bluetooth

#endif

#endif
