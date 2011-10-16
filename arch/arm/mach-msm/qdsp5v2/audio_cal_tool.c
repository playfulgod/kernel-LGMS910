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
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/kthread.h>
#include <linux/wait.h>
#include <linux/mutex.h>
#include <linux/io.h>
#include <mach/qdsp5v2/snddev_ecodec.h>
#include <mach/qdsp5v2/snddev_icodec.h>
#include <linux/android_pmem.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/dma-mapping.h>
#include <linux/msm_audio.h>
#include <mach/dal.h>
#include <mach/qdsp5v2/audio_dev_ctl.h>
#include <mach/qdsp5v2/audpp.h>
#include <mach/qdsp5v2/audpreproc.h>
#include <mach/qdsp5v2/qdsp5audppcmdi.h>
#include <mach/qdsp5v2/qdsp5audpreproccmdi.h>
#include <mach/qdsp5v2/qdsp5audpreprocmsg.h>
#include <mach/qdsp5v2/qdsp5audppmsg.h>
#include <mach/qdsp5v2/audio_acdbi.h>
#include <mach/qdsp5v2/acdb_commands.h>
#include <mach/qdsp5v2/audio_acdb_def.h>
#include <mach/debug_mm.h>
#include <asm/ioctls.h>
/* BEGIN:0010882        ehgrace.kim@lge.com     2010.11.15*/
/* MOD: add add the subsystem control for call mode */	
#define LGE_AUDIO_PATH	1
/* END:0010882        ehgrace.kim@lge.com     2010.11.15*/


//#if defined(LG_HW_REV5) || defined(LG_HW_REV6)
//#define LINEIN_AUXPGA 1
//#endif 

// IOCTL parameter
#define SND_IOCTL_MAGIC 's'
#define SND_SET_AUDCAL_PARAM _IOWR(SND_IOCTL_MAGIC, 13, struct msm_snd_set_audcal_param *)

// ADIE config array pack/unpack
#define ADIE_CODEC_PACK_ENTRY(reg, mask, val) ((val)|(mask << 8)|(reg << 16))
#define ADIE_CODEC_UNPACK_ENTRY(packed, reg, mask, val) \
	do { \
		((reg) = ((packed >> 16) & (0xff))); \
		((mask) = ((packed >> 8) & (0xff))); \
		((val) = ((packed) & (0xff))); \
	} while (0);

#if LGE_AUDIO_PATH	
/*BEGIN: 0012454 kiran.kanneganti@lge.com 2010-12-15*/
/*Mod 0012454: calibration is supported for 12 devices*/
/* BEGIN:0010882        ehgrace.kim@lge.com     2010.11.15*/
/* MOD: add add the subsystem control for call mode */
/* BEGIN:0014024  kiran.kanneganti@lge.com     2011.01.13*/
/* MOD:0014024 Calibration support for loop back devices  */
/*jy0127.jang@lge.com MS910_AUDIO_CHANGE 2011-04-07*/
/*added speaker phone loopback*/
#define NUMBER_OF_DEVICES  19
/* END:0014024  kiran.kanneganti@lge.com     2011.01.13*/
/* END:0010882        ehgrace.kim@lge.com     2010.11.15*/
/*END: 0012454 kiran.kanneganti@lge.com 2010-12-15*/
#else
#define NUMBER_OF_DEVICES  10	// number of devices need to configure ADIE
								// actually 11 devices are needed include BT SCO RX
								// BT SCO RX needs setttings for RX vol (WB, NB) min/max
#endif
/* BEGIN:0014024  kiran.kanneganti@lge.com     2011.01.13*/
/* MOD:0014024 Calibration support for loop back devices  */
#define LOOPBACK_MAIN_MIC 0x51
#define LOOPBACK_BACK_MIC 0x52
#define LOOPBACK_HANDSET_RX 0x53
#define LOOPBACK_HEADSET_TX 0x54
#define LOOPBACK_HEADSET_RX 0x55
/* END:0014024  kiran.kanneganti@lge.com     2011.01.13*/
/* device driver file operation ++ */
/*[START] jy0127.jang@lge.com MS910_AUDIO_CHANGE 2011-04-07*/
/*added speaker phone loopback*/
#define LOOPBACK_SPEAKERPHONE_TX 0x56
#define LOOPBACK_SPEAKERPHONE_RX 0x57
/*[END] jy0127.jang@lge.com MS910_AUDIO_CHANGE 2011-04-07*/

struct audio_cal {
	struct mutex lock;

	int opened;
	int enabled;
	int running;
};

static struct audio_cal audio_cal_file;

static int audio_cal_open(struct inode *ip, struct file *fp);
static long audio_cal_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
static int audio_cal_close(struct inode *inode, struct file *file);

static const struct file_operations audio_cal_fops = {
	.owner = THIS_MODULE,
	.open = audio_cal_open,
	.release = audio_cal_close,
	.unlocked_ioctl = audio_cal_ioctl,
	.llseek = no_llseek,
};

struct miscdevice audio_cal_misc = {
	.minor	= MISC_DYNAMIC_MINOR,
	.name	= "msm_audio_cal",
	.fops	= &audio_cal_fops,
};

/* device driver file operation -- */


/* parameter from user space ++ */
typedef int voc_codec_type;
typedef int voccal_property_enum_type;

struct msm_snd_set_audcal_param {
	voc_codec_type voc_codec;	/* voc_codec */
	voccal_property_enum_type voccal_param_type;	/* voccal_param_type */
	int param_val;	/* param_val */
	int get_flag;  //get_flag = 0 for set, get_flag = 1 for get
	uint32_t get_param;
};
/* parameter from user space -- */

/*
struct adie_codec_action_unit {
	u32 type;
	u32 action;
};*/


/////////////////////////////////////////////////
/* EXTERNAL global variables from snddev_marimba  ++ */

// RX volume min/max ++
extern struct snddev_icodec_data snddev_iearpiece_data;
extern struct snddev_icodec_data snddev_ihs_stereo_rx_data;
extern struct snddev_icodec_data snddev_ihs_mono_rx_data ;
extern struct snddev_icodec_data snddev_ispeaker_rx_data;
extern struct snddev_ecodec_data snddev_bt_sco_earpiece_data;
//extern struct snddev_ecodec_data snddev_iearpiece_audience_device;
//extern  struct snddev_ecodec_data snddev_ispeaker_audience_rx_device;
extern struct snddev_icodec_data snddev_ihs_stereo_speaker_stereo_rx_data;
#if LGE_AUDIO_PATH	
/* BEGIN:0010882        ehgrace.kim@lge.com     2010.11.15*/
/* MOD: add add the subsystem control for call mode */	
extern struct snddev_icodec_data snddev_ispeaker_phone_rx_data;
/* END:0010882        ehgrace.kim@lge.com     2010.11.15*/
#endif

struct snddev_icodec_data *ptr_icodec = NULL;
struct snddev_ecodec_data *ptr_ecodec = NULL;
// RX volume min/max --

// Marimba setting ++
extern struct adie_codec_action_unit iearpiece_48KHz_osr256_actions[];
extern struct adie_codec_action_unit imic_8KHz_osr256_actions[];
extern struct adie_codec_action_unit ihs_stereo_rx_48KHz_osr256_actions[];
extern struct adie_codec_action_unit ihs_mono_rx_48KHz_osr256_actions[];
extern struct adie_codec_action_unit ihs_mono_tx_48KHz_osr256_actions[];
extern struct adie_codec_action_unit ispeaker_rx_48KHz_osr256_actions[];
/* BEGIN:0012384 ehgrace.kim@lge.com     2010.12.14*/
/* MOD: [Audio]change tx gain for recording mode & delete the dummy src*/
extern struct adie_codec_action_unit ispeaker_tx_48KHz_osr256_actions[];
/* END:0012384 ehgrace.kim@lge.com     2010.12.14*/
extern struct adie_codec_action_unit idual_mic_endfire_8KHz_osr256_actions[];
extern struct adie_codec_action_unit ispk_dual_mic_ef_8KHz_osr256_actions[];
extern struct adie_codec_action_unit ihs_stereo_speaker_stereo_rx_48KHz_osr256_actions[];
// Marimba setting --

#if LGE_AUDIO_PATH	
/* BEGIN:0010882        ehgrace.kim@lge.com     2010.11.15*/
/* MOD: add add the subsystem control for call mode */	
/*BEGIN: 0012454 kiran.kanneganti@lge.com 2010-12-15*/
/*Mod 0012454: Changed headset mode actions for call. Call use 8K actions*/
extern struct adie_codec_action_unit ihs_phone_tx_8KHz_osr256_actions[];
/*BEGIN: 0016769 kiran.kanneganti@lge.com 2011-02-22*/
/*MOD 0016769:  add 48k actions for tuning in hidden menu*/
extern struct adie_codec_action_unit ihs_phone_tx_48KHz_osr256_actions[];
/* END: 0016769 kiran.kanneganti@lge.com 2011-02-22 */
/*END: 0012454 kiran.kanneganti@lge.com 2010-12-15*/
extern struct adie_codec_action_unit ispeaker_phone_tx_8KHz_osr256_actions[];
extern struct adie_codec_action_unit ispeaker_phone_rx_48KHz_osr256_actions[];
/* BEGIN:0012384 ehgrace.kim@lge.com     2010.12.14*/
/* MOD: [Audio]change tx gain for recording mode & delete the dummy src*/
extern struct adie_codec_action_unit rec_imic_48KHz_osr256_actions[];
/* END:0012384 ehgrace.kim@lge.com     2010.12.14*/
/* END:0010882        	ehgrace.kim@lge.com     2010.11.15*/

#endif
#if LOOPBACK_SUPPORT
/* BEGIN:0014024  kiran.kanneganti@lge.com     2011.01.13*/
/* MOD:0014024 Calibration support for loop back devices  */
extern struct adie_codec_action_unit testmode_handset_tx_8KHz_osr256_actions[];
extern struct adie_codec_action_unit testmode_handset_back_mic_tx_8KHz_osr256_actions[];
extern struct adie_codec_action_unit testmode_handset_rx_8KHz_osr256_actions[];
extern struct adie_codec_action_unit ihs_test_mode_mono_tx_8KHz_osr256_actions[];
extern struct adie_codec_action_unit ihs_test_mode_mono_rx_8KHz_osr256_actions[];
/* END:0014024  kiran.kanneganti@lge.com     2011.01.13*/
/*[START] jy0127.jang@lge.com MS910_AUDIO_CHANGE 2011-04-07*/
/*added speaker phone loopback*/
extern struct adie_codec_action_unit testmode_speakerphone_tx_8KHz_osr256_actions[];
extern struct adie_codec_action_unit testmode_speakerphone_rx_8KHz_osr256_actions[];
/*[END] jy0127.jang@lge.com MS910_AUDIO_CHANGE 2011-04-07*/
#endif
// #if LINEIN_AUXPGA
// extern struct adie_codec_action_unit adie_spk_AuxPga_actions[];
// extern struct adie_codec_action_unit adie_handset_AuxPga_actions[];
// #endif

// Mic Bias setting ++
int mic_bias0_handset_mic = 1700;
int mic_bias1_handset_mic = 1700;
//extern int mic_bias0_aud_hmic ;
//extern int mic_bias1_aud_hmic ;
int mic_bias0_spkmic = 1700;
int mic_bias1_spkmic = 1700;
//extern int mic_bias0_aud_spkmic ;
//extern int mic_bias1_aud_spkmic;
int mic_bias0_handset_endfire = 1700;
int mic_bias1_handset_endfire = 1700;
//extern int mic_bias0_handset_broadside ;
//extern int mic_bias1_handset_broadside ;
int mic_bias0_spk_endfire = 1700;
int mic_bias1_spk_endfire = 1700;
//extern int mic_bias0_spk_broadside ;
//extern int mic_bias1_spk_broadside ;
#if LGE_AUDIO_PATH	
/* BEGIN:0010882        ehgrace.kim@lge.com     2010.11.15*/
/* MOD: add add the subsystem control for call mode */	
int mic_bias0_spkmic2 = 1700;
int mic_bias1_spkmic2 = 1700;
int mic_bias0_handset_rec_mic = 1700;
int mic_bias1_handset_rec_mic = 1700;
/* END:0010882        ehgrace.kim@lge.com     2010.11.15*/
#endif
// Mic Bias setting --

/* EXTERNAL global variables from snddev_marimba  -- */
/////////////////////////////////////////////////
#if LGE_AUDIO_PATH

/* BEGIN:0010882        ehgrace.kim@lge.com     2010.11.15*/
/* MOD: add add the subsystem control for call mode */	
/*
	*0x01 --- Handset Speaker 			--->	iearpiece_48KHz_osr256_actions : HANDSET_RX_48000_OSR_256
	*0x2E --- Handset Mic (front)			--->	imic_8KHz_osr256_actions : MIC1_RIGHT_AUX_IN_LEFT_8000_OSR_256

	*0x05 --- Headset Streo   			--->	ihs_stereo_rx_48KHz_osr256_actions : HEADSET_STEREO_RX_LEGACY_48000_OSR_256
	*0x04 --- Headset Mono   			--->	ihs_mono_rx_48KHz_osr256_actions : HEADSET_MONO_RX_LEGACY_48000_OSR_256
	*0x02 --- Headset Mono Mic    		--->	ihs_mono_tx_48KHz_osr256_actions : HEADSET_MONO_TX_48000_OSR_256
	*0x03 --- Headset Phone Mic      	--->	ihs_phone_tx_48KHz_osr256_actions : HEADSET_PHONE_TX_8000_OSR_256

	*0x08 --- Speaker Rx Stereo		--->	ispeaker_rx_48KHz_osr256_actions : SPEAKER_RX_48000_OSR_256

	*0x07 --- Speaker Phone Rx Mono		--->	ispeaker_phone_rx_48KHz_osr256_actions : SPEAKER_PHONE_RX_48000_OSR_256
	*0x2D --- Speaker Phone Mic (rear)	--->	ispeaker_phone_tx_8KHz_osr256_actions : MIC1_RIGHT_AUX_IN_LEFT_8000_OSR_256

	*0x14 --- stereo [headset+Speaker]	--->	ihs_stereo_speaker_stereo_rx_48KHz_osr256_actions : HEADSET_STEREO_SPEAKER_STEREO_RX_CAPLESS_48000_OSR_256
	*0x30 --- Speaker Tx Single Mic(rear)	--->	ispk_dual_mic_ef_8KHz_osr256_actions : SPEAKER_BACK_TX_8000_OSR_256
	*0x1A --- Handset Tx Single Mic(front))	--->	ispeaker_tx_8KHz_osr256_actions : SPEAKER_TX_8000_OSR_256

	0x0A --- stereo [headset+Speaker]	--->	No QTR configuration

//	0x32 --- Handset Audience Rx				adie_handset_AuxPga_actions	
//	0x31 --- Speaker Audience Rx				adie_spk_AuxPga_actions
//	0x34 --- Handset Audience Tx
//	0x33 --- Speaker Audience Tx
//	0x2C --- Handset Dual Mic BroadSide
//	0x2B --- Speaker Dual Mic BroadSide

*/
#else

/*
	0x05 --- Headset Streo   			--->	ihs_stereo_rx_48KHz_osr256_actions : HEADSET_STEREO_RX_LEGACY_48000_OSR_256
	0x04 --- Headset Mono   			--->	ihs_mono_rx_48KHz_osr256_actions : HEADSET_MONO_RX_LEGACY_48000_OSR_256
	0x03 --- Headset Mic      			--->	ihs_mono_tx_48KHz_osr256_actions : HEADSET_MONO_TX_8000_OSR_256
											ihs_mono_tx_8KHz_osr256_actions : HEADSET_MONO_TX_16000_OSR_256

	0x01 --- Handset Speaker 			--->	iearpiece_48KHz_osr256_actions : HANDSET_RX_48000_OSR_256
	0x02 --- Handset Mic (front)   		--->	imic_8KHz_osr256_actions : MIC1_RIGHT_AUX_IN_LEFT_8000_OSR_256
	0x2E --- Handset Mic (rear)			--->	idual_mic_endfire_8KHz_osr256_actions : MIC1_LEFT_AUX_IN_RIGHT_8000_OSR_256

	0x07 --- Speaker Rx Mono			--->	ispeaker_rx_48KHz_osr256_actions : SPEAKER_RX_48000_OSR_256
	0x06 --- Speaker Mic (front)		--->	ispeaker_tx_8KHz_osr256_actions : MIC1_RIGHT_AUX_IN_LEFT_8000_OSR_256
	0x2D --- Speaker Mic (rear)			--->	ispk_dual_mic_ef_8KHz_osr256_actions : MIC1_LEFT_AUX_IN_RIGHT_8000_OSR_256

	0x14 --- stereo [headset+Speaker]	--->	ihs_stereo_speaker_stereo_rx_48KHz_osr256_actions : HEADSET_STEREO_SPEAKER_STEREO_RX_CAPLESS_48000_OSR_256
	0x0A --- stereo [headset+Speaker]	--->	No QTR configuration

//	0x32 --- Handset Audience Rx				adie_handset_AuxPga_actions	
//	0x31 --- Speaker Audience Rx				adie_spk_AuxPga_actions
//	0x34 --- Handset Audience Tx
//	0x33 --- Speaker Audience Tx
//	0x2C --- Handset Dual Mic BroadSide
//	0x2B --- Speaker Dual Mic BroadSide

*/
#endif
/////////////////////////////////////////////////////
/* INTERNAL global variables saving data for calibration  ++ */

/* QTR ADIE register index ++ */
/* BEGIN:0012384 ehgrace.kim@lge.com     2010.12.14*/
/* MOD: [Audio]change tx gain for recording mode & delete the dummy src*/

// HANDSET RX   0
#define HANDSET_RX_CODECGAIN_LEFT_INDEX			17
#define HANDSET_RX_CODEC_FILTER_INDEX			4

//HANDSET TX   1
#define HANDSET_TX_CODECGAIN_RIGHT_INDEX 		18
#define HANDSET_TX_CODECGAIN_LEFT_INDEX			19
#define HANDSET_TX_CODECGAIN_ST_INDEX			16
#define HANDSET_TX_MICGAIN_RIGHT_INDEX 			12
#define HANDSET_TX_MICGAIN_LEFT_INDEX 			13

//HEADSET RX STEREO   2
#define HEADSET_STEREO_RX_CODECGAIN_LEFT_INDEX			48
#define HEADSET_STEREO_RX_CODECGAIN_RIGHT_INDEX			49
#define HEADSET_STEREO_RX_CODEC_FILTER_INDEX			8
#define HEADSET_STEREO_RX_HEADPHONEGAIN_LEFT_INDEX		46
#define HEADSET_STEREO_RX_HEADPHONEGAIN_RIGHT_INDEX 	47

//HEADSET RX MONO    3

#define HEADSET_MONO_RX_CODEC_FILTER_INDEX   			8
#define HEADSET_MONO_RX_HEADPHONEGAIN_LEFT_INDEX 		49	
#define HEADSET_MONO_RX_HEADPHONEGAIN_RIGHT_INDEX 		50
#define HEADSET_MONO_RX_CODECGAIN_LEFT_INDEX 			51

// HEADSET TX    4
#define HEADSET_TX_CODECGAIN_LEFT_INDEX 	16
#define HEADSET_TX_CODECGAIN_ST_INDEX		14
#define HEADSET_TX_MICGAIN_LEFT_INDEX 		11

// HEADSET PHONE TX    5   
#define HEADSET2_TX_MICGAIN_LEFT_INDEX 	11
#define HEADSET2_TX_CODECGAIN_ST_INDEX	14
#define HEADSET2_TX_CODECGAIN_LEFT_INDEX 	16

//SPEAKER RX   6 
#define SPK_RX_CODECGAIN_RIGHT_INDEX		16
#define SPK_RX_CODECGAIN_LEFT_INDEX			15
#define SPK_RX_CODEC_FILTER_INDEX			4

//SPEAKER TX    7
#define SPK_TX_MICGAIN_LEFT_INDEX	11
#define SPK_TX_CODECGAIN_LEFT_INDEX	15


//SPEAKER STEREO RX  8 
#define SPK_STEREO_RX_CODEC_FILTER_INDEX		4
#define SPK_STEREO_RX_CODECGAIN_LEFT_INDEX	15
#define SPK_STEREO_RX_CODECGAIN_RIGHT_INDEX	16

// SPEAKER PHONE TX   9

#define SPK_PHONE_TX_MICGAIN_LEFT_INDEX		11
#define SPK_PHONE_TX_CODECGAIN_LEFT_INDEX	15

// HEADSET+SPEAKER RX    10
#define HEADSETSPK_RX_CODECGAIN_LEFT_INDEX			41
#define HEADSETSPK_RX_CODECGAIN_RIGHT_INDEX 		42
#define HEADSETSPK_RX_CODEC_FILTER_INDEX		 	8
#define HEADSETSPK_RX_HEADSETGAIN_LEFT_INDEX 		39
#define HEADSETSPK_RX_HEADSETGAIN_RIGHT_INDEX   	40

//HANDSET REC TX   11
#define HANDSET_REC_TX_CODECGAIN_LEFT_INDEX			16
#define HANDSET_REC_TX_CODECGAIN_ST_INDEX			14
#define HANDSET_REC_TX_MICGAIN_LEFT_INDEX 			11
/* BEGIN:0014024  kiran.kanneganti@lge.com     2011.01.13*/
/* MOD:0014024 Calibration support for loop back devices  */
//LoopBack Main Mic Tx 12
#define LB_MAIN_MIC_TX_CODECGAIN_LEFT_INDEX			16
#define LB_MAIN_MIC_TX_CODECGAIN_ST_INDEX			14
#define LB_MAIN_MIC_TX_MICGAIN_LEFT_INDEX 			11

//LoopBack Back Mic Tx 13
#define LB_BACK_MIC_TX_CODECGAIN_LEFT_INDEX			16
#define LB_BACK_MIC_TX_CODECGAIN_ST_INDEX			14
#define LB_BACK_MIC_TX_MICGAIN_LEFT_INDEX 			11

//LoopBack Handset Rx 14
#define LB_HANDSET_RX_CODECGAIN_LEFT_INDEX			18
#define LB_HANDSET_RX_CODEC_FILTER_INDEX			5

//LoopBack Headset TX 15
#define LB_HEADSET_TX_MICGAIN_LEFT_INDEX 	11
#define LB_HEADSET_TX_CODECGAIN_ST_INDEX	14
#define LB_HEADSET_TX_CODECGAIN_LEFT_INDEX 	16

//LoopBack Headset Rx 16
#define LB_HEADSET_MONO_RX_CODEC_FILTER_INDEX   			8
#define LB_HEADSET_MONO_RX_HEADPHONEGAIN_LEFT_INDEX 		49	
#define LB_HEADSET_MONO_RX_HEADPHONEGAIN_RIGHT_INDEX 		50
#define LB_HEADSET_MONO_RX_CODECGAIN_LEFT_INDEX 			51
/* END:0014024  kiran.kanneganti@lge.com     2011.01.13*/

/*[START] jy0127.jang@lge.com MS910_AUDIO_CHANGE 2011-04-07*/
/*added speaker phone loopback*/
//LoopBack Main Mic Tx 17
#define LB_SPEAKERPHONE_TX_CODECGAIN_LEFT_INDEX			15
#define LB_SPEAKERPHONE_TX_CODECGAIN_ST_INDEX			14
#define LB_SPEAKERPHONE_TX_MICGAIN_LEFT_INDEX 			11

//LoopBack Handset Rx 18
#define LB_SPEAKERPHONE_RX_CODECGAIN_LEFT_INDEX			15
#define LB_SPEAKERPHONE_RX_CODEC_FILTER_INDEX			4
/*[END] jy0127.jang@lge.com MS910_AUDIO_CHANGE 2011-04-07*/

/* QTR ADIE register index -- */

/* Marimba configuration parameter ++ */
struct codec_index_data
{
	u8 index;
	u8 data;
};

/* configuration of each device */
struct audio_params
{
	struct  codec_index_data  codec_Rx_left_gain;
	struct  codec_index_data  codec_Rx_right_gain;
	struct  codec_index_data  codec_Tx_left_gain;
	struct  codec_index_data  codec_Tx_right_gain;
	struct  codec_index_data  codec_St_gain;
	struct  codec_index_data  codec_Rx_filter_left;
	struct  codec_index_data  codec_Rx_filter_right;
	struct  codec_index_data  codec_micamp_left;
	struct  codec_index_data  codec_micamp_right;
	struct  codec_index_data  codec_headset_left;
	struct  codec_index_data  codec_headset_right;
//	struct  codec_index_data  codec_AuxPGA_left;
//	struct  codec_index_data  codec_AuxPGA_right;	
};

/*
#define MARIMBA_CODEC_CDC_LRXG		0x84	[7:0]	// Left RX gain
#define MARIMBA_CODEC_CDC_RRXG		0x85	[7:0]	// Right RX gain
#define MARIMBA_CODEC_CDC_LTXG		0x86	[7:0]	// Left TX gain
#define MARIMBA_CODEC_CDC_RTXG		0x87	[7:0]	// Right TX gain
#define MARIMBA_CODEC_CDC_ST		0x8B	[7:0]	// TX Side Tone gain
#define RX_FILTER_CONTROL_REG		0x24	[6:5]	// RX Filter Enable/Disable 6:left, 5:right
#define MICAMP_RIGHT_REG				0x0E	[6:5]	// TX MIC Amp gain, 00:0, 01:4.5, 1x:24dB
#define MICAMP_LEFT_REG				0x0D[6:5]	// TX MIC Amp gain
#define HEADPHONE_LEFT_GAIN_REG		0x3B	[7:2]	// RX Headphone LEFT
#define HEADPHONE_RIGHT_GAIN_REG		0x3C[7:2]	// RX Headphone RIGHT
*/

struct audio_params device_cal_data[NUMBER_OF_DEVICES];

/* Marimba configuration parameter -- */

/* RX volume parameter ++ */
struct volumelevel{
	s32 max_level;
	s32  min_level;
};

// 0 : NB, 1 : WB
/* BEGIN:0015657        ehgrace.kim@lge.com     2011.02.08*/
/* MOD: change the handset rx min vol for HW request */

/*BEGIN:0016596 	kiran.kanneganti@lge.com 	2011.02.21 */
/*MOD : modifiy the RxVolume as for HW request */
struct volumelevel handset_rx[2] 		= {{100,-2000},{100,-2000}};
/*END:0016596 	kiran.kanneganti@lge.com 	2011.02.21*/

