/* arch/arm/mach-msm/rpc_server_time_remote.c
 *
 * Copyright (C) 2007 Google, Inc.
 * Copyright (c) 2009-2011 Code Aurora Forum. All rights reserved.
 * Author: Iliyan Malchev <ibm@android.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <mach/msm_rpcrouter.h>
#include "rpc_server_time_remote.h"
#include <linux/rtc.h>
#include <linux/android_alarm.h>
#include <linux/rtc-msm.h>

/* time_remote_mtoa server definitions. */

#define TIME_REMOTE_MTOA_PROG 0x3000005d
#define TIME_REMOTE_MTOA_VERS_OLD 0
#define TIME_REMOTE_MTOA_VERS 0x9202a8e4
#define TIME_REMOTE_MTOA_VERS_COMP 0x00010002
#define RPC_TIME_REMOTE_MTOA_NULL   0
#define RPC_TIME_TOD_SET_APPS_BASES 2
#define RPC_TIME_GET_APPS_USER_TIME 3

struct rpc_time_tod_set_apps_bases_args {
	uint32_t tick;
	uint64_t stamp;
};

static int read_rtc0_time(struct msm_rpc_server *server,
		   struct rpc_request_hdr *req,
		   unsigned len)
{
	int err;
	unsigned long tm_sec;
	uint32_t size = 0;
	void *reply;
	uint32_t output_valid;
	uint32_t rpc_status = RPC_ACCEPTSTAT_SYSTEM_ERR;
	struct rtc_time tm;
	struct rtc_device *rtc = rtc_class_open(CONFIG_RTC_HCTOSYS_DEVICE);

	if (rtc == NULL) {
		pr_err("%s: unable to open rtc device (%s)\n",
			__FILE__, CONFIG_RTC_HCTOSYS_DEVICE);
		goto send_reply;
	}

	err = rtc_read_time(rtc, &tm);
	if (err) {
		pr_err("%s: Error reading rtc device (%s) : %d\n",
			__FILE__, CONFIG_RTC_HCTOSYS_DEVICE, err);
		goto close_dev;
	}

	err = rtc_valid_tm(&tm);
	if (err) {
		pr_err("%s: Invalid RTC time (%s)\n",
			__FILE__, CONFIG_RTC_HCTOSYS_DEVICE);
		goto close_dev;
	}

	rtc_tm_to_time(&tm, &tm_sec);
	rpc_status = RPC_ACCEPTSTAT_SUCCESS;

close_dev:
	rtc_class_close(rtc);

send_reply:
	reply = msm_rpc_server_start_accepted_reply(server, req->xid,
						    rpc_status);
	if (rpc_status == RPC_ACCEPTSTAT_SUCCESS) {
		output_valid = *((uint32_t *)(req + 1));
		*(uint32_t *)reply = output_valid;
		size = sizeof(uint32_t);
		if (be32_to_cpu(output_valid)) {
			reply += sizeof(uint32_t);
			*(uint32_t *)reply = cpu_to_be32(tm_sec);
			size += sizeof(uint32_t);
		}
	}
	err = msm_rpc_server_send_accepted_reply(server, size);
	if (err)
		pr_err("%s: send accepted reply failed: %d\n", __func__, err);

	return 1;
}

static int handle_rpc_call(struct msm_rpc_server *server,
			   struct rpc_request_hdr *req, unsigned len)
{
	struct timespec ts, tv;

