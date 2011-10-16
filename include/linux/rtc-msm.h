/* Copyright (c) 2011, Code Aurora Forum. All rights reserved.
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

#ifndef __RTC_MSM_H__
#define __RTC_MSM_H__

/*
 * This is the only function which updates the xtime structure. This
 * function is supposed to be called only once during kernel initialization.
 * But we need to call this function whenever we receive an RTC update
 * from MODEM.
 */
int rtc_hctosys(void);

extern void msm_pm_set_max_sleep_time(int64_t sleep_time_ns);
int64_t msm_timer_get_sclk_time(int64_t *period);
int64_t msmrtc_get_tickatsuspend(void);

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
void msmrtc_set_tickatsuspend(int64_t now);
#endif
bool msmrtc_is_suspended(void);

#endif  /* __RTC_MSM_H__ */
