/* Copyright (c) 2009-2010, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */
#include <linux/platform_device.h>
#include <linux/debugfs.h>
#include <linux/mfd/msm-adie-codec.h>
#include <linux/uaccess.h>
#include <mach/qdsp5v2/snddev_icodec.h>
#include <mach/qdsp5v2/marimba_profile.h>
#include <mach/qdsp5v2/aux_pcm.h>
#include <mach/qdsp5v2/snddev_ecodec.h>
#include <mach/qdsp5v2/audio_dev_ctl.h>
#include <mach/qdsp5v2/snddev_virtual.h>
#include <mach/board.h>
#include <asm/mach-types.h>
#include <mach/gpio.h>
#include <mach/qdsp5v2/snddev_mi2s.h>
#include <mach/qdsp5v2/mi2s.h>
#include <mach/qdsp5v2/audio_acdb_def.h>

#if defined (CONFIG_MACH_LGE_BRYCE)

/* BEGIN:0010882        ehgrace.kim@lge.com     2010.11.15*/
/* MOD: add the call mode */
#define LGE_AUDIO_PATH 1
/* END:0010882        ehgrace.kim@lge.com     2010.11.15*/
/* CONFIG_MACH_LGE_BRYCE	ehgrace.kim 10.05.03
	add LGE amp
*/
#include <mach/board-bryce.h>
// srinivas.mittapalli@lge.com 16Sept2010 baikal 0009676
#define LGE_CUST_CAL   // undef this macro after tuning Done 
#endif

/* define the value for BT_SCO */
#define BT_SCO_PCM_CTL_VAL (PCM_CTL__RPCM_WIDTH__LINEAR_V |\
				PCM_CTL__TPCM_WIDTH__LINEAR_V)
#define BT_SCO_DATA_FORMAT_PADDING (DATA_FORMAT_PADDING_INFO__RPCM_FORMAT_V |\
				DATA_FORMAT_PADDING_INFO__TPCM_FORMAT_V)
#define BT_SCO_AUX_CODEC_INTF   AUX_CODEC_INTF_CTL__PCMINTF_DATA_EN_V


#ifdef CONFIG_DEBUG_FS
static struct dentry *debugfs_hsed_config;
static void snddev_hsed_config_modify_setting(int type);
static void snddev_hsed_config_restore_setting(void);
#endif

/* srinivas.mittapalli@lge.com 16Sept2010 baikal 0009676 */
#ifdef  LGE_CUST_CAL
/* daniel.kang@lge.com ++ Nov 17 // 0011018: Remove Audience Code */
/* 1. Remove unnecessary mic bias variables */
/* 2. Original variables are in audio_cal_tool.c */
extern int mic_bias0_handset_mic;	// = 1000;
extern int mic_bias1_handset_mic;	// = 1100;
extern int mic_bias0_spkmic;		// = 1400;
extern int mic_bias1_spkmic;		// = 1500;
extern int mic_bias0_handset_endfire;	// = 1800;
extern int mic_bias1_handset_endfire;	// = 1900;
extern int mic_bias0_spk_endfire;	// = 2200;
extern int mic_bias1_spk_endfire;	// = 2300;
/* daniel.kang@lge.com -- Nov 17 // 0011018: Remove Audience Code */
#if LGE_AUDIO_PATH
/* BEGIN:0010882        ehgrace.kim@lge.com     2010.11.15*/
/* MOD: add the call mode */
extern int mic_bias0_spkmic2;  // = 1700;
extern int mic_bias1_spkmic2;  // =1700;
extern int mic_bias0_handset_rec_mic;	// = 1700;
extern int mic_bias1_handset_rec_mic;	// = 1700;

/* RX volume parameter ++ */
struct volumelevel{
	s32 max_level;
	s32 min_level;
};

extern struct volumelevel handset_rx[2];
extern struct volumelevel headset_stereo_rx[2];
extern struct volumelevel headset_mono_rx[2];
extern struct volumelevel spk_rx[2];
extern struct volumelevel headset_spk_rx[2];
extern struct volumelevel BT_earpiece_rx[2];
extern struct volumelevel spk_stereo_rx[2];
/* END:0010882        ehgrace.kim@lge.com     2010.11.15*/
#endif

#endif //LGE_CUST_CAl
/* srinivas.mittapalli@lge.com 16Sept2010 */

/* daniel.kang@lge.com comments ++ */
/* To release static variable (struct adie_codec_action_unit XXXXXXXX) remove static in case of LGE_CUST_CAL */
/* daniel.kang@lge.com comments ++ */

/////////////////////////////////////////////////////////////////////////////////////////
// Handset mono RX (only for voice call)
/////////////////////////////////////////////////////////////////////////////////////////
#ifndef LGE_CUST_CAL
static 
#endif 
struct adie_codec_action_unit iearpiece_48KHz_osr256_actions[] =
	HANDSET_RX_48000_OSR_256;

static struct adie_codec_hwsetting_entry iearpiece_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = iearpiece_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(iearpiece_48KHz_osr256_actions),
	}
};

static struct adie_codec_dev_profile iearpiece_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = iearpiece_settings,
	.setting_sz = ARRAY_SIZE(iearpiece_settings),
};
/* baikal ID 0009676::srinivas.mittapalli@lge.com 11Oct2010 */
#ifndef LGE_CUST_CAL
static 
#endif
struct snddev_icodec_data snddev_iearpiece_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "handset_rx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_HANDSET_SPKR,
	.profile = &iearpiece_profile,
	.channel_mode = 1,
	.pmctl_id = NULL,
	.pmctl_id_sz = 0,
	.default_sample_rate = 48000,
	.pamp_on = NULL,
	.pamp_off = NULL,
	.property = SIDE_TONE_MASK,
	.max_voice_rx_vol[VOC_NB_INDEX] = -200,
	.min_voice_rx_vol[VOC_NB_INDEX] = -1700,
	.max_voice_rx_vol[VOC_WB_INDEX] = -200,
	.min_voice_rx_vol[VOC_WB_INDEX] = -1700
};

static struct platform_device msm_iearpiece_device = {
	.name = "snddev_icodec",
	.id = 0,
	.dev = { .platform_data = &snddev_iearpiece_data },
};

/////////////////////////////////////////////////////////////////////////////////////////
// Handset TX Dual Mic with main mic as primary
//              ( QCT default configuration is Single, MIC1 - in Bryce Back mic)
/////////////////////////////////////////////////////////////////////////////////////////

#if defined (CONFIG_MACH_LGE_BRYCE)
/////////////////////////
#ifndef LGE_CUST_CAL
static 
#endif
struct adie_codec_action_unit imic_8KHz_osr256_actions[] =
	MIC1_RIGHT_AUX_IN_LEFT_8000_OSR_256;

//TODO: Need to be verified from QCOM.
/*QCOM Dual MIC using 8K config for 16 & 48 K.*/
static struct adie_codec_hwsetting_entry imic_settings[] = {
	{
		.freq_plan = 8000,
		.osr = 256,
		.actions = imic_8KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(imic_8KHz_osr256_actions),
	},/* 8KHz profile can be used for 16Khz */
	{
		.freq_plan = 16000,
		.osr = 256,
		.actions = imic_8KHz_osr256_actions, //imic_16KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(imic_8KHz_osr256_actions), //ARRAY_SIZE(imic_16KHz_osr256_actions),
	},/* 8KHz profile can be used for 48Khz */
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = imic_8KHz_osr256_actions,//imic_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(imic_8KHz_osr256_actions), //ARRAY_SIZE(imic_48KHz_osr256_actions),
	}
};

static struct adie_codec_dev_profile imic_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = imic_settings,
	.setting_sz = ARRAY_SIZE(imic_settings),
};
/*BEGIN: 0012072 kiran.kanneganti@lge.com 2010-12-09*/
/*Mod 0012072:  Mic Bias code change*/
#if defined(LG_HW_REV4)
static enum hsed_controller imic_pmctl_id[] = {PM_HSED_CONTROLLER_0,PM_HSED_CONTROLLER_1};
#else
static enum hsed_controller imic_pmctl_id[] = {PM_HSED_CONTROLLER_0};
#endif
/*END: 0012072 kiran.kanneganti@lge.com 2010-12-09*/
static struct snddev_icodec_data snddev_imic_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "handset_tx",
	.copp_id = 0,
// daniel.kang@lge.com ++ 3rd Nov
// 0010463: Enable Fluence in handset, speaker TX
	.acdb_id = ACDB_ID_HANDSET_MIC_ENDFIRE,	// ACDB_ID_HANDSET_MIC,
// daniel.kang@lge.com -- 3rd Nov
	.profile = &imic_profile,
	.channel_mode = 2,
	.pmctl_id = imic_pmctl_id,
	.pmctl_id_sz = ARRAY_SIZE(imic_pmctl_id),
	.default_sample_rate = 48000,
/* CONFIG_MACH_LGE_BRYCE	ehgrace.kim 10.05.03
	add LGE amp*/

/* daniel.kang@lge.com ++ */
/* change pamp_on/off name : mic_amp_on controls the mic swith, not turn on amp */
/* MSM_mic_routing_config makes CAM_MIC_EN switch 0 for camcorder recording */

/* daniel.kang@lge.com ++ Nov 17 // 0011018: Remove Audience Code */
#if defined(LG_HW_REV4) || defined(LG_HW_REV5)
	.pamp_on = &lge_snddev_MSM_mic_route_config,
#else
	.pamp_on = NULL,
#endif
/* daniel.kang@lge.com -- Nov 17 // 0011018: Remove Audience Code */
	.pamp_off = NULL,
// srinivas.mittapalli@lge.com 16Sept2010 ++
// baikal 0009676
#ifdef LGE_CUST_CAL
 	.micbias_current = {&mic_bias0_handset_mic,&mic_bias1_handset_mic},
#else
	.micbias_current = {1700,1700}, // Need to keep the Tuned values
#endif //LGE_CUST_CAL
// srinivas.mittapalli@lge.com 16Sept2010 --
/* daniel.kang@lge.com -- */
};

static struct platform_device msm_imic_device = {
	.name = "snddev_icodec",
	.id = 1,
	.dev = { .platform_data = &snddev_imic_data },
};


//////////////////////////////////////////////////////////////////////
#else	// Qualcomm Single MIC if not defined (CONFIG_MACH_LGE_BRYCE)
//////////////////////////////////////////////////////////////////////

static struct adie_codec_action_unit imic_8KHz_osr256_actions[] =
	HANDSET_TX_8000_OSR_256;

static struct adie_codec_action_unit imic_16KHz_osr256_actions[] =
	HANDSET_TX_16000_OSR_256;

static struct adie_codec_action_unit imic_48KHz_osr256_actions[] =
	HANDSET_TX_48000_OSR_256;

static struct adie_codec_hwsetting_entry imic_settings[] = {
	{
		.freq_plan = 8000,
		.osr = 256,
		.actions = imic_8KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(imic_8KHz_osr256_actions),
	},
	{
		.freq_plan = 16000,
		.osr = 256,
		.actions = imic_16KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(imic_16KHz_osr256_actions),
	},
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = imic_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(imic_48KHz_osr256_actions),
	}
};

static struct adie_codec_dev_profile imic_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = imic_settings,
	.setting_sz = ARRAY_SIZE(imic_settings),
};

static enum hsed_controller imic_pmctl_id[] = {PM_HSED_CONTROLLER_0};

static struct snddev_icodec_data snddev_imic_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "handset_tx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_HANDSET_MIC,
	.profile = &imic_profile,
	.channel_mode = 1,
	.pmctl_id = imic_pmctl_id,
	.pmctl_id_sz = ARRAY_SIZE(imic_pmctl_id),
	.default_sample_rate = 48000,
	.pamp_on = NULL,
	.pamp_off = NULL,
};

static struct platform_device msm_imic_device = {
	.name = "snddev_icodec",
	.id = 1,
	.dev = { .platform_data = &snddev_imic_data },
};

/////////////////////////
#endif  // (CONFIG_MACH_LGE_BRYCE)

/////////////////////////////////////////////////////////////////////////////////////////
// Headset Stereo RX : no change (voice? and sound)
/////////////////////////////////////////////////////////////////////////////////////////
#ifndef LGE_CUST_CAL
static 
#endif
#if defined (LGE_HW_MS910_REV5)
struct adie_codec_action_unit ihs_stereo_rx_48KHz_osr256_actions[] =
   LG_HEADSET_STEREO_RX_CAPLESS_48000_OSR_256;
#else
 struct adie_codec_action_unit ihs_stereo_rx_48KHz_osr256_actions[] =
   HEADSET_STEREO_RX_LEGACY_48000_OSR_256;
