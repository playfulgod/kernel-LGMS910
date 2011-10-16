/* Copyright (c) 2008-2009, Code Aurora Forum. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Code Aurora Forum nor
 *       the names of its contributors may be used to endorse or promote
 *       products derived from this software without specific prior written
 *       permission.
 *
 * Alternatively, provided that this notice is retained in full, this software
 * may be relicensed by the recipient under the terms of the GNU General Public
 * License version 2 ("GPL") and only version 2, in which case the provisions of
 * the GPL apply INSTEAD OF those given above.  If the recipient relicenses the
 * software under the GPL, then the identification text in the MODULE_LICENSE
 * macro must be changed to reflect "GPLv2" instead of "Dual BSD/GPL".  Once a
 * recipient changes the license terms to the GPL, subsequent recipients shall
 * not relicense under alternate licensing terms, including the BSD or dual
 * BSD/GPL terms.  In addition, the following license statement immediately
 * below and between the words START and END shall also then apply when this
 * software is relicensed under the GPL:
 *
 * START
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License version 2 and only version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * END
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */
 
/* CONFIG_MACH_LGE  sungmin.shin  10.04.19
	Create new file for LGIT mddi panel.
*/

#include "msm_fb.h"
#include "mddihost.h"
#include "mddihosti.h"
#include "mddi_lgit.h"
#include <linux/gpio.h>
#include <mach/gpio.h>
#include "msm_fb_def.h"
#include <mach/vreg.h>
/* jihye.ahn   2010-11-20    for switching mddi type 1 & type 2 */
#if defined(LG_HW_REV5) || defined(LG_HW_REV6)  || defined(LG_HW_REV7)
#define FEATURE_SWITCH_TYPE1_TYPE2
#endif

#define MDDI_CLIENT_CORE_BASE  0x108000
#define LCD_CONTROL_BLOCK_BASE 0x110000
#define SPI_BLOCK_BASE         0x120000
#define PWM_BLOCK_BASE         0x140000
#define SYSTEM_BLOCK1_BASE     0x160000

#define TTBUSSEL    (MDDI_CLIENT_CORE_BASE|0x18)
#define DPSET0      (MDDI_CLIENT_CORE_BASE|0x1C)
#define DPSET1      (MDDI_CLIENT_CORE_BASE|0x20)
#define DPSUS       (MDDI_CLIENT_CORE_BASE|0x24)
#define DPRUN       (MDDI_CLIENT_CORE_BASE|0x28)
#define SYSCKENA    (MDDI_CLIENT_CORE_BASE|0x2C)

#define BITMAP0     (MDDI_CLIENT_CORE_BASE|0x44)
#define BITMAP1     (MDDI_CLIENT_CORE_BASE|0x48)
#define BITMAP2     (MDDI_CLIENT_CORE_BASE|0x4C)
#define BITMAP3     (MDDI_CLIENT_CORE_BASE|0x50)
#define BITMAP4     (MDDI_CLIENT_CORE_BASE|0x54)

#define SRST        (LCD_CONTROL_BLOCK_BASE|0x00)
#define PORT_ENB    (LCD_CONTROL_BLOCK_BASE|0x04)
#define START       (LCD_CONTROL_BLOCK_BASE|0x08)
#define PORT        (LCD_CONTROL_BLOCK_BASE|0x0C)

#define INTFLG      (LCD_CONTROL_BLOCK_BASE|0x18)
#define INTMSK      (LCD_CONTROL_BLOCK_BASE|0x1C)
#define MPLFBUF     (LCD_CONTROL_BLOCK_BASE|0x20)

#define PXL         (LCD_CONTROL_BLOCK_BASE|0x30)
#define HCYCLE      (LCD_CONTROL_BLOCK_BASE|0x34)
#define HSW         (LCD_CONTROL_BLOCK_BASE|0x38)
#define HDE_START   (LCD_CONTROL_BLOCK_BASE|0x3C)
#define HDE_SIZE    (LCD_CONTROL_BLOCK_BASE|0x40)
#define VCYCLE      (LCD_CONTROL_BLOCK_BASE|0x44)
#define VSW         (LCD_CONTROL_BLOCK_BASE|0x48)
#define VDE_START   (LCD_CONTROL_BLOCK_BASE|0x4C)
#define VDE_SIZE    (LCD_CONTROL_BLOCK_BASE|0x50)
#define WAKEUP      (LCD_CONTROL_BLOCK_BASE|0x54)
#define REGENB      (LCD_CONTROL_BLOCK_BASE|0x5C)
#define VSYNIF      (LCD_CONTROL_BLOCK_BASE|0x60)
#define WRSTB       (LCD_CONTROL_BLOCK_BASE|0x64)
#define RDSTB       (LCD_CONTROL_BLOCK_BASE|0x68)
#define ASY_DATA    (LCD_CONTROL_BLOCK_BASE|0x6C)
#define ASY_DATB    (LCD_CONTROL_BLOCK_BASE|0x70)
#define ASY_DATC    (LCD_CONTROL_BLOCK_BASE|0x74)
#define ASY_DATD    (LCD_CONTROL_BLOCK_BASE|0x78)
#define ASY_DATE    (LCD_CONTROL_BLOCK_BASE|0x7C)
#define ASY_DATF    (LCD_CONTROL_BLOCK_BASE|0x80)
#define ASY_DATG    (LCD_CONTROL_BLOCK_BASE|0x84)
#define ASY_DATH    (LCD_CONTROL_BLOCK_BASE|0x88)
#define ASY_CMDSET  (LCD_CONTROL_BLOCK_BASE|0x8C)
#define MONI        (LCD_CONTROL_BLOCK_BASE|0xB0)
#define VPOS        (LCD_CONTROL_BLOCK_BASE|0xC0)

#define SSICTL      (SPI_BLOCK_BASE|0x00)
#define SSITIME     (SPI_BLOCK_BASE|0x04)
#define SSITX       (SPI_BLOCK_BASE|0x08)
#define SSIINTS     (SPI_BLOCK_BASE|0x14)