/* END:0015657        ehgrace.kim@lge.com     2011.02.08*/
struct volumelevel headset_stereo_rx[2]	= {{-700,-2200},{-900,-2400}};
/* BEGIN:0016258        ehgrace.kim@lge.com     2011.02.17*/
/* MOD: change the audio calibration data for HW calibration */
//struct volumelevel headset_mono_rx[2]	= {{-700,-2200},{-900,-2400}};
struct volumelevel headset_mono_rx[2]	= {{0,-1500},{0,-1500}};
/* END:0016258        ehgrace.kim@lge.com     2011.02.17*/
struct volumelevel spk_rx[2]			= {{1000,-500},{1000,-500}};
//struct volumelevel handset_aud_rx[2]		= {{400,-1100},{400,-1100}};
//struct volumelevel spk_aud_rx[2]			= {{400,-1100},{400,-1100}};
struct volumelevel headset_spk_rx[2]	= {{-500,-2000},{-500,-2000}};
struct volumelevel BT_earpiece_rx[2]	= {{400,-1100},{400,-1100}};
#if LGE_AUDIO_PATH
/* BEGIN:0010882        ehgrace.kim@lge.com     2010.11.15*/
/* MOD: add add the subsystem control for call mode */
struct volumelevel spk_stereo_rx[2] = {{1000,-500},{1000,-500}};
/* END:0010882        ehgrace.kim@lge.com     2010.11.15*/
#endif

/* RX volume parameter -- */

/* 1st parameter is device id by acdb id */

/* 2nd parameter of setAudCalParam */
enum audcal_set_type
{
	MIC_BIAS_0 =			0,	// TX
	MIC_BIAS_1 =			1,	// TX
	CODEC_RX_LEFT =			3,
	CODEC_RX_RIGHT =		4,
	CODEC_TX_LEFT =			5,	// TX
	CODEC_TX_RIGHT =		6,	// TX
	CODEC_ST =				7,	// TX
	CODEC_RX_FILTER_LEFT =	8,
	CODEC_RX_FILTER_RIGHT =	9,	
	MICAMP_GAIN_LEFT =		10,	// TX
	MICAMP_GAIN_RIGHT =		11,	// TX
	HEADPHONE_LEFT_GAIN =	12,
	HEADPHONE_RIGHT_GAIN =	13,
//	AUXPGA_LEFT_GAIN =		14,
//	AUXPGA_RIGHT_GAIN =		15,
	RX_VOLUME_MIN_NB = 		16,
	RX_VOLUME_MAX_NB = 		17,
	RX_VOLUME_MIN_WB = 		18,
	RX_VOLUME_MAX_WB = 		19
};


void Snddev_Fill_Device_Data()
{
	u8 reg=0, mask=0, val=0;
	u8 temp =0;
	memset(device_cal_data, 0x00,sizeof(device_cal_data));
	
/*************************************************** [0] Handset Rx**************************************************/
	ADIE_CODEC_UNPACK_ENTRY(iearpiece_48KHz_osr256_actions[HANDSET_RX_CODECGAIN_LEFT_INDEX].action,reg, mask, val);
	device_cal_data[0].codec_Rx_left_gain.index = HANDSET_RX_CODECGAIN_LEFT_INDEX;
	device_cal_data[0].codec_Rx_left_gain.data = val;
	printk("Rx left Gain = %d\n",val);
	ADIE_CODEC_UNPACK_ENTRY(iearpiece_48KHz_osr256_actions[HANDSET_RX_CODEC_FILTER_INDEX].action,reg, mask, val);
	temp =val;
	temp = temp>>6;
	temp = temp & 0x01;
	device_cal_data[0].codec_Rx_filter_left.index = HANDSET_RX_CODEC_FILTER_INDEX;
	device_cal_data[0].codec_Rx_filter_left.data = temp;	
	temp =val;
	temp = temp>>5;
	temp = temp & 0x01;	
	device_cal_data[0].codec_Rx_filter_right.index = HANDSET_RX_CODEC_FILTER_INDEX;	
	device_cal_data[0].codec_Rx_filter_right.data = temp;	

	printk(" Handset Rx left Gain = %x\n",device_cal_data[0].codec_Rx_left_gain.data);
	printk(" Handset Rx Left Filter status = %x\n",device_cal_data[0].codec_Rx_filter_left.data);
	printk(" Handset Rx Right Filter status = %x\n",device_cal_data[0].codec_Rx_filter_right.data);

/*************************************************** [1] Handset Tx **************************************************/
	ADIE_CODEC_UNPACK_ENTRY(imic_8KHz_osr256_actions[HANDSET_TX_MICGAIN_RIGHT_INDEX].action,reg, mask, val);
	device_cal_data[1].codec_micamp_right.index = HANDSET_TX_MICGAIN_RIGHT_INDEX;
	val = val & 0x60;
	val = val>>5;
	if(val>=2)
		val =2;
	device_cal_data[1].codec_micamp_right.data = val;
	ADIE_CODEC_UNPACK_ENTRY(imic_8KHz_osr256_actions[HANDSET_TX_MICGAIN_LEFT_INDEX].action,reg, mask, val);
	device_cal_data[1].codec_micamp_left.index = HANDSET_TX_MICGAIN_LEFT_INDEX;
	val = val & 0x60;
	val = val>>5;	
	if(val>=2)
		val =2;
	device_cal_data[1].codec_micamp_left.data = val;
	ADIE_CODEC_UNPACK_ENTRY(imic_8KHz_osr256_actions[HANDSET_TX_CODECGAIN_ST_INDEX].action,reg, mask, val);
	device_cal_data[1].codec_St_gain.index = HANDSET_TX_CODECGAIN_ST_INDEX;
	device_cal_data[1].codec_St_gain.data = val;	
	ADIE_CODEC_UNPACK_ENTRY(imic_8KHz_osr256_actions[HANDSET_TX_CODECGAIN_RIGHT_INDEX].action,reg, mask, val);
	device_cal_data[1].codec_Tx_right_gain.index = HANDSET_TX_CODECGAIN_RIGHT_INDEX;
	device_cal_data[1].codec_Tx_right_gain.data = val;		
	ADIE_CODEC_UNPACK_ENTRY(imic_8KHz_osr256_actions[HANDSET_TX_CODECGAIN_LEFT_INDEX].action,reg, mask, val);
	device_cal_data[1].codec_Tx_left_gain.index = HANDSET_TX_CODECGAIN_LEFT_INDEX;
	device_cal_data[1].codec_Tx_left_gain.data = val;	

	printk(" Handset Tx MicAmp Right Gain = %x\n",device_cal_data[1].codec_micamp_right.data);
	printk(" Handset Tx MicAmp Left Gain = %x\n",device_cal_data[1].codec_micamp_left.data);
	printk(" Handset Tx ST Gain = %x\n",device_cal_data[1].codec_St_gain.data);
	printk(" Handset Tx Left Gain = %x\n",device_cal_data[1].codec_Tx_left_gain.data);
	printk(" Handset Tx Right Gain = %x\n",device_cal_data[1].codec_Tx_right_gain.data);
	
/***************************************************** [2] Headset Rx Stereo*******************************************/
	ADIE_CODEC_UNPACK_ENTRY(ihs_stereo_rx_48KHz_osr256_actions[HEADSET_STEREO_RX_CODEC_FILTER_INDEX].action,reg, mask, val);
	temp =val;
	temp = temp >> 6;
	temp = temp & 0x01;
	device_cal_data[2].codec_Rx_filter_left.index = HEADSET_STEREO_RX_CODEC_FILTER_INDEX;
	device_cal_data[2].codec_Rx_filter_left.data = temp;
	temp =val;
	temp = temp >> 5;
	temp = temp & 0x01;	
	device_cal_data[2].codec_Rx_filter_right.index = HEADSET_STEREO_RX_CODEC_FILTER_INDEX;
	device_cal_data[2].codec_Rx_filter_right.data = temp;
	ADIE_CODEC_UNPACK_ENTRY(ihs_stereo_rx_48KHz_osr256_actions[HEADSET_STEREO_RX_HEADPHONEGAIN_LEFT_INDEX].action,reg, mask, val);
	temp = val;
	temp = temp>>2;
	device_cal_data[2].codec_headset_left.index = HEADSET_STEREO_RX_HEADPHONEGAIN_LEFT_INDEX;	
	device_cal_data[2].codec_headset_left.data = temp;
	ADIE_CODEC_UNPACK_ENTRY(ihs_stereo_rx_48KHz_osr256_actions[HEADSET_STEREO_RX_HEADPHONEGAIN_RIGHT_INDEX].action,reg, mask, val);
	temp = val;
	temp = temp>>2;	
	device_cal_data[2].codec_headset_right.index = HEADSET_STEREO_RX_HEADPHONEGAIN_RIGHT_INDEX;
	device_cal_data[2].codec_headset_right.data = temp;	
	ADIE_CODEC_UNPACK_ENTRY(ihs_stereo_rx_48KHz_osr256_actions[HEADSET_STEREO_RX_CODECGAIN_LEFT_INDEX].action,reg, mask, val);
	device_cal_data[2].codec_Rx_left_gain.index = HEADSET_STEREO_RX_CODECGAIN_LEFT_INDEX;	
	device_cal_data[2].codec_Rx_left_gain.data = val;
	ADIE_CODEC_UNPACK_ENTRY(ihs_stereo_rx_48KHz_osr256_actions[HEADSET_STEREO_RX_CODECGAIN_RIGHT_INDEX].action,reg, mask, val);
	device_cal_data[2].codec_Rx_right_gain.index = HEADSET_STEREO_RX_CODECGAIN_RIGHT_INDEX;	
	device_cal_data[2].codec_Rx_right_gain.data = val;	

	printk(" Headset Rx Stereo Left Filter = %x\n",device_cal_data[2].codec_Rx_filter_left.data);
	printk(" Headset Rx Stereo Right Filter = %x\n",device_cal_data[2].codec_Rx_filter_right.data);
	printk(" Headset Rx Stereo Headset Left Gain = %x\n",device_cal_data[2].codec_headset_left.data);	
	printk(" Headset Rx Stereo Headset Right Gain = %x\n",device_cal_data[2].codec_headset_right.data);
	printk(" Headset Rx Stereo Left Gain = %x\n",device_cal_data[2].codec_Rx_left_gain.data);
	printk(" Headset Rx Stereo Right Gain = %x\n",device_cal_data[2].codec_Rx_right_gain.data);
	
/*************************************************** [3] Headset Rx Mono ********************************************/
	ADIE_CODEC_UNPACK_ENTRY(ihs_mono_rx_48KHz_osr256_actions[HEADSET_MONO_RX_CODEC_FILTER_INDEX].action,reg, mask, val);
	temp =val;
	temp = temp >> 6;
	temp = temp & 0x01;	
	device_cal_data[3].codec_Rx_filter_left.index = HEADSET_MONO_RX_CODEC_FILTER_INDEX;
	device_cal_data[3].codec_Rx_filter_left.data = temp;
	temp =val;
	temp = temp >> 5;
	temp = temp & 0x01;	
	device_cal_data[3].codec_Rx_filter_right.index = HEADSET_MONO_RX_CODEC_FILTER_INDEX;
	device_cal_data[3].codec_Rx_filter_right.data = temp;
	ADIE_CODEC_UNPACK_ENTRY(ihs_mono_rx_48KHz_osr256_actions[HEADSET_MONO_RX_HEADPHONEGAIN_LEFT_INDEX].action,reg, mask, val);
       temp = val;
       temp = temp >>2;
	device_cal_data[3].codec_headset_left.index = HEADSET_MONO_RX_HEADPHONEGAIN_LEFT_INDEX;	
	device_cal_data[3].codec_headset_left.data = temp;
	ADIE_CODEC_UNPACK_ENTRY(ihs_mono_rx_48KHz_osr256_actions[HEADSET_MONO_RX_HEADPHONEGAIN_RIGHT_INDEX].action,reg, mask, val);
       temp = val;
       temp = temp >>2;
	device_cal_data[3].codec_headset_right.index = HEADSET_MONO_RX_HEADPHONEGAIN_RIGHT_INDEX;
	device_cal_data[3].codec_headset_right.data = temp;	
	ADIE_CODEC_UNPACK_ENTRY(ihs_mono_rx_48KHz_osr256_actions[HEADSET_MONO_RX_CODECGAIN_LEFT_INDEX].action,reg, mask, val);
	device_cal_data[3].codec_Rx_left_gain.index = HEADSET_MONO_RX_CODECGAIN_LEFT_INDEX;	
	device_cal_data[3].codec_Rx_left_gain.data = val;

#if 0
	ADIE_CODEC_UNPACK_ENTRY(ihs_mono_rx_48KHz_osr256_actions[HEADSET_MONO_RX_CODECGAIN_RIGHT_INDEX].action,reg, mask, val);
	device_cal_data[3].codec_Rx_right_gain.index = HEADSET_MONO_RX_CODECGAIN_RIGHT_INDEX;	
	device_cal_data[3].codec_Rx_right_gain.data = val;
#endif 

	printk(" Headset Rx Mono Left Filter = %x\n",device_cal_data[3].codec_Rx_filter_left.data);
	printk(" Headset Rx Mono Right Filter = %x\n",device_cal_data[3].codec_Rx_filter_right.data);
	printk(" Headset Rx Mono Headset Left Gain = %x\n",device_cal_data[3].codec_headset_left.data);	
	printk(" Headset Rx Mono Headset Right Gain = %x\n",device_cal_data[3].codec_headset_right.data);
	printk(" Headset Rx Mono Left Gain = %x\n",device_cal_data[3].codec_Rx_left_gain.data);
#if 0	
	printk(" Headset Rx Mono Right Gain = %x\n",device_cal_data[3].codec_Rx_right_gain.data);
#endif
/******************************************************* [4] Headset Tx********************************************/
	ADIE_CODEC_UNPACK_ENTRY(ihs_mono_tx_48KHz_osr256_actions[HEADSET_TX_MICGAIN_LEFT_INDEX].action,reg, mask, val);
	device_cal_data[4].codec_micamp_left.index = HEADSET_TX_MICGAIN_LEFT_INDEX;
	val = val & 0x60;
	val = val>>5;	
	if(val>=2)
		val =2;
	device_cal_data[4].codec_micamp_left.data = val;	
	ADIE_CODEC_UNPACK_ENTRY(ihs_mono_tx_48KHz_osr256_actions[HEADSET_TX_CODECGAIN_ST_INDEX].action,reg, mask, val);
	device_cal_data[4].codec_St_gain.index = HEADSET_TX_CODECGAIN_ST_INDEX;
	device_cal_data[4].codec_St_gain.data = val;	
	ADIE_CODEC_UNPACK_ENTRY(ihs_mono_tx_48KHz_osr256_actions[HEADSET_TX_CODECGAIN_LEFT_INDEX].action,reg, mask, val);
	device_cal_data[4].codec_Tx_left_gain.index = HEADSET_TX_CODECGAIN_LEFT_INDEX;
	device_cal_data[4].codec_Tx_left_gain.data = val;	

	printk(" Headset Tx MicAmp Left Gain = %x\n",device_cal_data[4].codec_micamp_left.data);
	printk(" Headset Tx ST Gain = %x\n",device_cal_data[4].codec_St_gain.data);
	printk(" Headset Tx Left Gain = %x\n",device_cal_data[4].codec_Tx_left_gain.data);

/***************************************************** [5] Headset Phone Tx******************************************/
/*BEGIN: 0012454 kiran.kanneganti@lge.com 2010-12-15*/
/*Mod 0012454: Changed headset mode actions for call. Call use 8K actions*/
	ADIE_CODEC_UNPACK_ENTRY(ihs_phone_tx_8KHz_osr256_actions[HEADSET_TX_MICGAIN_LEFT_INDEX].action,reg, mask, val);
	device_cal_data[5].codec_micamp_left.index = HEADSET_TX_MICGAIN_LEFT_INDEX;
	val = val & 0x60;
	val = val>>5;	
	if(val>=2)
		val =2;
	device_cal_data[5].codec_micamp_left.data = val;	
	ADIE_CODEC_UNPACK_ENTRY(ihs_phone_tx_8KHz_osr256_actions[HEADSET_TX_CODECGAIN_ST_INDEX].action,reg, mask, val);
	device_cal_data[5].codec_St_gain.index = HEADSET_TX_CODECGAIN_ST_INDEX;
	device_cal_data[5].codec_St_gain.data = val;	
	ADIE_CODEC_UNPACK_ENTRY(ihs_phone_tx_8KHz_osr256_actions[HEADSET_TX_CODECGAIN_LEFT_INDEX].action,reg, mask, val);
	device_cal_data[5].codec_Tx_left_gain.index = HEADSET_TX_CODECGAIN_LEFT_INDEX;
	device_cal_data[5].codec_Tx_left_gain.data = val;	

	printk(" Headset Phone Tx MicAmp Left Gain = %x\n",device_cal_data[5].codec_micamp_left.data);
	printk(" Headset Phone Tx ST Gain = %x\n",device_cal_data[5].codec_St_gain.data);
	printk(" Headset Phone Tx Left Gain = %x\n",device_cal_data[5].codec_Tx_left_gain.data);
/*END: 0012454 kiran.kanneganti@lge.com 2010-12-15*/
/**************************************************** [6] Speaker Rx Stereo for Mono***********************************/
	ADIE_CODEC_UNPACK_ENTRY(ispeaker_rx_48KHz_osr256_actions[SPK_RX_CODEC_FILTER_INDEX].action,reg, mask, val);
	temp =val;
	temp = temp >> 6;
	temp = temp & 0x01;	
	device_cal_data[6].codec_Rx_filter_left.index = SPK_RX_CODEC_FILTER_INDEX;
	device_cal_data[6].codec_Rx_filter_left.data = temp;	
	temp =val;
	temp = temp >> 5;
	temp = temp & 0x01;
	device_cal_data[6].codec_Rx_filter_right.index = SPK_RX_CODEC_FILTER_INDEX;
	device_cal_data[6].codec_Rx_filter_right.data = temp;	
	ADIE_CODEC_UNPACK_ENTRY(ispeaker_rx_48KHz_osr256_actions[SPK_RX_CODECGAIN_LEFT_INDEX].action,reg, mask, val);
	device_cal_data[6].codec_Rx_left_gain.index = SPK_RX_CODECGAIN_LEFT_INDEX;
	device_cal_data[6].codec_Rx_left_gain.data = val;	
	ADIE_CODEC_UNPACK_ENTRY(ispeaker_rx_48KHz_osr256_actions[SPK_RX_CODECGAIN_RIGHT_INDEX].action,reg, mask, val);
	device_cal_data[6].codec_Rx_right_gain.index = SPK_RX_CODECGAIN_RIGHT_INDEX;
	device_cal_data[6].codec_Rx_right_gain.data = val;	

	printk(" Speaker Rx Filter Left status = %x\n",device_cal_data[6].codec_Rx_filter_left.data);
	printk(" Speaker Rx Filter Right status = %x\n",device_cal_data[6].codec_Rx_filter_right.data);
	printk(" Speaker Rx Left Gain = %x\n",device_cal_data[6].codec_Rx_left_gain.data);
	printk(" Speaker Rx Right Gain = %x\n",device_cal_data[6].codec_Rx_right_gain.data);

/***************************************************** [7] Speaker Tx*************************************************/
/* BEGIN:0010882        ehgrace.kim@lge.com     2010.11.15*/
/* MOD: add add the subsystem control for call mode */
#if 1
/* BEGIN:0012384 ehgrace.kim@lge.com     2010.12.14*/
/* MOD: [Audio]change tx gain for recording mode & delete the dummy src*/
	ADIE_CODEC_UNPACK_ENTRY(ispeaker_tx_48KHz_osr256_actions[SPK_TX_MICGAIN_LEFT_INDEX].action,reg, mask, val);
	printk("[test] %s : reg %x, mask %x, val %x\n", reg, mask, val);
	device_cal_data[7].codec_micamp_right.index = SPK_TX_MICGAIN_LEFT_INDEX; //mono
#else
	ADIE_CODEC_UNPACK_ENTRY(ispeaker_tx_8KHz_osr256_actions[SPK_TX_MICGAIN_RIGHT_INDEX].action,reg, mask, val);
	device_cal_data[7].codec_micamp_right.index = SPK_TX_MICGAIN_RIGHT_INDEX;
#endif
/* END:0010882        ehgrace.kim@lge.com     2010.11.15*/
	val = val & 0x60;
	val = val>>5;	
	if(val>=2)
		val =2;	
	device_cal_data[7].codec_micamp_right.data = val;	
	ADIE_CODEC_UNPACK_ENTRY(ispeaker_tx_48KHz_osr256_actions[SPK_TX_MICGAIN_LEFT_INDEX].action,reg, mask, val);
	device_cal_data[7].codec_micamp_left.index = SPK_TX_MICGAIN_LEFT_INDEX;
	val = val & 0x60;
	val = val>>5;	
	if(val>=2)
		val =2;	
	device_cal_data[7].codec_micamp_left.data = val;	
/* BEGIN:0010882        ehgrace.kim@lge.com     2010.11.15*/
/* MOD: add add the subsystem control for call mode */
#if 1
	ADIE_CODEC_UNPACK_ENTRY(ispeaker_tx_48KHz_osr256_actions[SPK_TX_CODECGAIN_LEFT_INDEX].action,reg, mask, val);
	device_cal_data[7].codec_Tx_right_gain.index = SPK_TX_CODECGAIN_LEFT_INDEX; //mono
#else
	ADIE_CODEC_UNPACK_ENTRY(ispeaker_tx_8KHz_osr256_actions[SPK_TX_CODECGAIN_ST_INDEX].action,reg, mask, val);
	device_cal_data[7].codec_St_gain.index = SPK_TX_CODECGAIN_ST_INDEX;
	device_cal_data[7].codec_St_gain.data = val;
	ADIE_CODEC_UNPACK_ENTRY(ispeaker_tx_8KHz_osr256_actions[SPK_TX_CODECGAIN_RIGHT_INDEX].action,reg, mask, val);
	device_cal_data[7].codec_Tx_right_gain.index = SPK_TX_CODECGAIN_RIGHT_INDEX;
#endif
/* END:0010882        ehgrace.kim@lge.com     2010.11.15*/
	device_cal_data[7].codec_Tx_right_gain.data = val;
	ADIE_CODEC_UNPACK_ENTRY(ispeaker_tx_48KHz_osr256_actions[SPK_TX_CODECGAIN_LEFT_INDEX].action,reg, mask, val);
	device_cal_data[7].codec_Tx_left_gain.index = SPK_TX_CODECGAIN_LEFT_INDEX;
	device_cal_data[7].codec_Tx_left_gain.data = val;

	printk(" Speaker Phone Tx MicAmp Right Gain = %x\n",device_cal_data[7].codec_micamp_right.data);
	printk(" Speaker Phone Tx MicAmp Left Gain = %x\n",device_cal_data[7].codec_micamp_left.data);
/* BEGIN:0010882        ehgrace.kim@lge.com     2010.11.15*/
/* MOD: add add the subsystem control for call mode */
//	printk(" Speaker Tx ST Gain = %x\n",device_cal_data[6].codec_St_gain.data);
/* END:0010882        ehgrace.kim@lge.com     2010.11.15*/
	printk(" Speaker Phone Tx Left Gain = %x\n",device_cal_data[7].codec_Tx_left_gain.data);
	printk(" Speaker Phone Tx Right Gain = %x\n",device_cal_data[7].codec_Tx_right_gain.data);
/* END:0012384 ehgrace.kim@lge.com     2010.12.14*/


/**************************************** [8] Speaker Phone Rx****************************************************/
	ADIE_CODEC_UNPACK_ENTRY(ispeaker_phone_rx_48KHz_osr256_actions[SPK_RX_CODEC_FILTER_INDEX].action,reg, mask, val);
	
	temp = temp >> 6;
	temp = temp & 0x01;	
	device_cal_data[8].codec_Rx_filter_left.index = SPK_RX_CODEC_FILTER_INDEX;
	device_cal_data[8].codec_Rx_filter_left.data = temp;	
	temp =val;
	temp = temp >> 5;
	temp = temp & 0x01;
	device_cal_data[8].codec_Rx_filter_right.index = SPK_RX_CODEC_FILTER_INDEX;
	device_cal_data[8].codec_Rx_filter_right.data = temp;	
	ADIE_CODEC_UNPACK_ENTRY(ispeaker_phone_rx_48KHz_osr256_actions[SPK_RX_CODECGAIN_LEFT_INDEX].action,reg, mask, val);
	device_cal_data[8].codec_Rx_left_gain.index = SPK_RX_CODECGAIN_LEFT_INDEX;
	device_cal_data[8].codec_Rx_left_gain.data = val;	
	ADIE_CODEC_UNPACK_ENTRY(ispeaker_phone_rx_48KHz_osr256_actions[SPK_RX_CODECGAIN_RIGHT_INDEX].action,reg, mask, val);
	device_cal_data[8].codec_Rx_right_gain.index = SPK_RX_CODECGAIN_RIGHT_INDEX;
	device_cal_data[8].codec_Rx_right_gain.data = val;	

	printk(" Speaker Phone Rx Filter Left status = %x\n",device_cal_data[8].codec_Rx_filter_left.data);
	printk(" Speaker Phone Rx Filter Right status = %x\n",device_cal_data[8].codec_Rx_filter_right.data);
	printk(" Speaker Phone Rx Left Gain = %x\n",device_cal_data[8].codec_Rx_left_gain.data);
	printk(" Speaker Phone Rx Right Gain = %x\n",device_cal_data[8].codec_Rx_right_gain.data);
	
/******************************************** [9] Speaker Phone Mic**************************************************/
/* BEGIN:0010882        ehgrace.kim@lge.com     2010.11.15*/
/* MOD: add add the subsystem control for call mode */
	ADIE_CODEC_UNPACK_ENTRY(ispeaker_phone_tx_8KHz_osr256_actions[SPK_PHONE_TX_MICGAIN_LEFT_INDEX].action,reg, mask, val);
	printk("[test] %s : reg %x, mask %x, val %x\n", reg, mask, val);
	device_cal_data[9].codec_micamp_right.index = SPK_PHONE_TX_MICGAIN_LEFT_INDEX; //mono
	val = val & 0x60;
	val = val>>5;	
	if(val>=2)
		val =2;	
	device_cal_data[9].codec_micamp_right.data = val;	
	ADIE_CODEC_UNPACK_ENTRY(ispeaker_phone_tx_8KHz_osr256_actions[SPK_PHONE_TX_MICGAIN_LEFT_INDEX].action,reg, mask, val);
	device_cal_data[9].codec_micamp_left.index = SPK_PHONE_TX_MICGAIN_LEFT_INDEX;
	val = val & 0x60;
	val = val>>5;	
	if(val>=2)
		val =2;	
	device_cal_data[9].codec_micamp_left.data = val;	
	ADIE_CODEC_UNPACK_ENTRY(ispeaker_phone_tx_8KHz_osr256_actions[SPK_PHONE_TX_CODECGAIN_LEFT_INDEX].action,reg, mask, val);
	device_cal_data[9].codec_Tx_right_gain.index = SPK_PHONE_TX_CODECGAIN_LEFT_INDEX; //mono
	device_cal_data[9].codec_Tx_right_gain.data = val;
	ADIE_CODEC_UNPACK_ENTRY(ispeaker_phone_tx_8KHz_osr256_actions[SPK_PHONE_TX_CODECGAIN_LEFT_INDEX].action,reg, mask, val);
	device_cal_data[9].codec_Tx_left_gain.index = SPK_PHONE_TX_CODECGAIN_LEFT_INDEX;
	device_cal_data[9].codec_Tx_left_gain.data = val;

	printk(" Speaker Phone Tx MicAmp Right Gain = %x\n",device_cal_data[9].codec_micamp_right.data);
	printk(" Speaker Phone Tx MicAmp Left Gain = %x\n",device_cal_data[9].codec_micamp_left.data);
/* BEGIN:0010882        ehgrace.kim@lge.com     2010.11.15*/
/* MOD: add add the subsystem control for call mode */
//	printk(" Speaker Tx ST Gain = %x\n",device_cal_data[6].codec_St_gain.data);
/* END:0010882        ehgrace.kim@lge.com     2010.11.15*/
	printk(" Speaker Phone Tx Left Gain = %x\n",device_cal_data[9].codec_Tx_left_gain.data);
	printk(" Speaker Phone Tx Right Gain = %x\n",device_cal_data[9].codec_Tx_right_gain.data);

/******************************************* [10] Stereo [ Headset + Speaker ] Rx*****************************************/
	ADIE_CODEC_UNPACK_ENTRY(ihs_stereo_speaker_stereo_rx_48KHz_osr256_actions[HEADSETSPK_RX_CODEC_FILTER_INDEX].action,reg, mask, val);
	temp =val;
	temp = temp >> 6;
	temp = temp & 0x01;
	device_cal_data[10].codec_Rx_filter_left.index = HEADSETSPK_RX_CODEC_FILTER_INDEX;
	device_cal_data[10].codec_Rx_filter_left.data = temp;
	temp =val;
	temp = temp >> 5;
	temp = temp & 0x01;	
	device_cal_data[10].codec_Rx_filter_right.index = HEADSETSPK_RX_CODEC_FILTER_INDEX;
	device_cal_data[10].codec_Rx_filter_right.data = temp;
	ADIE_CODEC_UNPACK_ENTRY(ihs_stereo_speaker_stereo_rx_48KHz_osr256_actions[HEADSETSPK_RX_HEADSETGAIN_LEFT_INDEX].action,reg, mask, val);
	temp = val;
	temp = temp >>2;
	device_cal_data[10].codec_headset_left.index = HEADSETSPK_RX_HEADSETGAIN_LEFT_INDEX;	
	device_cal_data[10].codec_headset_left.data = temp;
	ADIE_CODEC_UNPACK_ENTRY(ihs_stereo_speaker_stereo_rx_48KHz_osr256_actions[HEADSETSPK_RX_HEADSETGAIN_RIGHT_INDEX].action,reg, mask, val);
	temp = val;
    temp = temp >>2;
	device_cal_data[10].codec_headset_right.index = HEADSETSPK_RX_HEADSETGAIN_RIGHT_INDEX;
	device_cal_data[10].codec_headset_right.data = temp;	
	ADIE_CODEC_UNPACK_ENTRY(ihs_stereo_speaker_stereo_rx_48KHz_osr256_actions[HEADSETSPK_RX_CODECGAIN_LEFT_INDEX].action,reg, mask, val);
	device_cal_data[10].codec_Rx_left_gain.index = HEADSETSPK_RX_CODECGAIN_LEFT_INDEX;	
	device_cal_data[10].codec_Rx_left_gain.data = val;
	ADIE_CODEC_UNPACK_ENTRY(ihs_stereo_speaker_stereo_rx_48KHz_osr256_actions[HEADSETSPK_RX_CODECGAIN_RIGHT_INDEX].action,reg, mask, val);
	device_cal_data[10].codec_Rx_right_gain.index = HEADSETSPK_RX_CODECGAIN_RIGHT_INDEX;	
	device_cal_data[10].codec_Rx_right_gain.data = val;	
	
	printk(" Spk Plus Headset Rx Left Filter = %x\n",device_cal_data[10].codec_Rx_filter_left.data);
	printk(" Spk Plus Headset Rx Right Filter = %x\n",device_cal_data[10].codec_Rx_filter_right.data);
	printk(" Spk Plus Headset Rx Headset Left Gain = %x\n",device_cal_data[10].codec_headset_left.data);	
	printk(" Spk Plus Headset Rx Headset Right Gain = %x\n",device_cal_data[10].codec_headset_right.data);
	printk(" Spk Plus Headset Rx Left Gain = %x\n",device_cal_data[10].codec_Rx_left_gain.data);
	printk(" Spk Plus Headset Rx Right Gain = %x\n",device_cal_data[10].codec_Rx_right_gain.data);

/************************************************ [11] Handset Rec Tx****************************************************/
/* BEGIN:0012384 ehgrace.kim@lge.com     2010.12.14*/
/* MOD: [Audio]change tx gain for recording mode & delete the dummy src*/
	ADIE_CODEC_UNPACK_ENTRY(rec_imic_48KHz_osr256_actions[HANDSET_REC_TX_MICGAIN_LEFT_INDEX].action,reg, mask, val);
	device_cal_data[11].codec_micamp_left.index = HANDSET_REC_TX_MICGAIN_LEFT_INDEX;
	val = val & 0x60;
	val = val>>5;	
	if(val>=2)
		val =2;
	device_cal_data[11].codec_micamp_left.data = val;
	ADIE_CODEC_UNPACK_ENTRY(rec_imic_48KHz_osr256_actions[HANDSET_REC_TX_CODECGAIN_ST_INDEX].action,reg, mask, val);
	device_cal_data[11].codec_St_gain.index = HANDSET_REC_TX_CODECGAIN_ST_INDEX;
	device_cal_data[11].codec_St_gain.data = val;	
	ADIE_CODEC_UNPACK_ENTRY(rec_imic_48KHz_osr256_actions[HANDSET_REC_TX_CODECGAIN_LEFT_INDEX].action,reg, mask, val);
	device_cal_data[11].codec_Tx_left_gain.index = HANDSET_REC_TX_CODECGAIN_LEFT_INDEX;
	device_cal_data[11].codec_Tx_left_gain.data = val;	
/*BEGIN: 0012454 kiran.kanneganti@lge.com 2010-12-15*/
/*Mod 0012454: Corrected the logs for debugging*/
	printk(" Handset Rec Tx MicAmp Left Gain = %x\n",device_cal_data[11].codec_micamp_left.data);
	printk(" Handset Rec Tx ST Gain = %x\n",device_cal_data[11].codec_St_gain.data);
	printk(" Handset Rec Tx Left Gain = %x\n",device_cal_data[11].codec_Tx_left_gain.data);
/*END: 0012454 kiran.kanneganti@lge.com 2010-12-15*/
/* END:0012384 ehgrace.kim@lge.com     2010.12.14*/
#if LOOPBACK_SUPPORT
/* BEGIN:0014024  kiran.kanneganti@lge.com     2011.01.13*/
/* MOD:0014024 Calibration support for loop back devices  */
/************************************************ [12] LoopBack Main Mic Tx****************************************************/
	 ADIE_CODEC_UNPACK_ENTRY(testmode_handset_tx_8KHz_osr256_actions[LB_MAIN_MIC_TX_MICGAIN_LEFT_INDEX].action,reg, mask, val);
	 device_cal_data[12].codec_micamp_left.index = LB_MAIN_MIC_TX_MICGAIN_LEFT_INDEX;
	 val = val & 0x60;
	 val = val>>5;	 
	 if(val>=2)
		 val =2;
	 device_cal_data[12].codec_micamp_left.data = val;
	 ADIE_CODEC_UNPACK_ENTRY(testmode_handset_tx_8KHz_osr256_actions[LB_MAIN_MIC_TX_CODECGAIN_ST_INDEX].action,reg, mask, val);
	 device_cal_data[12].codec_St_gain.index = LB_MAIN_MIC_TX_CODECGAIN_ST_INDEX;
	 device_cal_data[12].codec_St_gain.data = val;	 
	 ADIE_CODEC_UNPACK_ENTRY(testmode_handset_tx_8KHz_osr256_actions[LB_MAIN_MIC_TX_CODECGAIN_LEFT_INDEX].action,reg, mask, val);
	 device_cal_data[12].codec_Tx_left_gain.index = LB_MAIN_MIC_TX_CODECGAIN_LEFT_INDEX;
	 device_cal_data[12].codec_Tx_left_gain.data = val;  
	 printk(" LB MAIN Tx MicAmp Left Gain = %x\n",device_cal_data[12].codec_micamp_left.data);
	 printk(" LB MAIN Tx ST Gain = %x\n",device_cal_data[12].codec_St_gain.data);
	 printk(" LB MAIN Tx Left Gain = %x\n",device_cal_data[12].codec_Tx_left_gain.data);

/************************************************ [13] LoopBack BACK Mic Tx****************************************************/
	 ADIE_CODEC_UNPACK_ENTRY(testmode_handset_back_mic_tx_8KHz_osr256_actions[LB_BACK_MIC_TX_MICGAIN_LEFT_INDEX].action,reg, mask, val);
	 device_cal_data[13].codec_micamp_left.index = LB_BACK_MIC_TX_MICGAIN_LEFT_INDEX;
	 val = val & 0x60;
	 val = val>>5;	 
	 if(val>=2)
		 val =2;
	 device_cal_data[13].codec_micamp_left.data = val;
	 ADIE_CODEC_UNPACK_ENTRY(testmode_handset_back_mic_tx_8KHz_osr256_actions[LB_BACK_MIC_TX_CODECGAIN_ST_INDEX].action,reg, mask, val);
	 device_cal_data[13].codec_St_gain.index = LB_BACK_MIC_TX_CODECGAIN_ST_INDEX;
	 device_cal_data[13].codec_St_gain.data = val;	 
	 ADIE_CODEC_UNPACK_ENTRY(testmode_handset_back_mic_tx_8KHz_osr256_actions[LB_BACK_MIC_TX_CODECGAIN_LEFT_INDEX].action,reg, mask, val);
	 device_cal_data[13].codec_Tx_left_gain.index = LB_BACK_MIC_TX_CODECGAIN_LEFT_INDEX;
	 device_cal_data[13].codec_Tx_left_gain.data = val;  
	 printk(" LB Back Tx MicAmp Left Gain = %x\n",device_cal_data[13].codec_micamp_left.data);
	 printk(" LB Back Tx ST Gain = %x\n",device_cal_data[13].codec_St_gain.data);
	 printk(" LB Back Tx Left Gain = %x\n",device_cal_data[13].codec_Tx_left_gain.data);
 
 /*************************************************** [14]LoopBack Handset Rx**************************************************/
	 ADIE_CODEC_UNPACK_ENTRY(testmode_handset_rx_8KHz_osr256_actions[LB_HANDSET_RX_CODECGAIN_LEFT_INDEX].action,reg, mask, val);
	 device_cal_data[14].codec_Rx_left_gain.index = LB_HANDSET_RX_CODECGAIN_LEFT_INDEX;
	 device_cal_data[14].codec_Rx_left_gain.data = val;
	 ADIE_CODEC_UNPACK_ENTRY(testmode_handset_rx_8KHz_osr256_actions[LB_HANDSET_RX_CODEC_FILTER_INDEX].action,reg, mask, val);
	 temp =val;
	 temp = temp>>6;
	 temp = temp & 0x01;
	 device_cal_data[14].codec_Rx_filter_left.index = LB_HANDSET_RX_CODEC_FILTER_INDEX;
	 device_cal_data[14].codec_Rx_filter_left.data = temp;	 
	 temp =val;
	 temp = temp>>5;
	 temp = temp & 0x01; 
	 device_cal_data[14].codec_Rx_filter_right.index = LB_HANDSET_RX_CODEC_FILTER_INDEX; 
	 device_cal_data[14].codec_Rx_filter_right.data = temp;	 
 
	 printk(" LB Handset Rx left Gain = %x\n",device_cal_data[14].codec_Rx_left_gain.data);
	 printk(" LB Handset Rx Left Filter status = %x\n",device_cal_data[14].codec_Rx_filter_left.data);
	 printk(" LB Handset Rx Right Filter status = %x\n",device_cal_data[14].codec_Rx_filter_right.data);
	 
 /***************************************************** [15]LoopBack Headset Phone Tx******************************************/
	 ADIE_CODEC_UNPACK_ENTRY(ihs_test_mode_mono_tx_8KHz_osr256_actions[LB_HEADSET_TX_MICGAIN_LEFT_INDEX].action,reg, mask, val);
	 device_cal_data[15].codec_micamp_left.index = LB_HEADSET_TX_MICGAIN_LEFT_INDEX;
	 val = val & 0x60;
	 val = val>>5;	 
	 if(val>=2)
		 val =2;
	 device_cal_data[15].codec_micamp_left.data = val;	 
	 ADIE_CODEC_UNPACK_ENTRY(ihs_test_mode_mono_tx_8KHz_osr256_actions[LB_HEADSET_TX_CODECGAIN_ST_INDEX].action,reg, mask, val);
	 device_cal_data[15].codec_St_gain.index = LB_HEADSET_TX_CODECGAIN_ST_INDEX;
	 device_cal_data[15].codec_St_gain.data = val;	 
	 ADIE_CODEC_UNPACK_ENTRY(ihs_test_mode_mono_tx_8KHz_osr256_actions[LB_HEADSET_TX_CODECGAIN_LEFT_INDEX].action,reg, mask, val);
	 device_cal_data[15].codec_Tx_left_gain.index = LB_HEADSET_TX_CODECGAIN_LEFT_INDEX;
	 device_cal_data[15].codec_Tx_left_gain.data = val;	 
 
	 printk(" LB Headset Phone Tx MicAmp Left Gain = %x\n",device_cal_data[15].codec_micamp_left.data);
	 printk(" LB Headset Phone Tx ST Gain = %x\n",device_cal_data[15].codec_St_gain.data);
	 printk(" LB Headset Phone Tx Left Gain = %x\n",device_cal_data[15].codec_Tx_left_gain.data);
	 
 /*************************************************** [16] LoopBack Headset Rx Mono ********************************************/
	 ADIE_CODEC_UNPACK_ENTRY(ihs_test_mode_mono_rx_8KHz_osr256_actions[LB_HEADSET_MONO_RX_CODEC_FILTER_INDEX].action,reg, mask, val);
	 temp =val;
	 temp = temp >> 6;
	 temp = temp & 0x01; 
	 device_cal_data[16].codec_Rx_filter_left.index = LB_HEADSET_MONO_RX_CODEC_FILTER_INDEX;
	 device_cal_data[16].codec_Rx_filter_left.data = temp;
	 temp =val;
	 temp = temp >> 5;
	 temp = temp & 0x01; 
	 device_cal_data[16].codec_Rx_filter_right.index = LB_HEADSET_MONO_RX_CODEC_FILTER_INDEX;
	 device_cal_data[16].codec_Rx_filter_right.data = temp;
	 ADIE_CODEC_UNPACK_ENTRY(ihs_test_mode_mono_rx_8KHz_osr256_actions[LB_HEADSET_MONO_RX_HEADPHONEGAIN_LEFT_INDEX].action,reg, mask, val);
		temp = val;
		temp = temp >>2;
	 device_cal_data[16].codec_headset_left.index = LB_HEADSET_MONO_RX_HEADPHONEGAIN_LEFT_INDEX; 
	 device_cal_data[16].codec_headset_left.data = temp;
	 ADIE_CODEC_UNPACK_ENTRY(ihs_test_mode_mono_rx_8KHz_osr256_actions[LB_HEADSET_MONO_RX_HEADPHONEGAIN_RIGHT_INDEX].action,reg, mask, val);
		temp = val;
		temp = temp >>2;
	 device_cal_data[16].codec_headset_right.index = LB_HEADSET_MONO_RX_HEADPHONEGAIN_RIGHT_INDEX;
	 device_cal_data[16].codec_headset_right.data = temp; 
	 ADIE_CODEC_UNPACK_ENTRY(ihs_test_mode_mono_rx_8KHz_osr256_actions[LB_HEADSET_MONO_RX_CODECGAIN_LEFT_INDEX].action,reg, mask, val);
	 device_cal_data[16].codec_Rx_left_gain.index = LB_HEADSET_MONO_RX_CODECGAIN_LEFT_INDEX; 
	 device_cal_data[16].codec_Rx_left_gain.data = val;
 
	 printk(" LB Headset Rx Mono Left Filter = %x\n",device_cal_data[16].codec_Rx_filter_left.data);
	 printk(" LB Headset Rx Mono Right Filter = %x\n",device_cal_data[16].codec_Rx_filter_right.data);
	 printk(" LB Headset Rx Mono Headset Left Gain = %x\n",device_cal_data[16].codec_headset_left.data); 
	 printk(" LB Headset Rx Mono Headset Right Gain = %x\n",device_cal_data[16].codec_headset_right.data);
	 printk(" LB Headset Rx Mono Left Gain = %x\n",device_cal_data[16].codec_Rx_left_gain.data);
/* END:0014024  kiran.kanneganti@lge.com     2011.01.13*/

/*[START] jy0127.jang@lge.com MS910_AUDIO_CHANGE 2011-04-07*/
/*added speaker phone loopback*/
/************************************************ [17] SpeakerPhone loopback Tx****************************************************/
	 ADIE_CODEC_UNPACK_ENTRY(testmode_handset_tx_8KHz_osr256_actions[LB_SPEAKERPHONE_TX_MICGAIN_LEFT_INDEX].action,reg, mask, val);
	 device_cal_data[17].codec_micamp_left.index = LB_SPEAKERPHONE_TX_MICGAIN_LEFT_INDEX;
	 val = val & 0x60;
	 val = val>>5;	 
	 if(val>=2)
		 val =2;
	 device_cal_data[17].codec_micamp_left.data = val;
	 ADIE_CODEC_UNPACK_ENTRY(testmode_handset_tx_8KHz_osr256_actions[LB_SPEAKERPHONE_TX_CODECGAIN_ST_INDEX].action,reg, mask, val);
	 device_cal_data[17].codec_St_gain.index = LB_SPEAKERPHONE_TX_CODECGAIN_ST_INDEX;
	 device_cal_data[17].codec_St_gain.data = val;	 
	 ADIE_CODEC_UNPACK_ENTRY(testmode_handset_tx_8KHz_osr256_actions[LB_SPEAKERPHONE_TX_CODECGAIN_LEFT_INDEX].action,reg, mask, val);
	 device_cal_data[17].codec_Tx_left_gain.index = LB_SPEAKERPHONE_TX_CODECGAIN_LEFT_INDEX;
	 device_cal_data[17].codec_Tx_left_gain.data = val;  
	 printk(" LB MAIN Tx MicAmp Left Gain = %x\n",device_cal_data[17].codec_micamp_left.data);
	 printk(" LB MAIN Tx ST Gain = %x\n",device_cal_data[17].codec_St_gain.data);
	 printk(" LB MAIN Tx Left Gain = %x\n",device_cal_data[17].codec_Tx_left_gain.data);

/*************************************************** [18]SpeakerPhone loopback Rx**************************************************/
	ADIE_CODEC_UNPACK_ENTRY(testmode_handset_rx_8KHz_osr256_actions[LB_SPEAKERPHONE_RX_CODECGAIN_LEFT_INDEX].action,reg, mask, val);
	device_cal_data[18].codec_Rx_left_gain.index = LB_SPEAKERPHONE_RX_CODECGAIN_LEFT_INDEX;
	device_cal_data[18].codec_Rx_left_gain.data = val;
	ADIE_CODEC_UNPACK_ENTRY(testmode_handset_rx_8KHz_osr256_actions[LB_SPEAKERPHONE_RX_CODEC_FILTER_INDEX].action,reg, mask, val);
	temp =val;
	temp = temp>>6;
	temp = temp & 0x01;
	device_cal_data[18].codec_Rx_filter_left.index = LB_SPEAKERPHONE_RX_CODEC_FILTER_INDEX;
	device_cal_data[18].codec_Rx_filter_left.data = temp;	
	temp =val;
	temp = temp>>5;
	temp = temp & 0x01; 
	device_cal_data[18].codec_Rx_filter_right.index = LB_SPEAKERPHONE_RX_CODEC_FILTER_INDEX; 
	device_cal_data[18].codec_Rx_filter_right.data = temp;	

	printk(" LB Handset Rx left Gain = %x\n",device_cal_data[18].codec_Rx_left_gain.data);
	printk(" LB Handset Rx Left Filter status = %x\n",device_cal_data[18].codec_Rx_filter_left.data);
	printk(" LB Handset Rx Right Filter status = %x\n",device_cal_data[18].codec_Rx_filter_right.data);
/*[END] jy0127.jang@lge.com MS910_AUDIO_CHANGE 2011-04-07*/


#endif
 }
 