	switch (req->procedure) {
	case RPC_TIME_REMOTE_MTOA_NULL:
		return 0;

	case RPC_TIME_TOD_SET_APPS_BASES: {
		struct rpc_time_tod_set_apps_bases_args *args;
		args = (struct rpc_time_tod_set_apps_bases_args *)(req + 1);
		args->tick = be32_to_cpu(args->tick);
		args->stamp = be64_to_cpu(args->stamp);
		printk(KERN_INFO "RPC_TIME_TOD_SET_APPS_BASES:\n"
		       "\ttick = %d\n"
		       "\tstamp = %lld\n",
		       args->tick, args->stamp);

		getnstimeofday(&ts);

/* CR 291540 - Function/Feature Failure (Partial)
 * system uptime gets corrupted with overflow in slow clock.
 * 
 * Problem description
 * During power collapse, APPS sleep time is calculated using slow clock
 * ticks. The calculation of sleep time does not considers slow clock
 * overflow and thus If slow clock overflows during suspend state then we
 * get wrong sleep time and thus system uptime values gets corrupted.
 * earlier sleep time was calculates as follows:
 * sleep = current_sclk_tick - suspend_state_sclk_tick
 * 
 * Failure frequency: Occasionally
 * Scenario frequency: Uncommon
 * Change description
 * Modified the sleep time calculation to include slow clock overflow as follows:
 * Now sleep time is calculated as:
 * sleep = Maximum_sclk_tick_val - suspend_state_sclk_tick + current_sclk_tick.
 * Files affected
 * kernel/drivers/rtc/rtc-msm.c
 * kernel/arch/arm/mach-msm/rpc_server_time_remote.c
*/	
		if (msmrtc_is_suspended()) {
#if 1 //QCT SBA 404016
			int64_t now, sleep, tick_at_suspend, sclk_max;
			now = msm_timer_get_sclk_time(&sclk_max);
#else
			int64_t now, sleep, tick_at_suspend;
			now = msm_timer_get_sclk_time(NULL);
#endif
			
			tick_at_suspend = msmrtc_get_tickatsuspend();
			if (now && tick_at_suspend) {
#if 1 //QCT SBA 404016
				if (now < tick_at_suspend) {
					sleep = sclk_max - tick_at_suspend +
							now;
				} else {
					sleep = now - tick_at_suspend;
				}
#else
				sleep = now - tick_at_suspend;
#endif
				timespec_add_ns(&ts, sleep);
/* CR 293735 - Function/Feature Failure (Partial)
 * When system was in suspend, could make invalid "elapsed system time" at AP side.
 * 
 * Problem description
 * When system is in suspend mode and if more than one network time update
 * comes while system is in suspend mode then the uptime gets corrupted.
 * 
 * Failure frequency: Occasionally
 * Scenario frequency: Uncommon
 * Change description
 * Added change to modify tick_at_suspend (variable that stores time tick
 * while entering suspend) after each update to the current time tick.
 * Setting the suspend mode variable in case alarm time expires the current
 * RTC time as in thic case also system enters suspend mode.
 * to sdcc irq handlers returning IRQ_NONE without handling the interrupt.
 * When interrupt is disabled the communication between the SDIO client and
 * SDCC host controller is stopped while a pending command is in progress
 * and hence WLAN operation is stuck forever and LOGP recovery cannot be
 * processed.
 * Files affected
 * arch/arm/mach-msm/rpc_server_time_remote.c
 * drivers/rtc/rtc-msm.c
 * include/linux/rtc-msm.h
 */
#if 1 //QCT SBA 404017
				msmrtc_set_tickatsuspend(now);
#endif
			} else
				pr_err("%s: Invalid ticks from SCLK"
					"now=%lld tick_at_suspend=%lld",
					__func__, now, tick_at_suspend);

		}
		rtc_hctosys();
		getnstimeofday(&tv);
		/* Update the alarm information with the new time info. */
		alarm_update_timedelta(ts, tv);
		return 0;
	}

	case RPC_TIME_GET_APPS_USER_TIME:
		return read_rtc0_time(server, req, len);

	default:
		return -ENODEV;
	}
}

static struct msm_rpc_server rpc_server[] = {
	{
		.prog = TIME_REMOTE_MTOA_PROG,
		.vers = TIME_REMOTE_MTOA_VERS_OLD,
		.rpc_call = handle_rpc_call,
	},
	{
		.prog = TIME_REMOTE_MTOA_PROG,
		.vers = TIME_REMOTE_MTOA_VERS,
		.rpc_call = handle_rpc_call,
	},
	{
		.prog = TIME_REMOTE_MTOA_PROG,
		.vers = TIME_REMOTE_MTOA_VERS_COMP,
		.rpc_call = handle_rpc_call,
	},
};

static int __init rpc_server_init(void)
{
	/* Dual server registration to support backwards compatibility vers */
	int ret;
	ret = msm_rpc_create_server(&rpc_server[2]);
	if (ret < 0)
		return ret;
	ret = msm_rpc_create_server(&rpc_server[1]);
	if (ret < 0)
		return ret;
	return msm_rpc_create_server(&rpc_server[0]);
}


module_init(rpc_server_init);
