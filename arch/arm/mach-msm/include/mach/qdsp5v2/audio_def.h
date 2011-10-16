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
#ifndef _MACH_QDSP5_V2_AUDIO_DEF_H
#define _MACH_QDSP5_V2_AUDIO_DEF_H

/* daniel.kang@lge.com 100924 ++ */
#define TEST_MISC_DRVR 0
#define LOOPBACK_SUPPORT 1
/* daniel.kang@lge.com 100924 -- */

/* Define sound device capability */
#define SNDDEV_CAP_RX 0x1 /* RX direction */
#define SNDDEV_CAP_TX 0x2 /* TX direction */
#define SNDDEV_CAP_VOICE 0x4 /* Support voice call */
#define SNDDEV_CAP_PLAYBACK 0x8 /* Support playback */
#define SNDDEV_CAP_FM 0x10 /* Support FM radio */
#define SNDDEV_CAP_TTY 0x20 /* Support TTY */
#define SNDDEV_CAP_ANC 0x40 /* Support ANC */
#define VOC_NB_INDEX	0
#define VOC_WB_INDEX	1
#define VOC_RX_VOL_ARRAY_NUM	2

/* Device volume types . In Current deisgn only one of these are supported. */
#define SNDDEV_DEV_VOL_DIGITAL  0x1  /* Codec Digital volume control */
#define SNDDEV_DEV_VOL_ANALOG   0x2  /* Codec Analog volume control */

#define SIDE_TONE_MASK	0x01

/* daniel.kang@lge.com 100924 ++ */
#if LOOPBACK_SUPPORT
void headset_loopback_for_testmode(u32 loopbacken);
void handset_loopback_for_testmode(u32 loopbacken);
void QTR_handset_loopback_for_testmode(u32 loopbacken);
/*Baikal ID:0009964,BT pcm loopback support. ++kiran.kanneganti@lge.com*/
void bt_sco_loopback_for_testmode(u32 loopbacken);
/*jy0127.jang@lge.com MS910_AUDIO_CHANGE 2011-04-07*/
/*added speaker phone loopback*/
void QTR_speakerphone_loopback_for_testmode(u32 loopbacken);
#endif
/*BEGIN: 0011452 kiran.kanneganti@lge.com 2010-11-26*/
/*ADD 0011452: Noice cancellation check support for testmode*/
void lge_connect_disconnect_back_mic_path_inQTR(unsigned char on);
void lge_connect_disconnect_main_mic_path_inQTR(unsigned char on);
/* END: 0011452 kiran.kanneganti@lge.com 2010-11-26 */
#if TEST_MISC_DRVR
int audio_misc_driver_test(unsigned int test_fn);
#endif
/* daniel.kang@lge.com 100924 -- */

#endif /* _MACH_QDSP5_V2_AUDIO_DEF_H */