// EXPORT_SYMBOL(Snddev_Fill_Device_Data);
 
void Snddev_AudioCal_Set(struct msm_snd_set_audcal_param *aud_cal_data)
{
	struct  msm_snd_set_audcal_param *audcal_data_local = aud_cal_data;
	u8 reg,mask,val;
	u8 temp=0;
	printk("Voc Codec valiue = %d\n",(audcal_data_local->voc_codec));
	printk("Voccal param Type   = %d\n",(audcal_data_local->voccal_param_type));
	printk("Set / Get = %d\n",(audcal_data_local->get_flag));
	printk("Param Value = %d\n",(audcal_data_local->param_val));
	printk("Get Param Value = %d\n",(audcal_data_local->get_param));

	switch(audcal_data_local->voc_codec) {
/*************************************************** [0] Handset Rx**************************************************/
 		case ACDB_ID_HANDSET_SPKR:
		{
			ptr_icodec = &snddev_iearpiece_data;
			switch(aud_cal_data->voccal_param_type)
			{
				case CODEC_RX_LEFT:
					device_cal_data[0].codec_Rx_left_gain.data = (u8)aud_cal_data->param_val;
					ADIE_CODEC_UNPACK_ENTRY(iearpiece_48KHz_osr256_actions[17].action,reg, mask, val);
					val = (u8)aud_cal_data->param_val;
					printk("Set Rx gain left = %x\n",val);
					printk("Set Rx gain left index= %d\n",device_cal_data[0].codec_Rx_left_gain.index);
					iearpiece_48KHz_osr256_actions[HANDSET_RX_CODECGAIN_LEFT_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);						
					aud_cal_data->get_param  = aud_cal_data->param_val;						
					break;

				case CODEC_RX_FILTER_LEFT:
					device_cal_data[0].codec_Rx_filter_left.data = (u8)aud_cal_data->param_val;
					temp = (u8)aud_cal_data->param_val;
					ADIE_CODEC_UNPACK_ENTRY(iearpiece_48KHz_osr256_actions[HANDSET_RX_CODEC_FILTER_INDEX].action,reg, mask, val);
					if(temp ==0)
					{
						val = val & 0xBF;
					}
					else
					{
						val = val | 0x40;
					}
					printk("Set Rx Filter gain left = %x\n",val);
					printk("Set Rx Filter gain left index = %d\n",device_cal_data[0].codec_Rx_filter_left.index);
					iearpiece_48KHz_osr256_actions[HANDSET_RX_CODEC_FILTER_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);
					aud_cal_data->get_param  = aud_cal_data->param_val;
					break;
					
				case CODEC_RX_FILTER_RIGHT:
					device_cal_data[0].codec_Rx_filter_right.data = (u8)aud_cal_data->param_val;
					temp = (u8)aud_cal_data->param_val;
					ADIE_CODEC_UNPACK_ENTRY(iearpiece_48KHz_osr256_actions[HANDSET_RX_CODEC_FILTER_INDEX].action,reg, mask, val);
					if(temp ==0)
					{
						val = val & 0xDF;
					}
					else
					{
						val = val | 0x20;
					}
					printk("Set Rx Filter gain right = %x\n",val);
					printk("Set Rx Filter gain right index = %d\n",device_cal_data[0].codec_Rx_filter_right.index);
					iearpiece_48KHz_osr256_actions[HANDSET_RX_CODEC_FILTER_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);
					aud_cal_data->get_param  = aud_cal_data->param_val;
					break;

				case RX_VOLUME_MAX_NB:
					handset_rx[VOC_NB_INDEX].max_level = aud_cal_data->param_val;
					ptr_icodec->max_voice_rx_vol[VOC_NB_INDEX] = aud_cal_data->param_val;
					aud_cal_data->get_param  = aud_cal_data->param_val;
					break;
					
				case RX_VOLUME_MIN_NB:
					handset_rx[VOC_NB_INDEX].min_level = aud_cal_data->param_val;
					ptr_icodec->min_voice_rx_vol[VOC_NB_INDEX] = aud_cal_data->param_val;
					aud_cal_data->get_param  = aud_cal_data->param_val;					
					break;
					
				case RX_VOLUME_MAX_WB:
					handset_rx[VOC_WB_INDEX].max_level = aud_cal_data->param_val;
					ptr_icodec->max_voice_rx_vol[VOC_WB_INDEX] = aud_cal_data->param_val;
					aud_cal_data->get_param  = aud_cal_data->param_val;					
					break;

				case RX_VOLUME_MIN_WB:					
					handset_rx[VOC_WB_INDEX].min_level = aud_cal_data->param_val;
					ptr_icodec->min_voice_rx_vol[VOC_WB_INDEX] = aud_cal_data->param_val;
					aud_cal_data->get_param  = aud_cal_data->param_val;					
					break;

				case MIC_BIAS_0:
				case MIC_BIAS_1:
				case CODEC_RX_RIGHT:
				case CODEC_TX_LEFT:
				case CODEC_TX_RIGHT:
				case CODEC_ST:
				case MICAMP_GAIN_LEFT:
				case MICAMP_GAIN_RIGHT:
				case HEADPHONE_LEFT_GAIN:
				case HEADPHONE_RIGHT_GAIN:
//				case AUXPGA_LEFT_GAIN:
//				case AUXPGA_RIGHT_GAIN:
				default:
					aud_cal_data->get_param = 0x00;
					break;
							
			}
		}
		break;	
/*************************************************** [1] Handset Tx **************************************************/		
#if LGE_AUDIO_PATH
 		case ACDB_ID_HANDSET_MIC_ENDFIRE:
#else
 		case ACDB_ID_HANDSET_MIC:
#endif
		{
			switch(aud_cal_data->voccal_param_type)
			{
				case MIC_BIAS_0:
					mic_bias0_handset_mic = aud_cal_data->param_val;
					aud_cal_data->get_param = mic_bias0_handset_mic;
					break;
					
				case MIC_BIAS_1:
					mic_bias1_handset_mic = aud_cal_data->param_val;
					aud_cal_data->get_param = mic_bias1_handset_mic;	
					break;
					
				case CODEC_TX_LEFT:
					device_cal_data[1].codec_Tx_left_gain.data = (u8) aud_cal_data->param_val;
					ADIE_CODEC_UNPACK_ENTRY(imic_8KHz_osr256_actions[HANDSET_TX_CODECGAIN_LEFT_INDEX].action,reg, mask, val);
					val = (u8)aud_cal_data->param_val;
					printk("Set Tx Left Gain = %x\n",val);
					printk("Set Tx Left Gain index = %d\n",device_cal_data[1].codec_Tx_left_gain.index);
					imic_8KHz_osr256_actions[HANDSET_TX_CODECGAIN_LEFT_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);						
					aud_cal_data->get_param  = aud_cal_data->param_val;
					break;

				case CODEC_TX_RIGHT:
					device_cal_data[1].codec_Tx_right_gain.data = (u8) aud_cal_data->param_val;
					ADIE_CODEC_UNPACK_ENTRY(imic_8KHz_osr256_actions[HANDSET_TX_CODECGAIN_RIGHT_INDEX].action,reg, mask, val);
					val = (u8)aud_cal_data->param_val;
					printk("Set Tx Right Gain = %x\n",val);
					printk("Set Tx Right Gain index = %d\n",device_cal_data[1].codec_Tx_right_gain.index);
					imic_8KHz_osr256_actions[HANDSET_TX_CODECGAIN_RIGHT_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);						
					aud_cal_data->get_param  = aud_cal_data->param_val;
					break;


				case CODEC_ST:
					device_cal_data[1].codec_St_gain.data = (u8)aud_cal_data->param_val;
					ADIE_CODEC_UNPACK_ENTRY(imic_8KHz_osr256_actions[HANDSET_TX_CODECGAIN_ST_INDEX].action,reg, mask, val);
					val = (u8)aud_cal_data->param_val;
					printk("Set ST Gain = %x\n",val);
					printk("Set ST Gain index = %d\n",device_cal_data[1].codec_St_gain.index);
					imic_8KHz_osr256_actions[HANDSET_TX_CODECGAIN_ST_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);
					aud_cal_data->get_param  = aud_cal_data->param_val;
					break;

				case MICAMP_GAIN_LEFT:
					if(aud_cal_data->param_val == 0)
					{
						device_cal_data[1].codec_micamp_left.data = (u8)aud_cal_data->param_val;
						ADIE_CODEC_UNPACK_ENTRY(imic_8KHz_osr256_actions[HANDSET_TX_MICGAIN_LEFT_INDEX].action,reg, mask, val);
						val = val & 0x9F;
						printk("Set Micamp gain Left = %x\n",val);
						printk("Set Micamp gain Left index= %d\n",device_cal_data[1].codec_micamp_left.index);
						imic_8KHz_osr256_actions[HANDSET_TX_MICGAIN_LEFT_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);
						aud_cal_data->get_param  = aud_cal_data->param_val;
					}
					else
					{
						device_cal_data[1].codec_micamp_left.data = (u8)aud_cal_data->param_val;
						ADIE_CODEC_UNPACK_ENTRY(imic_8KHz_osr256_actions[HANDSET_TX_MICGAIN_LEFT_INDEX].action,reg, mask, val);
						val = val & 0xBF;
						temp = (u8)aud_cal_data->param_val;
						temp = temp & 0x03;
						temp = temp <<5;
						val = val | temp;
						printk("Set Micamp gain Left = %x\n",val);
						printk("Set Micamp gain Left index= %d\n",device_cal_data[1].codec_micamp_left.index);						
						imic_8KHz_osr256_actions[HANDSET_TX_MICGAIN_LEFT_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);
						aud_cal_data->get_param  = aud_cal_data->param_val;						
					}
					break;

				case MICAMP_GAIN_RIGHT:
					if(aud_cal_data->param_val == 0)
					{	
						device_cal_data[1].codec_micamp_right.data = (u8)aud_cal_data->param_val;
						ADIE_CODEC_UNPACK_ENTRY(imic_8KHz_osr256_actions[HANDSET_TX_MICGAIN_RIGHT_INDEX].action,reg, mask, val);
						val = val & 0x9F;
						printk("Set Micamp gain Right = %x\n",val);
						printk("Set Micamp gain Right index = %d\n",device_cal_data[1].codec_micamp_right.index);
						imic_8KHz_osr256_actions[HANDSET_TX_MICGAIN_RIGHT_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);
						aud_cal_data->get_param  = aud_cal_data->param_val;
					}
					else
					{
						device_cal_data[1].codec_micamp_right.data = (u8)aud_cal_data->param_val;
						ADIE_CODEC_UNPACK_ENTRY(imic_8KHz_osr256_actions[HANDSET_TX_MICGAIN_RIGHT_INDEX].action,reg, mask, val);
						val = val & 0xBF;
						temp = (u8)aud_cal_data->param_val;
						temp = temp & 0x03;
						temp = temp <<5;
						val = val | temp;
						printk("Set Micamp gain Right = %x\n",val);
						printk("Set Micamp gain Right index = %d\n",device_cal_data[1].codec_micamp_right.index);
						imic_8KHz_osr256_actions[HANDSET_TX_MICGAIN_RIGHT_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);
						aud_cal_data->get_param  = aud_cal_data->param_val;						
					}	
					break;
					
				case CODEC_RX_RIGHT:
				case CODEC_RX_LEFT:
				case HEADPHONE_LEFT_GAIN:
				case HEADPHONE_RIGHT_GAIN:
				case CODEC_RX_FILTER_LEFT:
				case CODEC_RX_FILTER_RIGHT:		
//				case AUXPGA_LEFT_GAIN:
//				case AUXPGA_RIGHT_GAIN:					
				default:
					aud_cal_data->get_param = 0x00;
					break;
								
			}
		}
		break;
/***************************************************** [2] Headset Rx Stereo*******************************************/
		case ACDB_ID_HEADSET_SPKR_STEREO:
		{
			ptr_icodec = &snddev_ihs_stereo_rx_data;
			
			switch(aud_cal_data->voccal_param_type)
			{

				case CODEC_RX_FILTER_LEFT:
					temp = (u8)aud_cal_data->param_val;
					device_cal_data[2].codec_Rx_filter_left.data = (u8)aud_cal_data->param_val;
					ADIE_CODEC_UNPACK_ENTRY(ihs_stereo_rx_48KHz_osr256_actions[HEADSET_STEREO_RX_CODEC_FILTER_INDEX].action,reg, mask, val);
					if(temp ==0)
					{
						val = val & 0xBF;
					}
					else
					{
						val = val | 0x40;
					}
					printk("Set Rx Filter gain left = %x\n",val);
					printk("Set Rx Filter gain left index= %d\n",device_cal_data[2].codec_Rx_filter_left.index);
					ihs_stereo_rx_48KHz_osr256_actions[HEADSET_STEREO_RX_CODEC_FILTER_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);
					aud_cal_data->get_param  = aud_cal_data->param_val;
					break;

				case CODEC_RX_FILTER_RIGHT:
					temp = (u8)aud_cal_data->param_val;
					device_cal_data[2].codec_Rx_filter_right.data = (u8)aud_cal_data->param_val;
					ADIE_CODEC_UNPACK_ENTRY(ihs_stereo_rx_48KHz_osr256_actions[HEADSET_STEREO_RX_CODEC_FILTER_INDEX].action,reg, mask, val);
					if(temp ==0)
					{
						val = val & 0xDF;
					}
					else
					{
						val = val | 0x20;
					}
					printk("Set Rx Filter gain Right = %x\n",val);
					printk("Set Rx Filter gain Right index= %d\n",device_cal_data[2].codec_Rx_filter_right.index);
					ihs_stereo_rx_48KHz_osr256_actions[HEADSET_STEREO_RX_CODEC_FILTER_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);
					aud_cal_data->get_param  = aud_cal_data->param_val;
					break;

				case HEADPHONE_LEFT_GAIN:
					device_cal_data[2].codec_headset_left.data = (u8)aud_cal_data->param_val;
					ADIE_CODEC_UNPACK_ENTRY(ihs_stereo_rx_48KHz_osr256_actions[HEADSET_STEREO_RX_HEADPHONEGAIN_LEFT_INDEX].action,reg, mask, val);
					//val = (u8)aud_cal_data->param_val;
					temp = (u8)aud_cal_data->param_val;
					temp = temp << 2;
					temp = temp & 0xFC;
					val = val & 0x03; 	
					val = val | temp;
					printk("Set Headset gain Left = %x\n",val);
					printk("Set Headset gain Left index = %d\n",device_cal_data[2].codec_headset_left.index);
					ihs_stereo_rx_48KHz_osr256_actions[HEADSET_STEREO_RX_HEADPHONEGAIN_LEFT_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);						
					aud_cal_data->get_param  = aud_cal_data->param_val;						
					break;

				case HEADPHONE_RIGHT_GAIN:
					device_cal_data[2].codec_headset_right.data = (u8)aud_cal_data->param_val;
					ADIE_CODEC_UNPACK_ENTRY(ihs_stereo_rx_48KHz_osr256_actions[HEADSET_STEREO_RX_HEADPHONEGAIN_RIGHT_INDEX].action,reg, mask, val);
					//val = (u8)aud_cal_data->param_val;
					temp = (u8)aud_cal_data->param_val;
					temp = temp << 2;
					temp = temp & 0xFC;
					val = val & 0x03; 	
					val = val | temp;
					printk("Set Headset gain Right = %x\n",val);			
					printk("Set Headset gain Right index = %d\n",device_cal_data[2].codec_headset_right);
					ihs_stereo_rx_48KHz_osr256_actions[HEADSET_STEREO_RX_HEADPHONEGAIN_RIGHT_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);						
					aud_cal_data->get_param  = aud_cal_data->param_val;						
					break;	
					
				case CODEC_RX_RIGHT:
					device_cal_data[2].codec_Rx_right_gain.data = (u8)aud_cal_data->param_val;
					ADIE_CODEC_UNPACK_ENTRY(ihs_stereo_rx_48KHz_osr256_actions[HEADSET_STEREO_RX_CODECGAIN_RIGHT_INDEX].action,reg, mask, val);
					val = (u8)aud_cal_data->param_val;
					printk("Set Headset Rx gain Right = %x\n",val);
					printk("Set Headset Rx gain Right index = %d\n",device_cal_data[2].codec_Rx_right_gain.index);
					ihs_stereo_rx_48KHz_osr256_actions[HEADSET_STEREO_RX_CODECGAIN_RIGHT_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);						
					aud_cal_data->get_param  = aud_cal_data->param_val;						
					break;
					
				case CODEC_RX_LEFT:
					device_cal_data[2].codec_Rx_left_gain.data = (u8)aud_cal_data->param_val;
					ADIE_CODEC_UNPACK_ENTRY(ihs_stereo_rx_48KHz_osr256_actions[HEADSET_STEREO_RX_CODECGAIN_LEFT_INDEX].action,reg, mask, val);
					val = (u8)aud_cal_data->param_val;
					printk("Set Headset Rx gain Left = %x\n",val);
					printk("Set Headset Rx gain Left index = %d\n",device_cal_data[2].codec_Rx_left_gain.index);
					ihs_stereo_rx_48KHz_osr256_actions[HEADSET_STEREO_RX_CODECGAIN_LEFT_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);						
					aud_cal_data->get_param  = aud_cal_data->param_val;						
					break;

				case RX_VOLUME_MAX_NB:
					headset_stereo_rx[VOC_NB_INDEX].max_level = aud_cal_data->param_val;
					ptr_icodec->max_voice_rx_vol[VOC_NB_INDEX] = aud_cal_data->param_val;
					aud_cal_data->get_param  = aud_cal_data->param_val;
					break;
					
				case RX_VOLUME_MIN_NB:
					headset_stereo_rx[VOC_NB_INDEX].min_level = aud_cal_data->param_val;
					ptr_icodec->min_voice_rx_vol[VOC_NB_INDEX] = aud_cal_data->param_val;
					aud_cal_data->get_param  = aud_cal_data->param_val;					
					break;
					
				case RX_VOLUME_MAX_WB:
					headset_stereo_rx[VOC_WB_INDEX].max_level = aud_cal_data->param_val;
					ptr_icodec->max_voice_rx_vol[VOC_WB_INDEX] = aud_cal_data->param_val;
					aud_cal_data->get_param  = aud_cal_data->param_val;					
					break;

				case RX_VOLUME_MIN_WB:					
					headset_stereo_rx[VOC_WB_INDEX].min_level = aud_cal_data->param_val;
					ptr_icodec->min_voice_rx_vol[VOC_WB_INDEX] = aud_cal_data->param_val;
					aud_cal_data->get_param  = aud_cal_data->param_val;					
					break;

				case CODEC_TX_LEFT:
				case CODEC_TX_RIGHT:
				case CODEC_ST:
				case MICAMP_GAIN_LEFT:
				case MICAMP_GAIN_RIGHT:
				case MIC_BIAS_0:
				case MIC_BIAS_1:
//				case AUXPGA_LEFT_GAIN:
//				case AUXPGA_RIGHT_GAIN:					
				default:
					aud_cal_data->get_param = 0x00;
					break;
								
			}
		}
		break;	
/*************************************************** [3] Headset Rx Mono ********************************************/
		case ACDB_ID_HEADSET_SPKR_MONO:
		{
			ptr_icodec = &snddev_ihs_mono_rx_data;
			
			switch(aud_cal_data->voccal_param_type)
			{
				case CODEC_RX_FILTER_LEFT:
					temp = (u8)aud_cal_data->param_val;
					device_cal_data[3].codec_Rx_filter_left.data = (u8)aud_cal_data->param_val;
					ADIE_CODEC_UNPACK_ENTRY(ihs_mono_rx_48KHz_osr256_actions[HEADSET_MONO_RX_CODEC_FILTER_INDEX].action,reg, mask, val);
					if(temp ==0)
					{
						val = val & 0xBF;
					}
					else
					{
						val = val | 0x40;
					}
					printk("Set Rx Filter gain left = %x\n",val);
					printk("Set Rx Filter gain left index= %d\n",device_cal_data[3].codec_Rx_filter_left.index);
					ihs_mono_rx_48KHz_osr256_actions[HEADSET_MONO_RX_CODEC_FILTER_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);
					aud_cal_data->get_param  = aud_cal_data->param_val;
					break;

				case CODEC_RX_FILTER_RIGHT:
					temp =(u8)aud_cal_data->param_val;
					device_cal_data[3].codec_Rx_filter_right.data = (u8)aud_cal_data->param_val;
					ADIE_CODEC_UNPACK_ENTRY(ihs_mono_rx_48KHz_osr256_actions[HEADSET_MONO_RX_CODEC_FILTER_INDEX].action,reg, mask, val);
					if(temp ==0)
					{
						val = val & 0xDF;
					}
					else
					{
						val = val | 0x20;
					}
					printk("Set Rx Filter gain right = %x\n",val);
					printk("Set Rx Filter gain right index= %d\n",device_cal_data[3].codec_Rx_filter_right.index);
					ihs_mono_rx_48KHz_osr256_actions[HEADSET_MONO_RX_CODEC_FILTER_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);
					aud_cal_data->get_param  = aud_cal_data->param_val;
					break;

				case HEADPHONE_LEFT_GAIN:
					device_cal_data[3].codec_headset_left.data = (u8)aud_cal_data->param_val;
					ADIE_CODEC_UNPACK_ENTRY(ihs_mono_rx_48KHz_osr256_actions[HEADSET_MONO_RX_HEADPHONEGAIN_LEFT_INDEX].action,reg, mask, val);
					//val = (u8)aud_cal_data->param_val;
					temp = (u8)aud_cal_data->param_val;
					temp = temp << 2;
					temp = temp & 0xFC;
					val = val & 0x03; 	
					val = val | temp;
					printk("Set Headset  gain left = %x\n",val);
					printk("Set Headset  gain left index= %d\n",device_cal_data[3].codec_headset_left.index);
					ihs_mono_rx_48KHz_osr256_actions[HEADSET_MONO_RX_HEADPHONEGAIN_LEFT_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);						
					aud_cal_data->get_param  = aud_cal_data->param_val;						
					break;

				case HEADPHONE_RIGHT_GAIN:
					device_cal_data[3].codec_headset_right.data = (u8)aud_cal_data->param_val;
					ADIE_CODEC_UNPACK_ENTRY(ihs_mono_rx_48KHz_osr256_actions[HEADSET_MONO_RX_HEADPHONEGAIN_RIGHT_INDEX].action,reg, mask, val);
					//val = (u8)aud_cal_data->param_val;
					temp = (u8)aud_cal_data->param_val;
					temp = temp << 2;
					temp = temp & 0xFC;
					val = val & 0x03; 	
					val = val | temp;
					printk("Set Headset gain right = %x\n",val);
					printk("Set Headset gain right index= %d\n",device_cal_data[3].codec_headset_right.index);
					ihs_mono_rx_48KHz_osr256_actions[HEADSET_MONO_RX_HEADPHONEGAIN_RIGHT_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);						
					aud_cal_data->get_param  = aud_cal_data->param_val;						
					break;	
// BEGIN :: srinivas.mittapalli@lge.com 2010-10-26 Removed Right Channel Gain for Headset Mono
#if 0
				case CODEC_RX_RIGHT:
					device_cal_data[3].codec_Rx_right_gain.data = (u8)aud_cal_data->param_val;
					ADIE_CODEC_UNPACK_ENTRY(ihs_mono_rx_48KHz_osr256_actions[HEADSET_MONO_RX_CODECGAIN_RIGHT_INDEX].action,reg, mask, val);
					val = (u8)aud_cal_data->param_val;
					printk("Set Rx gain right = %x\n",val);
					printk("Set Rx gain right index= %d\n",device_cal_data[3].codec_Rx_right_gain.index);					
					ihs_mono_rx_48KHz_osr256_actions[HEADSET_MONO_RX_CODECGAIN_RIGHT_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);						
					aud_cal_data->get_param  = aud_cal_data->param_val;						
					break;
#endif 					
				case CODEC_RX_LEFT:
					device_cal_data[3].codec_Rx_left_gain.data = (u8)aud_cal_data->param_val;
					ADIE_CODEC_UNPACK_ENTRY(ihs_mono_rx_48KHz_osr256_actions[HEADSET_MONO_RX_CODECGAIN_LEFT_INDEX].action,reg, mask, val);
					val = (u8)aud_cal_data->param_val;
					printk("Set Rx gain left = %x\n",val);
					printk("Set Rx gain left index= %d\n",device_cal_data[3].codec_Rx_left_gain.index);
					ihs_mono_rx_48KHz_osr256_actions[HEADSET_MONO_RX_CODECGAIN_LEFT_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);						
					aud_cal_data->get_param  = aud_cal_data->param_val;						
					break;
					

				case RX_VOLUME_MAX_NB:
					headset_mono_rx[VOC_NB_INDEX].max_level = aud_cal_data->param_val;
					ptr_icodec->max_voice_rx_vol[VOC_NB_INDEX] = aud_cal_data->param_val;
					aud_cal_data->get_param  = aud_cal_data->param_val;
					break;
					
				case RX_VOLUME_MIN_NB:
					headset_mono_rx[VOC_NB_INDEX].min_level = aud_cal_data->param_val;
					ptr_icodec->min_voice_rx_vol[VOC_NB_INDEX] = aud_cal_data->param_val;
					aud_cal_data->get_param  = aud_cal_data->param_val;					
					break;
					
				case RX_VOLUME_MAX_WB:
					headset_mono_rx[VOC_WB_INDEX].max_level = aud_cal_data->param_val;
					ptr_icodec->max_voice_rx_vol[VOC_WB_INDEX] = aud_cal_data->param_val;
					aud_cal_data->get_param  = aud_cal_data->param_val;					
					break;

				case RX_VOLUME_MIN_WB:					
					headset_mono_rx[VOC_WB_INDEX].min_level = aud_cal_data->param_val;
					ptr_icodec->min_voice_rx_vol[VOC_WB_INDEX] = aud_cal_data->param_val;
					aud_cal_data->get_param  = aud_cal_data->param_val;					
					break;

				case CODEC_TX_LEFT:
				case CODEC_TX_RIGHT:
				case CODEC_ST:
				case MICAMP_GAIN_LEFT:
				case MICAMP_GAIN_RIGHT:
				case MIC_BIAS_0:
				case MIC_BIAS_1:	
//				case AUXPGA_LEFT_GAIN:
//				case AUXPGA_RIGHT_GAIN:
				case CODEC_RX_RIGHT:
				default:
						aud_cal_data->get_param = 0x00;
						break;

								
			}
		}
		break;	
/******************************************************* [4] Headset Tx********************************************/		
#if LGE_AUDIO_PATH
 		case ACDB_ID_HANDSET_MIC: //4
#else
 		case ACDB_ID_HEADSET_MIC:
#endif
		{
			switch(aud_cal_data->voccal_param_type)
			{

				case MICAMP_GAIN_LEFT:
					if(aud_cal_data->param_val == 0)
					{
						device_cal_data[4].codec_micamp_left.data = (u8)aud_cal_data->param_val;
						ADIE_CODEC_UNPACK_ENTRY(ihs_mono_tx_48KHz_osr256_actions[HEADSET_TX_MICGAIN_LEFT_INDEX].action,reg, mask, val);
						val = val & 0x9F;
						printk("Set Mic Amp gain Left = %x\n",val);
						printk("Set Mic Amp gain Left index= %d\n",device_cal_data[4].codec_micamp_left.index);
						ihs_mono_tx_48KHz_osr256_actions[HEADSET_TX_MICGAIN_LEFT_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);
						aud_cal_data->get_param  = aud_cal_data->param_val;
					}
					else
					{
						device_cal_data[4].codec_micamp_left.data = (u8)aud_cal_data->param_val;
						ADIE_CODEC_UNPACK_ENTRY(ihs_mono_tx_48KHz_osr256_actions[HEADSET_TX_MICGAIN_LEFT_INDEX].action,reg, mask, val);
						val = val & 0xBF;
						temp = (u8)aud_cal_data->param_val;
						temp = temp & 0x03;
						temp = temp <<5;
						val = val | temp;
						printk("Set Mic Amp gain Left = %x\n",val);
						printk("Set Mic Amp gain Left index= %d\n",device_cal_data[4].codec_micamp_left.index);
						ihs_mono_tx_48KHz_osr256_actions[HEADSET_TX_MICGAIN_LEFT_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);
						aud_cal_data->get_param  = aud_cal_data->param_val;						
					}
					break;

				case CODEC_ST:
					device_cal_data[4].codec_St_gain.data = (u8)aud_cal_data->param_val;
					ADIE_CODEC_UNPACK_ENTRY(ihs_mono_tx_48KHz_osr256_actions[HEADSET_TX_CODECGAIN_ST_INDEX].action,reg, mask, val);
					val = (u8)aud_cal_data->param_val;
					printk("Set ST gain  = %x\n",val);					
					printk("Set ST gain  index= %d\n",device_cal_data[4].codec_St_gain.index);										
					ihs_mono_tx_48KHz_osr256_actions[HEADSET_TX_CODECGAIN_ST_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);
					aud_cal_data->get_param  = aud_cal_data->param_val;
					break;
					
				case CODEC_TX_LEFT:
					device_cal_data[4].codec_Tx_left_gain.data = aud_cal_data->param_val;
					ADIE_CODEC_UNPACK_ENTRY(ihs_mono_tx_48KHz_osr256_actions[HEADSET_TX_CODECGAIN_LEFT_INDEX].action,reg, mask, val);
					val = (u8)aud_cal_data->param_val;
					printk("Set Tx gain Left = %x\n",val);	
					printk("Set Tx gain Left index = %d\n",device_cal_data[4].codec_Tx_left_gain.index);	
					ihs_mono_tx_48KHz_osr256_actions[HEADSET_TX_CODECGAIN_LEFT_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);						
					aud_cal_data->get_param  = aud_cal_data->param_val;
					break;
				
				case CODEC_RX_RIGHT:
				case CODEC_RX_LEFT:
				case MICAMP_GAIN_RIGHT:
				case CODEC_TX_RIGHT:
				case HEADPHONE_LEFT_GAIN:
				case HEADPHONE_RIGHT_GAIN:
				case CODEC_RX_FILTER_LEFT:
				case CODEC_RX_FILTER_RIGHT:					
				case MIC_BIAS_0:
				case MIC_BIAS_1:	
//				case AUXPGA_LEFT_GAIN:
//				case AUXPGA_RIGHT_GAIN:					
				default:
						aud_cal_data->get_param = 0x00;
						break;

								
			}
		}
		break;

/* BEGIN:0010882        ehgrace.kim@lge.com     2010.11.15*/
/* MOD: add add the subsystem control for call mode */
/***************************************************** [5] Headset Phone Tx******************************************/
/*BEGIN: 0012454 kiran.kanneganti@lge.com 2010-12-15*/
/*Mod 0012454: Changed headset mode actions for call. Call use 8K actions*/

/*BEGIN: 0016769 kiran.kanneganti@lge.com 2011-02-22*/
/*MOD 0016769:	add 48k actions for tuning in hidden menu*/
 		case ACDB_ID_HEADSET_MIC:
		{
			switch(aud_cal_data->voccal_param_type)
			{

				case MICAMP_GAIN_LEFT:
					if(aud_cal_data->param_val == 0)
					{
						u8 reg2 = 0,mask2 = 0,val2 = 0;
						device_cal_data[5].codec_micamp_left.data = (u8)aud_cal_data->param_val;
						ADIE_CODEC_UNPACK_ENTRY(ihs_phone_tx_8KHz_osr256_actions[HEADSET_TX_MICGAIN_LEFT_INDEX].action,reg, mask, val);
						ADIE_CODEC_UNPACK_ENTRY(ihs_phone_tx_48KHz_osr256_actions[HEADSET_TX_MICGAIN_LEFT_INDEX].action,reg2,mask2,val2);
						val = val & 0x9F;
						val2 = val2 & 0x9F;
						printk("Set Mic Amp gain Left = %x\n",val);
						printk("Set Mic Amp gain Left index= %d\n",device_cal_data[5].codec_micamp_left.index);
						ihs_phone_tx_8KHz_osr256_actions[HEADSET_TX_MICGAIN_LEFT_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);
						ihs_phone_tx_48KHz_osr256_actions[HEADSET_TX_MICGAIN_LEFT_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg2,mask2,val2);
						aud_cal_data->get_param  = aud_cal_data->param_val;
					}
					else
					{
						u8 reg2 = 0,mask2 = 0,val2 = 0;
						device_cal_data[5].codec_micamp_left.data = (u8)aud_cal_data->param_val;
						ADIE_CODEC_UNPACK_ENTRY(ihs_phone_tx_8KHz_osr256_actions[HEADSET_TX_MICGAIN_LEFT_INDEX].action,reg, mask, val);
						ADIE_CODEC_UNPACK_ENTRY(ihs_phone_tx_48KHz_osr256_actions[HEADSET_TX_MICGAIN_LEFT_INDEX].action,reg2,mask2,val2);
						val = val & 0xBF;
						val2 = val2 & 0xBF; 
						temp = (u8)aud_cal_data->param_val;
						temp = temp & 0x03;
						temp = temp <<5;
						val = val | temp;
						val2 = val2 | temp;
						printk("Set Mic Amp gain Left = %x\n",val);
						printk("Set Mic Amp gain Left index= %d\n",device_cal_data[5].codec_micamp_left.index);
						ihs_phone_tx_8KHz_osr256_actions[HEADSET_TX_MICGAIN_LEFT_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);
						ihs_phone_tx_48KHz_osr256_actions[HEADSET_TX_MICGAIN_LEFT_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg2,mask2,val2);
						aud_cal_data->get_param  = aud_cal_data->param_val;						
					}
					break;

				case CODEC_ST:
					{
						u8 reg2 = 0,mask2 = 0,val2 = 0;
						device_cal_data[5].codec_St_gain.data = (u8)aud_cal_data->param_val;
						ADIE_CODEC_UNPACK_ENTRY(ihs_phone_tx_8KHz_osr256_actions[HEADSET_TX_CODECGAIN_ST_INDEX].action,reg, mask, val);
						ADIE_CODEC_UNPACK_ENTRY(ihs_phone_tx_48KHz_osr256_actions[HEADSET_TX_CODECGAIN_ST_INDEX].action,reg2,mask2,val2);
						val = (u8)aud_cal_data->param_val;
						val2 = (u8)aud_cal_data->param_val;
						printk("Set ST gain  = %x\n",val);					
						printk("Set ST gain  index= %d\n",device_cal_data[5].codec_St_gain.index);										
						ihs_phone_tx_8KHz_osr256_actions[HEADSET_TX_CODECGAIN_ST_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);
						ihs_phone_tx_48KHz_osr256_actions[HEADSET_TX_CODECGAIN_ST_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg2,mask2,val2);
						aud_cal_data->get_param  = aud_cal_data->param_val;
					}
					break;
					
				case CODEC_TX_LEFT:
					{
						u8 reg2 = 0,mask2 = 0,val2 = 0;
						device_cal_data[5].codec_Tx_left_gain.data = aud_cal_data->param_val;
						ADIE_CODEC_UNPACK_ENTRY(ihs_phone_tx_8KHz_osr256_actions[HEADSET_TX_CODECGAIN_LEFT_INDEX].action,reg, mask, val);
						ADIE_CODEC_UNPACK_ENTRY(ihs_phone_tx_48KHz_osr256_actions[HEADSET_TX_CODECGAIN_LEFT_INDEX].action,reg2,mask2,val2);
						val = (u8)aud_cal_data->param_val;
						val2 = (u8)aud_cal_data->param_val;
						printk("Set Tx gain Left = %x\n",val);	
						printk("Set Tx gain Left index = %d\n",device_cal_data[5].codec_Tx_left_gain.index);	
						ihs_phone_tx_8KHz_osr256_actions[HEADSET_TX_CODECGAIN_LEFT_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);	
						ihs_phone_tx_48KHz_osr256_actions[HEADSET_TX_CODECGAIN_LEFT_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg2,mask2,val2);
						aud_cal_data->get_param  = aud_cal_data->param_val;
					}
					break;
				
				case CODEC_RX_RIGHT:
				case CODEC_RX_LEFT:
				case MICAMP_GAIN_RIGHT:
				case CODEC_TX_RIGHT:
				case HEADPHONE_LEFT_GAIN:
				case HEADPHONE_RIGHT_GAIN:
				case CODEC_RX_FILTER_LEFT:
				case CODEC_RX_FILTER_RIGHT:					
				case MIC_BIAS_0:
				case MIC_BIAS_1:	
//				case AUXPGA_LEFT_GAIN:
//				case AUXPGA_RIGHT_GAIN:					
				default:
						aud_cal_data->get_param = 0x00;
						break;

								
			}
		}
		break;
/* END: 0016769 kiran.kanneganti@lge.com 2011-02-22 */

/*END: 0012454 kiran.kanneganti@lge.com 2010-12-15*/		
/**************************************************** [6] Speaker Rx Stereo for Mono***********************************/
		case ACDB_ID_SPKR_PHONE_STEREO:
		{
			ptr_icodec = &snddev_ispeaker_rx_data;
			switch(aud_cal_data->voccal_param_type)
			{
				case CODEC_RX_FILTER_LEFT:
					temp = (u8)aud_cal_data->param_val;
					device_cal_data[6].codec_Rx_filter_left.data = (u8)aud_cal_data->param_val;
					ADIE_CODEC_UNPACK_ENTRY(ispeaker_rx_48KHz_osr256_actions[SPK_RX_CODEC_FILTER_INDEX].action,reg, mask, val);
					if(temp ==0)
					{
						val = val & 0xBF;
					}
					else
					{
						val = val | 0x40;
					}
					printk("Set Rx Filter Left = %x\n",val);
					printk("Set Rx Filter Left index = %d\n",device_cal_data[6].codec_Rx_filter_left.index);
					ispeaker_rx_48KHz_osr256_actions[SPK_RX_CODEC_FILTER_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);
					aud_cal_data->get_param  = aud_cal_data->param_val;
					break;

				case CODEC_RX_FILTER_RIGHT:
					temp = (u8)aud_cal_data->param_val;
					device_cal_data[6].codec_Rx_filter_right.data = (u8)aud_cal_data->param_val;
					ADIE_CODEC_UNPACK_ENTRY(ispeaker_rx_48KHz_osr256_actions[SPK_RX_CODEC_FILTER_INDEX].action,reg, mask, val);
					if(temp ==0)
					{
						val = val & 0xDF;
					}
					else
					{
						val = val | 0x20;
					}
					printk("Set Rx Filter Right = %x\n",val);		
					printk("Set Rx Filter Right index = %d\n",device_cal_data[6].codec_Rx_filter_right.index);	
					ispeaker_rx_48KHz_osr256_actions[SPK_RX_CODEC_FILTER_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);
					aud_cal_data->get_param  = aud_cal_data->param_val;
					break;				

				case CODEC_RX_LEFT:
					device_cal_data[6].codec_Rx_left_gain.data = (u8)aud_cal_data->param_val;
					ADIE_CODEC_UNPACK_ENTRY(ispeaker_rx_48KHz_osr256_actions[SPK_RX_CODECGAIN_LEFT_INDEX].action,reg, mask, val);
					val = (u8)aud_cal_data->param_val;
					printk("Set Rx Left = %x\n",val);		
					printk("Set Rx Left index = %d\n",device_cal_data[6].codec_Rx_left_gain.index);	
					ispeaker_rx_48KHz_osr256_actions[SPK_RX_CODECGAIN_LEFT_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);						
					aud_cal_data->get_param  = aud_cal_data->param_val;						
					break;
				
				case CODEC_RX_RIGHT:
					device_cal_data[6].codec_Rx_right_gain.data = (u8)aud_cal_data->param_val;
					ADIE_CODEC_UNPACK_ENTRY(ispeaker_rx_48KHz_osr256_actions[SPK_RX_CODECGAIN_RIGHT_INDEX].action,reg, mask, val);
					val = (u8)aud_cal_data->param_val;
					printk("Set Rx Right = %x\n",val);
					printk("Set Rx Right index= %d\n",device_cal_data[6].codec_Rx_right_gain.index);
					ispeaker_rx_48KHz_osr256_actions[SPK_RX_CODECGAIN_RIGHT_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);						
					aud_cal_data->get_param  = aud_cal_data->param_val;						
					break;

				case RX_VOLUME_MAX_NB:
					spk_rx[VOC_NB_INDEX].max_level = aud_cal_data->param_val;
					ptr_icodec->max_voice_rx_vol[VOC_NB_INDEX] = aud_cal_data->param_val;
					aud_cal_data->get_param  = aud_cal_data->param_val;
					break;
					
				case RX_VOLUME_MIN_NB:
					spk_rx[VOC_NB_INDEX].min_level = aud_cal_data->param_val;
					ptr_icodec->min_voice_rx_vol[VOC_NB_INDEX] = aud_cal_data->param_val;
					aud_cal_data->get_param  = aud_cal_data->param_val;					
					break;
					
				case RX_VOLUME_MAX_WB:
					spk_rx[VOC_WB_INDEX].max_level = aud_cal_data->param_val;
					ptr_icodec->max_voice_rx_vol[VOC_WB_INDEX] = aud_cal_data->param_val;
					aud_cal_data->get_param  = aud_cal_data->param_val;					
					break;

				case RX_VOLUME_MIN_WB:					
					spk_rx[VOC_WB_INDEX].min_level = aud_cal_data->param_val;
					ptr_icodec->min_voice_rx_vol[VOC_WB_INDEX] = aud_cal_data->param_val;
					aud_cal_data->get_param  = aud_cal_data->param_val;					
					break;

				case MIC_BIAS_0:
				case MIC_BIAS_1:	
				case CODEC_TX_LEFT:
				case CODEC_TX_RIGHT:
				case CODEC_ST:
				case MICAMP_GAIN_LEFT:
				case MICAMP_GAIN_RIGHT:
				case HEADPHONE_LEFT_GAIN:
				case HEADPHONE_RIGHT_GAIN:
//				case AUXPGA_LEFT_GAIN:
//				case AUXPGA_RIGHT_GAIN:					
				default:
						aud_cal_data->get_param = 0x00;
						break;
								
			}
		}
		break;	
/***************************************************** [7] Speaker Tx*************************************************/
/* BEGIN:0012384 ehgrace.kim@lge.com     2010.12.14*/
/* MOD: [Audio]change tx gain for recording mode & delete the dummy src*/
 		case ACDB_ID_FM_TX:
		{
			switch(aud_cal_data->voccal_param_type)
			{
				case MIC_BIAS_0:
					mic_bias0_spkmic= aud_cal_data->param_val;
					aud_cal_data->get_param = mic_bias0_spkmic;
					break;
					
				case MIC_BIAS_1:
					mic_bias1_spkmic= aud_cal_data->param_val;
					aud_cal_data->get_param = mic_bias1_spkmic;
					break;
				case MICAMP_GAIN_LEFT:
					if(aud_cal_data->param_val == 0)
					{
						device_cal_data[7].codec_micamp_left.data = (u8)aud_cal_data->param_val;
						ADIE_CODEC_UNPACK_ENTRY(ispeaker_tx_48KHz_osr256_actions[SPK_TX_MICGAIN_LEFT_INDEX].action,reg, mask, val);
						val = val & 0x9F;
						printk("Set MicAmp Left = %x\n",val);
						printk("Set MicAmp Left index= %d\n",device_cal_data[7].codec_micamp_left.index);
						ispeaker_tx_48KHz_osr256_actions[SPK_TX_MICGAIN_LEFT_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);
						aud_cal_data->get_param  = aud_cal_data->param_val;
					}
					else
					{
						u8 temp;
						device_cal_data[7].codec_micamp_left.data = (u8)aud_cal_data->param_val;
						ADIE_CODEC_UNPACK_ENTRY(ispeaker_tx_48KHz_osr256_actions[SPK_TX_MICGAIN_LEFT_INDEX].action,reg, mask, val);
						val = val & 0x9F;
						temp = (u8)aud_cal_data->param_val;
						temp = temp & 0x03;
						temp = temp <<5;
						val = val | temp;
						printk("Set MicAmp Left = %x\n",val);
						printk("Set MicAmp Left index= %d\n",device_cal_data[7].codec_micamp_left.index);
						ispeaker_tx_48KHz_osr256_actions[SPK_TX_MICGAIN_LEFT_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);
						aud_cal_data->get_param  = aud_cal_data->param_val;						
					}
					break;	
				case CODEC_TX_LEFT:
					device_cal_data[7].codec_Tx_left_gain.data = (u8)aud_cal_data->param_val;
					ADIE_CODEC_UNPACK_ENTRY(ispeaker_tx_48KHz_osr256_actions[SPK_TX_CODECGAIN_LEFT_INDEX].action,reg, mask, val);
					val = (u8)aud_cal_data->param_val;
					printk("Set Tx Gain Left = %x\n",val);
					printk("Set Tx Gain Left index= %d\n",device_cal_data[7].codec_Tx_left_gain.index);
					ispeaker_tx_48KHz_osr256_actions[SPK_TX_CODECGAIN_LEFT_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);						
					aud_cal_data->get_param  = aud_cal_data->param_val;
					break;
				case CODEC_ST:
				case CODEC_TX_RIGHT:
				case CODEC_RX_RIGHT:
				case CODEC_RX_LEFT:
				case HEADPHONE_LEFT_GAIN:
				case HEADPHONE_RIGHT_GAIN:
				case CODEC_RX_FILTER_LEFT:
				case CODEC_RX_FILTER_RIGHT:	
//				case AUXPGA_LEFT_GAIN:
//				case AUXPGA_RIGHT_GAIN:	
				default:
						aud_cal_data->get_param = 0x00;					
						break;

								
			}
		}
		break;
/* END:0012384 ehgrace.kim@lge.com     2010.12.14*/
/**************************************** [8] Speaker Phone Rx****************************************************/
		case ACDB_ID_SPKR_PHONE_MONO:
		{
			ptr_icodec = &snddev_ispeaker_phone_rx_data;
			
			switch(aud_cal_data->voccal_param_type)
			{

				case CODEC_RX_FILTER_LEFT:
				{
					temp = (u8)aud_cal_data->param_val;
					device_cal_data[8].codec_Rx_filter_left.data = (u8)aud_cal_data->param_val;
					ADIE_CODEC_UNPACK_ENTRY(ispeaker_phone_rx_48KHz_osr256_actions[SPK_RX_CODEC_FILTER_INDEX].action,reg, mask, val);
					if(temp ==0)
					{
						val = val & 0xBF;
					}
					else
					{
						val = val | 0x40;
					}
					printk("Set Rx Filter Left = %x\n",val);
					printk("Set Rx Filter Left index = %d\n",device_cal_data[8].codec_Rx_filter_left.index);
					ispeaker_phone_rx_48KHz_osr256_actions[SPK_RX_CODEC_FILTER_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);
					aud_cal_data->get_param  = aud_cal_data->param_val;
				}
				break;

				case CODEC_RX_FILTER_RIGHT:
					{
						temp = (u8)aud_cal_data->param_val;
						device_cal_data[8].codec_Rx_filter_right.data = (u8)aud_cal_data->param_val;
						ADIE_CODEC_UNPACK_ENTRY(ispeaker_phone_rx_48KHz_osr256_actions[SPK_RX_CODEC_FILTER_INDEX].action,reg, mask, val);
						if(temp ==0)
						{
							val = val & 0xDF;
						}
						else
						{
							val = val | 0x20;
						}
						printk("Set Rx Filter Right = %x\n",val);		
						printk("Set Rx Filter Right index = %d\n",device_cal_data[8].codec_Rx_filter_right.index);	
						ispeaker_phone_rx_48KHz_osr256_actions[SPK_RX_CODEC_FILTER_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);
						aud_cal_data->get_param  = aud_cal_data->param_val;
					}	
					break;				

				case CODEC_RX_LEFT:
					device_cal_data[8].codec_Rx_left_gain.data = (u8)aud_cal_data->param_val;
					ADIE_CODEC_UNPACK_ENTRY(ispeaker_phone_rx_48KHz_osr256_actions[SPK_RX_CODECGAIN_LEFT_INDEX].action,reg, mask, val);
					val = (u8)aud_cal_data->param_val;
					printk("Set Rx Left = %x\n",val);		
					printk("Set Rx Left index = %d\n",device_cal_data[8].codec_Rx_left_gain.index);	
/*BEGIN:0012119 	ehgrace.kim@lge.com	2010.12.09*/
/*MOD:fix the bug for rx gain */
					ispeaker_phone_rx_48KHz_osr256_actions[SPK_RX_CODECGAIN_LEFT_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);						
/*END:0012119 	ehgrace.kim@lge.com	2010.12.09*/
					aud_cal_data->get_param  = aud_cal_data->param_val;						
					break;
				
				case CODEC_RX_RIGHT:
					device_cal_data[8].codec_Rx_right_gain.data = (u8)aud_cal_data->param_val;
					ADIE_CODEC_UNPACK_ENTRY(ispeaker_phone_rx_48KHz_osr256_actions[SPK_RX_CODECGAIN_RIGHT_INDEX].action,reg, mask, val);
					val = (u8)aud_cal_data->param_val;
					printk("Set Rx Right = %x\n",val);
					printk("Set Rx Right index= %d\n",device_cal_data[8].codec_Rx_right_gain.index);
					ispeaker_phone_rx_48KHz_osr256_actions[SPK_RX_CODECGAIN_RIGHT_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);						
					aud_cal_data->get_param  = aud_cal_data->param_val;						
					break;

				case RX_VOLUME_MAX_NB:
					spk_stereo_rx[VOC_NB_INDEX].max_level = aud_cal_data->param_val;
					ptr_icodec->max_voice_rx_vol[VOC_NB_INDEX] = aud_cal_data->param_val;
					aud_cal_data->get_param  = aud_cal_data->param_val;
					break;
					
				case RX_VOLUME_MIN_NB:
					spk_stereo_rx[VOC_NB_INDEX].min_level = aud_cal_data->param_val;
					ptr_icodec->min_voice_rx_vol[VOC_NB_INDEX] = aud_cal_data->param_val;
					aud_cal_data->get_param  = aud_cal_data->param_val;					
					break;
					
				case RX_VOLUME_MAX_WB:
					spk_stereo_rx[VOC_WB_INDEX].max_level = aud_cal_data->param_val;
					ptr_icodec->max_voice_rx_vol[VOC_WB_INDEX] = aud_cal_data->param_val;
					aud_cal_data->get_param  = aud_cal_data->param_val;					
					break;

				case RX_VOLUME_MIN_WB:					
					spk_stereo_rx[VOC_WB_INDEX].min_level = aud_cal_data->param_val;
					ptr_icodec->min_voice_rx_vol[VOC_WB_INDEX] = aud_cal_data->param_val;
					aud_cal_data->get_param  = aud_cal_data->param_val;					
					break;

				case MIC_BIAS_0:
				case MIC_BIAS_1:	
				case CODEC_TX_LEFT:
				case CODEC_TX_RIGHT:
				case CODEC_ST:
				case MICAMP_GAIN_LEFT:
				case MICAMP_GAIN_RIGHT:
				case HEADPHONE_LEFT_GAIN:
				case HEADPHONE_RIGHT_GAIN:
//				case AUXPGA_LEFT_GAIN:
//				case AUXPGA_RIGHT_GAIN:					
				default:
					aud_cal_data->get_param = 0x00;
					break;
			}
		}
		break;
/******************************************** [9] Speaker Phone Mic**************************************************/
 		case ACDB_ID_SPKR_PHONE_MIC:
		{
			switch(aud_cal_data->voccal_param_type)
			{
				case MIC_BIAS_0:
					mic_bias0_spkmic2= aud_cal_data->param_val;
					aud_cal_data->get_param = mic_bias0_spkmic2;
					break;
					
				case MIC_BIAS_1:
					mic_bias1_spkmic2= aud_cal_data->param_val;
					aud_cal_data->get_param = mic_bias1_spkmic2;
					break;
				case MICAMP_GAIN_LEFT:
					if(aud_cal_data->param_val == 0)
					{
						device_cal_data[9].codec_micamp_left.data = (u8)aud_cal_data->param_val;
						ADIE_CODEC_UNPACK_ENTRY(ispeaker_phone_tx_8KHz_osr256_actions[SPK_PHONE_TX_MICGAIN_LEFT_INDEX].action,reg, mask, val);
						val = val & 0x9F;
						printk("Set MicAmp Left = %x\n",val);
						printk("Set MicAmp Left index= %d\n",device_cal_data[9].codec_micamp_left.index);
						ispeaker_phone_tx_8KHz_osr256_actions[SPK_PHONE_TX_MICGAIN_LEFT_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);
						aud_cal_data->get_param  = aud_cal_data->param_val;
					}
					else
					{
						u8 temp;
						device_cal_data[9].codec_micamp_left.data = (u8)aud_cal_data->param_val;
						ADIE_CODEC_UNPACK_ENTRY(ispeaker_phone_tx_8KHz_osr256_actions[SPK_PHONE_TX_MICGAIN_LEFT_INDEX].action,reg, mask, val);
						val = val & 0x9F;
						temp = (u8)aud_cal_data->param_val;
						temp = temp & 0x03;
						temp = temp <<5;
						val = val | temp;
						printk("Set MicAmp Left = %x\n",val);
						printk("Set MicAmp Left index= %d\n",device_cal_data[9].codec_micamp_left.index);
						ispeaker_phone_tx_8KHz_osr256_actions[SPK_PHONE_TX_MICGAIN_LEFT_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);
						aud_cal_data->get_param  = aud_cal_data->param_val;						
					}
					break;	
				case CODEC_TX_LEFT:
					device_cal_data[9].codec_Tx_left_gain.data = (u8)aud_cal_data->param_val;
					ADIE_CODEC_UNPACK_ENTRY(ispeaker_phone_tx_8KHz_osr256_actions[SPK_PHONE_TX_CODECGAIN_LEFT_INDEX].action,reg, mask, val);
					val = (u8)aud_cal_data->param_val;
					printk("Set Tx Gain Left = %x\n",val);
					printk("Set Tx Gain Left index= %d\n",device_cal_data[9].codec_Tx_left_gain.index);
					ispeaker_phone_tx_8KHz_osr256_actions[SPK_PHONE_TX_CODECGAIN_LEFT_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);						
					aud_cal_data->get_param  = aud_cal_data->param_val;
					break;
				case CODEC_ST:
				case CODEC_TX_RIGHT:
				case CODEC_RX_RIGHT:
				case CODEC_RX_LEFT:
				case HEADPHONE_LEFT_GAIN:
				case HEADPHONE_RIGHT_GAIN:
				case CODEC_RX_FILTER_LEFT:
				case CODEC_RX_FILTER_RIGHT:	
//				case AUXPGA_LEFT_GAIN:
//				case AUXPGA_RIGHT_GAIN:	
				default:
						aud_cal_data->get_param = 0x00;					
						break;

								
			}
		}
		break;
/******************************************* [10] Stereo [ Headset + Speaker ] Rx*****************************************/
		case ACDB_ID_HEADSET_STEREO_PLUS_SPKR_STEREO_RX:
		{
			ptr_icodec = &snddev_ihs_stereo_speaker_stereo_rx_data;
			switch(aud_cal_data->voccal_param_type)
			{
				case CODEC_RX_FILTER_LEFT:
					temp = (u8)aud_cal_data->param_val;
					device_cal_data[10].codec_Rx_filter_left.data = (u8)aud_cal_data->param_val;
					ADIE_CODEC_UNPACK_ENTRY(ihs_stereo_speaker_stereo_rx_48KHz_osr256_actions[HEADSETSPK_RX_CODEC_FILTER_INDEX].action,reg, mask, val);
					if(temp ==0)
					{
						val = val & 0xBF;
					}
					else
					{
						val = val | 0x40;
					}
					printk("Set Filter left = %x\n",val);
/*BEGIN: 0012454 kiran.kanneganti@lge.com 2010-12-15*/
/*Mod 0012454: Corrected the log for debugging*/					
					printk("Set Filter left index= %d\n",device_cal_data[10].codec_Rx_filter_left.index);
/*END: 0012454 kiran.kanneganti@lge.com 2010-12-15*/					
					ihs_stereo_speaker_stereo_rx_48KHz_osr256_actions[HEADSETSPK_RX_CODEC_FILTER_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);
					aud_cal_data->get_param  = aud_cal_data->param_val;
					break;

				case CODEC_RX_FILTER_RIGHT:
					temp = (u8)aud_cal_data->param_val;
					device_cal_data[10].codec_Rx_filter_right.data = (u8)aud_cal_data->param_val;
					ADIE_CODEC_UNPACK_ENTRY(ihs_stereo_speaker_stereo_rx_48KHz_osr256_actions[HEADSETSPK_RX_CODEC_FILTER_INDEX].action,reg, mask, val);
					if(temp ==0)
					{
						val = val & 0xDF;
					}
					else
					{
						val = val | 0x20;
					}
					printk("Set Filter right = %x\n",val);
					printk("Set Filter right index= %d\n",device_cal_data[10].codec_Rx_filter_right.index);
					ihs_stereo_speaker_stereo_rx_48KHz_osr256_actions[HEADSETSPK_RX_CODEC_FILTER_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);
					aud_cal_data->get_param  = aud_cal_data->param_val;
					break;


				case HEADPHONE_LEFT_GAIN:
					device_cal_data[10].codec_headset_left.data = (u8)aud_cal_data->param_val;
					ADIE_CODEC_UNPACK_ENTRY(ihs_stereo_speaker_stereo_rx_48KHz_osr256_actions[HEADSETSPK_RX_HEADSETGAIN_LEFT_INDEX].action,reg, mask, val);
					//val = (u8)aud_cal_data->param_val;
					temp = (u8)aud_cal_data->param_val;
					temp = temp << 2;
					temp = temp & 0xFC;
					val = val & 0x03; 	
					val = val | temp;
					printk("Set headset Left Gain = %x\n",val);
					printk("Set headset Left Gain index= %d\n",device_cal_data[10].codec_headset_left.index);
					ihs_stereo_speaker_stereo_rx_48KHz_osr256_actions[HEADSETSPK_RX_HEADSETGAIN_LEFT_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);						
					aud_cal_data->get_param  = aud_cal_data->param_val;						
					break;

				case HEADPHONE_RIGHT_GAIN:
					device_cal_data[10].codec_headset_right.data = (u8)aud_cal_data->param_val;
					ADIE_CODEC_UNPACK_ENTRY(ihs_stereo_speaker_stereo_rx_48KHz_osr256_actions[HEADSETSPK_RX_HEADSETGAIN_RIGHT_INDEX].action,reg, mask, val);
					//val = (u8)aud_cal_data->param_val;
					temp = (u8)aud_cal_data->param_val;
					temp = temp << 2;
					temp = temp & 0xFC;
					val = val & 0x03; 	
					val = val | temp;
					printk("Set headset right Gain = %x\n",val);
					printk("Set headset right Gain index= %d\n",device_cal_data[10].codec_headset_right.index);
					ihs_stereo_speaker_stereo_rx_48KHz_osr256_actions[HEADSETSPK_RX_HEADSETGAIN_RIGHT_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);						
					aud_cal_data->get_param  = aud_cal_data->param_val;						
					break;	
						
				case CODEC_RX_RIGHT:
					device_cal_data[10].codec_Rx_right_gain.data = (u8)aud_cal_data->param_val;
					ADIE_CODEC_UNPACK_ENTRY(ihs_stereo_speaker_stereo_rx_48KHz_osr256_actions[HEADSETSPK_RX_CODECGAIN_RIGHT_INDEX].action,reg, mask, val);
					val = (u8)aud_cal_data->param_val;
					printk("Set Rx right Gain = %x\n",val);
					printk("Set Rx right Gain index = %d\n",device_cal_data[10].codec_Rx_right_gain.index);
					ihs_stereo_speaker_stereo_rx_48KHz_osr256_actions[HEADSETSPK_RX_CODECGAIN_RIGHT_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);						
					aud_cal_data->get_param  = aud_cal_data->param_val;						
					break;
						
				case CODEC_RX_LEFT:
					device_cal_data[10].codec_Rx_left_gain.data = (u8)aud_cal_data->param_val;
					ADIE_CODEC_UNPACK_ENTRY(ihs_stereo_speaker_stereo_rx_48KHz_osr256_actions[HEADSETSPK_RX_CODECGAIN_LEFT_INDEX].action,reg, mask, val);
					val = (u8)aud_cal_data->param_val;
					printk("Set Rx Left Gain = %x\n",val);
					printk("Set Rx Left Gain index= %d\n",device_cal_data[10].codec_Rx_left_gain.index);
					ihs_stereo_speaker_stereo_rx_48KHz_osr256_actions[HEADSETSPK_RX_CODECGAIN_LEFT_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);						
					aud_cal_data->get_param  = aud_cal_data->param_val;						
					break;

				case RX_VOLUME_MAX_NB:
					headset_spk_rx[VOC_NB_INDEX].max_level = aud_cal_data->param_val;
					ptr_icodec->max_voice_rx_vol[VOC_NB_INDEX] = aud_cal_data->param_val;
					aud_cal_data->get_param  = aud_cal_data->param_val;
					break;
						
				case RX_VOLUME_MIN_NB:
					headset_spk_rx[VOC_NB_INDEX].min_level = aud_cal_data->param_val;
					ptr_icodec->min_voice_rx_vol[VOC_NB_INDEX] = aud_cal_data->param_val;
					aud_cal_data->get_param  = aud_cal_data->param_val;					
					break;
						
				case RX_VOLUME_MAX_WB:
					headset_spk_rx[VOC_WB_INDEX].max_level = aud_cal_data->param_val;
					ptr_icodec->max_voice_rx_vol[VOC_WB_INDEX] = aud_cal_data->param_val;
					aud_cal_data->get_param  = aud_cal_data->param_val;					
					break;

				case RX_VOLUME_MIN_WB:					
					headset_spk_rx[VOC_WB_INDEX].min_level = aud_cal_data->param_val;
					ptr_icodec->min_voice_rx_vol[VOC_WB_INDEX] = aud_cal_data->param_val;
					aud_cal_data->get_param  = aud_cal_data->param_val;					
					break;

				case CODEC_ST:
				case CODEC_TX_LEFT:
				case CODEC_TX_RIGHT:
				case MICAMP_GAIN_LEFT:
				case MICAMP_GAIN_RIGHT:
				case MIC_BIAS_0:
				case MIC_BIAS_1:
//				case AUXPGA_LEFT_GAIN:
//				case AUXPGA_RIGHT_GAIN:		
				default:
					aud_cal_data->get_param = 0x00;
					break;

									
			}
		}
		break;
/************************************************ [11] Handset Rec Tx****************************************************/
/* BEGIN:0012384 ehgrace.kim@lge.com     2010.12.14*/
/* MOD: [Audio]change tx gain for recording mode & delete the dummy src*/
 		case ACDB_ID_I2S_TX:
		{
			switch(aud_cal_data->voccal_param_type)
			{
				case MIC_BIAS_0:
					mic_bias0_handset_rec_mic= aud_cal_data->param_val;
					aud_cal_data->get_param = mic_bias0_handset_rec_mic;
					break;
					
				case MIC_BIAS_1:
					mic_bias1_handset_rec_mic= aud_cal_data->param_val;
					aud_cal_data->get_param = mic_bias1_handset_rec_mic;
					break;
				case MICAMP_GAIN_LEFT:
					if(aud_cal_data->param_val == 0)
					{
						device_cal_data[11].codec_micamp_left.data = (u8)aud_cal_data->param_val;
						ADIE_CODEC_UNPACK_ENTRY(rec_imic_48KHz_osr256_actions[HANDSET_REC_TX_MICGAIN_LEFT_INDEX].action,reg, mask, val);
						val = val & 0x9F;
						printk("Set MicAmp Left = %x\n",val);
						printk("Set MicAmp Left index= %d\n",device_cal_data[11].codec_micamp_left.index);
						rec_imic_48KHz_osr256_actions[HANDSET_REC_TX_MICGAIN_LEFT_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);
						aud_cal_data->get_param  = aud_cal_data->param_val;
					}
					else
					{
						u8 temp;
						device_cal_data[11].codec_micamp_left.data = (u8)aud_cal_data->param_val;
						ADIE_CODEC_UNPACK_ENTRY(rec_imic_48KHz_osr256_actions[HANDSET_REC_TX_MICGAIN_LEFT_INDEX].action,reg, mask, val);
						val = val & 0x9F;
						temp = (u8)aud_cal_data->param_val;
						temp = temp & 0x03;
						temp = temp <<5;
						val = val | temp;
						printk("Set MicAmp Left = %x\n",val);
						printk("Set MicAmp Left index= %d\n",device_cal_data[11].codec_micamp_left.index);
						rec_imic_48KHz_osr256_actions[HANDSET_REC_TX_MICGAIN_LEFT_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);
						aud_cal_data->get_param  = aud_cal_data->param_val;						
					}
					break;	
				case CODEC_TX_LEFT:
					device_cal_data[11].codec_Tx_left_gain.data = (u8)aud_cal_data->param_val;
					ADIE_CODEC_UNPACK_ENTRY(rec_imic_48KHz_osr256_actions[HANDSET_REC_TX_CODECGAIN_LEFT_INDEX].action,reg, mask, val);
					val = (u8)aud_cal_data->param_val;
					printk("Set Tx Gain Left = %x\n",val);
					printk("Set Tx Gain Left index= %d\n",device_cal_data[11].codec_Tx_left_gain.index);
					rec_imic_48KHz_osr256_actions[HANDSET_REC_TX_CODECGAIN_LEFT_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);						
					aud_cal_data->get_param  = aud_cal_data->param_val;
					break;
				case CODEC_ST:
				case CODEC_TX_RIGHT:
				case CODEC_RX_RIGHT:
				case CODEC_RX_LEFT:
				case HEADPHONE_LEFT_GAIN:
				case HEADPHONE_RIGHT_GAIN:
				case CODEC_RX_FILTER_LEFT:
				case CODEC_RX_FILTER_RIGHT:	
//				case AUXPGA_LEFT_GAIN:
//				case AUXPGA_RIGHT_GAIN:	
				default:
						aud_cal_data->get_param = 0x00;					
						break;

								
			}
		}
		break;
/* END:0012384 ehgrace.kim@lge.com     2010.12.14*/
#if LOOPBACK_SUPPORT
/* BEGIN:0014024  kiran.kanneganti@lge.com     2011.01.13*/
/* MOD:0014024 Calibration support for loop back devices  */
/************************************************ [12] LoopBack Main Mic Tx****************************************************/
 		case LOOPBACK_MAIN_MIC:
		{
			switch(aud_cal_data->voccal_param_type)
			{
				case MICAMP_GAIN_LEFT:
					if(aud_cal_data->param_val == 0)
					{
						device_cal_data[12].codec_micamp_left.data = (u8)aud_cal_data->param_val;
						ADIE_CODEC_UNPACK_ENTRY(testmode_handset_tx_8KHz_osr256_actions[LB_MAIN_MIC_TX_MICGAIN_LEFT_INDEX].action,reg, mask, val);
						val = val & 0x9F;
						printk("Set MicAmp Left = %x\n",val);
						printk("Set MicAmp Left index= %d\n",device_cal_data[12].codec_micamp_left.index);
						testmode_handset_tx_8KHz_osr256_actions[LB_MAIN_MIC_TX_MICGAIN_LEFT_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);
						aud_cal_data->get_param  = aud_cal_data->param_val;
					}
					else
					{
						u8 temp;
						device_cal_data[12].codec_micamp_left.data = (u8)aud_cal_data->param_val;
						ADIE_CODEC_UNPACK_ENTRY(testmode_handset_tx_8KHz_osr256_actions[LB_MAIN_MIC_TX_MICGAIN_LEFT_INDEX].action,reg, mask, val);
						val = val & 0x9F;
						temp = (u8)aud_cal_data->param_val;
						temp = temp & 0x03;
						temp = temp <<5;
						val = val | temp;
						printk("Set MicAmp Left = %x\n",val);
						printk("Set MicAmp Left index= %d\n",device_cal_data[12].codec_micamp_left.index);
						testmode_handset_tx_8KHz_osr256_actions[LB_MAIN_MIC_TX_MICGAIN_LEFT_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);
						aud_cal_data->get_param  = aud_cal_data->param_val;						
					}
					break;	
				case CODEC_TX_LEFT:
					device_cal_data[12].codec_Tx_left_gain.data = (u8)aud_cal_data->param_val;
					ADIE_CODEC_UNPACK_ENTRY(testmode_handset_tx_8KHz_osr256_actions[LB_MAIN_MIC_TX_CODECGAIN_LEFT_INDEX].action,reg, mask, val);
					val = (u8)aud_cal_data->param_val;
					printk("Set Tx Gain Left = %x\n",val);
					printk("Set Tx Gain Left index= %d\n",device_cal_data[12].codec_Tx_left_gain.index);
					testmode_handset_tx_8KHz_osr256_actions[LB_MAIN_MIC_TX_CODECGAIN_LEFT_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);						
					aud_cal_data->get_param  = aud_cal_data->param_val;
					break;
				case CODEC_ST:	
					device_cal_data[12].codec_St_gain.data = (u8)aud_cal_data->param_val;
					ADIE_CODEC_UNPACK_ENTRY(testmode_handset_tx_8KHz_osr256_actions[LB_MAIN_MIC_TX_CODECGAIN_ST_INDEX].action,reg, mask, val);
					val = (u8)aud_cal_data->param_val;
					printk("Set ST gain  = %x\n",val);					
					printk("Set ST gain  index= %d\n",device_cal_data[12].codec_St_gain.index);										
					testmode_handset_tx_8KHz_osr256_actions[LB_MAIN_MIC_TX_CODECGAIN_ST_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);
					aud_cal_data->get_param  = aud_cal_data->param_val;
					break;
				case MIC_BIAS_0:
				case MIC_BIAS_1:	
				case CODEC_TX_RIGHT:
				case CODEC_RX_RIGHT:
				case CODEC_RX_LEFT:
				case HEADPHONE_LEFT_GAIN:
				case HEADPHONE_RIGHT_GAIN:
				case CODEC_RX_FILTER_LEFT:
				case CODEC_RX_FILTER_RIGHT:	
				default:
						aud_cal_data->get_param = 0x00;					
						break;

								
			}
		}
		break;

/************************************************ [13] LoopBack Back Mic Tx****************************************************/
 		case LOOPBACK_BACK_MIC:
		{
			switch(aud_cal_data->voccal_param_type)
			{
				case MICAMP_GAIN_LEFT:
					if(aud_cal_data->param_val == 0)
					{
						device_cal_data[13].codec_micamp_left.data = (u8)aud_cal_data->param_val;
						ADIE_CODEC_UNPACK_ENTRY(testmode_handset_back_mic_tx_8KHz_osr256_actions[LB_BACK_MIC_TX_MICGAIN_LEFT_INDEX].action,reg, mask, val);
						val = val & 0x9F;
						printk("Set MicAmp Left = %x\n",val);
						printk("Set MicAmp Left index= %d\n",device_cal_data[13].codec_micamp_left.index);
						testmode_handset_back_mic_tx_8KHz_osr256_actions[LB_BACK_MIC_TX_MICGAIN_LEFT_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);
						aud_cal_data->get_param  = aud_cal_data->param_val;
					}
					else
					{
						u8 temp;
						device_cal_data[13].codec_micamp_left.data = (u8)aud_cal_data->param_val;
						ADIE_CODEC_UNPACK_ENTRY(testmode_handset_back_mic_tx_8KHz_osr256_actions[LB_BACK_MIC_TX_MICGAIN_LEFT_INDEX].action,reg, mask, val);
						val = val & 0x9F;
						temp = (u8)aud_cal_data->param_val;
						temp = temp & 0x03;
						temp = temp <<5;
						val = val | temp;
						printk("Set MicAmp Left = %x\n",val);
						printk("Set MicAmp Left index= %d\n",device_cal_data[13].codec_micamp_left.index);
						testmode_handset_back_mic_tx_8KHz_osr256_actions[LB_BACK_MIC_TX_MICGAIN_LEFT_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);
						aud_cal_data->get_param  = aud_cal_data->param_val;						
					}
					break;	
				case CODEC_TX_LEFT:
					device_cal_data[13].codec_Tx_left_gain.data = (u8)aud_cal_data->param_val;
					ADIE_CODEC_UNPACK_ENTRY(testmode_handset_back_mic_tx_8KHz_osr256_actions[LB_BACK_MIC_TX_CODECGAIN_LEFT_INDEX].action,reg, mask, val);
					val = (u8)aud_cal_data->param_val;
					printk("Set Tx Gain Left = %x\n",val);
					printk("Set Tx Gain Left index= %d\n",device_cal_data[13].codec_Tx_left_gain.index);
					testmode_handset_back_mic_tx_8KHz_osr256_actions[LB_BACK_MIC_TX_CODECGAIN_LEFT_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);						
					aud_cal_data->get_param  = aud_cal_data->param_val;
					break;
					
				case CODEC_ST:	
					device_cal_data[13].codec_St_gain.data = (u8)aud_cal_data->param_val;
					ADIE_CODEC_UNPACK_ENTRY(testmode_handset_back_mic_tx_8KHz_osr256_actions[LB_BACK_MIC_TX_CODECGAIN_ST_INDEX].action,reg, mask, val);
					val = (u8)aud_cal_data->param_val;
					printk("Set ST gain  = %x\n",val);					
					printk("Set ST gain  index= %d\n",device_cal_data[13].codec_St_gain.index);										
					testmode_handset_back_mic_tx_8KHz_osr256_actions[LB_BACK_MIC_TX_CODECGAIN_ST_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);
					aud_cal_data->get_param  = aud_cal_data->param_val;
					break;
					
				case MIC_BIAS_0:
				case MIC_BIAS_1:	
				case CODEC_TX_RIGHT:
				case CODEC_RX_RIGHT:
				case CODEC_RX_LEFT:
				case HEADPHONE_LEFT_GAIN:
				case HEADPHONE_RIGHT_GAIN:
				case CODEC_RX_FILTER_LEFT:
				case CODEC_RX_FILTER_RIGHT:	
				default:
						aud_cal_data->get_param = 0x00;					
						break;

								
			}
		}
		break;

/***************************************************[14]LoopBack Handset Rx**************************************************/
 		case LOOPBACK_HANDSET_RX:
		{
			ptr_icodec = &snddev_iearpiece_data;
			switch(aud_cal_data->voccal_param_type)
			{
				case CODEC_RX_LEFT:
					device_cal_data[14].codec_Rx_left_gain.data = (u8)aud_cal_data->param_val;
					ADIE_CODEC_UNPACK_ENTRY(testmode_handset_rx_8KHz_osr256_actions[LB_HANDSET_RX_CODECGAIN_LEFT_INDEX].action,reg, mask, val);
					val = (u8)aud_cal_data->param_val;
					printk("Set Rx gain left = %x\n",val);
					printk("Set Rx gain left index= %d\n",device_cal_data[14].codec_Rx_left_gain.index);
					testmode_handset_rx_8KHz_osr256_actions[LB_HANDSET_RX_CODECGAIN_LEFT_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);						
					aud_cal_data->get_param  = aud_cal_data->param_val;						
					break;

				case CODEC_RX_FILTER_LEFT:
					device_cal_data[14].codec_Rx_filter_left.data = (u8)aud_cal_data->param_val;
					temp = (u8)aud_cal_data->param_val;
					ADIE_CODEC_UNPACK_ENTRY(testmode_handset_rx_8KHz_osr256_actions[LB_HANDSET_RX_CODEC_FILTER_INDEX].action,reg, mask, val);
					if(temp ==0)
					{
						val = val & 0xBF;
					}
					else
					{
						val = val | 0x40;
					}
					printk("Set Rx Filter gain left = %x\n",val);
					printk("Set Rx Filter gain left index = %d\n",device_cal_data[14].codec_Rx_filter_left.index);
					testmode_handset_rx_8KHz_osr256_actions[LB_HANDSET_RX_CODEC_FILTER_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);
					aud_cal_data->get_param  = aud_cal_data->param_val;
					break;
					
				case CODEC_RX_FILTER_RIGHT:
					device_cal_data[14].codec_Rx_filter_right.data = (u8)aud_cal_data->param_val;
					temp = (u8)aud_cal_data->param_val;
					ADIE_CODEC_UNPACK_ENTRY(testmode_handset_rx_8KHz_osr256_actions[LB_HANDSET_RX_CODEC_FILTER_INDEX].action,reg, mask, val);
					if(temp ==0)
					{
						val = val & 0xDF;
					}
					else
					{
						val = val | 0x20;
					}
					printk("Set Rx Filter gain right = %x\n",val);
					printk("Set Rx Filter gain right index = %d\n",device_cal_data[14].codec_Rx_filter_right.index);
					testmode_handset_rx_8KHz_osr256_actions[LB_HANDSET_RX_CODEC_FILTER_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);
					aud_cal_data->get_param  = aud_cal_data->param_val;
					break;

				case RX_VOLUME_MAX_NB:
					handset_rx[VOC_NB_INDEX].max_level = aud_cal_data->param_val;
					ptr_icodec->max_voice_rx_vol[VOC_NB_INDEX] = aud_cal_data->param_val;
					aud_cal_data->get_param  = aud_cal_data->param_val;
					break;
					
				case RX_VOLUME_MIN_NB:
					handset_rx[VOC_NB_INDEX].min_level = aud_cal_data->param_val;
					ptr_icodec->min_voice_rx_vol[VOC_NB_INDEX] = aud_cal_data->param_val;
					aud_cal_data->get_param  = aud_cal_data->param_val;					
					break;
					
				case RX_VOLUME_MAX_WB:
					handset_rx[VOC_WB_INDEX].max_level = aud_cal_data->param_val;
					ptr_icodec->max_voice_rx_vol[VOC_WB_INDEX] = aud_cal_data->param_val;
					aud_cal_data->get_param  = aud_cal_data->param_val;					
					break;

				case RX_VOLUME_MIN_WB:					
					handset_rx[VOC_WB_INDEX].min_level = aud_cal_data->param_val;
					ptr_icodec->min_voice_rx_vol[VOC_WB_INDEX] = aud_cal_data->param_val;
					aud_cal_data->get_param  = aud_cal_data->param_val;					
					break;

				case MIC_BIAS_0:
				case MIC_BIAS_1:
				case CODEC_RX_RIGHT:
				case CODEC_TX_LEFT:
				case CODEC_TX_RIGHT:
				case CODEC_ST:
				case MICAMP_GAIN_LEFT:
				case MICAMP_GAIN_RIGHT:
				case HEADPHONE_LEFT_GAIN:
				case HEADPHONE_RIGHT_GAIN:
				default:
					aud_cal_data->get_param = 0x00;
					break;
							
			}
		}
		break;	

/***************************************************** [15]LoopBack Headset Phone Tx******************************************/
 		case LOOPBACK_HEADSET_TX:
		{
			switch(aud_cal_data->voccal_param_type)
			{

				case MICAMP_GAIN_LEFT:
					if(aud_cal_data->param_val == 0)
					{
						device_cal_data[15].codec_micamp_left.data = (u8)aud_cal_data->param_val;
						ADIE_CODEC_UNPACK_ENTRY(ihs_test_mode_mono_tx_8KHz_osr256_actions[LB_HEADSET_TX_MICGAIN_LEFT_INDEX].action,reg, mask, val);
						val = val & 0x9F;
						printk("Set Mic Amp gain Left = %x\n",val);
						printk("Set Mic Amp gain Left index= %d\n",device_cal_data[15].codec_micamp_left.index);
						ihs_test_mode_mono_tx_8KHz_osr256_actions[LB_HEADSET_TX_MICGAIN_LEFT_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);
						aud_cal_data->get_param  = aud_cal_data->param_val;
					}
					else
					{
						device_cal_data[15].codec_micamp_left.data = (u8)aud_cal_data->param_val;
						ADIE_CODEC_UNPACK_ENTRY(ihs_test_mode_mono_tx_8KHz_osr256_actions[LB_HEADSET_TX_MICGAIN_LEFT_INDEX].action,reg, mask, val);
						val = val & 0xBF;
						temp = (u8)aud_cal_data->param_val;
						temp = temp & 0x03;
						temp = temp <<5;
						val = val | temp;
						printk("Set Mic Amp gain Left = %x\n",val);
						printk("Set Mic Amp gain Left index= %d\n",device_cal_data[15].codec_micamp_left.index);
						ihs_test_mode_mono_tx_8KHz_osr256_actions[LB_HEADSET_TX_MICGAIN_LEFT_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);
						aud_cal_data->get_param  = aud_cal_data->param_val;						
					}
					break;

				case CODEC_ST:
					device_cal_data[15].codec_St_gain.data = (u8)aud_cal_data->param_val;
					ADIE_CODEC_UNPACK_ENTRY(ihs_test_mode_mono_tx_8KHz_osr256_actions[LB_HEADSET_TX_CODECGAIN_ST_INDEX].action,reg, mask, val);
					val = (u8)aud_cal_data->param_val;
					printk("Set ST gain  = %x\n",val);					
					printk("Set ST gain  index= %d\n",device_cal_data[15].codec_St_gain.index);										
					ihs_test_mode_mono_tx_8KHz_osr256_actions[LB_HEADSET_TX_CODECGAIN_ST_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);
					aud_cal_data->get_param  = aud_cal_data->param_val;
					break;
					
				case CODEC_TX_LEFT:
					device_cal_data[15].codec_Tx_left_gain.data = aud_cal_data->param_val;
					ADIE_CODEC_UNPACK_ENTRY(ihs_test_mode_mono_tx_8KHz_osr256_actions[LB_HEADSET_TX_CODECGAIN_LEFT_INDEX].action,reg, mask, val);
					val = (u8)aud_cal_data->param_val;
					printk("Set Tx gain Left = %x\n",val);	
					printk("Set Tx gain Left index = %d\n",device_cal_data[15].codec_Tx_left_gain.index);	
					ihs_test_mode_mono_tx_8KHz_osr256_actions[LB_HEADSET_TX_CODECGAIN_LEFT_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);						
					aud_cal_data->get_param  = aud_cal_data->param_val;
					break;
				
				case CODEC_RX_RIGHT:
				case CODEC_RX_LEFT:
				case MICAMP_GAIN_RIGHT:
				case CODEC_TX_RIGHT:
				case HEADPHONE_LEFT_GAIN:
				case HEADPHONE_RIGHT_GAIN:
				case CODEC_RX_FILTER_LEFT:
				case CODEC_RX_FILTER_RIGHT:					
				case MIC_BIAS_0:
				case MIC_BIAS_1:	
				default:
						aud_cal_data->get_param = 0x00;
						break;

								
			}
		}
		break;

/*************************************************** [16] LoopBack Headset Rx Mono ********************************************/
		case LOOPBACK_HEADSET_RX:
		{
			ptr_icodec = &snddev_ihs_mono_rx_data;
			
			switch(aud_cal_data->voccal_param_type)
			{
				case CODEC_RX_FILTER_LEFT:
					temp = (u8)aud_cal_data->param_val;
					device_cal_data[16].codec_Rx_filter_left.data = (u8)aud_cal_data->param_val;
					ADIE_CODEC_UNPACK_ENTRY(ihs_test_mode_mono_rx_8KHz_osr256_actions[LB_HEADSET_MONO_RX_CODEC_FILTER_INDEX].action,reg, mask, val);
					if(temp ==0)
					{
						val = val & 0xBF;
					}
					else
					{
						val = val | 0x40;
					}
					printk("Set Rx Filter gain left = %x\n",val);
					printk("Set Rx Filter gain left index= %d\n",device_cal_data[16].codec_Rx_filter_left.index);
					ihs_test_mode_mono_rx_8KHz_osr256_actions[LB_HEADSET_MONO_RX_CODEC_FILTER_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);
					aud_cal_data->get_param  = aud_cal_data->param_val;
					break;

				case CODEC_RX_FILTER_RIGHT:
					temp =(u8)aud_cal_data->param_val;
					device_cal_data[16].codec_Rx_filter_right.data = (u8)aud_cal_data->param_val;
					ADIE_CODEC_UNPACK_ENTRY(ihs_test_mode_mono_rx_8KHz_osr256_actions[LB_HEADSET_MONO_RX_CODEC_FILTER_INDEX].action,reg, mask, val);
					if(temp ==0)
					{
						val = val & 0xDF;
					}
					else
					{
						val = val | 0x20;
					}
					printk("Set Rx Filter gain right = %x\n",val);
					printk("Set Rx Filter gain right index= %d\n",device_cal_data[16].codec_Rx_filter_right.index);
					ihs_test_mode_mono_rx_8KHz_osr256_actions[LB_HEADSET_MONO_RX_CODEC_FILTER_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);
					aud_cal_data->get_param  = aud_cal_data->param_val;
					break;

				case HEADPHONE_LEFT_GAIN:
					device_cal_data[16].codec_headset_left.data = (u8)aud_cal_data->param_val;
					ADIE_CODEC_UNPACK_ENTRY(ihs_test_mode_mono_rx_8KHz_osr256_actions[LB_HEADSET_MONO_RX_HEADPHONEGAIN_LEFT_INDEX].action,reg, mask, val);
					temp = (u8)aud_cal_data->param_val;
					temp = temp << 2;
					temp = temp & 0xFC;
					val = val & 0x03; 	
					val = val | temp;
					printk("Set Headset  gain left = %x\n",val);
					printk("Set Headset  gain left index= %d\n",device_cal_data[16].codec_headset_left.index);
					ihs_test_mode_mono_rx_8KHz_osr256_actions[LB_HEADSET_MONO_RX_HEADPHONEGAIN_LEFT_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);						
					aud_cal_data->get_param  = aud_cal_data->param_val;						
					break;

				case HEADPHONE_RIGHT_GAIN:
					device_cal_data[16].codec_headset_right.data = (u8)aud_cal_data->param_val;
					ADIE_CODEC_UNPACK_ENTRY(ihs_test_mode_mono_rx_8KHz_osr256_actions[LB_HEADSET_MONO_RX_HEADPHONEGAIN_RIGHT_INDEX].action,reg, mask, val);
					temp = (u8)aud_cal_data->param_val;
					temp = temp << 2;
					temp = temp & 0xFC;
					val = val & 0x03; 	
					val = val | temp;
					printk("Set Headset gain right = %x\n",val);
					printk("Set Headset gain right index= %d\n",device_cal_data[16].codec_headset_right.index);
					ihs_test_mode_mono_rx_8KHz_osr256_actions[LB_HEADSET_MONO_RX_HEADPHONEGAIN_RIGHT_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);						
					aud_cal_data->get_param  = aud_cal_data->param_val;						
					break;	
				case CODEC_RX_LEFT:
					device_cal_data[16].codec_Rx_left_gain.data = (u8)aud_cal_data->param_val;
					ADIE_CODEC_UNPACK_ENTRY(ihs_test_mode_mono_rx_8KHz_osr256_actions[LB_HEADSET_MONO_RX_CODECGAIN_LEFT_INDEX].action,reg, mask, val);
					val = (u8)aud_cal_data->param_val;
					printk("Set Rx gain left = %x\n",val);
					printk("Set Rx gain left index= %d\n",device_cal_data[16].codec_Rx_left_gain.index);
					ihs_test_mode_mono_rx_8KHz_osr256_actions[LB_HEADSET_MONO_RX_CODECGAIN_LEFT_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);						
					aud_cal_data->get_param  = aud_cal_data->param_val;						
					break;
					

				case RX_VOLUME_MAX_NB:
					headset_mono_rx[VOC_NB_INDEX].max_level = aud_cal_data->param_val;
					ptr_icodec->max_voice_rx_vol[VOC_NB_INDEX] = aud_cal_data->param_val;
					aud_cal_data->get_param  = aud_cal_data->param_val;
					break;
					
				case RX_VOLUME_MIN_NB:
					headset_mono_rx[VOC_NB_INDEX].min_level = aud_cal_data->param_val;
					ptr_icodec->min_voice_rx_vol[VOC_NB_INDEX] = aud_cal_data->param_val;
					aud_cal_data->get_param  = aud_cal_data->param_val;					
					break;
					
				case RX_VOLUME_MAX_WB:
					headset_mono_rx[VOC_WB_INDEX].max_level = aud_cal_data->param_val;
					ptr_icodec->max_voice_rx_vol[VOC_WB_INDEX] = aud_cal_data->param_val;
					aud_cal_data->get_param  = aud_cal_data->param_val;					
					break;

				case RX_VOLUME_MIN_WB:					
					headset_mono_rx[VOC_WB_INDEX].min_level = aud_cal_data->param_val;
					ptr_icodec->min_voice_rx_vol[VOC_WB_INDEX] = aud_cal_data->param_val;
					aud_cal_data->get_param  = aud_cal_data->param_val;					
					break;

				case CODEC_TX_LEFT:
				case CODEC_TX_RIGHT:
				case CODEC_ST:
				case MICAMP_GAIN_LEFT:
				case MICAMP_GAIN_RIGHT:
				case MIC_BIAS_0:
				case MIC_BIAS_1:	
				case CODEC_RX_RIGHT:
				default:
						aud_cal_data->get_param = 0x00;
						break;								
			}
		}
		break;	

/*[START] jy0127.jang@lge.com MS910_AUDIO_CHANGE 2011-04-07*/
/*added speaker phone loopback*/
/************************************************ [17] SPEAKERPHONE Tx****************************************************/
		case LOOPBACK_SPEAKERPHONE_TX :
		{
			switch(aud_cal_data->voccal_param_type)
			{
				case MICAMP_GAIN_LEFT:
					if(aud_cal_data->param_val == 0)
					{
						device_cal_data[17].codec_micamp_left.data = (u8)aud_cal_data->param_val;
						ADIE_CODEC_UNPACK_ENTRY(testmode_speakerphone_tx_8KHz_osr256_actions[LB_SPEAKERPHONE_TX_MICGAIN_LEFT_INDEX].action,reg, mask, val);
						val = val & 0x9F;
						printk("Set MicAmp Left = %x\n",val);
						printk("Set MicAmp Left index= %d\n",device_cal_data[17].codec_micamp_left.index);
						testmode_speakerphone_tx_8KHz_osr256_actions[LB_SPEAKERPHONE_TX_MICGAIN_LEFT_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);
						aud_cal_data->get_param  = aud_cal_data->param_val;
					}
					else
					{
						u8 temp;
						device_cal_data[17].codec_micamp_left.data = (u8)aud_cal_data->param_val;
						ADIE_CODEC_UNPACK_ENTRY(testmode_speakerphone_tx_8KHz_osr256_actions[LB_SPEAKERPHONE_TX_MICGAIN_LEFT_INDEX].action,reg, mask, val);
						val = val & 0x9F;
						temp = (u8)aud_cal_data->param_val;
						temp = temp & 0x03;
						temp = temp <<5;
						val = val | temp;
						printk("Set MicAmp Left = %x\n",val);
						printk("Set MicAmp Left index= %d\n",device_cal_data[17].codec_micamp_left.index);
						testmode_speakerphone_tx_8KHz_osr256_actions[LB_SPEAKERPHONE_TX_MICGAIN_LEFT_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);
						aud_cal_data->get_param  = aud_cal_data->param_val;						
					}
					break;	
				case CODEC_TX_LEFT:
					device_cal_data[17].codec_Tx_left_gain.data = (u8)aud_cal_data->param_val;
					ADIE_CODEC_UNPACK_ENTRY(testmode_speakerphone_tx_8KHz_osr256_actions[LB_SPEAKERPHONE_TX_CODECGAIN_LEFT_INDEX].action,reg, mask, val);
					val = (u8)aud_cal_data->param_val;
					printk("Set Tx Gain Left = %x\n",val);
					printk("Set Tx Gain Left index= %d\n",device_cal_data[17].codec_Tx_left_gain.index);
					testmode_speakerphone_tx_8KHz_osr256_actions[LB_SPEAKERPHONE_TX_CODECGAIN_LEFT_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);						
					aud_cal_data->get_param  = aud_cal_data->param_val;
					break;
				case CODEC_ST:	
					device_cal_data[17].codec_St_gain.data = (u8)aud_cal_data->param_val;
					ADIE_CODEC_UNPACK_ENTRY(testmode_speakerphone_tx_8KHz_osr256_actions[LB_SPEAKERPHONE_TX_CODECGAIN_ST_INDEX].action,reg, mask, val);
					val = (u8)aud_cal_data->param_val;
					printk("Set ST gain  = %x\n",val);					
					printk("Set ST gain  index= %d\n",device_cal_data[17].codec_St_gain.index);										
					testmode_speakerphone_tx_8KHz_osr256_actions[LB_SPEAKERPHONE_TX_CODECGAIN_ST_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);
					aud_cal_data->get_param  = aud_cal_data->param_val;
					break;
				case MIC_BIAS_0:
				case MIC_BIAS_1:	
				case CODEC_TX_RIGHT:
				case CODEC_RX_RIGHT:
				case CODEC_RX_LEFT:
				case HEADPHONE_LEFT_GAIN:
				case HEADPHONE_RIGHT_GAIN:
				case CODEC_RX_FILTER_LEFT:
				case CODEC_RX_FILTER_RIGHT:	
				default:
						aud_cal_data->get_param = 0x00;					
						break;

								
			}
		}
		break;
/*************************************************** [18] LoopBack Headset Rx Mono ********************************************/
		case LOOPBACK_SPEAKERPHONE_RX :
		{
			ptr_icodec = &snddev_iearpiece_data;
			switch(aud_cal_data->voccal_param_type)
			{
				case CODEC_RX_LEFT:
					device_cal_data[14].codec_Rx_left_gain.data = (u8)aud_cal_data->param_val;
					ADIE_CODEC_UNPACK_ENTRY(testmode_handset_rx_8KHz_osr256_actions[LB_SPEAKERPHONE_RX_CODECGAIN_LEFT_INDEX].action,reg, mask, val);
					val = (u8)aud_cal_data->param_val;
					printk("Set Rx gain left = %x\n",val);
					printk("Set Rx gain left index= %d\n",device_cal_data[14].codec_Rx_left_gain.index);
					testmode_speakerphone_rx_8KHz_osr256_actions[LB_SPEAKERPHONE_RX_CODECGAIN_LEFT_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);						
					aud_cal_data->get_param  = aud_cal_data->param_val;						
					break;

				case CODEC_RX_FILTER_LEFT:
					device_cal_data[14].codec_Rx_filter_left.data = (u8)aud_cal_data->param_val;
					temp = (u8)aud_cal_data->param_val;
					ADIE_CODEC_UNPACK_ENTRY(testmode_speakerphone_rx_8KHz_osr256_actions[LB_SPEAKERPHONE_RX_CODEC_FILTER_INDEX].action,reg, mask, val);
					if(temp ==0)
					{
						val = val & 0xBF;
					}
					else
					{
						val = val | 0x40;
					}
					printk("Set Rx Filter gain left = %x\n",val);
					printk("Set Rx Filter gain left index = %d\n",device_cal_data[14].codec_Rx_filter_left.index);
					testmode_speakerphone_rx_8KHz_osr256_actions[LB_SPEAKERPHONE_RX_CODEC_FILTER_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);
					aud_cal_data->get_param  = aud_cal_data->param_val;
					break;
					
				case CODEC_RX_FILTER_RIGHT:
					device_cal_data[14].codec_Rx_filter_right.data = (u8)aud_cal_data->param_val;
					temp = (u8)aud_cal_data->param_val;
					ADIE_CODEC_UNPACK_ENTRY(testmode_speakerphone_rx_8KHz_osr256_actions[LB_SPEAKERPHONE_RX_CODEC_FILTER_INDEX].action,reg, mask, val);
					if(temp ==0)
					{
						val = val & 0xDF;
					}
					else
					{
						val = val | 0x20;
					}
					printk("Set Rx Filter gain right = %x\n",val);
					printk("Set Rx Filter gain right index = %d\n",device_cal_data[14].codec_Rx_filter_right.index);
					testmode_speakerphone_rx_8KHz_osr256_actions[LB_SPEAKERPHONE_RX_CODEC_FILTER_INDEX].action = ADIE_CODEC_PACK_ENTRY(reg,mask,val);
					aud_cal_data->get_param  = aud_cal_data->param_val;
					break;

				case RX_VOLUME_MAX_NB:
					spk_stereo_rx[VOC_NB_INDEX].max_level = aud_cal_data->param_val;
					ptr_icodec->max_voice_rx_vol[VOC_NB_INDEX] = aud_cal_data->param_val;
					aud_cal_data->get_param  = aud_cal_data->param_val;
					break;
					
				case RX_VOLUME_MIN_NB:
					spk_stereo_rx[VOC_NB_INDEX].min_level = aud_cal_data->param_val;
					ptr_icodec->min_voice_rx_vol[VOC_NB_INDEX] = aud_cal_data->param_val;
					aud_cal_data->get_param  = aud_cal_data->param_val;					
					break;
					
				case RX_VOLUME_MAX_WB:
					spk_stereo_rx[VOC_WB_INDEX].max_level = aud_cal_data->param_val;
					ptr_icodec->max_voice_rx_vol[VOC_WB_INDEX] = aud_cal_data->param_val;
					aud_cal_data->get_param  = aud_cal_data->param_val;					
					break;

				case RX_VOLUME_MIN_WB:					
					spk_stereo_rx[VOC_WB_INDEX].min_level = aud_cal_data->param_val;
					ptr_icodec->min_voice_rx_vol[VOC_WB_INDEX] = aud_cal_data->param_val;
					aud_cal_data->get_param  = aud_cal_data->param_val;					
					break;

				case MIC_BIAS_0:
				case MIC_BIAS_1:
				case CODEC_RX_RIGHT:
				case CODEC_TX_LEFT:
				case CODEC_TX_RIGHT:
				case CODEC_ST:
				case MICAMP_GAIN_LEFT:
				case MICAMP_GAIN_RIGHT:
				case HEADPHONE_LEFT_GAIN:
				case HEADPHONE_RIGHT_GAIN:
				default:
					aud_cal_data->get_param = 0x00;
					break;
							
			}
		}
		break;			
#endif
/* END:0014024  kiran.kanneganti@lge.com     2011.01.13*/
/**************************************************For BT Rx volume***************************************************/
		case ACDB_ID_BT_SCO_SPKR:  		
		{
			ptr_ecodec = &snddev_bt_sco_earpiece_data;
				
			switch(aud_cal_data->voccal_param_type)
			{
				case RX_VOLUME_MAX_NB:
					BT_earpiece_rx[VOC_NB_INDEX].max_level = aud_cal_data->param_val;
					ptr_ecodec->max_voice_rx_vol[VOC_NB_INDEX] = aud_cal_data->param_val;
					aud_cal_data->get_param  = aud_cal_data->param_val;
					break;
						
				case RX_VOLUME_MIN_NB:
					BT_earpiece_rx[VOC_NB_INDEX].min_level = aud_cal_data->param_val;
					ptr_ecodec->min_voice_rx_vol[VOC_NB_INDEX] = aud_cal_data->param_val;
					aud_cal_data->get_param  = aud_cal_data->param_val;					
					break;
						
				case RX_VOLUME_MAX_WB:
					BT_earpiece_rx[VOC_WB_INDEX].max_level = aud_cal_data->param_val;
					ptr_ecodec->max_voice_rx_vol[VOC_WB_INDEX] = aud_cal_data->param_val;
					aud_cal_data->get_param  = aud_cal_data->param_val;					
					break;

				case RX_VOLUME_MIN_WB:					
					BT_earpiece_rx[VOC_WB_INDEX].min_level = aud_cal_data->param_val;
					ptr_ecodec->min_voice_rx_vol[VOC_WB_INDEX] = aud_cal_data->param_val;
					aud_cal_data->get_param  = aud_cal_data->param_val;					
					break;

				default:
					break;
				
			}

		}
		break;
		
		
 		default:
			aud_cal_data->get_param = 0x00;
			break;
 	}
 				
 }
 // EXPORT_SYMBOL(Snddev_AudioCal_Set);
 
 
 void Snddev_AudioCal_Get(struct msm_snd_set_audcal_param *aud_cal_data)
 {
 	struct  msm_snd_set_audcal_param *audcal_data_local = aud_cal_data;
 
 	printk("Voc Codec valiue = %d\n",(audcal_data_local->voc_codec));
 	printk("Voccal param Type   = %d\n",(audcal_data_local->voccal_param_type));
 	printk("Set / Get = %d\n",(audcal_data_local->get_flag));
 	printk("Param Value = %d\n",(audcal_data_local->param_val));
 	printk("Get Param Value = %d\n",(audcal_data_local->get_param));
 
 	switch(audcal_data_local->voc_codec)
 	{
/*************************************************** [0] Handset Rx**************************************************/
		case ACDB_ID_HANDSET_SPKR: 
		{
			switch(aud_cal_data->voccal_param_type)
			{
				
				case CODEC_RX_LEFT:
					aud_cal_data->get_param  = device_cal_data[0].codec_Rx_left_gain.data;
					break;
				
				case CODEC_RX_FILTER_LEFT:
					aud_cal_data->get_param  = device_cal_data[0].codec_Rx_filter_left.data;
					break;

				case CODEC_RX_FILTER_RIGHT:
					aud_cal_data->get_param  = device_cal_data[0].codec_Rx_filter_right.data;
					break;

				case RX_VOLUME_MAX_NB:	
					aud_cal_data->get_param  = handset_rx[VOC_NB_INDEX].max_level;
					break;
					
				case RX_VOLUME_MIN_NB:	
					aud_cal_data->get_param  = handset_rx[VOC_NB_INDEX].min_level;
					break;

				case RX_VOLUME_MAX_WB:	
					aud_cal_data->get_param  = handset_rx[VOC_WB_INDEX].max_level;
					break;

				case RX_VOLUME_MIN_WB:						
					aud_cal_data->get_param  = handset_rx[VOC_WB_INDEX].min_level;
					break;

				case MIC_BIAS_0:
				case MIC_BIAS_1:
				case CODEC_RX_RIGHT:
				case CODEC_TX_LEFT:
				case CODEC_TX_RIGHT:
				case CODEC_ST:
				case MICAMP_GAIN_LEFT:
				case MICAMP_GAIN_RIGHT:
				case HEADPHONE_LEFT_GAIN:
				case HEADPHONE_RIGHT_GAIN:
//				case AUXPGA_LEFT_GAIN:
//				case AUXPGA_RIGHT_GAIN:
				default:
					aud_cal_data->get_param = 0x00; 
					break;
								
			}
		
		}
 		break;			
/*[END] jy0127.jang@lge.com MS910_AUDIO_CHANGE 2011-04-07*/

/*************************************************** [1] Handset Tx **************************************************/
#if LGE_AUDIO_PATH
 		case ACDB_ID_HANDSET_MIC_ENDFIRE:
#else
 		case ACDB_ID_HANDSET_MIC:
#endif
		{
			switch(aud_cal_data->voccal_param_type)
			{
				case MIC_BIAS_0:
					aud_cal_data->get_param = mic_bias0_handset_mic;
					break;
					
				case MIC_BIAS_1:
					aud_cal_data->get_param = mic_bias1_handset_mic;
					break;
					
				case CODEC_TX_LEFT:
					aud_cal_data->get_param  = device_cal_data[1].codec_Tx_left_gain.data;
					break;
				
				case CODEC_TX_RIGHT:
					aud_cal_data->get_param  = device_cal_data[1].codec_Tx_right_gain.data;
					break;

				case CODEC_ST:
					aud_cal_data->get_param  = device_cal_data[1].codec_St_gain.data;
					break;
					
				case MICAMP_GAIN_LEFT:
					aud_cal_data->get_param  = device_cal_data[1].codec_micamp_left.data;
					break;

				case MICAMP_GAIN_RIGHT:
					aud_cal_data->get_param  = device_cal_data[1].codec_micamp_right.data;
					break;
					
				case CODEC_RX_RIGHT:
				case CODEC_RX_LEFT:
				case CODEC_RX_FILTER_LEFT:					
				case CODEC_RX_FILTER_RIGHT:	
				case HEADPHONE_LEFT_GAIN:
				case HEADPHONE_RIGHT_GAIN:
//				case AUXPGA_LEFT_GAIN:
//				case AUXPGA_RIGHT_GAIN:	
				default:
					aud_cal_data->get_param = 0x00; //
					break;
								
			}
		
		}
		break;
/***************************************************** [2] Headset Rx Stereo*******************************************/
		case ACDB_ID_HEADSET_SPKR_STEREO:
 		{
 			switch(aud_cal_data->voccal_param_type)
 			{
 					
				case CODEC_RX_LEFT:
 					aud_cal_data->get_param  = device_cal_data[2].codec_Rx_left_gain.data;
 					break;
						
 				case CODEC_RX_RIGHT:
 					aud_cal_data->get_param  = device_cal_data[2].codec_Rx_right_gain.data;
 					break;

 				case HEADPHONE_LEFT_GAIN:
 					aud_cal_data->get_param  = device_cal_data[2].codec_headset_left.data;
 					break;						

 				case HEADPHONE_RIGHT_GAIN:
 					aud_cal_data->get_param  = device_cal_data[2].codec_headset_right.data;
 					break;

 				case CODEC_RX_FILTER_LEFT:
 					aud_cal_data->get_param  = device_cal_data[2].codec_Rx_filter_left.data;
 					break;

 				case CODEC_RX_FILTER_RIGHT:
 					aud_cal_data->get_param  = device_cal_data[2].codec_Rx_filter_right.data;
 					break;						

				case RX_VOLUME_MAX_NB:	
					aud_cal_data->get_param  = headset_stereo_rx[VOC_NB_INDEX].max_level;
					break;
						
				case RX_VOLUME_MIN_NB:	
					aud_cal_data->get_param  = headset_stereo_rx[VOC_NB_INDEX].min_level;
					break;

				case RX_VOLUME_MAX_WB:	
					aud_cal_data->get_param  = headset_stereo_rx[VOC_WB_INDEX].max_level;
					break;

				case RX_VOLUME_MIN_WB:						
					aud_cal_data->get_param  = headset_stereo_rx[VOC_WB_INDEX].min_level;
					break;
						
 				case CODEC_TX_LEFT:
				case CODEC_TX_RIGHT:
 				case CODEC_ST:
 				case MICAMP_GAIN_LEFT:
				case MICAMP_GAIN_RIGHT:
				case MIC_BIAS_0:
				case MIC_BIAS_1:
//				case AUXPGA_LEFT_GAIN:
//				case AUXPGA_RIGHT_GAIN:	
				default:
					aud_cal_data->get_param = 0x00; 
					break;
 									
 			}
 			
 		}
 		break;
/*************************************************** [3] Headset Rx Mono ********************************************/
		case ACDB_ID_HEADSET_SPKR_MONO:
 		{
 			switch(aud_cal_data->voccal_param_type)
 			{
 					
				case CODEC_RX_LEFT:
 					aud_cal_data->get_param  = device_cal_data[3].codec_Rx_left_gain.data;
 					break;
						
 				case CODEC_RX_RIGHT:
 					aud_cal_data->get_param  = device_cal_data[3].codec_Rx_right_gain.data;
 					break;

 				case HEADPHONE_LEFT_GAIN:
 					aud_cal_data->get_param  = device_cal_data[3].codec_headset_left.data;
 					break;						

 				case HEADPHONE_RIGHT_GAIN:
 					aud_cal_data->get_param  = device_cal_data[3].codec_headset_right.data;
 					break;

 				case CODEC_RX_FILTER_LEFT:
 					aud_cal_data->get_param  = device_cal_data[3].codec_Rx_filter_left.data;
 					break;

 				case CODEC_RX_FILTER_RIGHT:
 					aud_cal_data->get_param  = device_cal_data[3].codec_Rx_filter_right.data;
 					break;						

				case RX_VOLUME_MAX_NB:	
					aud_cal_data->get_param  = headset_mono_rx[VOC_NB_INDEX].max_level;
					break;
						
				case RX_VOLUME_MIN_NB:	
					aud_cal_data->get_param  = headset_mono_rx[VOC_NB_INDEX].min_level;
					break;

				case RX_VOLUME_MAX_WB:	
					aud_cal_data->get_param  = headset_mono_rx[VOC_WB_INDEX].max_level;
					break;

				case RX_VOLUME_MIN_WB:						
					aud_cal_data->get_param  = headset_mono_rx[VOC_WB_INDEX].min_level;
					break;
						
				case CODEC_TX_LEFT:
				case CODEC_TX_RIGHT:
				case CODEC_ST:
				case MICAMP_GAIN_LEFT:
				case MICAMP_GAIN_RIGHT:
				case MIC_BIAS_0:
				case MIC_BIAS_1:
//				case AUXPGA_LEFT_GAIN:
//				case AUXPGA_RIGHT_GAIN:	
				default:
					aud_cal_data->get_param = 0x00; 
					break;
 									
 			}
 			
 		}
 		break;
/******************************************************* [4] Headset Tx********************************************/
        case ACDB_ID_HANDSET_MIC:
		{
			switch(aud_cal_data->voccal_param_type)
			{

				case CODEC_TX_LEFT:
					aud_cal_data->get_param  = device_cal_data[4].codec_Tx_left_gain.data;
					break;
				
				case CODEC_ST:
					aud_cal_data->get_param  = device_cal_data[4].codec_St_gain.data;
					break;
					
				case MICAMP_GAIN_LEFT:
					aud_cal_data->get_param  = device_cal_data[4].codec_micamp_left.data;
					break;

				case CODEC_RX_RIGHT:
				case CODEC_RX_LEFT:
				case CODEC_TX_RIGHT:					
				case MICAMP_GAIN_RIGHT:	
				case CODEC_RX_FILTER_LEFT:					
				case CODEC_RX_FILTER_RIGHT:	
				case HEADPHONE_LEFT_GAIN:
				case HEADPHONE_RIGHT_GAIN:
				case MIC_BIAS_0:
				case MIC_BIAS_1:
//				case AUXPGA_LEFT_GAIN:
//				case AUXPGA_RIGHT_GAIN:					
				default:
					aud_cal_data->get_param = 0x00; //
					break;
								
			}
		
		}
		break;
/***************************************************** [5] Headset Phone Tx******************************************/
        case ACDB_ID_HEADSET_MIC:
		{
			switch(aud_cal_data->voccal_param_type)
			{

				case CODEC_TX_LEFT:
					aud_cal_data->get_param  = device_cal_data[5].codec_Tx_left_gain.data;
					break;
				
				case CODEC_ST:
					aud_cal_data->get_param  = device_cal_data[5].codec_St_gain.data;
					break;
					
				case MICAMP_GAIN_LEFT:
					aud_cal_data->get_param  = device_cal_data[5].codec_micamp_left.data;
					break;

				case CODEC_RX_RIGHT:
				case CODEC_RX_LEFT:
				case CODEC_TX_RIGHT:					
				case MICAMP_GAIN_RIGHT:	
				case CODEC_RX_FILTER_LEFT:					
				case CODEC_RX_FILTER_RIGHT:	
				case HEADPHONE_LEFT_GAIN:
				case HEADPHONE_RIGHT_GAIN:
				case MIC_BIAS_0:
				case MIC_BIAS_1:
//				case AUXPGA_LEFT_GAIN:
//				case AUXPGA_RIGHT_GAIN:					
				default:
					aud_cal_data->get_param = 0x00; //
					break;
								
			}
		
		}
		break;
/**************************************************** [6] Speaker Rx Stereo for Mono***********************************/
		case ACDB_ID_SPKR_PHONE_STEREO:
 		{
 			switch(aud_cal_data->voccal_param_type)
 			{
  				case CODEC_RX_FILTER_LEFT:
 					aud_cal_data->get_param  = device_cal_data[6].codec_Rx_filter_left.data;
 					break;

 				case CODEC_RX_FILTER_RIGHT:
 					aud_cal_data->get_param  = device_cal_data[6].codec_Rx_filter_right.data;
 					break;
						
				case CODEC_RX_LEFT:
 					aud_cal_data->get_param  = device_cal_data[6].codec_Rx_left_gain.data;
 					break;
						
				case CODEC_RX_RIGHT:
 					aud_cal_data->get_param  = device_cal_data[6].codec_Rx_right_gain.data;
 					break;						

				case RX_VOLUME_MAX_NB:	
					aud_cal_data->get_param  = spk_rx[VOC_NB_INDEX].max_level;
					break;
						
				case RX_VOLUME_MIN_NB:	
					aud_cal_data->get_param  = spk_rx[VOC_NB_INDEX].min_level;
					break;

				case RX_VOLUME_MAX_WB:	
					aud_cal_data->get_param  = spk_rx[VOC_WB_INDEX].max_level;
					break;

				case RX_VOLUME_MIN_WB:						
					aud_cal_data->get_param  = spk_rx[VOC_WB_INDEX].min_level;
					break;
						
				case MIC_BIAS_0:
				case MIC_BIAS_1:
				case CODEC_TX_LEFT:
				case CODEC_TX_RIGHT:
 				case CODEC_ST:
 				case MICAMP_GAIN_LEFT:
				case MICAMP_GAIN_RIGHT:
				case HEADPHONE_LEFT_GAIN:
				case HEADPHONE_RIGHT_GAIN:
//				case AUXPGA_LEFT_GAIN:
//				case AUXPGA_RIGHT_GAIN:	
					aud_cal_data->get_param = 0x00; 
					break;
 									
 			}
 			
 		}
 		break;			
/***************************************************** [7] Speaker Tx*************************************************/
 		case ACDB_ID_FM_TX:
		{
			switch(aud_cal_data->voccal_param_type)
			{
				case MIC_BIAS_0:
					aud_cal_data->get_param = mic_bias0_spkmic;
					break;
					
				case MIC_BIAS_1:
					aud_cal_data->get_param = mic_bias1_spkmic;
					break;
					
				case CODEC_TX_LEFT:
					aud_cal_data->get_param  = device_cal_data[7].codec_Tx_left_gain.data;
					break;
				
				case CODEC_TX_RIGHT:
					aud_cal_data->get_param  = device_cal_data[7].codec_Tx_right_gain.data;
					break;

				case CODEC_ST:
					aud_cal_data->get_param  = device_cal_data[7].codec_St_gain.data;
					break;
					
				case MICAMP_GAIN_LEFT:
					aud_cal_data->get_param  = device_cal_data[7].codec_micamp_left.data;
					break;

				case MICAMP_GAIN_RIGHT:
					aud_cal_data->get_param  = device_cal_data[7].codec_micamp_right.data;
					break;
					
				case CODEC_RX_RIGHT:
				case CODEC_RX_LEFT:
				case CODEC_RX_FILTER_LEFT:					
				case CODEC_RX_FILTER_RIGHT:	
				case HEADPHONE_LEFT_GAIN:
				case HEADPHONE_RIGHT_GAIN:
//				case AUXPGA_LEFT_GAIN:
//				case AUXPGA_RIGHT_GAIN:	
				default:
					aud_cal_data->get_param = 0x00; //
					break;
								
			}
		
		}
		break;
/**************************************** [8] Speaker Phone Rx****************************************************/		
            case ACDB_ID_SPKR_PHONE_MONO:
 			{
 				switch(aud_cal_data->voccal_param_type)
 				{
 					
 					case CODEC_RX_FILTER_LEFT:
 						aud_cal_data->get_param  = device_cal_data[8].codec_Rx_filter_left.data;
 						break;

 					case CODEC_RX_FILTER_RIGHT:
 						aud_cal_data->get_param  = device_cal_data[8].codec_Rx_filter_right.data;
 						break;
						
					case CODEC_RX_LEFT:
 						aud_cal_data->get_param  = device_cal_data[8].codec_Rx_left_gain.data;
 						break;
						
					case CODEC_RX_RIGHT:
 						aud_cal_data->get_param  = device_cal_data[8].codec_Rx_right_gain.data;
 						break;						

					case RX_VOLUME_MAX_NB:	
						aud_cal_data->get_param  = spk_stereo_rx[VOC_NB_INDEX].max_level;
						break;
						
					case RX_VOLUME_MIN_NB:	
						aud_cal_data->get_param  = spk_stereo_rx[VOC_NB_INDEX].min_level;
						break;

					case RX_VOLUME_MAX_WB:	
						aud_cal_data->get_param  = spk_stereo_rx[VOC_WB_INDEX].max_level;
						break;

					case RX_VOLUME_MIN_WB:						
						aud_cal_data->get_param  = spk_stereo_rx[VOC_WB_INDEX].min_level;
						break;
						
					case MIC_BIAS_0:
					case MIC_BIAS_1:
					case CODEC_TX_LEFT:
					case CODEC_TX_RIGHT:
 					case CODEC_ST:
 					case MICAMP_GAIN_LEFT:
					case MICAMP_GAIN_RIGHT:
					case HEADPHONE_LEFT_GAIN:
					case HEADPHONE_RIGHT_GAIN:
//					case AUXPGA_LEFT_GAIN:
//					case AUXPGA_RIGHT_GAIN:	
						aud_cal_data->get_param = 0x00; 
						break;
 									
 				}
 			
 			}
 			break;			
/* END:0010882        ehgrace.kim@lge.com     2010.11.15*/
/******************************************** [9] Speaker Phone Mic**************************************************/
 		case ACDB_ID_SPKR_PHONE_MIC:
		{
			switch(aud_cal_data->voccal_param_type)
			{
				case MIC_BIAS_0:
					aud_cal_data->get_param = mic_bias0_spkmic2;
					break;
					
				case MIC_BIAS_1:
					aud_cal_data->get_param = mic_bias1_spkmic2;
					break;
					
				case CODEC_TX_LEFT:
					aud_cal_data->get_param  = device_cal_data[9].codec_Tx_left_gain.data;
					break;
				
				case CODEC_TX_RIGHT:
					aud_cal_data->get_param  = device_cal_data[9].codec_Tx_right_gain.data;
					break;

				case CODEC_ST:
					aud_cal_data->get_param  = device_cal_data[9].codec_St_gain.data;
					break;
					
				case MICAMP_GAIN_LEFT:
					aud_cal_data->get_param  = device_cal_data[9].codec_micamp_left.data;
					break;

				case MICAMP_GAIN_RIGHT:
					aud_cal_data->get_param  = device_cal_data[9].codec_micamp_right.data;
					break;
					
				case CODEC_RX_RIGHT:
				case CODEC_RX_LEFT:
				case CODEC_RX_FILTER_LEFT:					
				case CODEC_RX_FILTER_RIGHT:	
				case HEADPHONE_LEFT_GAIN:
				case HEADPHONE_RIGHT_GAIN:
//				case AUXPGA_LEFT_GAIN:
//				case AUXPGA_RIGHT_GAIN:	
				default:
					aud_cal_data->get_param = 0x00; //
					break;
								
			}
		
		}
		break;
/******************************************* [10] Stereo [ Headset + Speaker ] Rx*****************************************/
		case ACDB_ID_HEADSET_STEREO_PLUS_SPKR_STEREO_RX:
 		{
 			switch(aud_cal_data->voccal_param_type)
 			{
				case CODEC_RX_LEFT:
 					aud_cal_data->get_param  = device_cal_data[10].codec_Rx_left_gain.data;
 					break;
						
 				case CODEC_RX_RIGHT:
 					aud_cal_data->get_param  = device_cal_data[10].codec_Rx_right_gain.data;
 					break;

 				case HEADPHONE_LEFT_GAIN:
 					aud_cal_data->get_param  = device_cal_data[10].codec_headset_left.data;
 					break;						

				case HEADPHONE_RIGHT_GAIN:
					aud_cal_data->get_param  = device_cal_data[10].codec_headset_right.data;
					break;

				case CODEC_RX_FILTER_LEFT:
					aud_cal_data->get_param  = device_cal_data[10].codec_Rx_filter_left.data;
 					break;

				case CODEC_RX_FILTER_RIGHT:
					aud_cal_data->get_param  = device_cal_data[10].codec_Rx_filter_right.data;
					break;						

				case RX_VOLUME_MAX_NB:	
					aud_cal_data->get_param  = headset_spk_rx[VOC_NB_INDEX].max_level;
					break;
						
				case RX_VOLUME_MIN_NB:	
					aud_cal_data->get_param  = headset_spk_rx[VOC_NB_INDEX].min_level;
					break;

				case RX_VOLUME_MAX_WB:	
					aud_cal_data->get_param  = headset_spk_rx[VOC_WB_INDEX].max_level;
					break;

				case RX_VOLUME_MIN_WB:						
					aud_cal_data->get_param  = headset_spk_rx[VOC_WB_INDEX].min_level;
					break;

  				case CODEC_ST:
				case CODEC_TX_LEFT:
				case CODEC_TX_RIGHT:
 				case MICAMP_GAIN_LEFT:
				case MICAMP_GAIN_RIGHT:
				case MIC_BIAS_0:
				case MIC_BIAS_1:
//				case AUXPGA_LEFT_GAIN:
//				case AUXPGA_RIGHT_GAIN:	
				default:
					aud_cal_data->get_param = 0x00; 
					break;
 									
 			}
 			
 		}
 		break;	
/************************************************ [11] Handset Rec Tx****************************************************/
 		case ACDB_ID_I2S_TX:
		{
			switch(aud_cal_data->voccal_param_type)
			{
				case MIC_BIAS_0:
					aud_cal_data->get_param = mic_bias0_handset_rec_mic;
					break;
					
				case MIC_BIAS_1:
					aud_cal_data->get_param = mic_bias1_handset_rec_mic;
					break;
					
				case CODEC_TX_LEFT:
					aud_cal_data->get_param  = device_cal_data[11].codec_Tx_left_gain.data;
					break;
				
				case CODEC_TX_RIGHT:
					aud_cal_data->get_param  = device_cal_data[11].codec_Tx_right_gain.data;
					break;

				case CODEC_ST:
					aud_cal_data->get_param  = device_cal_data[11].codec_St_gain.data;
					break;
					
				case MICAMP_GAIN_LEFT:
					aud_cal_data->get_param  = device_cal_data[11].codec_micamp_left.data;
					break;

				case MICAMP_GAIN_RIGHT:
					aud_cal_data->get_param  = device_cal_data[11].codec_micamp_right.data;
					break;
					
				case CODEC_RX_RIGHT:
				case CODEC_RX_LEFT:
				case CODEC_RX_FILTER_LEFT:					
				case CODEC_RX_FILTER_RIGHT:	
				case HEADPHONE_LEFT_GAIN:
				case HEADPHONE_RIGHT_GAIN:
//				case AUXPGA_LEFT_GAIN:
//				case AUXPGA_RIGHT_GAIN:	
				default:
					aud_cal_data->get_param = 0x00; //
					break;
								
			}
		
		}
		break;
/* BEGIN:0014024  kiran.kanneganti@lge.com     2011.01.13*/
/* MOD:0014024 Calibration support for loop back devices  */
/************************************************ [12] LoopBack Main Mic Tx****************************************************/
 		case LOOPBACK_MAIN_MIC:
		{
			switch(aud_cal_data->voccal_param_type)
			{
					
				case CODEC_TX_LEFT:
					aud_cal_data->get_param  = device_cal_data[12].codec_Tx_left_gain.data;
					break;
				
				case CODEC_TX_RIGHT:
					aud_cal_data->get_param  = device_cal_data[12].codec_Tx_right_gain.data;
					break;

				case CODEC_ST:
					aud_cal_data->get_param  = device_cal_data[12].codec_St_gain.data;
					break;
					
				case MICAMP_GAIN_LEFT:
					aud_cal_data->get_param  = device_cal_data[12].codec_micamp_left.data;
					break;

				case MICAMP_GAIN_RIGHT:
					aud_cal_data->get_param  = device_cal_data[12].codec_micamp_right.data;
					break;
					
				case CODEC_RX_RIGHT:
				case CODEC_RX_LEFT:
				case CODEC_RX_FILTER_LEFT:					
				case CODEC_RX_FILTER_RIGHT:	
				case HEADPHONE_LEFT_GAIN:
				case HEADPHONE_RIGHT_GAIN:
				case MIC_BIAS_0:
				case MIC_BIAS_1:
				default:
					aud_cal_data->get_param = 0x00;
					break;
								
			}
		
		}
		break;

/************************************************  [13] LoopBack BACK Mic Tx****************************************************/
 		case LOOPBACK_BACK_MIC:
		{
			switch(aud_cal_data->voccal_param_type)
			{
					
				case CODEC_TX_LEFT:
					aud_cal_data->get_param  = device_cal_data[13].codec_Tx_left_gain.data;
					break;
				
				case CODEC_TX_RIGHT:
					aud_cal_data->get_param  = device_cal_data[13].codec_Tx_right_gain.data;
					break;

				case CODEC_ST:
					aud_cal_data->get_param  = device_cal_data[13].codec_St_gain.data;
					break;
					
				case MICAMP_GAIN_LEFT:
					aud_cal_data->get_param  = device_cal_data[13].codec_micamp_left.data;
					break;

				case MICAMP_GAIN_RIGHT:
					aud_cal_data->get_param  = device_cal_data[13].codec_micamp_right.data;
					break;
					
				case CODEC_RX_RIGHT:
				case CODEC_RX_LEFT:
				case CODEC_RX_FILTER_LEFT:					
				case CODEC_RX_FILTER_RIGHT:	
				case HEADPHONE_LEFT_GAIN:
				case HEADPHONE_RIGHT_GAIN:
				case MIC_BIAS_0:
				case MIC_BIAS_1:
				default:
					aud_cal_data->get_param = 0x00;
					break;
								
			}
		
		}
		break;	

/*************************************************** [14]LoopBack Handset Rx**************************************************/
		case LOOPBACK_HANDSET_RX: 
		{
			switch(aud_cal_data->voccal_param_type)
			{
				
				case CODEC_RX_LEFT:
					aud_cal_data->get_param  = device_cal_data[14].codec_Rx_left_gain.data;
					break;
				
				case CODEC_RX_FILTER_LEFT:
					aud_cal_data->get_param  = device_cal_data[14].codec_Rx_filter_left.data;
					break;

				case CODEC_RX_FILTER_RIGHT:
					aud_cal_data->get_param  = device_cal_data[14].codec_Rx_filter_right.data;
					break;

				case RX_VOLUME_MAX_NB:	
					aud_cal_data->get_param  = handset_rx[VOC_NB_INDEX].max_level;
					break;
					
				case RX_VOLUME_MIN_NB:	
					aud_cal_data->get_param  = handset_rx[VOC_NB_INDEX].min_level;
					break;

				case RX_VOLUME_MAX_WB:	
					aud_cal_data->get_param  = handset_rx[VOC_WB_INDEX].max_level;
					break;

				case RX_VOLUME_MIN_WB:						
					aud_cal_data->get_param  = handset_rx[VOC_WB_INDEX].min_level;
					break;

				case MIC_BIAS_0:
				case MIC_BIAS_1:
				case CODEC_RX_RIGHT:
				case CODEC_TX_LEFT:
				case CODEC_TX_RIGHT:
				case CODEC_ST:
				case MICAMP_GAIN_LEFT:
				case MICAMP_GAIN_RIGHT:
				case HEADPHONE_LEFT_GAIN:
				case HEADPHONE_RIGHT_GAIN:
				default:
					aud_cal_data->get_param = 0x00; 
					break;
								
			}
		
		}
 		break;
		
/***************************************************** [15]LoopBack Headset Phone Tx******************************************/
		case LOOPBACK_HEADSET_TX:
		{
			switch(aud_cal_data->voccal_param_type)
			{

				case CODEC_TX_LEFT:
					aud_cal_data->get_param  = device_cal_data[15].codec_Tx_left_gain.data;
					break;
				
				case CODEC_ST:
					aud_cal_data->get_param  = device_cal_data[15].codec_St_gain.data;
					break;
					
				case MICAMP_GAIN_LEFT:
					aud_cal_data->get_param  = device_cal_data[15].codec_micamp_left.data;
					break;

				case CODEC_RX_RIGHT:
				case CODEC_RX_LEFT:
				case CODEC_TX_RIGHT:					
				case MICAMP_GAIN_RIGHT: 
				case CODEC_RX_FILTER_LEFT:					
				case CODEC_RX_FILTER_RIGHT: 
				case HEADPHONE_LEFT_GAIN:
				case HEADPHONE_RIGHT_GAIN:
				case MIC_BIAS_0:
				case MIC_BIAS_1:
				default:
					aud_cal_data->get_param = 0x00;
					break;
								
			}
		
		}
		break;
		
/*************************************************** [16] LoopBack Headset Rx Mono ********************************************/
		case LOOPBACK_HEADSET_RX:
		{
			switch(aud_cal_data->voccal_param_type)
			{
					
				case CODEC_RX_LEFT:
					aud_cal_data->get_param  = device_cal_data[16].codec_Rx_left_gain.data;
					break;
						
				case CODEC_RX_RIGHT:
					aud_cal_data->get_param  = device_cal_data[16].codec_Rx_right_gain.data;
					break;

				case HEADPHONE_LEFT_GAIN:
					aud_cal_data->get_param  = device_cal_data[16].codec_headset_left.data;
					break;						

				case HEADPHONE_RIGHT_GAIN:
					aud_cal_data->get_param  = device_cal_data[16].codec_headset_right.data;
					break;

				case CODEC_RX_FILTER_LEFT:
					aud_cal_data->get_param  = device_cal_data[16].codec_Rx_filter_left.data;
					break;

				case CODEC_RX_FILTER_RIGHT:
					aud_cal_data->get_param  = device_cal_data[16].codec_Rx_filter_right.data;
					break;						

				case RX_VOLUME_MAX_NB:	
					aud_cal_data->get_param  = headset_mono_rx[VOC_NB_INDEX].max_level;
					break;
						
				case RX_VOLUME_MIN_NB:	
					aud_cal_data->get_param  = headset_mono_rx[VOC_NB_INDEX].min_level;
					break;

				case RX_VOLUME_MAX_WB:	
					aud_cal_data->get_param  = headset_mono_rx[VOC_WB_INDEX].max_level;
					break;

				case RX_VOLUME_MIN_WB:						
					aud_cal_data->get_param  = headset_mono_rx[VOC_WB_INDEX].min_level;
					break;
						
				case CODEC_TX_LEFT:
				case CODEC_TX_RIGHT:
				case CODEC_ST:
				case MICAMP_GAIN_LEFT:
				case MICAMP_GAIN_RIGHT:
				case MIC_BIAS_0:
				case MIC_BIAS_1:
				default:
					aud_cal_data->get_param = 0x00; 
					break;									
			}			
		}
		break;
/*[START] jy0127.jang@lge.com MS910_AUDIO_CHANGE 2011-04-07*/
/*added speaker phone loopback*/

/************************************************  [17] speakerphone Tx****************************************************/
 		case LOOPBACK_SPEAKERPHONE_TX :
		{
			switch(aud_cal_data->voccal_param_type)
			{
					
				case CODEC_TX_LEFT:
					aud_cal_data->get_param  = device_cal_data[17].codec_Tx_left_gain.data;
					break;
				
				case CODEC_TX_RIGHT:
					aud_cal_data->get_param  = device_cal_data[17].codec_Tx_right_gain.data;
					break;

				case CODEC_ST:
					aud_cal_data->get_param  = device_cal_data[17].codec_St_gain.data;
					break;
					
				case MICAMP_GAIN_LEFT:
					aud_cal_data->get_param  = device_cal_data[17].codec_micamp_left.data;
					break;

				case MICAMP_GAIN_RIGHT:
					aud_cal_data->get_param  = device_cal_data[17].codec_micamp_right.data;
					break;
					
				case CODEC_RX_RIGHT:
				case CODEC_RX_LEFT:
				case CODEC_RX_FILTER_LEFT:					
				case CODEC_RX_FILTER_RIGHT:	
				case HEADPHONE_LEFT_GAIN:
				case HEADPHONE_RIGHT_GAIN:
				case MIC_BIAS_0:
				case MIC_BIAS_1:
				default:
					aud_cal_data->get_param = 0x00;
					break;
								
			}
		
		}
		break;

/*************************************************** [18]speakerphone Rx**************************************************/
		case LOOPBACK_SPEAKERPHONE_RX: 
		{
			switch(aud_cal_data->voccal_param_type)
			{
				
				case CODEC_RX_LEFT:
					aud_cal_data->get_param  = device_cal_data[18].codec_Rx_left_gain.data;
					break;
				
				case CODEC_RX_FILTER_LEFT:
					aud_cal_data->get_param  = device_cal_data[18].codec_Rx_filter_left.data;
					break;

				case CODEC_RX_FILTER_RIGHT:
					aud_cal_data->get_param  = device_cal_data[18].codec_Rx_filter_right.data;
					break;

				case RX_VOLUME_MAX_NB:	
					aud_cal_data->get_param  = spk_stereo_rx[VOC_NB_INDEX].max_level;
					break;
					
				case RX_VOLUME_MIN_NB:	
					aud_cal_data->get_param  = spk_stereo_rx[VOC_NB_INDEX].min_level;
					break;

				case RX_VOLUME_MAX_WB:	
					aud_cal_data->get_param  = spk_stereo_rx[VOC_WB_INDEX].max_level;
					break;

				case RX_VOLUME_MIN_WB:						
					aud_cal_data->get_param  = spk_stereo_rx[VOC_WB_INDEX].min_level;
					break;

				case MIC_BIAS_0:
				case MIC_BIAS_1:
				case CODEC_RX_RIGHT:
				case CODEC_TX_LEFT:
				case CODEC_TX_RIGHT:
				case CODEC_ST:
				case MICAMP_GAIN_LEFT:
				case MICAMP_GAIN_RIGHT:
				case HEADPHONE_LEFT_GAIN:
				case HEADPHONE_RIGHT_GAIN:
				default:
					aud_cal_data->get_param = 0x00; 
					break;
								
			}
		
		}
 		break;		
/*[END] jy0127.jang@lge.com MS910_AUDIO_CHANGE 2011-04-07*/
		
/* END:0014024  kiran.kanneganti@lge.com     2011.01.13*/
/*******************************For BT Rx Volume**********************************************************/
		case ACDB_ID_BT_SCO_SPKR:
		{
			switch(aud_cal_data->voccal_param_type)
			{
				case RX_VOLUME_MAX_NB:	
					aud_cal_data->get_param  = BT_earpiece_rx[VOC_NB_INDEX].max_level;
					break;
					
				case RX_VOLUME_MIN_NB:	
					aud_cal_data->get_param  = BT_earpiece_rx[VOC_NB_INDEX].min_level;
					break;

				case RX_VOLUME_MAX_WB:	
					aud_cal_data->get_param  = BT_earpiece_rx[VOC_WB_INDEX].max_level;
					break;

				case RX_VOLUME_MIN_WB:						
					aud_cal_data->get_param  = BT_earpiece_rx[VOC_WB_INDEX].min_level;
					break;

				case MIC_BIAS_0:
				case MIC_BIAS_1:
				case CODEC_RX_LEFT:
				case CODEC_RX_RIGHT:
				case CODEC_TX_LEFT:
				case CODEC_TX_RIGHT:
				case CODEC_ST:
				case CODEC_RX_FILTER_RIGHT:					
				case CODEC_RX_FILTER_LEFT:	
				case MICAMP_GAIN_LEFT:
				case MICAMP_GAIN_RIGHT:
				case HEADPHONE_LEFT_GAIN:
				case HEADPHONE_RIGHT_GAIN:
//				case AUXPGA_RIGHT_GAIN:	
				default:
					aud_cal_data->get_param = 0x00;
					break;						
						
			}
					
		}
		break;
		
		default:
			aud_cal_data->get_param = 0x00;
			break;
		
 	}
 }
 