#define TIMER0LOAD    (PWM_BLOCK_BASE|0x00)
#define TIMER0CTRL    (PWM_BLOCK_BASE|0x08)
#define PWM0OFF       (PWM_BLOCK_BASE|0x1C)
#define TIMER1LOAD    (PWM_BLOCK_BASE|0x20)
#define TIMER1CTRL    (PWM_BLOCK_BASE|0x28)
#define PWM1OFF       (PWM_BLOCK_BASE|0x3C)
#define TIMER2LOAD    (PWM_BLOCK_BASE|0x40)
#define TIMER2CTRL    (PWM_BLOCK_BASE|0x48)
#define PWM2OFF       (PWM_BLOCK_BASE|0x5C)
#define PWMCR         (PWM_BLOCK_BASE|0x68)

#define GPIOIS      (GPIO_BLOCK_BASE|0x08)
#define GPIOIEV     (GPIO_BLOCK_BASE|0x10)
#define GPIOIC      (GPIO_BLOCK_BASE|0x20)

#define WKREQ       (SYSTEM_BLOCK1_BASE|0x00)
#define CLKENB      (SYSTEM_BLOCK1_BASE|0x04)
#define DRAMPWR     (SYSTEM_BLOCK1_BASE|0x08)
#define INTMASK     (SYSTEM_BLOCK1_BASE|0x0C)
#define CNT_DIS     (SYSTEM_BLOCK1_BASE|0x10)

// BEGIN : munho.lee@lge.com 2010-10-21
// ADD :0010076: [Display] To remove noise display When the system resumes,suspends,boots 
extern atomic_t lcd_bootup_handle;
// END : munho.lee@lge.com 2010-10-21

// BEGIN : munho.lee@lge.com 2010-10-26
// ADD :0010205: [Display] To change resolution for removing display noise. 
extern atomic_t lcd_event_handled;
// END : munho.lee@lge.com 2010-10-26

extern void mddi_host_register_write_commands(
	unsigned reg_addr, unsigned count, uint32 reg_val[],
	boolean wait, mddi_llist_done_cb_type done_cb, mddi_host_type host);

static uint32 mddi_lgit_curr_vpos;
static boolean mddi_lgit_monitor_refresh_value = FALSE;
static boolean mddi_lgit_report_refresh_measurements = FALSE;

/* Modifications to timing to increase refresh rate to > 60Hz.
 *   20MHz dot clock.
 *   646 total rows.
 *   506 total columns.
 *   refresh rate = 61.19Hz
 */
static uint32 mddi_lgit_rows_per_second = 50046;
static uint32 mddi_lgit_usecs_per_refresh = 16344;
static uint32 mddi_lgit_rows_per_refresh = 818;
extern boolean mddi_vsync_detect_enabled;

static msm_fb_vsync_handler_type mddi_lgit_vsync_handler;
static void *mddi_lgit_vsync_handler_arg;
static uint16 mddi_lgit_vsync_attempts;

static struct msm_panel_common_pdata *mddi_lgit_pdata;

static int mddi_lgit_lcd_on(struct platform_device *pdev);
static int mddi_lgit_lcd_off(struct platform_device *pdev);

static int ch_used[3];

// BEGIN : munho.lee@lge.com 2010-10-26
// ADD :0010205: [Display] To change resolution for removing display noise. 
struct timer_list timer_mdp;

static void display_on(void);

static void check_mdp_clkon(unsigned long arg)
{
//	printk("## LMH_TEST %s \n", __func__);
	if(atomic_read(&lcd_event_handled))
		display_on();
	else
		mod_timer(&timer_mdp, jiffies + msecs_to_jiffies(5));	
		
}
// END : munho.lee@lge.com 2010-10-26

typedef enum {
	MDDI_CMD_WRITE,
	MDDI_CMD_READ, 
	MDDI_CMD_DELAY,
	MDDI_CMD_LAST_ENTRY,
} mddi_cmd_type;

struct mddi_reg_table {
	mddi_cmd_type cmd;
  unsigned reg;
	unsigned count;
  unsigned int val_list[5];
};

#define LG4572_REG_NOP	      0x00
#define LG4572_REG_SWRESET    0x01
#define LG4572_REG_RDDPM      0x0A
#define LG4572_REG_RDDMADCTL  0x0B
#define LG4572_REG_RDDCOLMOD  0x0C
#define LG4572_REG_RDDIM      0x0D
#define LG4572_REG_RDDSM      0x0E
#define LG4572_REG_SLPIN      0x10
#define LG4572_REG_SLPOUT     0x11
#define LG4572_REG_PTLON      0x12
#define LG4572_REG_NRTON      0x13
#define LG4572_REG_INVOFF     0x20
#define LG4572_REG_INVON      0x21
#define LG4572_REG_DISPOFF    0x28
#define LG4572_REG_DISPON     0x29
#define LG4572_REG_CASET      0x2A
#define LG4572_REG_PASET      0x2B
#define LG4572_REG_RAMWR      0x2C
#define LG4572_REG_RAMRD      0x2E
#define LG4572_REG_PTLAR      0x30
#define LG4572_REG_TEOFF      0x34
#define LG4572_REG_TEON       0x35
#define LG4572_REG_MADCTL     0x36
#define LG4572_REG_COLMOD     0x3A
#define LG4572_REG_WRDISBV    0x51
#define LG4572_REG_RDDISBV    0x52
#define LG4572_REG_WRCTRLD    0x53
#define LG4572_REG_RDCTRLD    0x54
#define LG4572_REG_WRCABC     0x55
#define LG4572_REG_RDCABC     0x56
#define LG4572_REG_PANELSET   0xB2
#define LG4572_REG_PANELDRV   0xB3
#define LG4572_REG_DISPMODE   0xB4
#define LG4572_REG_DISPCTL1   0xB5
#define LG4572_REG_DISPCTL2   0xB6
#define LG4572_REG_DISPCTL3   0xB7
#define LG4572_REG_OSCSET     0xC0
#define LG4572_REG_PWRCTL1    0xC1
#define LG4572_REG_PWRCTL2    0xC2
#define LG4572_REG_PWRCTL3    0xC3
#define LG4572_REG_PWRCTL4    0xC4
#define LG4572_REG_PWRCTL5    0xC5
#define LG4572_REG_PWRCTL6    0xC6
#define LG4572_REG_RGAMMAP    0xD0
#define LG4572_REG_RGAMMAN    0xD1
#define LG4572_REG_GGAMMAP    0xD2
#define LG4572_REG_GGAMMAN    0xD3
#define LG4572_REG_BGAMMAP    0xD4
#define LG4572_REG_BGAMMAN    0xD5
#define LG4572_REG_MDDICTL    0xE0

