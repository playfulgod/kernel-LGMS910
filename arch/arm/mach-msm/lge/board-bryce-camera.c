/* arch/arm/mach-msm/board-bryce-camera.c
 * Copyright (C) 2010 LGE, Inc.
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
#include <linux/types.h>
#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <asm/setup.h>
#include <mach/gpio.h>
#include <mach/board.h>
#include <mach/msm_iomap.h>
#include <mach/vreg.h>
#include <mach/board-bryce.h>
#include <mach/camera.h>


#define LG_DUAL_CAMERA_SUPPORT

//#define CAMERA_MCLK_CHANGE_MODE


/* bh6172 PMIC */
#if defined(LG_HW_REV1)
#define CAM_SUBPM_GPIO_RESET_N		   	142 //EVB1
#else
#define CAM_SUBPM_GPIO_RESET_N		   	143 //EVB2,Rev.A,Rev.B
#endif

#define CAM_SUBPM_GPIO_I2C_SCL		  	102
#define CAM_SUBPM_GPIO_I2C_SDA 		  	103
#define CAM_SUBPM_I2C_SLAVE_ADDR	 	0x9E>>1

#if defined(LG_HW_REV1)
#define CAM_GPIO_I2C_SCL				0
#define CAM_GPIO_I2C_SDA 				1
#endif

/* isx006 main camera - 5M Camera*/
#define MCAM_GPIO_PWDN					98
#define MCAM_GPIO_RESET_N			    100
#define MCAM_I2C_SLAVE_ADDR	 	        0x34>>1

/* mt9m113 VT camera - 1.3M Camera */ 
#if defined(LG_HW_REV1) || defined(LG_HW_REV2)
#define VT_CAM_GPIO_PWDN				73 //EVB2
#define VT_CAM_GPIO_RESET_N				72 //EVB2
#elif defined(LG_HW_REV3) || defined(LG_HW_REV4) 
#define VT_CAM_GPIO_PWDN				46
#define VT_CAM_GPIO_RESET_N				45
#else
#define VT_CAM_GPIO_PWDN				120    
#define VT_CAM_GPIO_RESET_N				45
#endif
#define VT_CAM_I2C_SLAVE_ADDR	 	    0x7A>>1 

/* flash driver */
#if 0//defined(LG_HW_REV6)
#define CAM_FLASH_EN				    84
#else
#define CAM_FLASH_EN				    31
#endif

/* aat1270 flash */
#if defined(LG_HW_REV1) || defined(LG_HW_REV2) 
#define CAM_FLASH_LED_TORCH				79
#define CAM_FLASH_INHIBIT			    99
#elif defined(LG_HW_REV3) || defined(LG_HW_REV4) || defined(LG_HW_REV5)
#define CAM_FLASH_LED_TORCH				49
#define CAM_FLASH_INHIBIT			    99
#else
/* lm3559 flash */
#define CAM_FLASH_GPIO_I2C_SCL			49
#define CAM_FLASH_GPIO_I2C_SDA			99	
#define CAM_FLASH_I2C_SLAVE_ADDR	 	0xA6>>1 
#endif


int mclk_rate = 19200000;//24000000;

/*====================================================================================
								BH6172 SUB PMIC
  =====================================================================================*/