// EXPORT_SYMBOL(Snddev_AudioCal_Get);
/* END:0012384 ehgrace.kim@lge.com     2010.12.14*/


static long audio_cal_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct audio_cal *audio_cal_ptr = file->private_data;
	int rc = -EINVAL;
	struct msm_snd_set_audcal_param audcal;
	MM_INFO("audio_cal_ioctl Called \n");
	MM_INFO("audio_cal_ioctl cmd = %d \n ",cmd);

	mutex_lock(&audio_cal_ptr->lock);
	switch(cmd)
	{
		case  SND_SET_AUDCAL_PARAM:
			MM_INFO(" audio_cal_ioctl SND_SET_MICBIAS_PARAM cmd \n ");
			if (copy_from_user(&audcal, (void __user *) arg, sizeof(audcal)))
			{
				printk("audio_cal_ioctl: invalid pointer.\n");
				rc = -EFAULT;
				break;
			}
#if 1	// daniel.kang for debugging			
			MM_INFO("Voc Codec valiue = %d\n",(audcal.voc_codec));
			MM_INFO("Voccal param Type   = %d\n",(audcal.voccal_param_type));
			MM_INFO("Set / Get = %d\n",(audcal.get_flag));
			MM_INFO("Param Value = %d\n",(audcal.param_val));
			MM_INFO("Get Param Value = %d\n",(audcal.get_param));
#endif			
			if((audcal.get_flag) == 1)
				Snddev_AudioCal_Set(&audcal);
			else
				Snddev_AudioCal_Get(&audcal);				

			if (copy_to_user((void __user*)arg, &audcal, sizeof(audcal))){
					MM_INFO("audio_cal_ioctl sent invalid data\n");
			}
			break;
		
		default:
			MM_INFO("audio_cal_ioctl default cmd\n");
			break;

	}
	mutex_unlock(&audio_cal_ptr->lock);

	MM_INFO("audio_cal_ioctl Done \n");

	return 0;
	
}