#define REGFLAG_DELAY	      0xFFFE
#define REGFLAG_END_OF_TABLE	0xFFFF

struct mddi_cmd_table {
	uint32 reg;
	uint32 count;
	uint32 seq[12];
};
#define CONFIG_MACH_LGE_BRYCE  1 //sabina

static struct mddi_cmd_table poweron_table[] = {
#if defined(LG_HW_REV1) || defined(LG_HW_REV2)
	/* === Display Mode Setting  === */
	/* Display Inversion */
	{ LG4572_REG_INVOFF, 1, {0x0} },
	{ REGFLAG_DELAY, 50, {} },

	/* Display Inversion */
	{ LG4572_REG_INVOFF, 1, {0x0} },
	/* Interface Pixel Format (24BIT:0x0007, 18BIT:0x0006, 16BIT:0x0005) */
	{ LG4572_REG_COLMOD, 1, {0x0005} },
	/* Panel Characteristics Setting */
	{ LG4572_REG_PANELSET, 2, {0x0020, 0x00C8} },
	/* Panel Drive Setting */
	{ LG4572_REG_PANELDRV, 1, {0x0000} },
	/* Display Mode Control */
	{ LG4572_REG_DISPMODE, 1, {0x0004} },
	/* Display Control Setting */
	{ LG4572_REG_DISPCTL1, 5, {0x0008, 0x0010, 0x0010, 0x0000, 0x0020} },
	{ LG4572_REG_DISPCTL2, 6, {0x0001, 0x0018, 0x0002, 0x0040, 0x0010, 0x0000} },
	{ LG4572_REG_DISPCTL3, 5, {0x0053, 0x0006, 0x000C, 0x0000, 0x0000} },
	/* Internal Oscillator Setting */
	{ LG4572_REG_OSCSET, 2, {0x0001, 0x001C} },

	/* === Power Setting === */
	{ LG4572_REG_PWRCTL3, 5, {0x0007, 0x0004, 0x0003, 0x0003, 0x0004} },
	{ LG4572_REG_PWRCTL4, 6, {0x0011, 0x0013, 0x000F, 0x000F, 0x0001, 0x0009} },
	{ LG4572_REG_PWRCTL5, 1, {0x0055} },
	{ LG4572_REG_PWRCTL6, 2, {0x0021, 0x0040} },

	/* === Gamma Setting === */
	/* Set Positive, Negative Gamma Curve for Red, Green, Blue */
	{ LG4572_REG_RGAMMAP, 9,
	  {0x0000, 0x0067, 0x0076, 0x0027, 0x000B, 0x0001, 0x0061, 0x0045, 0x0004} },
	{ LG4572_REG_RGAMMAN, 9,
	  {0x0000, 0x0067, 0x0076, 0x0027, 0x000B, 0x0001, 0x0061, 0x0045, 0x0004} },
	{ LG4572_REG_GGAMMAP, 9,
	  {0x0000, 0x0067, 0x0076, 0x0027, 0x000B, 0x0001, 0x0061, 0x0045, 0x0004} },
	{ LG4572_REG_GGAMMAN, 9,
	  {0x0000, 0x0067, 0x0076, 0x0027, 0x000B, 0x0001, 0x0061, 0x0045, 0x0004} },
	{ LG4572_REG_BGAMMAP, 9,
	  {0x0000, 0x0067, 0x0076, 0x0027, 0x000B, 0x0001, 0x0061, 0x0045, 0x0004} },
	{ LG4572_REG_BGAMMAN, 9,
	  {0x0000, 0x0067, 0x0076, 0x0027, 0x000B, 0x0001, 0x0061, 0x0045, 0x0004} },

	/* MDDI Control, LPM ON */
	{ LG4572_REG_MDDICTL, 6, {0x0031, 0x0003, 0x0000, 0x0000, 0x0002, 0x0000} },

	/* Brightness Control ON */
	{ LG4572_REG_WRDISBV, 1, {0x00FF} },
	{ LG4572_REG_WRCABC, 1, {0x0002} },
	{ LG4572_REG_WRCTRLD, 1, {0x002C} },

	/* Sleep Out */
	{ LG4572_REG_SLPOUT, 1, {0x0000} },
	{ REGFLAG_DELAY, 150, {} },

	/* Display ON */
	{ LG4572_REG_DISPON, 1, {0x0000} },
	/* jihye.ahn 10.08.20 tearing effect line on */
	{ LG4572_REG_TEON, 1, {0x0000} },
#endif 
/* jihye.ahn   10-09-28   add Rev.C feature */
#if defined(LG_HW_REV3) || defined(LG_HW_REV4)
	/* Display Inversion */
	{ LG4572_REG_INVOFF, 1, {0x0} },
	/* jihye.ahn 10.08.20 tearing effect line on & set for 16bpp*/
	{ LG4572_REG_TEON, 1, {0x0} },
	{ LG4572_REG_COLMOD, 1, {0x0005} },
	/* Panel Characteristics Setting */
	{ LG4572_REG_PANELSET, 2, {0x0000, 0x00C8} },
	/* Panel Drive Setting */
	{ LG4572_REG_PANELDRV, 1, {0x0000} },
	/* Display Mode Control */
	{ LG4572_REG_DISPMODE, 1, {0x0004} },
	/* Display Control Setting */
	{ LG4572_REG_DISPCTL1, 5, {0x0042, 0x0010, 0x0010, 0x0000, 0x0020} },
	{ LG4572_REG_DISPCTL2, 6, {0x000B, 0x001F, 0x003C, 0x0013, 0x0013, 0x001F} },
	{ LG4572_REG_DISPCTL3, 5, {0x0046, 0x0006, 0x000C, 0x0000, 0x0000} },
	
