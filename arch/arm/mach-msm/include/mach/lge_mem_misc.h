/* arch/arm/mach-msm/include/mach/lge_ers.h
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
#ifndef __ASM_ARCH_MSM_LGE_ERS_H
#define __ASM_ARCH_MSM_LGE_ERS_H

#include <linux/types.h>


#if __GNUC__
#define __WEAK __attribute__((weak))
#endif

#ifdef CONFIG_LGE_RAM_CONSOLE
/* allocate 128K * 2 instead of ram_console's original size 128K
 * this is for storing kernel panic log which is used by lk loader
 */
#define MSM7X27_EBI1_CS0_BASE	PHYS_OFFSET
#define MSM7X30_EBI1_CS0_BASE	PHYS_OFFSET
#define LGE_RAM_CONSOLE_SIZE    (128 * SZ_1K * 2)

void __init lge_add_ramconsole_devices(void);
#endif /*CONFIG_LGE_RAM_CONSOLE*/

#ifdef CONFIG_LGE_ERS
#define PANIC_MAGIC_KEY	0x12345678
#define CRASH_ARM9		0x87654321

struct panic_log_dump {
	unsigned int magic_key;
	unsigned int size;
	unsigned char buffer[0];
};

/* current misc_buffer's size is 4k bytes */
struct misc_buffer {
	unsigned int magic_key;
#ifdef CONFIG_LGE_LTE_ERS
	unsigned int lte_magic;
	unsigned int lte_reserved;
#endif
	unsigned int size;
	unsigned char buffer[0];
};

enum {
	REBOOT_KEY_PRESS = 0,
	REBOOT_KEY_NOT_PRESS,
};

struct lge_panic_handler_platform_data {
	int (*reboot_key_detect)(void);
};

void __init lge_add_panic_handler_devices(void);
void __init lge_add_ers_devices(void);
//struct ram_console_buffer *get_ram_console_buffer(void);
void lge_set_reboot_reason(unsigned int reason);

#ifdef CONFIG_LGE_LTE_ERS
#define LGE_RAM_CONSOLE_MISC_SIZE		(4 * SZ_1K) // original 128k 
#define LTE_LOG_SIZE		((128 * SZ_1K) - LGE_RAM_CONSOLE_MISC_SIZE)

/*********************** BEGIN PACK() Definition ***************************/
#if defined __GNUC__
  #define PACK(x)       x __attribute__((__packed__))
  #define PACKED        __attribute__((__packed__))
#elif defined __arm
  #define PACK(x)       __packed x
  #define PACKED        __packed
#else
  #error No PACK() macro defined for this compiler
#endif
/********************** END PACK() Definition *****************************/

#define	HIM_LTE_CRASH_DUMP		0xF0E2

typedef PACKED struct
{
	unsigned int cmd;		// save : Save Cmd = 0x10
	unsigned int attribute;	// start (0x10), continue (0x20), end (0x01)
	unsigned int file_num;	// lte_total_dump_0, 1, ..., incase file_num is 0 it will be the last file
	unsigned int  file_seq;	// 1, 2, ... N
	unsigned int  raw_data_size; // byte
	unsigned int reserved[3];
}S_L2K_RAM_DUMP_HDR_TYPE;

typedef enum
{
	S_L2K_RAM_DUMP_CMD_SAVE = 0x10,	
}S_L2K_RAM_DUMP_CMD_TYPE;

typedef enum
{
	S_L2K_RAM_DUMP_ATTR_END = 0x01,
	S_L2K_RAM_DUMP_ATTR_START = 0x10,
	S_L2K_RAM_DUMP_ATTR_CONTINUE = 0x20,
	S_L2K_RAM_DUMP_ATTR_PREV = 0xF0,
	
}S_L2K_RAM_DUMP_ATTRIBUTE_TYPE;
#endif /*CONFIG_LGE_LTE_ERS*/

#endif /*CONFIG_LGE_ERS*/


#endif /*__ASM_ARCH_MSM_LGE_ERS_H*/