static int audio_cal_open(struct inode *ip, struct file *fp)
{
	struct audio_cal *audio_cal_ptr = &audio_cal_file;
	int rc = 0;

	MM_INFO("audio_cal_open Called \n");
	if (audio_cal_ptr->opened)
	{
		MM_INFO("audio cal driver already open\n");
		return -EPERM;
	}	

	Snddev_Fill_Device_Data();
	audio_cal_ptr->opened = 1;
	fp->private_data = audio_cal_ptr;

	return rc;
}

static int audio_cal_close(struct inode *inode, struct file *file)
{
	struct audio_cal *audio_cal_ptr = file->private_data;

	MM_INFO("audio_cal_close Called \n");

	audio_cal_ptr->opened = 0; 
	
	return 0;
}

static int __init audio_cal_init(void)
{
	struct audio_cal *audio_cal_ptr = &audio_cal_file;

	mutex_init(&audio_cal_ptr->lock);
	MM_INFO("audio_cal_init Called \n");
	return misc_register(&audio_cal_misc);

}

static void __exit audio_cal_exit(void)
{
	MM_INFO("audio_cal_exit Called \n");
}

module_init(audio_cal_init);
module_exit(audio_cal_exit);

MODULE_DESCRIPTION("MSM 7x30 Audio ACDB driver");
MODULE_LICENSE("GPL v2");
