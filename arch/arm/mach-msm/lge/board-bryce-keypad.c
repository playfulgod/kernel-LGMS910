/* arch/arm/mach-msm/board-bryce-keypad.c
 * Copyright (C) 2007-2009 HTC Corporation.
 * Author: Thomas Tsai <thomas_tsai@htc.com>
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

#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/gpio_event.h>
#include <asm/mach-types.h>
//#include "gpio_chip.h"

static char *keycaps = "--qwerty";

#undef MODULE_PARAM_PREFIX
#define MODULE_PARAM_PREFIX "board_bryce."
module_param_named(keycaps, keycaps, charp, 0);

#if defined(LG_HW_REV3) || defined(LG_HW_REV4)
#define NUM_COL_0_GPIO	19
#define NUM_ROW_0_GPIO	20
#define NUM_ROW_1_GPIO	56
#else
#if defined (LGE_HW_MS910_REV2)|| defined (LGE_HW_MS910_REV3)
#define NUM_COL_0_GPIO  56
#define NUM_ROW_0_GPIO  24
#define NUM_ROW_1_GPIO  19
#else // defined (LGE_HW_MS910_REV4)|| defined (LGE_HW_MS910_REV5)
#define NUM_COL_0_GPIO  20
#define NUM_ROW_0_GPIO  19
#define NUM_ROW_1_GPIO  56
#endif /* LGE_HW_MS910_REV2 */
#endif

static unsigned int bryce_col_gpios[] = { NUM_COL_0_GPIO }; //output pins

/* KP_MKIN2 (GPIO40) is not used? */
static unsigned int bryce_row_gpios[] = { NUM_ROW_0_GPIO, NUM_ROW_1_GPIO }; //input pins

#define KEYMAP_INDEX(col, row) ((col)*ARRAY_SIZE(bryce_row_gpios) + (row))

/*scan matrix key*/
static const unsigned short bryce_keymap[ARRAY_SIZE(bryce_col_gpios) *
					ARRAY_SIZE(bryce_row_gpios)] = {
#if 0 //FIXME temp assign
	[KEYMAP_INDEX(0, 0)] = KEY_END, 
	[KEYMAP_INDEX(0, 1)] = KEY_BACK, 
#else
	[KEYMAP_INDEX(0, 0)] = KEY_VOLUMEUP, 
	[KEYMAP_INDEX(0, 1)] = KEY_VOLUMEDOWN, 
#endif
};

static struct gpio_event_matrix_info bryce_keypad_matrix_info = {
	.info.func = gpio_event_matrix_func,
	.keymap = bryce_keymap,
	.output_gpios = bryce_col_gpios,
	.input_gpios = bryce_row_gpios,
	.noutputs = ARRAY_SIZE(bryce_col_gpios),
	.ninputs = ARRAY_SIZE(bryce_row_gpios),
	.settle_time.tv.nsec = 40 * NSEC_PER_USEC,
	.poll_time.tv.nsec = 20 * NSEC_PER_MSEC,
	.debounce_delay.tv.nsec = 50 * NSEC_PER_MSEC,
	.flags = GPIOKPF_ACTIVE_HIGH |
		 GPIOKPF_REMOVE_PHANTOM_KEYS |
		 GPIOKPF_PRINT_UNMAPPED_KEYS /*| GPIOKPF_PRINT_MAPPED_KEYS*/
};

static struct gpio_event_info *bryce_keypad_info[] = {
	&bryce_keypad_matrix_info.info
};

static struct gpio_event_platform_data bryce_keypad_data = {
	.name = "bryce-keypad",
	.info = bryce_keypad_info,
	.info_count = ARRAY_SIZE(bryce_keypad_info)
};

static struct platform_device bryce_keypad_device = {
	.name = GPIO_EVENT_DEV_NAME,
	.id = 0,
	.dev		= {
		.platform_data	= &bryce_keypad_data,
	},
};

static int __init bryce_init_keypad(void)
{
	bryce_keypad_matrix_info.keymap = bryce_keymap;

	return platform_device_register(&bryce_keypad_device);
}

device_initcall(bryce_init_keypad);