#endif
static struct adie_codec_hwsetting_entry ihs_stereo_rx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ihs_stereo_rx_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(ihs_stereo_rx_48KHz_osr256_actions),
	}
};

static struct adie_codec_dev_profile ihs_stereo_rx_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = ihs_stereo_rx_settings,
	.setting_sz = ARRAY_SIZE(ihs_stereo_rx_settings),
};

/* baikal ID 0009676::srinivas.mittapalli@lge.com 11Oct2010 */
#ifndef LGE_CUST_CAL
static 
#endif
struct snddev_icodec_data snddev_ihs_stereo_rx_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "headset_stereo_rx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_HEADSET_SPKR_STEREO,
	.profile = &ihs_stereo_rx_profile,
	.channel_mode = 2,
	.default_sample_rate = 48000,
/* CONFIG_MACH_LGE_BRYCE	ehgrace.kim 10.05.03
	add LGE amp
*/
#if defined (CONFIG_MACH_LGE_BRYCE)
	.pamp_on = &lge_snddev_hs_amp_on,
	.pamp_off = &lge_snddev_amp_off,
#else
	.pamp_on = NULL,
	.pamp_off = NULL,
#endif
#if defined (CONFIG_MACH_LGE_BRYCE)
#if defined (LGE_HW_MS910_REV5)
	.voltage_on = msm_snddev_hsed_voltage_on,
	.voltage_off = msm_snddev_hsed_voltage_off,
#else
	.voltage_on = NULL,
	.voltage_off = NULL,
#endif
#endif	
	.property = SIDE_TONE_MASK,
	.max_voice_rx_vol[VOC_NB_INDEX] = -700,
	.min_voice_rx_vol[VOC_NB_INDEX] = -2200,
	.max_voice_rx_vol[VOC_WB_INDEX] = -900,
	.min_voice_rx_vol[VOC_WB_INDEX] = -2400
};

static struct platform_device msm_ihs_stereo_rx_device = {
	.name = "snddev_icodec",
	.id = 2,
	.dev = { .platform_data = &snddev_ihs_stereo_rx_data },
};

/////////////////////////////////////////////////////////////////////////////////////////
// Headset mono RX : no change (voice?)
/////////////////////////////////////////////////////////////////////////////////////////
//jy0127.jang@lge.com audio config change for rev1.0
#ifndef LGE_CUST_CAL
static 
#endif
#if defined (LGE_HW_MS910_REV5)
 struct adie_codec_action_unit ihs_mono_rx_48KHz_osr256_actions[] =
//HEADSET_RX_LEGACY_48000_OSR_256;
//Baikal ID 0009962 :: Headset Mono configuration for voice call ++kiran.kanneganti@lge.com
HEADSET_MONO_RX_CAPLESS_48000_OSR_256;
//--kiran.kanneganti@lge.com
#else
 struct adie_codec_action_unit ihs_mono_rx_48KHz_osr256_actions[] =
HEADSET_MONO_RX_LEGACY_48000_OSR_256;
#endif
static struct adie_codec_hwsetting_entry ihs_mono_rx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ihs_mono_rx_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(ihs_mono_rx_48KHz_osr256_actions),
	}
};

static struct adie_codec_dev_profile ihs_mono_rx_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = ihs_mono_rx_settings,
	.setting_sz = ARRAY_SIZE(ihs_mono_rx_settings),
};

/* baikal ID 0009676::srinivas.mittapalli@lge.com 11Oct2010 */
#ifndef LGE_CUST_CAL
static 
#endif
 struct snddev_icodec_data snddev_ihs_mono_rx_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "headset_mono_rx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_HEADSET_SPKR_MONO,
	.profile = &ihs_mono_rx_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
#if defined (CONFIG_MACH_LGE_BRYCE) //Baikal ID 0009962 Headset Mono Configuration for call.
/* BEGIN:0012120        ehgrace.kim@lge.com     2010.12.10*/
/* MOD: fix to control the hs amp for call mode */
	.pamp_on = &lge_snddev_hs_phone_amp_on, //ehgrace.kim@lge.com
/* END:0012120        ehgrace.kim@lge.com     2010.12.10*/
	.pamp_off = &lge_snddev_amp_off,
#else
	.pamp_on = NULL,
	.pamp_off = NULL,
#endif
#if defined (CONFIG_MACH_LGE_BRYCE)
#if defined (LGE_HW_MS910_REV5)
	.voltage_on = msm_snddev_hsed_voltage_on,
	.voltage_off = msm_snddev_hsed_voltage_off,
#else
	.voltage_on = NULL,
	.voltage_off = NULL,
#endif
#endif	
	.property = SIDE_TONE_MASK,
	.max_voice_rx_vol[VOC_NB_INDEX] = -700,
	.min_voice_rx_vol[VOC_NB_INDEX] = -2200,
	.max_voice_rx_vol[VOC_WB_INDEX] = -900,
	.min_voice_rx_vol[VOC_WB_INDEX] = -2400,

};

static struct platform_device msm_ihs_mono_rx_device = {
	.name = "snddev_icodec",
	.id = 3,
	.dev = { .platform_data = &snddev_ihs_mono_rx_data },
};

/////////////////////////////////////////////////////////////////////////////////////////
// Headset stereo RX (FFA) : don't care
/////////////////////////////////////////////////////////////////////////////////////////

static struct adie_codec_action_unit ihs_ffa_stereo_rx_48KHz_osr256_actions[] =
	HEADSET_STEREO_RX_CAPLESS_48000_OSR_256;

static struct adie_codec_hwsetting_entry ihs_ffa_stereo_rx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ihs_ffa_stereo_rx_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(ihs_ffa_stereo_rx_48KHz_osr256_actions),
	}
};

#ifdef CONFIG_DEBUG_FS
static struct adie_codec_action_unit
	ihs_ffa_stereo_rx_class_d_legacy_48KHz_osr256_actions[] =
	HEADSET_STEREO_RX_CLASS_D_LEGACY_48000_OSR_256;

static struct adie_codec_hwsetting_entry
	ihs_ffa_stereo_rx_class_d_legacy_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions =
		ihs_ffa_stereo_rx_class_d_legacy_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE
		(ihs_ffa_stereo_rx_class_d_legacy_48KHz_osr256_actions),
	}
};

static struct adie_codec_action_unit
	ihs_ffa_stereo_rx_class_ab_legacy_48KHz_osr256_actions[] =
	HEADSET_STEREO_RX_LEGACY_48000_OSR_256;

static struct adie_codec_hwsetting_entry
	ihs_ffa_stereo_rx_class_ab_legacy_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions =
		ihs_ffa_stereo_rx_class_ab_legacy_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE
		(ihs_ffa_stereo_rx_class_ab_legacy_48KHz_osr256_actions),
	}
};
#endif

static struct adie_codec_dev_profile ihs_ffa_stereo_rx_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = ihs_ffa_stereo_rx_settings,
	.setting_sz = ARRAY_SIZE(ihs_ffa_stereo_rx_settings),
};

static struct snddev_icodec_data snddev_ihs_ffa_stereo_rx_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "headset_stereo_rx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_HEADSET_SPKR_STEREO,
	.profile = &ihs_ffa_stereo_rx_profile,
	.channel_mode = 2,
	.default_sample_rate = 48000,
	.voltage_on = msm_snddev_hsed_voltage_on,
	.voltage_off = msm_snddev_hsed_voltage_off,
	.max_voice_rx_vol[VOC_NB_INDEX] = -700,
	.min_voice_rx_vol[VOC_NB_INDEX] = -2200,
	.max_voice_rx_vol[VOC_WB_INDEX] = -900,
	.min_voice_rx_vol[VOC_WB_INDEX] = -2400,
};

static struct platform_device msm_ihs_ffa_stereo_rx_device = {
	.name = "snddev_icodec",
	.id = 4,
	.dev = { .platform_data = &snddev_ihs_ffa_stereo_rx_data },
};

/////////////////////////////////////////////////////////////////////////////////////////
// Headset mono RX (FFA) : don't care
/////////////////////////////////////////////////////////////////////////////////////////

static struct adie_codec_action_unit ihs_ffa_mono_rx_48KHz_osr256_actions[] =
	HEADSET_RX_CAPLESS_48000_OSR_256;

static struct adie_codec_hwsetting_entry ihs_ffa_mono_rx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ihs_ffa_mono_rx_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(ihs_ffa_mono_rx_48KHz_osr256_actions),
	}
};

static struct adie_codec_dev_profile ihs_ffa_mono_rx_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = ihs_ffa_mono_rx_settings,
	.setting_sz = ARRAY_SIZE(ihs_ffa_mono_rx_settings),
};

static struct snddev_icodec_data snddev_ihs_ffa_mono_rx_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "headset_mono_rx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_HEADSET_SPKR_MONO,
	.profile = &ihs_ffa_mono_rx_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_hsed_voltage_on,
	.pamp_off = msm_snddev_hsed_voltage_off,
	.max_voice_rx_vol[VOC_NB_INDEX] = -700,
	.min_voice_rx_vol[VOC_NB_INDEX] = -2200,
	.max_voice_rx_vol[VOC_WB_INDEX] = -900,
	.min_voice_rx_vol[VOC_WB_INDEX] = -2400,
};

static struct platform_device msm_ihs_ffa_mono_rx_device = {
	.name = "snddev_icodec",
	.id = 5,
	.dev = { .platform_data = &snddev_ihs_ffa_mono_rx_data },
};

/////////////////////////////////////////////////////////////////////////////////////////
// Headset mono TX : (voice and sound)
/////////////////////////////////////////////////////////////////////////////////////////

static struct adie_codec_action_unit ihs_mono_tx_8KHz_osr256_actions[] =
	HEADSET_MONO_TX_8000_OSR_256;

static struct adie_codec_action_unit ihs_mono_tx_16KHz_osr256_actions[] =
	HEADSET_MONO_TX_16000_OSR_256;
#ifndef LGE_CUST_CAL
static 
#endif
struct adie_codec_action_unit ihs_mono_tx_48KHz_osr256_actions[] =
	HEADSET_MONO_TX_48000_OSR_256;

static struct adie_codec_hwsetting_entry ihs_mono_tx_settings[] = {
	{
		.freq_plan = 8000,
		.osr = 256,
		.actions = ihs_mono_tx_8KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(ihs_mono_tx_8KHz_osr256_actions),
	},
	{
		.freq_plan = 16000,
		.osr = 256,
		.actions = ihs_mono_tx_16KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(ihs_mono_tx_16KHz_osr256_actions),
	},
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ihs_mono_tx_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(ihs_mono_tx_48KHz_osr256_actions),
	}
};

static struct adie_codec_dev_profile ihs_mono_tx_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = ihs_mono_tx_settings,
	.setting_sz = ARRAY_SIZE(ihs_mono_tx_settings),
};

static struct snddev_icodec_data snddev_ihs_mono_tx_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "headset_mono_tx",
	.copp_id = 0,
/* BEGIN:0010882        ehgrace.kim@lge.com     2010.11.15*/
/* MOD: add the call mode */
#if LGE_AUDIO_PATH
	.acdb_id = ACDB_ID_HANDSET_MIC,  //this is temporary value
#else
	.acdb_id = ACDB_ID_HEADSET_MIC,
#endif
/* END:0010882        ehgrace.kim@lge.com     2010.11.15*/
	.profile = &ihs_mono_tx_profile,
	.channel_mode = 1,
	.pmctl_id = NULL,
	.pmctl_id_sz = 0,
	.default_sample_rate = 48000,
/*BEGIN: 0012072 kiran.kanneganti@lge.com 2010-12-09*/
/*Mod 0012072:  Mic routing code change*/	
#if defined (CONFIG_MACH_LGE_BRYCE)
	.pamp_on = NULL,
	.pamp_off = NULL,
#else
	.pamp_on = msm_snddev_tx_route_config,
	.pamp_off = msm_snddev_tx_route_deconfig,
#endif
/*END: 0012072 kiran.kanneganti@lge.com 2010-12-09*/	
};

static struct platform_device msm_ihs_mono_tx_device = {
	.name = "snddev_icodec",
	.id = 6,
	.dev = { .platform_data = &snddev_ihs_mono_tx_data },
};

/////////////////////////////////////////////////////////////////////////////////////////
// FM Radio : don't care
/////////////////////////////////////////////////////////////////////////////////////////

static struct adie_codec_action_unit ifmradio_handset_osr64_actions[] =
	FM_HANDSET_OSR_64;