	/* === Power Setting === */
	/* Internal Oscillator Setting */
	{ LG4572_REG_OSCSET, 2, {0x0001, 0x0011} },
	/* Power Control */
	{ LG4572_REG_PWRCTL3, 5, {0x0007, 0x0003, 0x0004, 0x0004, 0x0004} },
	/* jihye.ahn [start] 2010-09-13 for LCD display test */
	{ LG4572_REG_PWRCTL4, 6, {0x0012, 0x0024, 0x0018, 0x0018, 0x0001, 0x0049} },
	{ LG4572_REG_PWRCTL5, 1, {0x0063} },
	{ LG4572_REG_PWRCTL6, 2, {0x0041, 0x0063} },

	/* === Gamma Setting === */
	/* Set Positive, Negative Gamma Curve for Red, Green, Blue */
	{ LG4572_REG_RGAMMAP, 9,
	  {0x0003, 0x0007, 0x0073, 0x0035, 0x0000, 0x0001, 0x0020, 0x0000, 0x0003} },
	{ LG4572_REG_GGAMMAP, 9,
	  {0x0003, 0x0007, 0x0073, 0x0035, 0x0000, 0x0001, 0x0020, 0x0000, 0x0003} },
	{ LG4572_REG_BGAMMAP, 9,
	  {0x0003, 0x0007, 0x0073, 0x0035, 0x0000, 0x0001, 0x0020, 0x0000, 0x0003} },
	{ LG4572_REG_RGAMMAN, 9,
	  {0x0003, 0x0007, 0x0073, 0x0035, 0x0000, 0x0001, 0x0020, 0x0000, 0x0003} },
	{ LG4572_REG_GGAMMAN, 9,
	  {0x0003, 0x0007, 0x0073, 0x0035, 0x0000, 0x0001, 0x0020, 0x0000, 0x0003} },
	{ LG4572_REG_BGAMMAN, 9,
	  {0x0003, 0x0007, 0x0073, 0x0035, 0x0000, 0x0001, 0x0020, 0x0000, 0x0003} },
	/* Sleep Out */
	{ LG4572_REG_SLPOUT, 1, {0x0000} },
// BEGIN : munho.lee@lge.com 2010-10-26
// MOD :0010205: [Display] To change resolution for removing display noise. 
#if 0
	{ REGFLAG_DELAY, 100, {} },	/* jihye.ahn	10-09-06	reduce init time */
	/* Display ON */
	{ LG4572_REG_DISPON, 1, {0x0000} },
#endif	
// END : munho.lee@lge.com 2010-10-26

#endif
/* LGE_S  jihye.ahn   10-10-11    modified initial code for Rev.C to fix line defects */
#if defined(LG_HW_REV5) || defined(LG_HW_REV6)  || defined(LG_HW_REV7)
	/* Display Inversion */
	{ LG4572_REG_INVOFF, 1, {0x0} },
	/* jihye.ahn 10.08.20 tearing effect line on & set for 16bpp*/
	{ LG4572_REG_TEON, 1, {0x0} },
	/* jihye.ahn 11.01.14 change to 24bit bpp*/
	{ LG4572_REG_COLMOD, 1, {0x0077} },
	/* Panel Characteristics Setting */
	{ LG4572_REG_PANELSET, 2, {0x0000, 0x00C8} },
	/* Panel Drive Setting */
	{ LG4572_REG_PANELDRV, 1, {0x0000} },
	/* Display Mode Control */
	{ LG4572_REG_DISPMODE, 1, {0x0004} },
	/* Display Control Setting */
	{ LG4572_REG_DISPCTL1, 5, {0x0042, 0x0010, 0x0010, 0x0000, 0x0020} },
	{ LG4572_REG_DISPCTL2, 6, {0x000B, 0x000F, 0x003C, 0x0013, 0x0013, 0x00E8} },
	{ LG4572_REG_DISPCTL3, 5, {0x0046, 0x0006, 0x000C, 0x0000, 0x0000} },
	/* MDDI Control */ //jihye.ahn
	//{ LG4572_REG_MDDICTL, 6, {0x0030, 0x0003, 0x0000, 0x0007, 0x0002, 0x0000} },
	{ LG4572_REG_MDDICTL, 6, {0x0030, 0x0003, 0x0005, 0x000C, 0x0006, 0x0005} },
	
	/* === Power Setting === */
	/* Internal Oscillator Setting */
	//{ LG4572_REG_OSCSET, 2, {0x0001, 0x0015} },
	{ LG4572_REG_OSCSET, 2, {0x0001, 0x0013} },  // spec. version 1.2
	/* Power Control */
	{ LG4572_REG_PWRCTL3, 5, {0x0007, 0x0003, 0x0004, 0x0004, 0x0004} },
	/* jihye.ahn [start] 2010-09-13 for LCD display test */
	//{ LG4572_REG_PWRCTL4, 6, {0x0012, 0x0024, 0x0018, 0x0018, 0x0004, 0x0049} },
	{ LG4572_REG_PWRCTL4, 6, {0x0012, 0x0024, 0x0018, 0x0018, 0x0002, 0x0049} },  // spec. version 1.2
	//{ LG4572_REG_PWRCTL5, 1, {0x0069} },
	{ LG4572_REG_PWRCTL5, 1, {0x006D} }, // spec. version 1.2
	{ LG4572_REG_PWRCTL6, 2, {0x0041, 0x0063} },

	/* === Gamma Setting === */
	/* Set Positive, Negative Gamma Curve for Red, Green, Blue */
    { LG4572_REG_RGAMMAP, 9,
	  {0x0000, 0x0016, 0x0062, 0x0035, 0x0002, 0x0000, 0x0030, 0x0000, 0x0003} },
	{ LG4572_REG_GGAMMAP, 9,
           {0x0000, 0x0016, 0x0062, 0x0035, 0x0002, 0x0000, 0x0030, 0x0000, 0x0003} },
	{ LG4572_REG_BGAMMAP, 9,
           {0x0000, 0x0016, 0x0062, 0x0035, 0x0002, 0x0000, 0x0030, 0x0000, 0x0003} },
	{ LG4572_REG_RGAMMAN, 9,
            {0x0000, 0x0016, 0x0062, 0x0035, 0x0002, 0x0000, 0x0030, 0x0000, 0x0003} },
	{ LG4572_REG_GGAMMAN, 9,
            {0x0000, 0x0016, 0x0062, 0x0035, 0x0002, 0x0000, 0x0030, 0x0000, 0x0003} },
	{ LG4572_REG_BGAMMAN, 9,
            {0x0000, 0x0016, 0x0062, 0x0035, 0x0002, 0x0000, 0x0030, 0x0000, 0x0003} },
        /* Sleep Out */
	{ LG4572_REG_SLPOUT, 1, {0x0000} },
// BEGIN : munho.lee@lge.com 2010-10-26
// MOD :0010205: [Display] To change resolution for removing display noise. 
#if 0	
	{ REGFLAG_DELAY, 100, {} },	/* jihye.ahn	10-09-06	reduce init time */
	/* Display ON */
	{ LG4572_REG_DISPON, 1, {0x0000} },
#endif	
// END : munho.lee@lge.com 2010-10-26
#endif
/* LGE_E  jihye.ahn   10-10-11    modified initial code for Rev.C to fix line defects */
};

