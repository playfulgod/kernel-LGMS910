/* Copyright (c) 2009, Code Aurora Forum. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of Code Aurora Forum, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef __ASM__ARCH_OEM_RAPI_CLIENT_H
#define __ASM__ARCH_OEM_RAPI_CLIENT_H

/*
 * OEM RAPI CLIENT Driver header file
 */

#include <linux/types.h>
#include <mach/msm_rpcrouter.h>

enum {
	OEM_RAPI_CLIENT_EVENT_NONE = 0,

	/*
	 * list of oem rapi client events
	 */

#if defined (CONFIG_LGE_SUPPORT_RAPI)
	/* LGE_CHANGES_S [khlee@lge.com] 2009-12-04, [VS740] use OEMRAPI */
	LG_FW_RAPI_START = 100,
	LG_FW_RAPI_CLIENT_EVENT_GET_LINE_TYPE = LG_FW_RAPI_START,
	LG_FW_TESTMODE_EVENT_FROM_ARM11 = LG_FW_RAPI_START + 1,
	LG_FW_A2M_BATT_INFO_GET = LG_FW_RAPI_START + 2,
	LG_FW_A2M_PSEUDO_BATT_INFO_SET = LG_FW_RAPI_START + 3,
	LG_FW_MEID_GET = LG_FW_RAPI_START + 4,
	/* LGE_CHANGE_S 
	 * SUPPORT TESTMODE FOR AIRPLAN MODE
	 * 2010-07-12 taehung.kim@lge.com
	 */
	LG_FW_SET_OPERATION_MODE = LG_FW_RAPI_START + 5,
	LG_FW_A2M_BLOCK_CHARGING_SET = LG_FW_RAPI_START + 6,
/* BEGIN: 0013860 jihoon.lee@lge.com 20110111 */
/* ADD 0013860: [FACTORY RESET] ERI file save */
#ifdef CONFIG_LGE_ERI_DOWNLOAD
	LGE_REQUEST_ERI_RPC = LG_FW_RAPI_START + 7,
#endif
/* END: 0013860 jihoon.lee@lge.com 20110111 */
#endif
	/* [yk.kim@lge.com] 2011-01-25, get manual test mode NV */
#ifdef CONFIG_LGE_USB_GADGET_SUPPORT_FACTORY_USB
	LG_FW_MANUAL_TEST_MODE = LG_FW_RAPI_START + 8,
#endif
/* BEGIN: 0016311 jihoon.lee@lge.com 20110217 */
/* ADD 0016311: [POWER OFF] CALL EFS_SYNC */
#ifdef CONFIG_LGE_SUPPORT_RAPI
	LGE_RPC_HANDLE_REQUEST = LG_FW_RAPI_START + 9,
#endif
/* END: 0016311 jihoon.lee@lge.com 20110217 */
#ifdef CONFIG_LGE_USB_GADGET_LLDM_DRIVER
    LG_FW_LLDM_SDIO_INFO_SET = LG_FW_RAPI_START + 10,
    LG_FW_LLDM_SDIO_INFO_GET = LG_FW_RAPI_START + 11,
#endif

#ifdef CONFIG_LGE_FOTA_MISC_INFO
	LGE_REMOTE_RPC_REQUEST_VAL = LG_FW_RAPI_START + 12,
#endif
	/* LGE_CHANGES_S [jaeho.cho@lge.com] 2010-10-02, charger logo notification to modem */
#ifdef CONFIG_LGE_CHARGING_MODE_INFO
	LG_FW_CHG_LOGO_MODE = LG_FW_RAPI_START + 13,
#endif

#ifdef CONFIG_LGE_LTE_USB_SWITCH_REQUEST
	LG_FW_LTE_USB_SWITCH_REQUEST = LG_FW_RAPI_START + 14,
#endif
	OEM_RAPI_CLIENT_EVENT_MAX

};

/* BEGIN: 0016311 jihoon.lee@lge.com 20110217 */
/* ADD 0016311: [POWER OFF] CALL EFS_SYNC */
#ifdef CONFIG_LGE_SUPPORT_RAPI
enum {
	LGE_CLIENT_CMD_START = 0,
#ifdef CONFIG_LGE_SYNC_CMD
	LGE_SYNC_REQUEST = 1,
#endif
	LGE_SW_VERSION_INFO = 2,
	LGE_MIN_INFO = 3,
	LGE_TESTMODE_MANUAL_TEST_INFO = 4,
	LGE_MEID_INFO = 5,
	LGE_ESN_INFO = 6,
	LGE_CLIENT_CMD_MAX = 0xF,
};
#endif
/* ADD: [FOTA] LGE_FOTA_MISC_INFO */
/* END: 0016311 jihoon.lee@lge.com 20110217 */

struct oem_rapi_client_streaming_func_cb_arg {
	uint32_t  event;
	void      *handle;
	uint32_t  in_len;
	char      *input;
	uint32_t out_len_valid;
	uint32_t output_valid;
	uint32_t output_size;
};

struct oem_rapi_client_streaming_func_cb_ret {
	uint32_t *out_len;
	char *output;
};

struct oem_rapi_client_streaming_func_arg {
	uint32_t event;
	int (*cb_func)(struct oem_rapi_client_streaming_func_cb_arg *,
		       struct oem_rapi_client_streaming_func_cb_ret *);
	void *handle;
	uint32_t in_len;
	char *input;
	uint32_t out_len_valid;
	uint32_t output_valid;
	uint32_t output_size;
};

struct oem_rapi_client_streaming_func_ret {
	uint32_t *out_len;
	char *output;
};

int oem_rapi_client_streaming_function(
	struct msm_rpc_client *client,
	struct oem_rapi_client_streaming_func_arg *arg,
	struct oem_rapi_client_streaming_func_ret *ret);

int oem_rapi_client_close(void);

struct msm_rpc_client *oem_rapi_client_init(void);

#endif