static struct adie_codec_hwsetting_entry ifmradio_handset_settings[] = {
	{
		.freq_plan = 8000,
		.osr = 256,
		.actions = ifmradio_handset_osr64_actions,
		.action_sz = ARRAY_SIZE(ifmradio_handset_osr64_actions),
	}
};

static struct adie_codec_dev_profile ifmradio_handset_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = ifmradio_handset_settings,
	.setting_sz = ARRAY_SIZE(ifmradio_handset_settings),
};

static struct snddev_icodec_data snddev_ifmradio_handset_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_FM),
	.name = "fmradio_handset_rx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_LP_FM_SPKR_PHONE_STEREO_RX,
	.profile = &ifmradio_handset_profile,
	.channel_mode = 1,
	.default_sample_rate = 8000,
	.pamp_on = NULL,
	.pamp_off = NULL,
	.dev_vol_type = SNDDEV_DEV_VOL_DIGITAL,
};

static struct platform_device msm_ifmradio_handset_device = {
	.name = "snddev_icodec",
	.id = 7,
	.dev = { .platform_data = &snddev_ifmradio_handset_data },
};

/////////////////////////////////////////////////////////////////////////////////////////
// Speaker RX (sound)
/////////////////////////////////////////////////////////////////////////////////////////

/* CONFIG_MACH_LGE_BRYCE	ehgrace.kim 10.05.03
	changed the speaker rx setting
*/
#if defined (CONFIG_MACH_LGE_BRYCE)
#ifndef LGE_CUST_CAL
static 
#endif
struct adie_codec_action_unit ispeaker_rx_48KHz_osr256_actions[] =
   SPEAKER_RX_48000_OSR_256;
#else
static struct adie_codec_action_unit ispeaker_rx_48KHz_osr256_actions[] =
   SPEAKER_STEREO_RX_48000_OSR_256;
#endif

static struct adie_codec_hwsetting_entry ispeaker_rx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ispeaker_rx_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(ispeaker_rx_48KHz_osr256_actions),
	}
};

static struct adie_codec_dev_profile ispeaker_rx_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = ispeaker_rx_settings,
	.setting_sz = ARRAY_SIZE(ispeaker_rx_settings),
};
//Bryce Speaker is Mono. kiran.kanneganti@lge.com
/* baikal ID 0009676::srinivas.mittapalli@lge.com 11Oct2010 */
#ifndef LGE_CUST_CAL
static 
#endif
 struct snddev_icodec_data snddev_ispeaker_rx_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "speaker_stereo_rx",
	.copp_id = 0,
/* BEGIN:0010882        ehgrace.kim@lge.com     2010.11.15*/
/* MOD: add the call mode */
#if LGE_AUDIO_PATH
	.acdb_id = ACDB_ID_SPKR_PHONE_STEREO,  //this is the temporary value
#else
	.acdb_id = ACDB_ID_SPKR_PHONE_MONO,
#endif
/* END:0010882        ehgrace.kim@lge.com     2010.11.15*/
	.profile = &ispeaker_rx_profile,
	.channel_mode = 1,
	.pmctl_id = NULL,
	.pmctl_id_sz = 0,
	.default_sample_rate = 48000,
/* CONFIG_MACH_LGE_BRYCE	ehgrace.kim 10.05.03
	add LGE amp
*/
#if defined (CONFIG_MACH_LGE_BRYCE)
	.pamp_on = &lge_snddev_spk_amp_on,
	.pamp_off = &lge_snddev_amp_off,
#else
	.pamp_on = &msm_snddev_poweramp_on,
	.pamp_off = &msm_snddev_poweramp_off,
#endif
	.max_voice_rx_vol[VOC_NB_INDEX] = 1000,
	.min_voice_rx_vol[VOC_NB_INDEX] = -500,
	.max_voice_rx_vol[VOC_WB_INDEX] = 1000,
	.min_voice_rx_vol[VOC_WB_INDEX] = -500,
};

static struct platform_device msm_ispeaker_rx_device = {
	.name = "snddev_icodec",
	.id = 8,
	.dev = { .platform_data = &snddev_ispeaker_rx_data },

};


/* BEGIN: 0013064 kiran.kanneganti@lge.com 2010-12-23*/ 
/* ADD 0013064: New Rx device for skype*/
/////////////////////////////////////////////////////////////////////////////////////////
//Skype  Speaker RX 
/////////////////////////////////////////////////////////////////////////////////////////

#ifndef LGE_CUST_CAL
static 
#endif
struct adie_codec_action_unit i_skype_speaker_rx_48KHz_osr256_actions[] =
   SPEAKER_SKYPE_RX_48000_OSR_256;

static struct adie_codec_hwsetting_entry i_skype_speaker_rx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = i_skype_speaker_rx_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(i_skype_speaker_rx_48KHz_osr256_actions),
	}
};

static struct adie_codec_dev_profile i_skype_speaker_rx_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = i_skype_speaker_rx_settings,
	.setting_sz = ARRAY_SIZE(i_skype_speaker_rx_settings),
};

#ifndef LGE_CUST_CAL
static 
#endif
 struct snddev_icodec_data snddev_i_skype_speaker_rx_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "skype_speaker_stereo_rx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_SKYPE_SPEAKER_RX,
	.profile = &i_skype_speaker_rx_profile,
	.channel_mode = 1,
	.pmctl_id = NULL,
	.pmctl_id_sz = 0,
	.default_sample_rate = 48000,
/* CONFIG_MACH_LGE_BRYCE	ehgrace.kim 10.05.03
	add LGE amp*/
#if defined (CONFIG_MACH_LGE_BRYCE)
	.pamp_on = &lge_snddev_spk_amp_on,
	.pamp_off = &lge_snddev_amp_off,
#else
	.pamp_on = &msm_snddev_poweramp_on,
	.pamp_off = &msm_snddev_poweramp_off,
#endif
	.max_voice_rx_vol[VOC_NB_INDEX] = 1000,
	.min_voice_rx_vol[VOC_NB_INDEX] = -500,
	.max_voice_rx_vol[VOC_WB_INDEX] = 1000,
	.min_voice_rx_vol[VOC_WB_INDEX] = -500,
};

static struct platform_device msm_skype_speaker_rx_device = {
	.name = "snddev_icodec",
	.id = 30,
	.dev = { .platform_data = &snddev_i_skype_speaker_rx_data },

};
/* END: 0013064 kiran.kanneganti@lge.com 2010-12-23 */

/////////////////////////////////////////////////////////////////////////////////////////
// FM Speaker : don't care
/////////////////////////////////////////////////////////////////////////////////////////

static struct adie_codec_action_unit ifmradio_speaker_osr64_actions[] =
	FM_SPEAKER_OSR_64;

static struct adie_codec_hwsetting_entry ifmradio_speaker_settings[] = {
	{
		.freq_plan = 8000,
		.osr = 256,
		.actions = ifmradio_speaker_osr64_actions,
		.action_sz = ARRAY_SIZE(ifmradio_speaker_osr64_actions),
	}
};

static struct adie_codec_dev_profile ifmradio_speaker_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = ifmradio_speaker_settings,
	.setting_sz = ARRAY_SIZE(ifmradio_speaker_settings),
};

static struct snddev_icodec_data snddev_ifmradio_speaker_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_FM),
	.name = "fmradio_speaker_rx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_LP_FM_SPKR_PHONE_STEREO_RX,
	.profile = &ifmradio_speaker_profile,
	.channel_mode = 1,
	.default_sample_rate = 8000,
	.pamp_on = &msm_snddev_poweramp_on,
	.pamp_off = &msm_snddev_poweramp_off,
	.dev_vol_type = SNDDEV_DEV_VOL_DIGITAL,
};

static struct platform_device msm_ifmradio_speaker_device = {
	.name = "snddev_icodec",
	.id = 9,
	.dev = { .platform_data = &snddev_ifmradio_speaker_data },
};

/////////////////////////////////////////////////////////////////////////////////////////
// FM headset : don't care
/////////////////////////////////////////////////////////////////////////////////////////

static struct adie_codec_action_unit ifmradio_headset_osr64_actions[] =
	FM_HEADSET_STEREO_CLASS_D_LEGACY_OSR_64;

static struct adie_codec_hwsetting_entry ifmradio_headset_settings[] = {
	{
		.freq_plan = 8000,
		.osr = 256,
		.actions = ifmradio_headset_osr64_actions,
		.action_sz = ARRAY_SIZE(ifmradio_headset_osr64_actions),
	}
};

static struct adie_codec_dev_profile ifmradio_headset_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = ifmradio_headset_settings,
	.setting_sz = ARRAY_SIZE(ifmradio_headset_settings),
};

static struct snddev_icodec_data snddev_ifmradio_headset_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_FM),
	.name = "fmradio_headset_rx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_LP_FM_HEADSET_SPKR_STEREO_RX,
	.profile = &ifmradio_headset_profile,
	.channel_mode = 1,
	.default_sample_rate = 8000,
	.pamp_on = NULL,
	.pamp_off = NULL,
	.dev_vol_type = SNDDEV_DEV_VOL_DIGITAL,
};

static struct platform_device msm_ifmradio_headset_device = {
	.name = "snddev_icodec",
	.id = 10,
	.dev = { .platform_data = &snddev_ifmradio_headset_data },
};

/////////////////////////////////////////////////////////////////////////////////////////
// FM headset (FFA) : don't care
/////////////////////////////////////////////////////////////////////////////////////////

static struct adie_codec_action_unit ifmradio_ffa_headset_osr64_actions[] =
	FM_HEADSET_CLASS_AB_STEREO_CAPLESS_OSR_64;

static struct adie_codec_hwsetting_entry ifmradio_ffa_headset_settings[] = {
	{
		.freq_plan = 8000,
		.osr = 256,
		.actions = ifmradio_ffa_headset_osr64_actions,
		.action_sz = ARRAY_SIZE(ifmradio_ffa_headset_osr64_actions),
	}
};

static struct adie_codec_dev_profile ifmradio_ffa_headset_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = ifmradio_ffa_headset_settings,
	.setting_sz = ARRAY_SIZE(ifmradio_ffa_headset_settings),
};

static struct snddev_icodec_data snddev_ifmradio_ffa_headset_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_FM),
	.name = "fmradio_headset_rx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_LP_FM_HEADSET_SPKR_STEREO_RX,
	.profile = &ifmradio_ffa_headset_profile,
	.channel_mode = 1,
	.default_sample_rate = 8000,
	.pamp_on = msm_snddev_hsed_voltage_on,
	.pamp_off = msm_snddev_hsed_voltage_off,
	.dev_vol_type = SNDDEV_DEV_VOL_DIGITAL,
};

static struct platform_device msm_ifmradio_ffa_headset_device = {
	.name = "snddev_icodec",
	.id = 11,
	.dev = { .platform_data = &snddev_ifmradio_ffa_headset_data },
};

/////////////////////////////////////////////////////////////////////////////////////////
// BT Voice RX
/////////////////////////////////////////////////////////////////////////////////////////

/* baikal ID 0009676::srinivas.mittapalli@lge.com 11Oct2010 */
#ifndef LGE_CUST_CAL
static 
#endif
struct snddev_ecodec_data snddev_bt_sco_earpiece_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "bt_sco_rx",
	.copp_id = 1,
	.acdb_id = ACDB_ID_BT_SCO_SPKR,
	.channel_mode = 1,
	.conf_pcm_ctl_val = BT_SCO_PCM_CTL_VAL,
	.conf_aux_codec_intf = BT_SCO_AUX_CODEC_INTF,
	.conf_data_format_padding_val = BT_SCO_DATA_FORMAT_PADDING,
	.max_voice_rx_vol[VOC_NB_INDEX] = 400,
	.min_voice_rx_vol[VOC_NB_INDEX] = -1100,
	.max_voice_rx_vol[VOC_WB_INDEX] = 400,
	.min_voice_rx_vol[VOC_WB_INDEX] = -1100,
/* daniel.kang@lge.com ++ */
/* Since we add the following element, we need to initialize them */
#if defined (CONFIG_MACH_LGE_BRYCE)
//include pmic to add mic bias for external codec also. Kiran kanneganti. 26-aug-2010		
	.pmctl_id = NULL,
	.pmctl_id_sz = 0,
#endif
/* daniel.kang@lge.com -- */
};

/////////////////////////////////////////////////////////////////////////////////////////
// BT Voice TX
/////////////////////////////////////////////////////////////////////////////////////////

static struct snddev_ecodec_data snddev_bt_sco_mic_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "bt_sco_tx",
	.copp_id = 1,
	.acdb_id = ACDB_ID_BT_SCO_MIC,
	.channel_mode = 1,
	.conf_pcm_ctl_val = BT_SCO_PCM_CTL_VAL,
	.conf_aux_codec_intf = BT_SCO_AUX_CODEC_INTF,
	.conf_data_format_padding_val = BT_SCO_DATA_FORMAT_PADDING,
