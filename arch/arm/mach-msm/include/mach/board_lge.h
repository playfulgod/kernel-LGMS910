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
#ifndef __ASM_ARCH_MSM_BOARD_LGE_H
#define __ASM_ARCH_MSM_BOARD_LGE_H

#include <linux/types.h>
#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/i2c-gpio.h>
#include <linux/rfkill.h>
#include <linux/platform_device.h>
#include <asm/setup.h>

#if __GNUC__
#define __WEAK __attribute__((weak))
#endif

#if 0//def CONFIG_ANDROID_RAM_CONSOLE
#define MSM7X30_EBI0_CS0_BASE	0x07A00000
#define BRYCE_RAM_CONSOLE_BASE	MSM7X30_EBI0_CS0_BASE
#define BRYCE_RAM_CONSOLE_SIZE	(128 * SZ_1K)
#define BRYCE_RAM_MISC_BASE		MSM7X30_EBI0_CS0_BASE + BRYCE_RAM_CONSOLE_SIZE
#define BRYCE_RAM_MISC_SIZE		(4 * SZ_1K) // original 128k 
#if defined(CONFIG_LGE_LTE_ERS)
#define BRYCE_RAM_LTE_LOG	(MSM7X30_EBI0_CS0_BASE+BRYCE_RAM_MISC_SIZE)
#define LTE_LOG_START		(BRYCE_RAM_LTE_LOG+BRYCE_RAM_MISC_SIZE)
#define LTE_LOG_SIZE		((128 * SZ_1K) - BRYCE_RAM_MISC_SIZE)
#endif // CONFIG_LGE_LTE_ERS
#endif // CONFIG_ANDROID_RAM_CONSOLE

#define MSM_PMEM_SF_SIZE	0x1700000
#define MSM_FB_SIZE			0x500000
#define MSM_GPU_PHYS_SIZE	SZ_2M

#define MSM_PMEM_ADSP_SIZE		0x1900000 //0x1800000 : extend for VT
#define PMEM_KERNEL_EBI1_SIZE	0x600000
#define MSM_PMEM_AUDIO_SIZE		0x200000

#ifdef CONFIG_LGE_HIDDEN_RESET_PATCH
extern int hidden_reset_enable;
extern int on_hidden_reset;
void *lge_get_fb_addr(void);
void *lge_get_fb_copy_virt_addr(void);
void *lge_get_fb_copy_phys_addr(void);
#endif // CONFIG_LGE_HIDDEN_RESET_PATCH
void lge_set_reboot_reason(unsigned int);
#endif