static struct mddi_cmd_table poweroff_table[] = {
	/* Display OFF */
	{ LG4572_REG_DISPOFF, 1, {0x0000} },
	{ REGFLAG_DELAY, 40, {} },
	/* Sleep IN */
	{ LG4572_REG_SLPIN, 1, {0x0000} },	
	{ REGFLAG_DELAY, 150, {} },
	/* Deep Sleep */
	{ LG4572_REG_PWRCTL1, 1, {0x0001} },
	{ REGFLAG_DELAY, 20, {} },
};
	/* jihye.ahn [end] 2010-09-13 for LCD display test */
/* LGE_S   jihye.ahn   10-10-20   added LCD on/off interface for CDMA test mode */
static struct mddi_cmd_table display_on_table[] = {
		{ LG4572_REG_DISPON, 1, {0x0000} },
	};
	
static struct mddi_cmd_table display_off_table[] = {
		{ LG4572_REG_DISPOFF, 1, {0x0000} },
	};
/* LGE_E   jihye.ahn   10-10-20   added LCD on/off interface for CDMA test mode */

static void mddi_lg4572_write_commands(struct mddi_cmd_table *table, unsigned int count)
{
	unsigned int index;
	bool endflag = FALSE;

	for(index=0; !endflag && index < count; index++) {
		uint32 reg = table[index].reg;

		switch(reg) {
		case REGFLAG_DELAY:
			mddi_wait(table[index].count);
			break;
		case REGFLAG_END_OF_TABLE:
			endflag = TRUE;
			break;
		default:
			mddi_host_register_write_commands(
				reg, table[index].count, table[index].seq, 0, 0, 0);
			break;
		}
	}
}

/* jihye.ahn    2010-11-17  to remove noise when lcd resumes */
extern void refresh_buffer();
static int lcd_on = 0;
static void lgit_common_initial_setup(void)
{
	mddi_lg4572_write_commands(poweron_table, ARRAY_SIZE(poweron_table));
        if(lcd_on){
         refresh_buffer();
         lcd_on = 0;
            }
	return;
}

static void lgit_common_powerdown(void)
{
	mddi_lg4572_write_commands(poweroff_table, ARRAY_SIZE(poweroff_table));
	/* jihye.ahn	10-09-06	removed previous LCD data */
//	gpio_set_value(95, 0);
	return;
}

/* LGE_S   jihye.ahn   10-10-20   added LCD on/off interface for CDMA test mode */
static void display_on(void)
{
	mddi_lg4572_write_commands(display_on_table, ARRAY_SIZE(display_on_table));
	return;
}

static void display_off(void)
{
	mddi_lg4572_write_commands(display_off_table, ARRAY_SIZE(display_off_table));
	return;
}
/* LGE_E   jihye.ahn   10-10-20   added LCD on/off interface for CDMA test mode */

static void mddi_lgit_lcd_set_backlight(struct msm_fb_data_type *mfd)
{

}

static void mddi_lgit_vsync_set_handler(msm_fb_vsync_handler_type handler,	/* ISR to be executed */
					   void *arg)
{
	boolean error = FALSE;
	unsigned long flags;

	/* Disable interrupts */
	spin_lock_irqsave(&mddi_host_spin_lock, flags);
	/* INTLOCK(); */

	if (mddi_lgit_vsync_handler != NULL) {
		error = TRUE;
	} else {
		/* Register the handler for this particular GROUP interrupt source */
		mddi_lgit_vsync_handler = handler;
		mddi_lgit_vsync_handler_arg = arg;
	}

	/* Restore interrupts */
	spin_unlock_irqrestore(&mddi_host_spin_lock, flags);
	/* MDDI_INTFREE(); */
	if (error) {
		MDDI_MSG_ERR("MDDI: Previous Vsync handler never called\n");
	} else {
		/* Enable the vsync wakeup */
		mddi_queue_register_write(INTMSK, 0x0000, FALSE, 0);

		mddi_lgit_vsync_attempts = 1;
		mddi_vsync_detect_enabled = TRUE;
	}
}				/* mddi_lgit_vsync_set_handler */