/* daniel.kang@lge.com ++ */
/* Since we add the following element, we need to initialize them */
#if defined (CONFIG_MACH_LGE_BRYCE)
//include pmic to add mic bias for external codec also. Kiran kanneganti. 26-aug-2010		
	.pmctl_id = NULL,
	.pmctl_id_sz = 0,
#endif
/* daniel.kang@lge.com -- */
};

struct platform_device msm_bt_sco_earpiece_device = {
	.name = "msm_snddev_ecodec",
	.id = 0,
	.dev = { .platform_data = &snddev_bt_sco_earpiece_data },
};

struct platform_device msm_bt_sco_mic_device = {
	.name = "msm_snddev_ecodec",
	.id = 1,
	.dev = { .platform_data = &snddev_bt_sco_mic_data },
};


/////////////////////////////////////////////////////////////////////////////////////////
// Dual Mic Handset TX (Back mic as primary) sound
/////////////////////////////////////////////////////////////////////////////////////////
#ifndef LGE_CUST_CAL
static 
#endif
struct adie_codec_action_unit idual_mic_endfire_8KHz_osr256_actions[] =
/* Kiran.kanneganti@lge.com 07-SEP-2010 ++ */
/* for QCOM dual Mic Support */
#if defined (CONFIG_MACH_LGE_BRYCE)
	MIC1_LEFT_AUX_IN_RIGHT_8000_OSR_256;
#else /* This is the qualcomm default code */
	MIC1_LEFT_LINE_IN_RIGHT_8000_OSR_256;
#endif
/* Kiran.kanneganti@lge.com 07-SEP-2010 -- */
static struct adie_codec_hwsetting_entry idual_mic_endfire_settings[] = {
	{
		.freq_plan = 8000,
		.osr = 256,
		.actions = idual_mic_endfire_8KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(idual_mic_endfire_8KHz_osr256_actions),
	}, /* 8KHz profile can be used for 16KHz */
	{
		.freq_plan = 16000,
		.osr = 256,
		.actions = idual_mic_endfire_8KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(idual_mic_endfire_8KHz_osr256_actions),
	}, /* 8KHz profile can be used for 48KHz */
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = idual_mic_endfire_8KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(idual_mic_endfire_8KHz_osr256_actions),
	}
};

static struct adie_codec_dev_profile idual_mic_endfire_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = idual_mic_endfire_settings,
	.setting_sz = ARRAY_SIZE(idual_mic_endfire_settings),
};
/* Kiran.kanneganti@lge.com 07-SEP-2010 ++ */
/* for QCOM dual Mic Support */
#if defined (CONFIG_MACH_LGE_BRYCE)
/*BEGIN: 0012072 kiran.kanneganti@lge.com 2010-12-09*/
/*Mod 0012072:  Mic Bias code change*/
#if defined(LG_HW_REV4)
static enum hsed_controller idual_mic_endfire_pmctl_id[] = {
	PM_HSED_CONTROLLER_0,PM_HSED_CONTROLLER_1
};
#else
static enum hsed_controller idual_mic_endfire_pmctl_id[] = {
	PM_HSED_CONTROLLER_0
};
#endif
/*END: 0012072 kiran.kanneganti@lge.com 2010-12-09*/
#else /* This is the qualcomm default code */
static enum hsed_controller idual_mic_endfire_pmctl_id[] = {
	PM_HSED_CONTROLLER_0, PM_HSED_CONTROLLER_2
};
#endif
/* Kiran.kanneganti@lge.com 07-SEP-2010 -- */

static struct snddev_icodec_data snddev_idual_mic_endfire_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "handset_dual_mic_endfire_tx",
	.copp_id = 0,
/*BEGIN: 0012072 kiran.kanneganti@lge.com 2010-12-09*/
/*DEL 0012072: Removed Unused code*/
	.acdb_id = ACDB_ID_HANDSET_MIC_ENDFIRE,
	.profile = &idual_mic_endfire_profile,
	.channel_mode = 2,
/*END: 0012072 kiran.kanneganti@lge.com 2010-12-09*/
	.default_sample_rate = 48000,
	.pmctl_id = idual_mic_endfire_pmctl_id,
	.pmctl_id_sz = ARRAY_SIZE(idual_mic_endfire_pmctl_id),
/* daniel.kang@lge.com ++ Nov 17 // 0011018: Remove Audience Code */
#if defined(LG_HW_REV4) || defined(LG_HW_REV5)
	.pamp_on = &lge_snddev_MSM_mic_route_config,
#else
	.pamp_on = NULL,
#endif
/* daniel.kang@lge.com -- Nov 17 // 0011018: Remove Audience Code */
	.pamp_off = NULL,
/* Kiran.kanneganti@lge.com 07-SEP-2010 -- */
// srinivas.mittapalli@lge.com 16Sept2010 baikal 0009676
#ifdef  LGE_CUST_CAL
 	.micbias_current = {&mic_bias0_handset_endfire,&mic_bias1_handset_endfire},
#else
	.micbias_current = {1700,1700}, // Need to keep the Tuned values
#endif //LGE_CUST_CAL
// srinivas.mittapalli@lge.com 16Sept2010
};

static struct platform_device msm_idual_mic_endfire_device = {
	.name = "snddev_icodec",
	.id = 12,
	.dev = { .platform_data = &snddev_idual_mic_endfire_data },
};


static struct snddev_icodec_data\
		snddev_idual_mic_endfire_real_stereo_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "handset_dual_mic_endfire_tx_real_stereo",
	.copp_id = 0,
	.acdb_id = PSEUDO_ACDB_ID,
	.profile = &idual_mic_endfire_profile,
	.channel_mode = REAL_STEREO_CHANNEL_MODE,
	.default_sample_rate = 48000,
	.pmctl_id = idual_mic_endfire_pmctl_id,
	.pmctl_id_sz = ARRAY_SIZE(idual_mic_endfire_pmctl_id),
	.pamp_on = NULL,
	.pamp_off = NULL,
};

static struct platform_device msm_real_stereo_tx_device = {
	.name = "snddev_icodec",
	.id = 26,
	.dev = { .platform_data =
			&snddev_idual_mic_endfire_real_stereo_data },
};

static struct adie_codec_action_unit idual_mic_bs_8KHz_osr256_actions[] =
	MIC1_LEFT_AUX_IN_RIGHT_8000_OSR_256;

static struct adie_codec_hwsetting_entry idual_mic_broadside_settings[] = {
	{
		.freq_plan = 8000,
		.osr = 256,
		.actions = idual_mic_bs_8KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(idual_mic_bs_8KHz_osr256_actions),
	}, /* 8KHz profile can be used for 16KHz */
	{
		.freq_plan = 16000,
		.osr = 256,
		.actions = idual_mic_bs_8KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(idual_mic_bs_8KHz_osr256_actions),
	}, /* 8KHz profile can be used for 16KHz */
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = idual_mic_bs_8KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(idual_mic_bs_8KHz_osr256_actions),
	}
};

static struct adie_codec_dev_profile idual_mic_broadside_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = idual_mic_broadside_settings,
	.setting_sz = ARRAY_SIZE(idual_mic_broadside_settings),
};

static enum hsed_controller idual_mic_broadside_pmctl_id[] = {
	PM_HSED_CONTROLLER_0, PM_HSED_CONTROLLER_2
};

static struct snddev_icodec_data snddev_idual_mic_broadside_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "handset_dual_mic_broadside_tx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_HANDSET_MIC_BROADSIDE,
	.profile = &idual_mic_broadside_profile,
	.channel_mode = 2,
	.default_sample_rate = 48000,
	.pmctl_id = idual_mic_broadside_pmctl_id,
	.pmctl_id_sz = ARRAY_SIZE(idual_mic_broadside_pmctl_id),
	.pamp_on = NULL,
	.pamp_off = NULL,
};

static struct platform_device msm_idual_mic_broadside_device = {
	.name = "snddev_icodec",
	.id = 13,
	.dev = { .platform_data = &snddev_idual_mic_broadside_data },
};

/////////////////////////////////////////////////////////////////////////////////////////
// Dual Mic TX Speaker Mode : Back Mic is primary
/////////////////////////////////////////////////////////////////////////////////////////
#ifndef LGE_CUST_CAL
static 
#endif
struct adie_codec_action_unit ispk_dual_mic_ef_8KHz_osr256_actions[] =
/*BEGIN: 0012072 kiran.kanneganti@lge.com 2010-12-09*/
/*Del 0012072:  Removed unused code*/
#if defined (CONFIG_MACH_LGE_BRYCE)//for QCOM dual Mic Support. Kiran.kanneganti@lge.com
	MIC1_LEFT_AUX_IN_RIGHT_8000_OSR_256;
/*END: 0012072 kiran.kanneganti@lge.com 2010-12-09*/
#else
	SPEAKER_MIC1_LEFT_LINE_IN_RIGHT_8000_OSR_256;
#endif

static struct adie_codec_hwsetting_entry ispk_dual_mic_ef_settings[] = {
	{
		.freq_plan = 8000,
		.osr = 256,
		.actions = ispk_dual_mic_ef_8KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(ispk_dual_mic_ef_8KHz_osr256_actions),
	}, /* 8KHz profile can be used for 16Khz */
	{
		.freq_plan = 16000,
		.osr = 256,
		.actions = ispk_dual_mic_ef_8KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(ispk_dual_mic_ef_8KHz_osr256_actions),
	}, /* 8KHz profile can be used for 48KHz */
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ispk_dual_mic_ef_8KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(ispk_dual_mic_ef_8KHz_osr256_actions),
	},
};

static struct adie_codec_dev_profile ispk_dual_mic_ef_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = ispk_dual_mic_ef_settings,
	.setting_sz = ARRAY_SIZE(ispk_dual_mic_ef_settings),
};

static struct snddev_icodec_data snddev_spk_idual_mic_endfire_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "speaker_dual_mic_endfire_tx",
	.copp_id = 0,
/*BEGIN: 0012072 kiran.kanneganti@lge.com 2010-12-09*/
/*Del 0012072: removed Unused code*/
	.acdb_id = ACDB_ID_SPKR_PHONE_MIC_ENDFIRE,
	.profile = &ispk_dual_mic_ef_profile,
	.channel_mode = 2,
/*END: 0012072 kiran.kanneganti@lge.com 2010-12-09*/
	.default_sample_rate = 48000,
	.pmctl_id = idual_mic_endfire_pmctl_id,
	.pmctl_id_sz = ARRAY_SIZE(idual_mic_endfire_pmctl_id),
/* daniel.kang@lge.com ++ Nov 17 // 0011018: Remove Audience Code */
#if defined(LG_HW_REV4) || defined(LG_HW_REV5)
	.pamp_on = &lge_snddev_MSM_mic_route_config,
#else
	.pamp_on = NULL,
#endif
/* daniel.kang@lge.com -- Nov 17 // 0011018: Remove Audience Code */
	.pamp_off = NULL,
// srinivas.mittapalli@lge.com 16Sept2010 baikal 0009676
#ifdef LGE_CUST_CAL
 	.micbias_current = {&mic_bias0_spk_endfire,&mic_bias1_spk_endfire},
#else
	.micbias_current = {1700,1700}, // Need to keep the Tuned values
#endif //LGE_CUST_CAL
// srinivas.mittapalli@lge.com 16Sept2010
};

static struct platform_device msm_spk_idual_mic_endfire_device = {
	.name = "snddev_icodec",
	.id = 14,
	.dev = { .platform_data = &snddev_spk_idual_mic_endfire_data },
};

/////////////////////////////////////////////////////////////////////////////////////////
// Dual Mic TX Broadside : don't care
/////////////////////////////////////////////////////////////////////////////////////////

static struct adie_codec_action_unit ispk_dual_mic_bs_8KHz_osr256_actions[] =
	SPEAKER_MIC1_LEFT_AUX_IN_RIGHT_8000_OSR_256;

static struct adie_codec_hwsetting_entry ispk_dual_mic_bs_settings[] = {
	{
		.freq_plan = 8000,
		.osr = 256,
		.actions = ispk_dual_mic_bs_8KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(ispk_dual_mic_bs_8KHz_osr256_actions),
	}, /* 8KHz profile can be used for 16Khz */
	{
		.freq_plan = 16000,
		.osr = 256,
		.actions = ispk_dual_mic_bs_8KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(ispk_dual_mic_bs_8KHz_osr256_actions),
	}, /* 8KHz profile can be used for 48KHz */
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ispk_dual_mic_bs_8KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(ispk_dual_mic_bs_8KHz_osr256_actions),
	},
};

