/* arch/arm/mach-msm/lge/board-alohav-misc.c
 * Copyright (C) 2009 LGE, Inc.
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
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/i2c-gpio.h>
#include <linux/power_supply.h>
#include <asm/setup.h>
#include <mach/gpio.h>
#include <mach/vreg.h>
#include <mach/pmic.h>
#include <mach/msm_battery.h>
#include <mach/board.h>
#include <mach/msm_iomap.h>
#include <asm/io.h>

#include <mach/board-bryce.h>

/* Vibrator Functions for Android Vibrator Driver */
#define VIBE_IC_VOLTAGE			3000  //3050	/* Change from 3000 to 3050 */

/* sungwoo.cho 10.09.20 - START
 * GPIO setting for vibrator
 * GPIO_LIN_MOTO_PWM is set by 23 even though it is connected to 33 
 * for fixed REV.B and REV.C
 * Until now(REV.B) GPIO 23 is not used.
 * Added the condition of LG_HW_REV5.
*/

#if 1//defined(LG_HW_REV3) || defined(LG_HW_REV4) || defined(LG_HW_REV5) || defined(LG_HW_REV6) || defined(LG_HW_REV7)
#define GPIO_LIN_MOTOR_PWM			23
#endif

#if 1//defined(LG_HW_REV2) || defined(LG_HW_REV3) || defined(LG_HW_REV4) || defined(LG_HW_REV5) || defined(LG_HW_REV6) || defined(LG_HW_REV7)
#define GPIO_LIN_MOTOR_EN		        91
#endif
/* sungwoo.cho 10.09.20 - END */

#define GP_MN_CLK_MDIV_REG		0x004C
#define GP_MN_CLK_NDIV_REG		0x0050
#define GP_MN_CLK_DUTY_REG		0x0054

/* about 22.93 kHz, should be checked */
#define GPMN_M_DEFAULT			21
#define GPMN_N_DEFAULT			4500
/* default duty cycle = disable motor ic */
#define GPMN_D_DEFAULT			(GPMN_N_DEFAULT >> 1) 
#define PWM_MAX_HALF_DUTY		((GPMN_N_DEFAULT >> 1) - 80) /* minimum operating spec. should be checked */

#define GPMN_M_MASK				0x01FF
#define GPMN_N_MASK				0x1FFF
#define GPMN_D_MASK				0x1FFF

#define REG_WRITEL(value, reg)	writel(value, (MSM_WEB_BASE+reg))

int bryce_vibrator_power_set(int enable)
{
	struct vreg *vibe_vreg;
	static int is_enabled = 0;
/* sungwoo.cho 10.08.12 - START
   vreg setting for vibrator
*/
#if defined(LG_HW_REV1)
	vibe_vreg = vreg_get(NULL, "gp10");
#endif
#if 1//defined(LG_HW_REV2) || defined(LG_HW_REV3) || defined(LG_HW_REV4) || defined(LG_HW_REV5) || defined(LG_HW_REV6) || defined(LG_HW_REV7)
	vibe_vreg = vreg_get(NULL, "gp13");
#endif
/* sungwoo.cho 10.08.12 - END */

	if (IS_ERR(vibe_vreg)) {
		printk(KERN_ERR "%s: vreg_get failed\n", __FUNCTION__);
		return PTR_ERR(vibe_vreg);
	}

	if (enable) {
		if (is_enabled) {
			//printk(KERN_INFO "vibrator power was enabled, already\n");
			return 0;
		}
		
		/* 3000 mV for Motor IC */
		if (vreg_set_level(vibe_vreg, VIBE_IC_VOLTAGE) <0) {		
			printk(KERN_ERR "%s: vreg_set_level failed\n", __FUNCTION__);
			return -EIO;
		}
		
		if (vreg_enable(vibe_vreg) < 0 ) {
			printk(KERN_ERR "%s: vreg_enable failed\n", __FUNCTION__);
			return -EIO;
		}
		is_enabled = 1;
	} else {
		if (!is_enabled) {
			//printk(KERN_INFO "vibrator power was disabled, already\n");
			return 0;
		}
		
		if (vreg_set_level(vibe_vreg, 0) <0) {		
			printk(KERN_ERR "%s: vreg_set_level failed\n", __FUNCTION__);
			return -EIO;
		}
		
		if (vreg_disable(vibe_vreg) < 0) {
			printk(KERN_ERR "%s: vreg_disable failed\n", __FUNCTION__);
			return -EIO;
		}
		is_enabled = 0;
	}
	return 0;
}
// BEGIN : munho.lee@lge.com 2011-02-01
// ADD: 0015191: [Vibrator] Vibrator irregular fix
int vib_on=0;
int isVibrator(void)
{
	return vib_on;
}
EXPORT_SYMBOL(isVibrator);
// END : munho.lee@lge.com 2011-02-01