static void mddi_lgit_lcd_vsync_detected(boolean detected)
{
	/* static timetick_type start_time = 0; */
	static struct timeval start_time;
	static boolean first_time = TRUE;
	/* uint32 mdp_cnt_val = 0; */
	/* timetick_type elapsed_us; */
	struct timeval now;
	uint32 elapsed_us;
	uint32 num_vsyncs;

	if ((detected) || (mddi_lgit_vsync_attempts > 5)) {
		if ((detected) && (mddi_lgit_monitor_refresh_value)) {
			/* if (start_time != 0) */
			if (!first_time) {
				jiffies_to_timeval(jiffies, &now);
				elapsed_us =
				    (now.tv_sec - start_time.tv_sec) * 1000000 +
				    now.tv_usec - start_time.tv_usec;
				/*
				 * LCD is configured for a refresh every usecs,
				 *  so to determine the number of vsyncs that
				 *  have occurred since the last measurement
				 *  add half that to the time difference and
				 *  divide by the refresh rate.
				 */
				num_vsyncs = (elapsed_us +
					      (mddi_lgit_usecs_per_refresh >>
					       1)) /
				    mddi_lgit_usecs_per_refresh;
				/*
				 * LCD is configured for * hsyncs (rows) per
				 * refresh cycle. Calculate new rows_per_second
				 * value based upon these new measurements.
				 * MDP can update with this new value.
				 */
				mddi_lgit_rows_per_second =
				    (mddi_lgit_rows_per_refresh * 1000 *
				     num_vsyncs) / (elapsed_us / 1000);
			}
			/* start_time = timetick_get(); */
			first_time = FALSE;
			jiffies_to_timeval(jiffies, &start_time);
			if (mddi_lgit_report_refresh_measurements) {
				(void)mddi_queue_register_read_int(VPOS,
								   &mddi_lgit_curr_vpos);
				/* mdp_cnt_val = MDP_LINE_COUNT; */
			}
		}
		/* if detected = TRUE, client initiated wakeup was detected */
		if (mddi_lgit_vsync_handler != NULL) {
			(*mddi_lgit_vsync_handler)
			    (mddi_lgit_vsync_handler_arg);
			mddi_lgit_vsync_handler = NULL;
		}
		mddi_vsync_detect_enabled = FALSE;
		mddi_lgit_vsync_attempts = 0;
		/* need to disable the interrupt wakeup */
		if (!mddi_queue_register_write_int(INTMSK, 0x0001))
			MDDI_MSG_ERR("Vsync interrupt disable failed!\n");
		if (!detected) {
			/* give up after 5 failed attempts but show error */
			MDDI_MSG_NOTICE("Vsync detection failed!\n");
		} else if ((mddi_lgit_monitor_refresh_value) &&
			   (mddi_lgit_report_refresh_measurements)) {
			MDDI_MSG_NOTICE("  Last Line Counter=%d!\n",
					mddi_lgit_curr_vpos);
		/* MDDI_MSG_NOTICE("  MDP Line Counter=%d!\n",mdp_cnt_val); */
			MDDI_MSG_NOTICE("  Lines Per Second=%d!\n",
					mddi_lgit_rows_per_second);
		}
		/* clear the interrupt */
		if (!mddi_queue_register_write_int(INTFLG, 0x0001))
			MDDI_MSG_ERR("Vsync interrupt clear failed!\n");
	} else {
		/* if detected = FALSE, we woke up from hibernation, but did not
		 * detect client initiated wakeup.
		 */
		mddi_lgit_vsync_attempts++;
	}
}