static struct adie_codec_dev_profile ispk_dual_mic_bs_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = ispk_dual_mic_bs_settings,
	.setting_sz = ARRAY_SIZE(ispk_dual_mic_bs_settings),
};
static struct snddev_icodec_data snddev_spk_idual_mic_broadside_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "speaker_dual_mic_broadside_tx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_SPKR_PHONE_MIC_BROADSIDE,
	.profile = &ispk_dual_mic_bs_profile,
	.channel_mode = 2,
	.default_sample_rate = 48000,
	.pmctl_id = idual_mic_broadside_pmctl_id,
	.pmctl_id_sz = ARRAY_SIZE(idual_mic_broadside_pmctl_id),
	.pamp_on = NULL,
	.pamp_off = NULL,
};

static struct platform_device msm_spk_idual_mic_broadside_device = {
	.name = "snddev_icodec",
	.id = 15,
	.dev = { .platform_data = &snddev_spk_idual_mic_broadside_data },
};

/////////////////////////////////////////////////////////////////////////////////////////
// TTY headset TX
/////////////////////////////////////////////////////////////////////////////////////////

static struct adie_codec_action_unit itty_hs_mono_tx_8KHz_osr256_actions[] =
	TTY_HEADSET_MONO_TX_8000_OSR_256;

static struct adie_codec_hwsetting_entry itty_hs_mono_tx_settings[] = {
	/* 8KHz, 16KHz, 48KHz TTY Tx devices can shared same set of actions */
	{
		.freq_plan = 8000,
		.osr = 256,
		.actions = itty_hs_mono_tx_8KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(itty_hs_mono_tx_8KHz_osr256_actions),
	},
	{
		.freq_plan = 16000,
		.osr = 256,
		.actions = itty_hs_mono_tx_8KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(itty_hs_mono_tx_8KHz_osr256_actions),
	},
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = itty_hs_mono_tx_8KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(itty_hs_mono_tx_8KHz_osr256_actions),
	}
};

static struct adie_codec_dev_profile itty_hs_mono_tx_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = itty_hs_mono_tx_settings,
	.setting_sz = ARRAY_SIZE(itty_hs_mono_tx_settings),
};

static struct snddev_icodec_data snddev_itty_hs_mono_tx_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE | SNDDEV_CAP_TTY),
	.name = "tty_headset_mono_tx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_TTY_HEADSET_MIC,
	.profile = &itty_hs_mono_tx_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.pmctl_id = NULL,
	.pmctl_id_sz = 0,
	.pamp_on = NULL,
	.pamp_off = NULL,
};

static struct platform_device msm_itty_hs_mono_tx_device = {
	.name = "snddev_icodec",
	.id = 16,
	.dev = { .platform_data = &snddev_itty_hs_mono_tx_data },
};

/////////////////////////////////////////////////////////////////////////////////////////
// TTY headset RX
/////////////////////////////////////////////////////////////////////////////////////////
#if 0
static struct adie_codec_action_unit itty_hs_mono_rx_8KHz_osr256_actions[] =
	TTY_HEADSET_MONO_RX_CLASS_D_8000_OSR_256;

static struct adie_codec_action_unit itty_hs_mono_rx_16KHz_osr256_actions[] =
	TTY_HEADSET_MONO_RX_CLASS_D_16000_OSR_256;

static struct adie_codec_action_unit itty_hs_mono_rx_48KHz_osr256_actions[] =
	TTY_HEADSET_MONO_RX_CLASS_D_48000_OSR_256;
#else
#if defined (LGE_HW_MS910_REV5)
static struct adie_codec_action_unit itty_hs_mono_rx_8KHz_osr256_actions[] =
    TTY_HEADSET_STEREO_RX_CAPLESS_48000_OSR_256;
static struct adie_codec_action_unit itty_hs_mono_rx_16KHz_osr256_actions[] =
    TTY_HEADSET_STEREO_RX_CAPLESS_48000_OSR_256;
static struct adie_codec_action_unit itty_hs_mono_rx_48KHz_osr256_actions[] =
    TTY_HEADSET_STEREO_RX_CAPLESS_48000_OSR_256;
#else
static struct adie_codec_action_unit itty_hs_mono_rx_8KHz_osr256_actions[] =
    TTY_HEADSET_STEREO_RX_LEGACY_48000_OSR_256;
static struct adie_codec_action_unit itty_hs_mono_rx_16KHz_osr256_actions[] =
    TTY_HEADSET_STEREO_RX_LEGACY_48000_OSR_256;
static struct adie_codec_action_unit itty_hs_mono_rx_48KHz_osr256_actions[] =
    TTY_HEADSET_STEREO_RX_LEGACY_48000_OSR_256;
#endif
#endif

static struct adie_codec_hwsetting_entry itty_hs_mono_rx_settings[] = {
	{
		.freq_plan = 8000,
		.osr = 256,
		.actions = itty_hs_mono_rx_8KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(itty_hs_mono_rx_8KHz_osr256_actions),
	},
	{
		.freq_plan = 16000,
		.osr = 256,
		.actions = itty_hs_mono_rx_16KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(itty_hs_mono_rx_16KHz_osr256_actions),
	},
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = itty_hs_mono_rx_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(itty_hs_mono_rx_48KHz_osr256_actions),
	}
};

static struct adie_codec_dev_profile itty_hs_mono_rx_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = itty_hs_mono_rx_settings,
	.setting_sz = ARRAY_SIZE(itty_hs_mono_rx_settings),
};

static struct snddev_icodec_data snddev_itty_hs_mono_rx_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE | SNDDEV_CAP_TTY),
	.name = "tty_headset_mono_rx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_TTY_HEADSET_SPKR,
	.profile = &itty_hs_mono_rx_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,

// daniel.kang@lge.com ++
// Baikal ID : 0010221, tty rx needs AMP
#if defined (CONFIG_MACH_LGE_BRYCE) 
    .pamp_on = &lge_snddev_hs_amp_on,
    .pamp_off = &lge_snddev_amp_off,
#else
    .pamp_on = NULL,
    .pamp_off = NULL,
#endif
// daniel.kang@lge.com --
#if defined (CONFIG_MACH_LGE_BRYCE)
#if defined (LGE_HW_MS910_REV5)
	.voltage_on = msm_snddev_hsed_voltage_on,
	.voltage_off = msm_snddev_hsed_voltage_off,
#else
	.voltage_on = NULL,
	.voltage_off = NULL,
#endif
#endif	
	.max_voice_rx_vol[VOC_NB_INDEX] = 0,
	.min_voice_rx_vol[VOC_NB_INDEX] = 0,
	.max_voice_rx_vol[VOC_WB_INDEX] = 0,
	.min_voice_rx_vol[VOC_WB_INDEX] = 0,
};

static struct platform_device msm_itty_hs_mono_rx_device = {
	.name = "snddev_icodec",
	.id = 17,
	.dev = { .platform_data = &snddev_itty_hs_mono_rx_data },
};

/////////////////////////////////////////////////////////////////////////////////////////
// Single Mic Speaker Phone Tx
//    
/////////////////////////////////////////////////////////////////////////////////////////

#if LGE_AUDIO_PATH
#ifndef LGE_CUST_CAL
static 
#endif
struct adie_codec_action_unit ispeaker_tx_8KHz_osr256_actions[] =
	SPEAKER_TX_8000_OSR_256;

#ifndef LGE_CUST_CAL
static 
#endif
struct adie_codec_action_unit ispeaker_tx_48KHz_osr256_actions[] =
	SPEAKER_TX_48000_OSR_256;

static struct adie_codec_hwsetting_entry ispeaker_tx_settings[] = {
	{
		.freq_plan = 8000,
		.osr = 256,
		.actions = ispeaker_tx_8KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(ispeaker_tx_8KHz_osr256_actions),
	},
	{ /* 8KHz profile is good for 16KHz */
		.freq_plan = 16000,
		.osr = 256,
		.actions = ispeaker_tx_8KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(ispeaker_tx_8KHz_osr256_actions),
	},
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ispeaker_tx_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(ispeaker_tx_48KHz_osr256_actions),
	}
};

static struct adie_codec_dev_profile ispeaker_tx_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = ispeaker_tx_settings,
	.setting_sz = ARRAY_SIZE(ispeaker_tx_settings),
};
/*BEGIN: 0012072 kiran.kanneganti@lge.com 2010-12-09*/
/*Mod 0012072:  Mic Bias code change*/

static enum hsed_controller ispk_pmctl_id[] = {PM_HSED_CONTROLLER_0};

static struct snddev_icodec_data snddev_ispeaker_tx_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "speaker_mono_tx",
	.copp_id = 0,
#if LGE_AUDIO_PATH
/* BEGIN:0013618 ehgrace.kim@lge.com 2011-01-13*/ 
/* ADD 0013618: New Tx device for Camcorder*/
	.acdb_id = ACDB_ID_CAMCORDER_TX,	
//	.acdb_id = ACDB_ID_FM_TX,	// ACDB_ID_SPKR_PHONE_MIC,
/* END:0013618 ehgrace.kim@lge.com 2011-01-13*/ 
#else
	.acdb_id = ACDB_ID_SPKR_PHONE_MIC,
#endif
	.profile = &ispeaker_tx_profile,
	.channel_mode = 1,
	.pmctl_id = ispk_pmctl_id,
	.pmctl_id_sz = ARRAY_SIZE(ispk_pmctl_id),
	.default_sample_rate = 48000,
#if LGE_AUDIO_PATH
	.pamp_on = NULL,
	.pamp_off = NULL,
#else
	.pamp_on = msm_snddev_tx_route_config,
	.pamp_off = msm_snddev_tx_route_deconfig,
#endif
#if  LGE_AUDIO_PATH
 	.micbias_current = {&mic_bias0_spkmic,&mic_bias1_spkmic},
#else
	.micbias_current = {1700,1700}, // Need to keep the Tuned vdalues
#endif 
};
/*END: 0012072 kiran.kanneganti@lge.com 2010-12-09*/
static struct platform_device msm_ispeaker_tx_device = {
	.name = "snddev_icodec",
	.id = 18,
	.dev = { .platform_data = &snddev_ispeaker_tx_data },
};
/*Del: 0012072 removed unused code kiran.kanneganti@lge.com 2010-12-09*/
#endif

/////////////////////////////////////////////////////////////////////////////////////////
// Handset RX FFA : don't care
/////////////////////////////////////////////////////////////////////////////////////////

static struct adie_codec_action_unit iearpiece_ffa_48KHz_osr256_actions[] =
	HANDSET_RX_48000_OSR_256_FFA;

static struct adie_codec_hwsetting_entry iearpiece_ffa_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = iearpiece_ffa_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(iearpiece_ffa_48KHz_osr256_actions),
	}
};

static struct adie_codec_dev_profile iearpiece_ffa_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = iearpiece_ffa_settings,
	.setting_sz = ARRAY_SIZE(iearpiece_ffa_settings),
};

static struct snddev_icodec_data snddev_iearpiece_ffa_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "handset_rx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_HANDSET_SPKR,
	.profile = &iearpiece_ffa_profile,
	.channel_mode = 1,
	.pmctl_id = NULL,
	.pmctl_id_sz = 0,
	.default_sample_rate = 48000,
	.pamp_on = NULL,
	.pamp_off = NULL,
	.max_voice_rx_vol[VOC_NB_INDEX] = -700,
	.min_voice_rx_vol[VOC_NB_INDEX] = -2200,
	.max_voice_rx_vol[VOC_WB_INDEX] = -1400,
	.min_voice_rx_vol[VOC_WB_INDEX] = -2900,
};

static struct platform_device msm_iearpiece_ffa_device = {
	.name = "snddev_icodec",
	.id = 19,
	.dev = { .platform_data = &snddev_iearpiece_ffa_data },
};

/////////////////////////////////////////////////////////////////////////////////////////
// Handset TX FFA : don't care
/////////////////////////////////////////////////////////////////////////////////////////

static struct adie_codec_action_unit imic_ffa_8KHz_osr256_actions[] =
	HANDSET_TX_8000_OSR_256_FFA;

static struct adie_codec_action_unit imic_ffa_16KHz_osr256_actions[] =
	HANDSET_TX_16000_OSR_256_FFA;

static struct adie_codec_action_unit imic_ffa_48KHz_osr256_actions[] =
	HANDSET_TX_48000_OSR_256_FFA;