void cam_subpm_reset(int reset_pin)
{
	gpio_tlmm_config(GPIO_CFG(CAM_SUBPM_GPIO_RESET_N, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	
	udelay(10);
	gpio_set_value(reset_pin, 1);
}

static struct gpio_i2c_pin cam_subpm_i2c_pin[] = {
	[0] = {
		.sda_pin	= CAM_SUBPM_GPIO_I2C_SDA,
		.scl_pin	= CAM_SUBPM_GPIO_I2C_SCL,
		.reset_pin	= 0,		
		.irq_pin	= 0,
	},
};

static struct i2c_gpio_platform_data cam_subpm_i2c_pdata = {
	.sda_is_open_drain = 0,
	.scl_is_open_drain = 0,
	.udelay = 2,
};

static struct platform_device cam_subpm_i2c_device = {
	.name = "i2c-gpio",
	.dev.platform_data = &cam_subpm_i2c_pdata,
};

static struct bh6172_platform_data cam_subpm_pdata = {
	.gpio_reset  = CAM_SUBPM_GPIO_RESET_N,
	.subpm_reset = cam_subpm_reset,	
};

static struct i2c_board_info cam_subpm_i2c_bdinfo[] = {
	[0] = {
		I2C_BOARD_INFO("bh6172", CAM_SUBPM_I2C_SLAVE_ADDR),
		.type = "bh6172",
		.platform_data = &cam_subpm_pdata,
	},
};
static void __init bryce_init_i2c_cam_subpm(int bus_num)
{
	cam_subpm_i2c_device.id = bus_num;

	init_gpio_i2c_pin(&cam_subpm_i2c_pdata, cam_subpm_i2c_pin[0], &cam_subpm_i2c_bdinfo[0]);

	i2c_register_board_info(bus_num, &cam_subpm_i2c_bdinfo[0], 1);

	platform_device_register(&cam_subpm_i2c_device);
}

/*====================================================================================
							LED  Flash  ( LM3559, AAT1270 )
  ====================================================================================*/
#ifdef CONFIG_MSM_CAMERA
static struct msm_camera_sensor_flash_src msm_flash_src = {
	.flash_sr_type = MSM_CAMERA_FLASH_SRC_PMIC,
};

static struct msm_camera_sensor_flash_data led_flash_data = {
	.flash_type = MSM_CAMERA_FLASH_LED,
	.flash_src  = &msm_flash_src,
};

static struct msm_camera_sensor_flash_data flash_none = {
	.flash_type = MSM_CAMERA_FLASH_NONE,
	.flash_src  = NULL,
};
#endif //CONFIG_MSM_CAMERA

#ifdef CONFIG_LM3559_FLASH
struct led_flash_platform_data lm3559_flash_pdata = {
	.gpio_flen		= CAM_FLASH_EN,
	.gpio_en_set	= -1,
	.gpio_inh		= -1,
};

static struct gpio_i2c_pin cam_flash_i2c_pin[] = {
	[0] = {
		.sda_pin	= CAM_FLASH_GPIO_I2C_SDA,
		.scl_pin	= CAM_FLASH_GPIO_I2C_SCL,
		.reset_pin	= 0,		
		.irq_pin	= 0,
	},
};

static struct i2c_gpio_platform_data cam_flash_i2c_pdata = {
	.sda_is_open_drain = 0,
	.scl_is_open_drain = 0,
	.udelay = 2,
};

static struct platform_device cam_flash_i2c_device = {
	.name = "i2c-gpio",
	.dev.platform_data = &cam_flash_i2c_pdata,
};

static struct i2c_board_info cam_flash_i2c_bdinfo[] = {
	[0] = {
		I2C_BOARD_INFO("lm3559", CAM_FLASH_I2C_SLAVE_ADDR),
		.type = "lm3559",
		.platform_data = &lm3559_flash_pdata,
	},
};
static void __init bryce_init_i2c_cam_flash(int bus_num)
{
	cam_flash_i2c_device.id = bus_num;

	init_gpio_i2c_pin(&cam_flash_i2c_pdata, cam_flash_i2c_pin[0], &cam_flash_i2c_bdinfo[0]);

	i2c_register_board_info(bus_num, &cam_flash_i2c_bdinfo[0], 1);

	platform_device_register(&cam_flash_i2c_device);
	
}

#endif // CONFIG_LM3559_FLASH

#ifdef CONFIG_AAT1270_FLASH
struct led_flash_platform_data aat1270_flash_pdata = {
	.gpio_flen		= CAM_FLASH_EN,
	.gpio_en_set	= CAM_FLASH_LED_TORCH,
	.gpio_inh		= CAM_FLASH_INHIBIT,
};

static struct platform_device aat1270_flash_device = {
	.name				= "aat1270_flash",
	.id					= -1,
	.dev.platform_data	= &aat1270_flash_pdata,
};
#endif // CONFIG_AAT1270_FLASH

/*====================================================================================
				  Camera Sensor  ( Main Camera : ISX006 , VT Camera : MT9M113)
  ====================================================================================*/
static struct i2c_board_info camera_i2c_devices[] = {
#ifdef CONFIG_ISX006	
   [0]= {
		  I2C_BOARD_INFO("isx006", MCAM_I2C_SLAVE_ADDR),
		  .type = "isx006",
	   },
#endif
#ifdef CONFIG_MT9M113	
   [1] = {
		I2C_BOARD_INFO("mt9m113", VT_CAM_I2C_SLAVE_ADDR),
		.type = "mt9m113",
	},
#endif
};

#if defined(LG_HW_REV1)
static struct gpio_i2c_pin camera_i2c_pin[] = {
	[0] = {
		.sda_pin	= CAM_GPIO_I2C_SDA,
		.scl_pin	= CAM_GPIO_I2C_SCL,
		.reset_pin	= 0,		
		.irq_pin	= 0,
	},
};

static struct i2c_gpio_platform_data camera_i2c_pdata = {
	.sda_pin	= CAM_GPIO_I2C_SDA,
	.scl_pin	= CAM_GPIO_I2C_SCL,
	.sda_is_open_drain = 0,
	.scl_is_open_drain = 0,
	.udelay = 0,
};

static struct platform_device camera_i2c_device = {
	.name = "i2c-gpio",
	.dev.platform_data = &camera_i2c_pdata,
};


static struct i2c_board_info camera_i2c_bdinfo[] = {
	[0] = {
		I2C_BOARD_INFO("isx006", MCAM_I2C_SLAVE_ADDR),
		.type = "isx006",
	},
};
static void __init bryce_init_i2c_camera(int bus_num)
{
	camera_i2c_device.id = bus_num;

	init_gpio_i2c_pin(&camera_i2c_pdata, camera_i2c_pin[0], &camera_i2c_bdinfo[0]);
	
	i2c_register_board_info(bus_num, &camera_i2c_bdinfo[0], 1);
	
	platform_device_register(&camera_i2c_device);

}
#endif

#ifdef CONFIG_MSM_CAMERA
static uint32_t camera_off_gpio_table[] = {
	/* parallel CAMERA interfaces */
	GPIO_CFG(4,  0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT0 */
	GPIO_CFG(5,  0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT1 */
	GPIO_CFG(6,  0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT2 */
	GPIO_CFG(7,  0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT3 */
	GPIO_CFG(8,  0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT4 */
	GPIO_CFG(9,  0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT5 */
	GPIO_CFG(10, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT6 */
	GPIO_CFG(11, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT7 */
	GPIO_CFG(12, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* PCLK */
	GPIO_CFG(13, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* HSYNC_IN */
	GPIO_CFG(14, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* VSYNC_IN */
	GPIO_CFG(15, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), /* MCLK */
};

static uint32_t camera_on_gpio_table[] = {
	/* parallel CAMERA interfaces */
	GPIO_CFG(4,  1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT0 */
	GPIO_CFG(5,  1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT1 */
	GPIO_CFG(6,  1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT2 */
	GPIO_CFG(7,  1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT3 */
	GPIO_CFG(8,  1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT4 */
	GPIO_CFG(9,  1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT5 */
	GPIO_CFG(10, 1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT6 */
	GPIO_CFG(11, 1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT7 */
	GPIO_CFG(12, 1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_16MA), /* PCLK */
	GPIO_CFG(13, 1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* HSYNC_IN */
	GPIO_CFG(14, 1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* VSYNC_IN */
	GPIO_CFG(15, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_6MA), /* MCLK */
};

static void config_gpio_table(uint32_t *table, int len)
{
	int n, rc;
	for (n = 0; n < len; n++) {
		rc = gpio_tlmm_config(table[n], GPIO_CFG_ENABLE);
		if (rc) {
			printk(KERN_ERR "%s: gpio_tlmm_config(%#x)=%d\n",
				__func__, table[n], rc);
			break;
		}
	}
}
int config_camera_on_gpios(void)
{
	config_gpio_table(camera_on_gpio_table,
		ARRAY_SIZE(camera_on_gpio_table));

	return 0;
}

void config_camera_off_gpios(void)
{
	config_gpio_table(camera_off_gpio_table,
		ARRAY_SIZE(camera_off_gpio_table));
}

int main_camera_power_off (void)
{
	gpio_set_value(MCAM_GPIO_RESET_N, 0);
	mdelay(5);
		
	gpio_set_value(MCAM_GPIO_PWDN, 0);
	mdelay(5);
	
#if 1 // MCLK ENABLE TEST
	msm_camio_clk_disable(CAMIO_CAM_MCLK_CLK);
#endif
	// 2.8V_MCAM_AF
	bh6172_set_output(BH6172_LDO2,0);
	bh6172_output_enable();
	mdelay(5);

	// 2.8V_MCAM_AVDD
	bh6172_set_output(BH6172_LDO3,0);
	bh6172_output_enable();
	mdelay(5);
			
#if !defined(LG_HW_REV1) && !defined(LG_HW_REV2)
	// 1.8V_VT_CAM_IOVDD  -- DVDD
	bh6172_set_output(BH6172_LDO5,0);
	bh6172_output_enable();
#endif
	// 2.8V_MCAM_IOVDD
	bh6172_set_output(BH6172_LDO1,0);
	bh6172_output_enable();
	mdelay(5);
		
	// 1.2V_MCAM_CORE
	bh6172_set_output(BH6172_SWREG,0);
	bh6172_output_enable();
	mdelay(5);
		

#if defined(LG_HW_REV3)
	// 2.8V_VT_CAM_AVDD
	bh6172_set_output(BH6172_LDO4,0);
	bh6172_output_enable();
#endif
    mdelay(10);
	printk("main_camera_power_off()\n");


	return 0;

}

int main_camera_power_on (void)
{
#if defined(LG_HW_REV3)
	// 2.8V_VT_CAM_AVDD
	bh6172_set_output(BH6172_LDO4,1);
	bh6172_output_enable();
	mdelay(10);
#endif
			
	
	// 1.2V_MCAM_CORE
	bh6172_set_output(BH6172_SWREG,1);
	bh6172_output_enable();
	
	// 1.8V_MCAM_IOVDD 
	bh6172_set_output(BH6172_LDO1,1);
	bh6172_output_enable();
	
#if !defined(LG_HW_REV1) && !defined(LG_HW_REV2)	
	// 1.8V_VT_CAM_IOVDD  -- DVDD
	bh6172_set_output(BH6172_LDO5,1);
	bh6172_output_enable();
#endif
		
	// 2.8V_MCAM_AVDD
	bh6172_set_output(BH6172_LDO3,1);
	bh6172_output_enable();
		
		
	// 2.8V_MCAM_AF
	bh6172_set_output(BH6172_LDO2,1);
	bh6172_output_enable();
	
	gpio_set_value(MCAM_GPIO_PWDN, 1);
	mdelay(1);

#if 1 // MCLK ENABLE TEST
	msm_camio_clk_enable(CAMIO_CAM_MCLK_CLK);
	mdelay(3);
#endif

	/* Input MCLK = 24MHz */
#ifdef CAMERA_MCLK_CHANGE_MODE
	msm_camio_clk_rate_set(mclk_rate);	
#else
	msm_camio_clk_rate_set(19200000);	
#endif

	mdelay(1);
	msm_camio_camif_pad_reg_reset();
	mdelay(1);

	gpio_set_value(MCAM_GPIO_RESET_N, 1);

	printk("main_camera_power_on()\n");
	return 0;

}


int vt_camera_power_off (void)
{
	gpio_set_value(VT_CAM_GPIO_RESET_N, 0);
	//gpio_set_value(VT_CAM_GPIO_PWDN, 0);
	mdelay(5);

#if 1 // MCLK ENABLE TEST
	msm_camio_clk_disable(CAMIO_CAM_MCLK_CLK);
#endif

#if !defined(LG_HW_REV1) && !defined(LG_HW_REV2)
	// 1.8V_MAIN_CAM_IOVDD	-- DVDD
	bh6172_set_output(BH6172_LDO1,0);
	bh6172_output_enable();
#endif	

	// 2.8V_VT_CAM_IOVDD
	bh6172_set_output(BH6172_LDO5,0);
	bh6172_output_enable();	
	mdelay(5);
	
	// 2.8V_VT_CAM_AVDD
	bh6172_set_output(BH6172_LDO4,0);
	bh6172_output_enable();
	mdelay(5);


	gpio_set_value(VT_CAM_GPIO_PWDN, 0);
	mdelay(1); 
	printk("vt_camera_power_off()\n");
	return 0;
	
}

int vt_camera_power_on (void)
{
	gpio_set_value(VT_CAM_GPIO_PWDN, 0);
	mdelay(1); 
	gpio_set_value(VT_CAM_GPIO_RESET_N, 1);
	  

        mdelay(5); 
	// 1.8V_VT_CAM_DVDD
	bh6172_set_output(BH6172_LDO5,1);
	bh6172_output_enable();	
	
#if !defined(LG_HW_REV1) && !defined(LG_HW_REV2)
	// 1.8V_MAIN_CAM_IOVDD	-- DVDD
	bh6172_set_output(BH6172_LDO1,1);
	bh6172_output_enable();
#endif

// 2.8V_VT_CAM_AVDD
	bh6172_set_output(BH6172_LDO4,1);
	bh6172_output_enable();
	mdelay(5);

	gpio_set_value(VT_CAM_GPIO_RESET_N, 0);
	
#if 1 // MCLK ENABLE TEST
	msm_camio_clk_enable(CAMIO_CAM_MCLK_CLK);
	mdelay(3);
#endif

	/* Input MCLK = 32MHz */
	msm_camio_clk_rate_set(32000000);	
	mdelay(1);
	msm_camio_camif_pad_reg_reset();
	mdelay(1);

	gpio_set_value(VT_CAM_GPIO_RESET_N, 1);
	mdelay(5);
	printk("vt_camera_power_on()\n");

	return 0;

}

void bryce_camera_set_init_gpio(void)
{
	gpio_set_value(MCAM_GPIO_PWDN, 0);
	gpio_set_value(MCAM_GPIO_RESET_N, 0);
	gpio_set_value(VT_CAM_GPIO_PWDN, 0);
	gpio_set_value(VT_CAM_GPIO_RESET_N, 0);	
}

struct resource msm_camera_resources[] = {
	{
		.start	= 0xA6000000,
		.end	= 0xA6000000 + SZ_1M - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= INT_VFE,
		.end	= INT_VFE,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct msm_camera_device_platform_data msm_main_camera_device_data = {
	.camera_power_on   = main_camera_power_on,
	.camera_power_off  = main_camera_power_off,
	.camera_gpio_on    = config_camera_on_gpios,
	.camera_gpio_off   = config_camera_off_gpios,
	.ioext.camifpadphy = 0xAB000000,
	.ioext.camifpadsz  = 0x00000400,
	.ioext.csiphy 	   = 0xA6100000,
	.ioext.csisz  	   = 0x00000400,
	.ioext.csiirq 	   = INT_CSI,
	.ioclk.mclk_clk_rate = 24000000,
	.ioclk.vfe_clk_rate  = 122880000,
};

static struct msm_camera_device_platform_data msm_vt_camera_device_data = {
	.camera_power_on   		= vt_camera_power_on,
	.camera_power_off  		= vt_camera_power_off,
	.camera_gpio_on    		= config_camera_on_gpios,
	.camera_gpio_off   		= config_camera_off_gpios,
	.ioext.camifpadphy 		= 0xAB000000,
	.ioext.camifpadsz  		= 0x00000400,
	.ioext.csiphy 	   		= 0xA6100000,
	.ioext.csisz  	   		= 0x00000400,
	.ioext.csiirq 	   		= INT_CSI,
	.ioclk.mclk_clk_rate = 24000000,
	.ioclk.vfe_clk_rate  = 122880000,
};

#ifdef CONFIG_ISX006
static struct msm_camera_sensor_info msm_camera_sensor_isx006_data = {
	.sensor_name      = "isx006",
	.sensor_reset     = MCAM_GPIO_RESET_N,
	.sensor_pwd       = MCAM_GPIO_PWDN,
	.vcm_pwd          = 0,
	.vcm_enable		  = 0,
	.pdata            = &msm_main_camera_device_data,
	.resource         = msm_camera_resources,
	.num_resources    = ARRAY_SIZE(msm_camera_resources),
	.flash_data       = &led_flash_data,
	.csi_if           = 0,  
};

struct platform_device msm_camera_sensor_isx006 = {
	.name      = "msm_camera_isx006",
	.dev       = {
		.platform_data = &msm_camera_sensor_isx006_data,
	},
};
#endif  //CONFIG_ISX006

#ifdef CONFIG_MT9M113
static struct msm_camera_sensor_info msm_camera_sensor_mt9m113_data = {
	.sensor_name      = "mt9m113",
	.sensor_reset     = VT_CAM_GPIO_RESET_N,
	.sensor_pwd       = VT_CAM_GPIO_PWDN,
	.vcm_pwd          = 0,
	.vcm_enable		  = 0,
	.pdata            = &msm_vt_camera_device_data,
	.resource         = msm_camera_resources,
	.num_resources    = ARRAY_SIZE(msm_camera_resources),
	.flash_data       = &flash_none,
	.csi_if           = 0,  
};

struct platform_device msm_camera_sensor_mt9m113 = {
	.name      = "msm_camera_mt9m113",
	.dev       = {
		.platform_data = &msm_camera_sensor_mt9m113_data,
	},
};
#endif //CONFIG_MT9M113
#endif //CONFIG_MSM_CAMERA

static struct platform_device *bryce_camera_devices[] __initdata = {
#ifdef CONFIG_AAT1270_FLASH
	&aat1270_flash_device,
#endif

#ifdef CONFIG_ISX006
	&msm_camera_sensor_isx006,
#endif

#ifdef LG_DUAL_CAMERA_SUPPORT
#ifdef CONFIG_MT9M113
	&msm_camera_sensor_mt9m113,
#endif
#endif // LG_DUAL_CAMERA_SUPPORT
};

void __init lge_add_camera_devices(void)
{
	bryce_camera_set_init_gpio();

	bryce_init_i2c_cam_subpm(16);

#if defined(LG_HW_REV1)
	bryce_init_i2c_camera(17);	
#elif defined(LG_HW_REV2)
 	i2c_register_board_info(0, camera_i2c_devices, ARRAY_SIZE(camera_i2c_devices));
#else
	i2c_register_board_info(4 /* QUP ID */, camera_i2c_devices, ARRAY_SIZE(camera_i2c_devices));
#endif

#if !defined(LG_HW_REV1)&& !defined(LG_HW_REV2) && !defined(LG_HW_REV3) && !defined(LG_HW_REV4) && !defined(LG_HW_REV5)
	bryce_init_i2c_cam_flash(17);
#endif

	platform_add_devices(bryce_camera_devices, ARRAY_SIZE(bryce_camera_devices));

}