static int mddi_lgit_lcd_on(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	struct vreg *lcd_vreg, *lcd2_vreg, *lcd3_vreg;
	int rc;
	printk(KERN_INFO "%s,LCD On Start\n",__func__);
        gpio_tlmm_config(GPIO_CFG(95, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE); //LCD_RESET_N
        gpio_set_value(95, 1);
        msleep(5);
        gpio_set_value(95, 0);

#if 0
#if defined(LG_HW_REV5) || defined(LG_HW_REV6)  || defined(LG_HW_REV7)
            lcd3_vreg = vreg_get(NULL, "rf"); //VCC
            if (IS_ERR(lcd3_vreg)) {
                    printk("%s: vreg_get(%s) failed (%ld)\n",
                        __func__, "rf", PTR_ERR(lcd3_vreg));
                    return -1;
                }
            if (lcd3_vreg) {
                rc = vreg_set_level(lcd3_vreg, 2800);
                if (rc) {
                    printk("%s: vreg_set level failed (%d)\n",
                        __func__, rc);
                }
                rc = vreg_enable(lcd3_vreg);
                if (rc) {
                    printk("%s: vreg_enable() = %d \n",
                        __func__, rc);
                }
            }
#endif

#ifdef LG_HW_REV1
            lcd2_vreg = vreg_get(NULL, "rf");
#endif

#if defined(LG_HW_REV2) || defined(LG_HW_REV3) || defined(LG_HW_REV4) || defined(LG_HW_REV5) || defined(LG_HW_REV6)  || defined(LG_HW_REV7)
            lcd2_vreg = vreg_get(NULL, "gp10"); //IOVCC
#endif
            if (IS_ERR(lcd2_vreg)) {
                printk("%s: vreg_get(%s) failed (%ld)\n",
                    __func__, "gp10", PTR_ERR(lcd2_vreg));
                return -1;
            }
            if (lcd2_vreg) {
                rc = vreg_set_level(lcd2_vreg, 1800);
                if (rc) {
                    printk("%s: vreg_set level failed (%d)\n",
                        __func__, rc);
                }
                rc = vreg_enable(lcd2_vreg);
                if (rc) {
                    printk("%s: vreg_enable() = %d \n",
                        __func__, rc);
                }
            }
            
        msleep(5);

#ifdef LG_HW_REV1
	lcd_vreg = vreg_get(NULL, "gp7");
#endif

#if defined(LG_HW_REV2) || defined(LG_HW_REV3) || defined(LG_HW_REV4) || defined(LG_HW_REV5) || defined(LG_HW_REV6)  || defined(LG_HW_REV7)
	lcd_vreg = vreg_get(NULL, "gp6"); //VCI
#endif
	
	if (IS_ERR(lcd_vreg)) {
		printk("%s: vreg_get(%s) failed (%ld)\n",
			__func__, "gp6", PTR_ERR(lcd_vreg));
		return -1;
	}
	if (lcd_vreg) {
		rc = vreg_set_level(lcd_vreg, 2800);
		if (rc) {
			printk("%s: vreg_set level failed (%d)\n",
				__func__, rc);
		}
		rc = vreg_enable(lcd_vreg);
		if (rc) {
			printk("%s: vreg_enable() = %d \n",
				__func__, rc);
		}
	}
    
        msleep(5);
        
        if (lcd3_vreg) {
		rc = vreg_set_level(lcd3_vreg, 0);
		if (rc) {
			printk("%s: vreg_set level failed (%d)\n",
				__func__, rc);
		}
		rc = vreg_disable(lcd3_vreg);
		if (rc) {
			printk("%s: vreg_enable() = %d \n",
				__func__, rc);
		}
	}
        
        if (lcd_vreg) {
            rc = vreg_set_level(lcd_vreg, 0);
            if (rc) {
                printk("%s: vreg_set level failed (%d)\n",
                    __func__, rc);
            }
            rc = vreg_disable(lcd_vreg);
            if (rc) {
                printk("%s: vreg_enable() = %d \n",
                    __func__, rc);
            }
        }
        
        msleep(50);

        if (lcd3_vreg) {
              rc = vreg_set_level(lcd3_vreg, 2800);
              if (rc) {
                  printk("%s: vreg_set level failed (%d)\n",
                      __func__, rc);
              }
              rc = vreg_enable(lcd3_vreg);
              if (rc) {
                  printk("%s: vreg_enable() = %d \n",
                      __func__, rc);
              }
          }
              
              if (lcd_vreg) {
                  rc = vreg_set_level(lcd_vreg, 2800);
                  if (rc) {
                      printk("%s: vreg_set level failed (%d)\n",
                          __func__, rc);
                  }
                  rc = vreg_enable(lcd_vreg);
                  if (rc) {
                      printk("%s: vreg_enable() = %d \n",
                          __func__, rc);
                  }
              }
#endif 
	mfd = platform_get_drvdata(pdev);
	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;


	msleep(1);
	gpio_set_value(95, 1);	
	msleep(5);   /* jihye.ahn	10-09-06	need enough delay time before initial set up */

         lgit_common_initial_setup();

 /* jihye.ahn   2010-11-20    for switching mddi type 1 & type 2 */
#ifdef FEATURE_SWITCH_TYPE1_TYPE2
                  mddi_change_to_type_2(MDDI_HOST_PRIM);         
#endif

// BEGIN : munho.lee@lge.com 2010-10-26
// ADD :0010205: [Display] To change resolution for removing display noise. 
	mod_timer(&timer_mdp, jiffies + msecs_to_jiffies(20));	
// END : munho.lee@lge.com 2010-10-26
	//printk("%s \n", __func__);
	printk(KERN_INFO "%s, LCD On End\n",__func__);
	return 0;
}

static int mddi_lgit_lcd_off(struct platform_device *pdev)
{
	printk("%s \n", __func__);
        
         lcd_on = 1;     /* jihye.ahn    2010-11-17  to remove noise when lcd resumes */

/* sabina.park	10.06.30
modify lcd off
*/
#ifdef CONFIG_MACH_LGE_BRYCE 
/* jihye.ahn	10-09-20	   add Rev.C board config 2.8V LCD_VCI*/	
	struct vreg *lcd_vreg, *lcd2_vreg, *lcd3_vreg;
	int rc;

	lgit_common_powerdown();
        
#ifdef LG_HW_REV1
	lcd_vreg = vreg_get(NULL, "gp7");
#endif
/* jihye.ahn	10-09-20	   add Rev.C board config 2.8V LCD_VCI*/
#if defined(LG_HW_REV2) || defined(LG_HW_REV3) || defined(LG_HW_REV4) || defined(LG_HW_REV5) || defined(LG_HW_REV6)  || defined(LG_HW_REV7)
	lcd_vreg = vreg_get(NULL, "gp6");
#endif
	if (IS_ERR(lcd_vreg)) {
		printk("%s: vreg_get(%s) failed (%ld)\n",
			__func__, "gp7", PTR_ERR(lcd_vreg));
		return -1;
	}
	if (lcd_vreg) {
		rc = vreg_disable(lcd_vreg);
		if (rc) {
			printk("%s: vreg_enable() = %d \n",
				__func__, rc);
		}
	}
    
#if defined(LG_HW_REV5) || defined(LG_HW_REV6)  || defined(LG_HW_REV7)
        lcd3_vreg = vreg_get(NULL, "rf");
    
        if (IS_ERR(lcd3_vreg)) {
            printk("%s: vreg_get(%s) failed (%ld)\n",
                    __func__, "gp7", PTR_ERR(lcd3_vreg));
            return -1;
        }
        if (lcd3_vreg) {
            rc = vreg_disable(lcd3_vreg);
            if (rc) {
                printk("%s: vreg_enable() = %d \n",
                    __func__, rc);
            }   
        }
#endif

        msleep(100);


#ifdef LG_HW_REV1
	lcd2_vreg = vreg_get(NULL, "rf");
#endif
/* jihye.ahn	10-09-20	   add Rev.C board config 2.8V LCD_VCI*/
#if defined(LG_HW_REV2) || defined(LG_HW_REV3) || defined(LG_HW_REV4) || defined(LG_HW_REV5) || defined(LG_HW_REV6)  || defined(LG_HW_REV7)
	lcd2_vreg = vreg_get(NULL, "gp10");
#endif
	if (IS_ERR(lcd2_vreg)) {
		printk("%s: vreg_get(%s) failed (%ld)\n",
			__func__, "rf", PTR_ERR(lcd2_vreg));
		return -1;
	}
	if (lcd2_vreg) {
		rc = vreg_disable(lcd2_vreg);
		if (rc) {
			printk("%s: vreg_enable() = %d \n",
				__func__, rc);
		}
	}
#else
	lgit_common_powerdown();
#endif
	
	return 0;
}

/* LGE_S   jihye.ahn   11-02-02   added LCD power control for CDMA test mode */
static int lcd_power_on(void)
{
	struct vreg *lcd_vreg, *lcd2_vreg, *lcd3_vreg;
	int rc;
         struct msm_panel_common_pdata *pdata = mddi_lgit_pdata;

          lcd_on = 1;  

#if defined(LG_HW_REV5) || defined(LG_HW_REV6)  || defined(LG_HW_REV7)
	lcd3_vreg = vreg_get(NULL, "rf");
	if (IS_ERR(lcd3_vreg)) {
			printk("%s: vreg_get(%s) failed (%ld)\n",
				__func__, "rf", PTR_ERR(lcd3_vreg));
			return -1;
		}
	if (lcd3_vreg) {
		rc = vreg_set_level(lcd3_vreg, 2800);
		if (rc) {
			printk("%s: vreg_set level failed (%d)\n",
				__func__, rc);
		}
		rc = vreg_enable(lcd3_vreg);
		if (rc) {
			printk("%s: vreg_enable() = %d \n",
				__func__, rc);
		}
	}
#endif

/* jihye.ahn	10-09-20	   add Rev.C board config */
#if defined(LG_HW_REV2) || defined(LG_HW_REV3) || defined(LG_HW_REV4) || defined(LG_HW_REV5) || defined(LG_HW_REV6)  || defined(LG_HW_REV7)
	lcd2_vreg = vreg_get(NULL, "gp10");
#endif
	if (IS_ERR(lcd2_vreg)) {
		printk("%s: vreg_get(%s) failed (%ld)\n",
			__func__, "gp10", PTR_ERR(lcd2_vreg));
		return -1;
	}
	if (lcd2_vreg) {
		rc = vreg_set_level(lcd2_vreg, 1800);
		if (rc) {
			printk("%s: vreg_set level failed (%d)\n",
				__func__, rc);
		}
		rc = vreg_enable(lcd2_vreg);
		if (rc) {
			printk("%s: vreg_enable() = %d \n",
				__func__, rc);
		}
	}

/* jihye.ahn	10-09-20	   add Rev.C board config */
#if defined(LG_HW_REV2) || defined(LG_HW_REV3) || defined(LG_HW_REV4) || defined(LG_HW_REV5) || defined(LG_HW_REV6)  || defined(LG_HW_REV7)
	lcd_vreg = vreg_get(NULL, "gp6");
#endif
	
	if (IS_ERR(lcd_vreg)) {
		printk("%s: vreg_get(%s) failed (%ld)\n",
			__func__, "gp6", PTR_ERR(lcd_vreg));
		return -1;
	}
	if (lcd_vreg) {
		rc = vreg_set_level(lcd_vreg, 2800);
		if (rc) {
			printk("%s: vreg_set level failed (%d)\n",
				__func__, rc);
		}
		rc = vreg_enable(lcd_vreg);
		if (rc) {
			printk("%s: vreg_enable() = %d \n",
				__func__, rc);
		}
	}
	
	gpio_tlmm_config(GPIO_CFG(95, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE); //LCD_RESET_N

	gpio_set_value(95, 0);
	msleep(20);
	gpio_set_value(95, 1);	
         msleep(20);

        lgit_common_initial_setup();

// BEGIN : munho.lee@lge.com 2010-10-26
// ADD :0010205: [Display] To change resolution for removing display noise. 
	mod_timer(&timer_mdp, jiffies + msecs_to_jiffies(20));	
// END : munho.lee@lge.com 2010-10-26

	return 0;
}
/* LGE_E   jihye.ahn   11-02-02   added LCD power control for CDMA test mode */

/* LGE_S   jihye.ahn   10-10-20   added LCD on/off interface for CDMA test mode */
ssize_t onoff_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	printk("%s : strat\n", __func__);

	return 0;
}
ssize_t onoff_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned int value, ret;

	sscanf(buf, "%d\n", &value);

	if (value == 0)
		display_off();

	else if (value == 1)
             ret = lcd_power_on();

	else
		printk("0 : off, 1 : on \n");

	return count;
}