int bryce_vibrator_pwn_set(int enable, int amp)
{
	printk("[vibrator] %s is called \n", __func__);
	int gain = ((PWM_MAX_HALF_DUTY*amp) >> 7)+ GPMN_D_DEFAULT;

	REG_WRITEL((GPMN_M_DEFAULT & GPMN_M_MASK), GP_MN_CLK_MDIV_REG);
	REG_WRITEL((~( GPMN_N_DEFAULT - GPMN_M_DEFAULT )&GPMN_N_MASK), GP_MN_CLK_NDIV_REG);
		
	printk("[vibrator] gain : %d, enable : %d  \n", gain, enable);
/* kwangdo.yi@lge.com 2010.09.06 S
add to avoid warning mesg in gpiolib
*/
	gpio_request(GPIO_LIN_MOTOR_PWM, "lin_motor_pwn");
/* sungwoo.cho@lge.com 2010.09.17 start
 * changed function number 0 -> 2, Function 2 is for PWM.
 */
// BEGIN : munho.lee@lge.com 2011-01-26
// DEL: 0014873: [Vibrator] Pwm off sequence. 
//	gpio_tlmm_config(GPIO_CFG(GPIO_LIN_MOTOR_PWM, 2, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_4MA), GPIO_CFG_ENABLE);
// END : munho.lee@lge.com 2011-01-26

/* sungwoo.cho@lge.com 2010.09.17 end */
/* kwangdo.yi@lge.com E 2010.09.06 */
	
	if (enable) {
// BEGIN : munho.lee@lge.com 2011-01-26
// ADD: 0014873: [Vibrator] Pwm off sequence. 
		gpio_tlmm_config(GPIO_CFG(GPIO_LIN_MOTOR_PWM, 2, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_4MA), GPIO_CFG_ENABLE);
// END : munho.lee@lge.com 2011-01-26		
		REG_WRITEL((gain & GPMN_D_MASK), GP_MN_CLK_DUTY_REG);
		gpio_direction_output(GPIO_LIN_MOTOR_PWM, 1);
// BEGIN : munho.lee@lge.com 2011-02-01
// MOD: 0015191: [Vibrator] Vibrator irregular fix
		vib_on=1;
// END : munho.lee@lge.com 2011-02-01
	} else {	
		REG_WRITEL(GPMN_D_DEFAULT, GP_MN_CLK_DUTY_REG);
// BEGIN : munho.lee@lge.com 2011-01-26
// MOD: 0014873: [Vibrator] Pwm off sequence. 
		gpio_tlmm_config(GPIO_CFG(GPIO_LIN_MOTOR_PWM, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_4MA), GPIO_CFG_ENABLE);
//		gpio_direction_output(GPIO_LIN_MOTOR_PWM, 0);
// END : munho.lee@lge.com 2011-01-26	
// BEGIN : munho.lee@lge.com 2011-02-01
// MOD: 0015191: [Vibrator] Vibrator irregular fix
		vib_on=0;
// END : munho.lee@lge.com 2011-02-01
	}
         printk("[vibrator] GPIO_LIN_MOTOR_PWM gpio value :%d  \n", gpio_get_value(GPIO_LIN_MOTOR_PWM));
	return 0;
}

int bryce_vibrator_ic_enable_set(int enable)
{
/* kwangdo.yi@lge.com 2010.09.06 S
	add to avoid warning mesg in gpiolib
	*/
	gpio_request(GPIO_LIN_MOTOR_EN, "lin_motor_en");
	gpio_tlmm_config(GPIO_CFG(GPIO_LIN_MOTOR_EN, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
/* kwangdo.yi@lge.com E 2010.09.06 */

	if (enable) {
		gpio_direction_output(GPIO_LIN_MOTOR_EN, 1);
	} else {
		gpio_direction_output(GPIO_LIN_MOTOR_EN, 0);
	}
	return 0;
}

static struct android_vibrator_platform_data bryce_vibrator_data = {
	.enable_status = 0,
	.power_set = bryce_vibrator_power_set,
	.pwn_set = bryce_vibrator_pwn_set,
	.ic_enable_set = bryce_vibrator_ic_enable_set,
};

static struct platform_device android_vibrator_device = {
	.name   = "android-vibrator",
	.id = -1,
	.dev = {
		.platform_data = &bryce_vibrator_data,
	},
};

static struct platform_device *bryce_misc_devices[] __initdata = {
	&android_vibrator_device,
};

void __init lge_add_misc_devices(void)
{
	platform_add_devices(bryce_misc_devices, ARRAY_SIZE(bryce_misc_devices));
}