static struct adie_codec_hwsetting_entry imic_ffa_settings[] = {
	{
		.freq_plan = 8000,
		.osr = 256,
		.actions = imic_ffa_8KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(imic_ffa_8KHz_osr256_actions),
	},
	{
		.freq_plan = 16000,
		.osr = 256,
		.actions = imic_ffa_16KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(imic_ffa_16KHz_osr256_actions),
	},
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = imic_ffa_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(imic_ffa_48KHz_osr256_actions),
	}
};

static struct adie_codec_dev_profile imic_ffa_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = imic_ffa_settings,
	.setting_sz = ARRAY_SIZE(imic_ffa_settings),
};

static struct snddev_icodec_data snddev_imic_ffa_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "handset_tx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_HANDSET_MIC,
	.profile = &imic_ffa_profile,
	.channel_mode = 1,
	.pmctl_id = imic_pmctl_id,
	.pmctl_id_sz = ARRAY_SIZE(imic_pmctl_id),
	.default_sample_rate = 48000,
	.pamp_on = NULL,
	.pamp_off = NULL,
};

static struct platform_device msm_imic_ffa_device = {
	.name = "snddev_icodec",
	.id = 20,
	.dev = { .platform_data = &snddev_imic_ffa_data },
};

/////////////////////////////////////////////////////////////////////////////////////////
// Handset and Speaker RX : Notification
/////////////////////////////////////////////////////////////////////////////////////////
#ifndef LGE_CUST_CAL
static 
#endif 
#if defined (LGE_HW_MS910_REV5)
struct adie_codec_action_unit
	ihs_stereo_speaker_stereo_rx_48KHz_osr256_actions[] =
	HEADSET_STEREO_SPEAKER_STEREO_RX_CAPLESS_48000_OSR_256;
#else
struct adie_codec_action_unit
	ihs_stereo_speaker_stereo_rx_48KHz_osr256_actions[] =
	HEADSET_STEREO_SPEAKER_STEREO_RX_LEGACY_48000_OSR_256;
#endif

static struct adie_codec_hwsetting_entry
	ihs_stereo_speaker_stereo_rx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ihs_stereo_speaker_stereo_rx_48KHz_osr256_actions,
		.action_sz =
		ARRAY_SIZE(ihs_stereo_speaker_stereo_rx_48KHz_osr256_actions),
	}
};

static struct adie_codec_dev_profile ihs_stereo_speaker_stereo_rx_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = ihs_stereo_speaker_stereo_rx_settings,
	.setting_sz = ARRAY_SIZE(ihs_stereo_speaker_stereo_rx_settings),
};
/* baikal ID 0009676::srinivas.mittapalli@lge.com 11Oct2010 */
#ifndef LGE_CUST_CAL
static 
#endif
 struct snddev_icodec_data snddev_ihs_stereo_speaker_stereo_rx_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "headset_stereo_speaker_stereo_rx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_HEADSET_STEREO_PLUS_SPKR_STEREO_RX,
	.profile = &ihs_stereo_speaker_stereo_rx_profile,
	.channel_mode = 2,
	.default_sample_rate = 48000,
//for sonification stream need to turn on both headset & speaker. 
// ++ kiran.kanneganti	
#if defined (CONFIG_MACH_LGE_BRYCE)
	.pamp_on = &lge_snddev_spk_hs_amp_on,
	.pamp_off = &lge_snddev_amp_off,
#else
	.pamp_on = &msm_snddev_poweramp_on,
	.pamp_off = &msm_snddev_poweramp_off,
#endif
#if defined (CONFIG_MACH_LGE_BRYCE)
#if defined (LGE_HW_MS910_REV5)
	.voltage_on = msm_snddev_hsed_voltage_on,
	.voltage_off = msm_snddev_hsed_voltage_off,
#else
	.voltage_on = NULL,
	.voltage_off = NULL,
#endif
#endif	
// -- kiran.kanneganti
	.max_voice_rx_vol[VOC_NB_INDEX] = -500,
	.min_voice_rx_vol[VOC_NB_INDEX] = -2000,
	.max_voice_rx_vol[VOC_WB_INDEX] = -500,
	.min_voice_rx_vol[VOC_WB_INDEX] = -2000,
};

static struct platform_device msm_ihs_stereo_speaker_stereo_rx_device = {
	.name = "snddev_icodec",
	.id = 21,
	.dev = { .platform_data = &snddev_ihs_stereo_speaker_stereo_rx_data },
};

/////////////////////////////////////////////////////////////////////////////////////////
// HDMI RX
/////////////////////////////////////////////////////////////////////////////////////////

static struct snddev_mi2s_data snddev_mi2s_stereo_rx_data = {
	.capability = SNDDEV_CAP_RX ,
	.name = "hdmi_stereo_rx",
	.copp_id = 3,
	.acdb_id = ACDB_ID_HDMI,
	.channel_mode = 2,
	.sd_lines = MI2S_SD_0,
	.route = msm_snddev_tx_route_config,
	.deroute = msm_snddev_tx_route_deconfig,
	.default_sample_rate = 48000,
};

static struct platform_device msm_snddev_mi2s_stereo_rx_device = {
	.name = "snddev_mi2s",
	.id = 0,
	.dev = { .platform_data = &snddev_mi2s_stereo_rx_data },
};

/////////////////////////////////////////////////////////////////////////////////////////
// FM TX : don't care
/////////////////////////////////////////////////////////////////////////////////////////

static struct snddev_mi2s_data snddev_mi2s_fm_tx_data = {
	.capability = SNDDEV_CAP_TX ,
	.name = "fmradio_stereo_tx",
	.copp_id = 2,
	.acdb_id = ACDB_ID_FM_TX,
	.channel_mode = 2,
	.sd_lines = MI2S_SD_3,
	.route = NULL,
	.deroute = NULL,
	.default_sample_rate = 48000,
};

static struct platform_device  msm_snddev_mi2s_fm_tx_device = {
	.name = "snddev_mi2s",
	.id = 1,
	.dev = { .platform_data = &snddev_mi2s_fm_tx_data},
};

static struct snddev_icodec_data snddev_fluid_imic_tx_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "handset_tx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_SPKR_PHONE_MIC,
	.profile = &ispeaker_tx_profile,
	.channel_mode = 1,
	.pmctl_id = ispk_pmctl_id,
	.pmctl_id_sz = ARRAY_SIZE(ispk_pmctl_id),
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_tx_route_config,
	.pamp_off = msm_snddev_tx_route_deconfig,
};

static struct platform_device msm_fluid_imic_tx_device = {
	.name = "snddev_icodec",
	.id = 22,
	.dev = { .platform_data = &snddev_fluid_imic_tx_data },
};

/////////////////////////////////////////////////////////////////////////////////////////
// fluid RX : don't care
/////////////////////////////////////////////////////////////////////////////////////////

static struct snddev_icodec_data snddev_fluid_iearpiece_rx_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "handset_rx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_SPKR_PHONE_STEREO,
	.profile = &ispeaker_rx_profile,
	.channel_mode = 2,
	.pmctl_id = NULL,
	.pmctl_id_sz = 0,
	.default_sample_rate = 48000,
	.pamp_on = &msm_snddev_poweramp_on,
	.pamp_off = &msm_snddev_poweramp_off,
	.max_voice_rx_vol[VOC_NB_INDEX] = -500,
	.min_voice_rx_vol[VOC_NB_INDEX] = -1000,
	.max_voice_rx_vol[VOC_WB_INDEX] = -500,
	.min_voice_rx_vol[VOC_WB_INDEX] = -1000,
};

static struct platform_device msm_fluid_iearpeice_rx_device = {
	.name = "snddev_icodec",
	.id = 23,
	.dev = { .platform_data = &snddev_fluid_iearpiece_rx_data },
};

/////////////////////////////////////////////////////////////////////////////////////////
// Fluid dual mic
/////////////////////////////////////////////////////////////////////////////////////////

static struct adie_codec_action_unit fluid_idual_mic_ef_8KHz_osr256_actions[] =
	MIC1_LEFT_AUX_IN_RIGHT_8000_OSR_256;

static struct adie_codec_hwsetting_entry fluid_idual_mic_endfire_settings[] = {
	{
		.freq_plan = 8000,
		.osr = 256,
		.actions = fluid_idual_mic_ef_8KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(fluid_idual_mic_ef_8KHz_osr256_actions),
	}, /* 8KHz profile can be used for 16KHz */
	{
		.freq_plan = 16000,
		.osr = 256,
		.actions = fluid_idual_mic_ef_8KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(fluid_idual_mic_ef_8KHz_osr256_actions),
	}, /* 8KHz profile can also be used for 48KHz */
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = fluid_idual_mic_ef_8KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(fluid_idual_mic_ef_8KHz_osr256_actions),
	}
};

static struct adie_codec_dev_profile fluid_idual_mic_endfire_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = fluid_idual_mic_endfire_settings,
	.setting_sz = ARRAY_SIZE(fluid_idual_mic_endfire_settings),
};

static enum hsed_controller fluid_idual_mic_endfire_pmctl_id[] = {
	PM_HSED_CONTROLLER_0, PM_HSED_CONTROLLER_2
};

static struct snddev_icodec_data snddev_fluid_idual_mic_endfire_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "handset_dual_mic_endfire_tx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_SPKR_PHONE_MIC_ENDFIRE,
	.profile = &fluid_idual_mic_endfire_profile,
	.channel_mode = 2,
	.default_sample_rate = 48000,
	.pmctl_id = fluid_idual_mic_endfire_pmctl_id,
	.pmctl_id_sz = ARRAY_SIZE(fluid_idual_mic_endfire_pmctl_id),
	.pamp_on = msm_snddev_tx_route_config,
	.pamp_off = msm_snddev_tx_route_deconfig,
};

static struct platform_device msm_fluid_idual_mic_endfire_device = {
	.name = "snddev_icodec",
	.id = 24,
	.dev = { .platform_data = &snddev_fluid_idual_mic_endfire_data },
};

/////////////////////////////////////////////////////////////////////////////////////////
// Fluid dual mic speaker
/////////////////////////////////////////////////////////////////////////////////////////

static struct snddev_icodec_data snddev_fluid_spk_idual_mic_endfire_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "speaker_dual_mic_endfire_tx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_SPKR_PHONE_MIC_ENDFIRE,
	.profile = &fluid_idual_mic_endfire_profile,
	.channel_mode = 2,
	.default_sample_rate = 48000,
	.pmctl_id = fluid_idual_mic_endfire_pmctl_id,
	.pmctl_id_sz = ARRAY_SIZE(fluid_idual_mic_endfire_pmctl_id),
	.pamp_on = msm_snddev_tx_route_config,
	.pamp_off = msm_snddev_tx_route_deconfig,
};

static struct platform_device msm_fluid_spk_idual_mic_endfire_device = {
	.name = "snddev_icodec",
	.id = 25,
	.dev = { .platform_data = &snddev_fluid_spk_idual_mic_endfire_data },
};

/////////////////////////////////////////////////////////////////////////////////////////
// BT A2DP TX/RX
/////////////////////////////////////////////////////////////////////////////////////////

static struct snddev_virtual_data snddev_a2dp_tx_data = {
	.capability = SNDDEV_CAP_TX,
	.name = "a2dp_tx",
	.copp_id = 5,
	.acdb_id = PSEUDO_ACDB_ID,
};

static struct snddev_virtual_data snddev_a2dp_rx_data = {
	.capability = SNDDEV_CAP_RX,
	.name = "a2dp_rx",
	.copp_id = 2,
	.acdb_id = PSEUDO_ACDB_ID,
};

static struct platform_device msm_a2dp_rx_device = {
	.name = "snddev_virtual",
	.id = 0,
	.dev = { .platform_data = &snddev_a2dp_rx_data },
};

static struct platform_device msm_a2dp_tx_device = {
	.name = "snddev_virtual",
	.id = 1,
	.dev = { .platform_data = &snddev_a2dp_tx_data },
};

/////////////////////////////////////////////////////////////////////////////////////////
// Virtual : not used : don't care
/////////////////////////////////////////////////////////////////////////////////////////

static struct snddev_virtual_data snddev_uplink_rx_data = {
	.capability = SNDDEV_CAP_RX,
	.name = "uplink_rx",
	.copp_id = 5,
	.acdb_id = PSEUDO_ACDB_ID,
};

static struct platform_device msm_uplink_rx_device = {
	.name = "snddev_virtual",
	.id = 2,
	.dev = { .platform_data = &snddev_uplink_rx_data },
};

/* BEGIN:0010882        ehgrace.kim@lge.com     2010.11.15*/
/* MOD: add the call mode */
#if LGE_AUDIO_PATH
//speaker_tx supports dual Mic & Miain Mic as primary. Kiran kanneganti@lge.com 09 SEP 2010 
#ifndef LGE_CUST_CAL
static 
#endif
struct adie_codec_action_unit ispeaker_phone_tx_8KHz_osr256_actions[] =
	SPEAKER_PHONE_TX_8000_OSR_256;