//#2011.07.28 - CTS FAIL android.permission.cts.FileSystemPermissionTest#testAllBlockDevicesAreNotReadableWritable
DEVICE_ATTR(onoff, 0660, onoff_show, onoff_store);
//DEVICE_ATTR(onoff, 0666, onoff_show, onoff_store);
/* LGE_E   jihye.ahn   10-10-20   added LCD on/off interface for CDMA test mode */

static int __devinit mddi_lgit_lcd_probe(struct platform_device *pdev)
{
	printk("%s \n", __func__);
	int ret;
	if (pdev->id == 0) {
		mddi_lgit_pdata = pdev->dev.platform_data;
		return 0;
	}
// BEGIN : munho.lee@lge.com 2010-10-26
// ADD :0010205: [Display] To change resolution for removing display noise. 
	setup_timer(&timer_mdp, check_mdp_clkon, (unsigned long)pdev);
// END : munho.lee@lge.com 2010-10-26

	msm_fb_add_device(pdev);
	/* jihye.ahn   10-10-20   added LCD on/off interface for CDMA test mode */
	ret = device_create_file(&pdev->dev, &dev_attr_onoff);

	return 0;
}

static struct platform_driver this_driver = {
	.probe  = mddi_lgit_lcd_probe,
	.driver = {
		.name   = "mddi_lgit",
	},
};

static struct msm_fb_panel_data lgit_panel_data = {
	.on 		= mddi_lgit_lcd_on,
	.off 		= mddi_lgit_lcd_off,
};

int mddi_lgit_device_register(struct msm_panel_info *pinfo,
					u32 channel, u32 panel)
{
	struct platform_device *pdev = NULL;
	int ret;

	printk("%s channel=%d, panel=%d\n", __func__, channel, panel);

	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	if ((channel != 0) &&
	    mddi_lgit_pdata && mddi_lgit_pdata->panel_num)
		if (mddi_lgit_pdata->panel_num() < 2)
			return -ENODEV;

	ch_used[channel] = TRUE;

	pdev = platform_device_alloc("mddi_lgit", (panel << 8)|channel);
	if (!pdev)
		return -ENOMEM;

	if (channel == LGIT_VGA_PRIM) {
/* sabina.park	10.06.30
modify set backlight
*/
#if CONFIG_MACH_LGE_BRYCE 
		lgit_panel_data.set_backlight = NULL;
#else
		lgit_panel_data.set_backlight =
				mddi_lgit_lcd_set_backlight;
#endif
		if (pinfo->lcd.vsync_enable) {
			lgit_panel_data.set_vsync_notifier =
				mddi_lgit_vsync_set_handler;
			mddi_lcd.vsync_detected =
				mddi_lgit_lcd_vsync_detected;
		}
	} else {
		lgit_panel_data.set_backlight = NULL;
		lgit_panel_data.set_vsync_notifier = NULL;
	}

	lgit_panel_data.panel_info = *pinfo;

	ret = platform_device_add_data(pdev, &lgit_panel_data,
		sizeof(lgit_panel_data));
	if (ret) {
		printk(KERN_ERR
		  "%s: platform_device_add_data failed!\n", __func__);
		goto err_device_put;
	}

	ret = platform_device_add(pdev);
	if (ret) {
		printk(KERN_ERR
		  "%s: platform_device_register failed!\n", __func__);
		goto err_device_put;
	}

	return 0;

err_device_put:
	platform_device_put(pdev);
	return ret;
}

static int __init mddi_lgit_lcd_init(void)
{
	return platform_driver_register(&this_driver);
}

module_init(mddi_lgit_lcd_init);