//TODO: Need to be verified from QCOM.
/*QCOM Dual MIC using 8K config for 16 & 48 K.*/
static struct adie_codec_hwsetting_entry ispeaker_phone_tx_settings[] = {
	{
		.freq_plan = 8000,
		.osr = 256,
		.actions = ispeaker_phone_tx_8KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(ispeaker_phone_tx_8KHz_osr256_actions),
	},
	{ /* 8KHz profile is good for 16KHz */
		.freq_plan = 16000,
		.osr = 256,
		.actions = ispeaker_phone_tx_8KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(ispeaker_phone_tx_8KHz_osr256_actions),
	},/* 8KHz profile can be used for 48Khz */
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ispeaker_phone_tx_8KHz_osr256_actions,//ispeaker_tx_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(ispeaker_phone_tx_8KHz_osr256_actions),//ARRAY_SIZE(ispeaker_tx_48KHz_osr256_actions),
	}
};

static struct adie_codec_dev_profile ispeaker_phone_tx_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = ispeaker_phone_tx_settings,
	.setting_sz = ARRAY_SIZE(ispeaker_phone_tx_settings),
};
/*BEGIN: 0012072 kiran.kanneganti@lge.com 2010-12-09*/
/*Mod 0012072:  Mic Bias code change*/
static enum hsed_controller ispk_phone_pmctl_id[] = {PM_HSED_CONTROLLER_0};

static struct snddev_icodec_data snddev_ispeaker_phone_tx_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "speaker_phone_tx",
	.copp_id = 0,
// daniel.kang@lge.com ++ 3rd Nov
// 0010463: Enable Fluence in handset, speaker TX
	.acdb_id = ACDB_ID_SPKR_PHONE_MIC,
// daniel.kang@lge.com -- 3rd Nov
	.profile = &ispeaker_phone_tx_profile,
	.channel_mode = 1,		// single mic
	.pmctl_id = ispk_phone_pmctl_id,
	.pmctl_id_sz = ARRAY_SIZE(ispk_phone_pmctl_id),
	.default_sample_rate = 48000,
/* CONFIG_MACH_LGE_BRYCE	ehgrace.kim 10.05.03
	add LGE amp*/
#if defined(LG_HW_REV4) || defined(LG_HW_REV5)
	.pamp_on = &lge_snddev_MSM_mic_route_config,
#else	
	.pamp_on = NULL,
#endif	
    .pamp_off = NULL,
// srinivas.mittapalli@lge.com 16Sept2010 baikal 0009676
#ifdef LGE_CUST_CAL
 	.micbias_current = {&mic_bias0_spkmic2,&mic_bias1_spkmic2},
#else
	.micbias_current = {1700,1700}, // Need to keep the Tuned values
#endif //LGE_CUST_CAL
// srinivas.mittapalli@lge.com 16Sept2010	
/* daniel.kang@lge.com -- */
};
/*END: 0012072 kiran.kanneganti@lge.com 2010-12-09*/
static struct platform_device msm_ispeaker_phone_tx_device = {
	.name = "snddev_icodec",
	.id = 26,
	.dev = { .platform_data = &snddev_ispeaker_phone_tx_data },
};
#endif

/////////////////////////////////////////////////////////////////////////////////////////
// Speaker RX (voice)
/////////////////////////////////////////////////////////////////////////////////////////

/* CONFIG_MACH_LGE_BRYCE	ehgrace.kim 10.05.03
	changed the speaker rx setting
*/
#if defined (CONFIG_MACH_LGE_BRYCE)
#ifndef LGE_CUST_CAL
static 
#endif
struct adie_codec_action_unit ispeaker_phone_rx_48KHz_osr256_actions[] =
   SPEAKER_PHONE_RX_48000_OSR_256;
#else
static struct adie_codec_action_unit ispeaker_phone_rx_48KHz_osr256_actions[] =
   SPEAKER_STEREO_RX_48000_OSR_256;
#endif

static struct adie_codec_hwsetting_entry ispeaker_phone_rx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ispeaker_phone_rx_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(ispeaker_phone_rx_48KHz_osr256_actions),
	}
};

static struct adie_codec_dev_profile ispeaker_phone_rx_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = ispeaker_phone_rx_settings,
	.setting_sz = ARRAY_SIZE(ispeaker_phone_rx_settings),
};
//Bryce Speaker is Mono. kiran.kanneganti@lge.com
/* baikal ID 0009676::srinivas.mittapalli@lge.com 11Oct2010 */
#ifndef LGE_CUST_CAL
static 
#endif
 struct snddev_icodec_data snddev_ispeaker_phone_rx_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "speaker_phone_rx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_SPKR_PHONE_MONO,
	.profile = &ispeaker_phone_rx_profile,
	.channel_mode = 1,
	.pmctl_id = NULL,
	.pmctl_id_sz = 0,
	.default_sample_rate = 48000,
/* CONFIG_MACH_LGE_BRYCE	ehgrace.kim 10.05.03
	add LGE amp
*/
/* BEGIN:0010882        ehgrace.kim@lge.com     2010.11.15*/
/* MOD: add add the subsystem control for call mode */
#if defined (CONFIG_MACH_LGE_BRYCE)
	.pamp_on = &lge_snddev_spk_phone_amp_on,
	.pamp_off = &lge_snddev_amp_off,
#else
	.pamp_on = &msm_snddev_poweramp_on,
	.pamp_off = &msm_snddev_poweramp_off,
#endif
/* END:0010882        ehgrace.kim@lge.com     2010.11.15*/
#if 0//LGE_AUDIO_PATH
        .max_voice_rx_vol[VOC_NB_INDEX] = &spk_stereo_rx[VOC_NB_INDEX].max_level,
        .min_voice_rx_vol[VOC_NB_INDEX] = &spk_stereo_rx[VOC_NB_INDEX].min_level,
        .max_voice_rx_vol[VOC_WB_INDEX] = &spk_stereo_rx[VOC_WB_INDEX].max_level,
        .min_voice_rx_vol[VOC_WB_INDEX] = &spk_stereo_rx[VOC_WB_INDEX].min_level,
#else
	.max_voice_rx_vol[VOC_NB_INDEX] = 1000,
	.min_voice_rx_vol[VOC_NB_INDEX] = -500,
	.max_voice_rx_vol[VOC_WB_INDEX] = 1000,
	.min_voice_rx_vol[VOC_WB_INDEX] = -500,
#endif
};

static struct platform_device msm_ispeaker_phone_rx_device = {
	.name = "snddev_icodec",
	.id = 27,
	.dev = { .platform_data = &snddev_ispeaker_phone_rx_data },

};


/////////////////////////////////////////////////////////////////////////////////////////
// Headset mono Phone TX : (voice)
/////////////////////////////////////////////////////////////////////////////////////////
/*BEGIN: 0012454 kiran.kanneganti@lge.com 2010-12-15*/
/*Mod 0012454: Changed headset mode actions for call. Call use 8K actions*/
#ifndef LGE_CUST_CAL
static 
#endif
struct adie_codec_action_unit ihs_phone_tx_8KHz_osr256_actions[] =
	HEADSET_PHONE_TX_8000_OSR_256;

#ifndef LGE_CUST_CAL
static 
#endif
struct adie_codec_action_unit ihs_phone_tx_16KHz_osr256_actions[] =
	HEADSET_PHONE_TX_16000_OSR_256;
/*END: 0012454 kiran.kanneganti@lge.com 2010-12-15*/	
#ifndef LGE_CUST_CAL
static 
#endif
struct adie_codec_action_unit ihs_phone_tx_48KHz_osr256_actions[] =
	HEADSET_PHONE_TX_48000_OSR_256;

/*BEGIN: 0016769 kiran.kanneganti@lge.com 2011-02-22*/
/*MOD 0016769:  16k & 8K defined to same. Use same actions*/
static struct adie_codec_hwsetting_entry ihs_phone_tx_settings[] = {
	{
		.freq_plan = 8000,
		.osr = 256,
		.actions = ihs_phone_tx_8KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(ihs_phone_tx_8KHz_osr256_actions),
	},
	{
		.freq_plan = 16000,
		.osr = 256,
/*		
		.actions = ihs_phone_tx_16KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(ihs_phone_tx_16KHz_osr256_actions),*/
		.actions = ihs_phone_tx_8KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(ihs_phone_tx_8KHz_osr256_actions),		
	},
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ihs_phone_tx_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(ihs_phone_tx_48KHz_osr256_actions),
	}
};
/* END: 0016769 kiran.kanneganti@lge.com 2011-02-22 */

static struct adie_codec_dev_profile ihs_phone_tx_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = ihs_phone_tx_settings,
	.setting_sz = ARRAY_SIZE(ihs_phone_tx_settings),
};

static struct snddev_icodec_data snddev_ihs_phone_tx_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "headset_phone_tx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_HEADSET_MIC,
	.profile = &ihs_phone_tx_profile,
	.channel_mode = 1,
	.pmctl_id = NULL,
	.pmctl_id_sz = 0,
	.default_sample_rate = 48000,
/*BEGIN: 0012072 kiran.kanneganti@lge.com 2010-12-09*/
/*Mod 0012072:  Mic routing code change*/	
#if defined (CONFIG_MACH_LGE_BRYCE)
	.pamp_on = NULL,
	.pamp_off = NULL,
#else
	.pamp_on = msm_snddev_tx_route_config,
	.pamp_off = msm_snddev_tx_route_deconfig,
#endif
/*END: 0012072 kiran.kanneganti@lge.com 2010-12-09*/
};

static struct platform_device msm_ihs_phone_tx_device = {
	.name = "snddev_icodec",
	.id = 28,
	.dev = { .platform_data = &snddev_ihs_phone_tx_data },
};

/////////////////////////////////////////////////////////////////////////////////////////
// Handset Record TX : (voice recorder)
/////////////////////////////////////////////////////////////////////////////////////////
struct adie_codec_action_unit rec_imic_8KHz_osr256_actions[] =
	HANDSET_REC_TX_8000_OSR_256;

static struct adie_codec_action_unit rec_imic_16KHz_osr256_actions[] =
	HANDSET_REC_TX_16000_OSR_256;
/* BEGIN:0012384 ehgrace.kim@lge.com     2010.12.14*/
/* MOD: [Audio]change tx gain for recording mode & delete the dummy src*/
#if 0
static 
#endif
struct adie_codec_action_unit rec_imic_48KHz_osr256_actions[] =
	HANDSET_REC_TX_48000_OSR_256;
/* END:0012384 ehgrace.kim@lge.com     2010.12.14*/

static struct adie_codec_hwsetting_entry rec_imic_settings[] = {
	{
		.freq_plan = 8000,
		.osr = 256,
		.actions = rec_imic_8KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(rec_imic_8KHz_osr256_actions),
	},
	{
		.freq_plan = 16000,
		.osr = 256,
		.actions = rec_imic_16KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(rec_imic_16KHz_osr256_actions),
	},
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = rec_imic_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(rec_imic_48KHz_osr256_actions),
	}
};

static struct adie_codec_dev_profile rec_imic_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = rec_imic_settings,
	.setting_sz = ARRAY_SIZE(rec_imic_settings),
};

static enum hsed_controller imic_rec_pmctl_id[] = {PM_HSED_CONTROLLER_0};

static struct snddev_icodec_data snddev_rec_imic_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "handset_record_tx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_I2S_TX,
	.profile = &rec_imic_profile,
	.channel_mode = 1,
	.pmctl_id = imic_rec_pmctl_id,
	.pmctl_id_sz = ARRAY_SIZE(imic_rec_pmctl_id),
	.default_sample_rate = 48000,
/*BEGIN: 0012072 kiran.kanneganti@lge.com 2010-12-09*/
/*Mod 0012072:  Mic routing code change*/	
#if defined(LG_HW_REV4) || defined(LG_HW_REV5)
	.pamp_on = &lge_snddev_MSM_mic_route_config,
#else	
	.pamp_on = NULL,
#endif	
/*END: 0012072 kiran.kanneganti@lge.com 2010-12-09*/
	.pamp_off = NULL,
#ifdef LGE_CUST_CAL
	.pmctl_id = imic_rec_pmctl_id,
	.pmctl_id_sz = ARRAY_SIZE(imic_rec_pmctl_id),
 	.micbias_current = {&mic_bias0_handset_rec_mic,&mic_bias1_handset_rec_mic},
#endif
};


static struct platform_device msm_rec_imic_tx_device = {
	.name = "snddev_icodec",
	.id = 29,//?
	.dev = { .platform_data = &snddev_rec_imic_data },
};
//#endif

/* END:0010882        ehgrace.kim@lge.com     2010.11.15*/
/* BEGIN:0013348 kiran.kanneganti@lge.com 2010-12-30*/ 
/* ADD 0013348: New Tx device for Voice recognition*/
/////////////////////////////////////////////////////////////////////////////////////////
// Voice recognition Tx: (Voice Search)
/////////////////////////////////////////////////////////////////////////////////////////
struct adie_codec_action_unit voice_recognition_imic_8KHz_osr256_actions[] =
	VOICE_RECOGNITION_TX_8000_OSR_256;

struct adie_codec_action_unit voice_recognition_imic_16KHz_osr256_actions[] =
	VOICE_RECOGNITION_TX_16000_OSR_256;

struct adie_codec_action_unit voice_recognition_imic_48KHz_osr256_actions[] =
	VOICE_RECOGNITION_TX_48000_OSR_256;

static struct adie_codec_hwsetting_entry voice_recognition_imic_settings[] = {
	{
		.freq_plan = 8000,
		.osr = 256,
		.actions = voice_recognition_imic_8KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(voice_recognition_imic_8KHz_osr256_actions),
	},
	{
		.freq_plan = 16000,
		.osr = 256,
		.actions = voice_recognition_imic_16KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(voice_recognition_imic_16KHz_osr256_actions),
	},
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = voice_recognition_imic_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(voice_recognition_imic_48KHz_osr256_actions),
	}
};

static struct adie_codec_dev_profile voice_recognition_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = voice_recognition_imic_settings,
	.setting_sz = ARRAY_SIZE(voice_recognition_imic_settings),
};

static enum hsed_controller voice_recognition_pmctl_id[] = {PM_HSED_CONTROLLER_0};

static struct snddev_icodec_data snddev_voice_recognition_data = {
	.capability = (SNDDEV_CAP_TX),
	.name = "voice_recognition_tx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_VOICE_RECOGNITION_TX,
	.profile = &voice_recognition_profile,
	.channel_mode = 1,
	.pmctl_id = voice_recognition_pmctl_id,
	.pmctl_id_sz = ARRAY_SIZE(voice_recognition_pmctl_id),
	.default_sample_rate = 48000,
#if defined(LG_HW_REV4) || defined(LG_HW_REV5)
	.pamp_on = &lge_snddev_MSM_mic_route_config,
#else	
	.pamp_on = NULL,
#endif	
	.pamp_off = NULL,
#ifdef LGE_CUST_CAL
 	.micbias_current = {&mic_bias0_handset_rec_mic,&mic_bias1_handset_rec_mic},
#endif
};

static struct platform_device msm_voice_recognition_tx_device = {
	.name = "snddev_icodec",
	.id = 31,
	.dev = { .platform_data = &snddev_voice_recognition_data },
};
/* END:0013348 kiran.kanneganti@lge.com 2010-12-30 */
/////////////////////////////////////////////////////////////////////////////////////////
/* Configurations list */
/////////////////////////////////////////////////////////////////////////////////////////

static struct platform_device *snd_devices_ffa[] __initdata = {
	&msm_iearpiece_ffa_device,
	&msm_imic_ffa_device,
	&msm_ifmradio_handset_device,
	&msm_ihs_ffa_stereo_rx_device,
	&msm_ihs_ffa_mono_rx_device,
	&msm_ihs_mono_tx_device,
	&msm_bt_sco_earpiece_device,
	&msm_bt_sco_mic_device,
	&msm_ispeaker_rx_device,
	&msm_ifmradio_speaker_device,
	&msm_ifmradio_ffa_headset_device,
	&msm_idual_mic_endfire_device,
	&msm_idual_mic_broadside_device,
	&msm_spk_idual_mic_endfire_device,
	&msm_spk_idual_mic_broadside_device,
	&msm_itty_hs_mono_tx_device,
	&msm_itty_hs_mono_rx_device,
	&msm_ispeaker_tx_device,
	&msm_ihs_stereo_speaker_stereo_rx_device,
	&msm_a2dp_rx_device,
	&msm_a2dp_tx_device,
	&msm_snddev_mi2s_stereo_rx_device,
	&msm_snddev_mi2s_fm_tx_device,
	&msm_uplink_rx_device,
	&msm_real_stereo_tx_device,
};

// Bryce uses this config
//jy0127.jang@lge.com for loopback
//static struct platform_device *snd_devices_surf[] __initdata = {
static struct platform_device *snd_devices_surf[]  = {
	&msm_iearpiece_device,
	&msm_imic_device,
	&msm_ihs_stereo_rx_device,
	&msm_ihs_mono_rx_device,
	&msm_ihs_mono_tx_device,
	&msm_bt_sco_earpiece_device,
	&msm_bt_sco_mic_device,
	&msm_ifmradio_handset_device,
	&msm_ispeaker_rx_device,
	&msm_ifmradio_speaker_device,
	&msm_ifmradio_headset_device,
	&msm_itty_hs_mono_tx_device,
	&msm_itty_hs_mono_rx_device,
	&msm_ispeaker_tx_device,
	&msm_ihs_stereo_speaker_stereo_rx_device,
	&msm_a2dp_rx_device,
	&msm_a2dp_tx_device,
	&msm_snddev_mi2s_stereo_rx_device,
	&msm_snddev_mi2s_fm_tx_device,
	&msm_uplink_rx_device,
	&msm_idual_mic_endfire_device,
	&msm_spk_idual_mic_endfire_device,
#if LGE_AUDIO_PATH
/* BEGIN:0010882        ehgrace.kim@lge.com     2010.11.15*/
/* MOD: add the call mode */
	&msm_ispeaker_phone_rx_device,
	&msm_ispeaker_phone_tx_device,
	&msm_ihs_phone_tx_device,
	&msm_rec_imic_tx_device,
/* BEGIN:0013064 kiran.kanneganti@lge.com 2010-12-23*/ 
/* ADD 0013064: New Rx device for skype*/	
	&msm_skype_speaker_rx_device,
/* END:0013064 kiran.kanneganti@lge.com 2010-12-23 */	
/* END:0010882        ehgrace.kim@lge.com     2010.11.15*/
/* BEGIN:0013348 kiran.kanneganti@lge.com 2010-12-30*/ 
/* ADD 0013348: New Tx device for Voice recognition*/
	&msm_voice_recognition_tx_device,
/* END:0013348 kiran.kanneganti@lge.com 2010-12-30 */	
#endif
};



static struct platform_device *snd_devices_fluid[] __initdata = {
	&msm_ihs_stereo_rx_device,
	&msm_ihs_mono_rx_device,
	&msm_ihs_mono_tx_device,
	&msm_ispeaker_rx_device,
	&msm_ispeaker_tx_device,
	&msm_fluid_imic_tx_device,
	&msm_fluid_iearpeice_rx_device,
	&msm_fluid_idual_mic_endfire_device,
	&msm_fluid_spk_idual_mic_endfire_device,
	&msm_a2dp_rx_device,
	&msm_a2dp_tx_device,
	&msm_snddev_mi2s_stereo_rx_device,
	&msm_uplink_rx_device,
	&msm_ifmradio_speaker_device,
	&msm_ifmradio_headset_device,
};

#ifdef CONFIG_DEBUG_FS
static void snddev_hsed_config_modify_setting(int type)
{
	struct platform_device *device;
	struct snddev_icodec_data *icodec_data;

	device = &msm_ihs_ffa_stereo_rx_device;
	icodec_data = (struct snddev_icodec_data *)device->dev.platform_data;

	if (icodec_data) {
		if (type == 1) {
			icodec_data->voltage_on = NULL;
			icodec_data->voltage_off = NULL;
			icodec_data->profile->settings =
				ihs_ffa_stereo_rx_class_d_legacy_settings;
			icodec_data->profile->setting_sz =
			ARRAY_SIZE(ihs_ffa_stereo_rx_class_d_legacy_settings);
		} else if (type == 2) {
			icodec_data->voltage_on = NULL;
			icodec_data->voltage_off = NULL;
			icodec_data->profile->settings =
				ihs_ffa_stereo_rx_class_ab_legacy_settings;
			icodec_data->profile->setting_sz =
			ARRAY_SIZE(ihs_ffa_stereo_rx_class_ab_legacy_settings);
		}
	}
}

static void snddev_hsed_config_restore_setting(void)
{
	struct platform_device *device;
	struct snddev_icodec_data *icodec_data;

	device = &msm_ihs_ffa_stereo_rx_device;
	icodec_data = (struct snddev_icodec_data *)device->dev.platform_data;

	if (icodec_data) {
		icodec_data->voltage_on = msm_snddev_hsed_voltage_on;
		icodec_data->voltage_off = msm_snddev_hsed_voltage_off;
		icodec_data->profile->settings = ihs_ffa_stereo_rx_settings;
		icodec_data->profile->setting_sz =
			ARRAY_SIZE(ihs_ffa_stereo_rx_settings);
	}
}

static ssize_t snddev_hsed_config_debug_write(struct file *filp,
	const char __user *ubuf, size_t cnt, loff_t *ppos)
{
	char *lb_str = filp->private_data;
	char cmd;

	if (get_user(cmd, ubuf))
		return -EFAULT;

	if (!strcmp(lb_str, "msm_hsed_config")) {
		switch (cmd) {
		case '0':
			snddev_hsed_config_restore_setting();
			break;

		case '1':
			snddev_hsed_config_modify_setting(1);
			break;

		case '2':
			snddev_hsed_config_modify_setting(2);
			break;

		default:
			break;
		}
	}
	return cnt;
}

static int snddev_hsed_config_debug_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;
	return 0;
}

static const struct file_operations snddev_hsed_config_debug_fops = {
	.open = snddev_hsed_config_debug_open,
	.write = snddev_hsed_config_debug_write
};
#endif
//Baikal Id:0009963. Rx voice volume change support from testmode. kiran.kanneganti@lge.com
/*******************************************************************************
*	Function Name :  lge_get_audio_device_data_address
*	Args : inetrger variable address to get number of devices
*	dependencies : None. 
*	Note: check Audio_misc_ctrl.c regarding the usage.
********************************************************************************/
//Use to manipulate or read the device data.
//++kiran.kanneganti@lge.com
struct platform_device** lge_get_audio_device_data_address( int* Num_of_Dev)
{
#if 1
	if (machine_is_lge_bryce())
	{
		*Num_of_Dev = ARRAY_SIZE(snd_devices_surf);
		return snd_devices_surf;
	}
	else //jy0127.jang@lge.com temp
	{
		*Num_of_Dev = ARRAY_SIZE(snd_devices_surf);
		return snd_devices_surf;
//		return NULL;
	}
#endif	
}
EXPORT_SYMBOL(lge_get_audio_device_data_address);
//--kiran.kanneganti@lge.com
/**********************************************************************/

void __ref msm_snddev_init(void)
{
#if 0
	if (machine_is_msm7x30_ffa() || machine_is_msm8x55_ffa() ||
		machine_is_msm8x55_svlte_ffa()) {
		platform_add_devices(snd_devices_ffa,
		ARRAY_SIZE(snd_devices_ffa));
#ifdef CONFIG_DEBUG_FS
		debugfs_hsed_config = debugfs_create_file("msm_hsed_config",
					S_IFREG | S_IRUGO, NULL,
		(void *) "msm_hsed_config", &snddev_hsed_config_debug_fops);
#endif
	} else if (machine_is_msm7x30_surf() || machine_is_msm8x55_surf() ||
		machine_is_msm8x55_svlte_surf()){
		platform_add_devices(snd_devices_surf,
		ARRAY_SIZE(snd_devices_surf));
		pr_err("%s: machine_is_lge_bryce\n", __func__);
		}
/* CONFIG_MACH_LGE_BRYCE	ehgrace.kim 10.05.03
	add LGE amp
*/
#if 1
#ifdef CONFIG_MACH_LGE_BRYCE
	else if (machine_is_lge_bryce()){
		platform_add_devices(snd_devices_surf,
		ARRAY_SIZE(snd_devices_surf));
		pr_err("%s: machine_is_lge_bryce\n", __func__);

		}
#endif
#endif
	else if (machine_is_msm7x30_fluid())
		platform_add_devices(snd_devices_fluid,
		ARRAY_SIZE(snd_devices_fluid));
	else
		pr_err("%s: Unknown machine type\n", __func__);
#endif		
	
	platform_add_devices(snd_devices_surf,
	ARRAY_SIZE(snd_devices_surf));
	pr_err("%s: machine_is_lge_bryce\n", __func__);
}
