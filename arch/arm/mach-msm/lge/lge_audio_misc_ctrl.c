/*  arch/arm/mach-msm/qdsp5v2/lge_audio_misc_ctl.c
 *
 * Copyright (C) 2009 LGE, Inc.
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
#include <mach/qdsp5v2/snddev_icodec.h>
#include <mach/qdsp5v2/snddev_ecodec.h>
#include <mach/qdsp5v2/snddev_mi2s.h>
#include <mach/qdsp5v2/snddev_virtual.h>
/*BEGIN: 0011452 kiran.kanneganti@lge.com 2010-11-26*/
/*ADD 0011452: Noice cancellation check support for testmode*/
#include <linux/mfd/msm-adie-codec.h>
#include <linux/delay.h>
/* END: 0011452 kiran.kanneganti@lge.com 2010-11-26 */
#include <mach/lge_audio_misc_ctl.h>

/*RPC versions*/
#define VOEM_IFPROG					0x300000A3
#define VOEM_IFVERS					0x00020002
/*RPC loopback proc */
#define CALLBACK_NULL     0xffffffff
#define RPC_VOEM_PACKET_LOOPBACK_PROC 5
/*RPC Rx vol proc*/
#define RPC_VOEM_RX_VOL_PROC 6
/*Packet loopback device ID's*/
#define HANDSET_PACKET_LOOPBACK_ID 0x1
#define HEADSET_PACKET_LOOPBACK_ID 0x2
#define QTR_HANDSET_PACKET_LOOPBACK_ID 0x3
#define BT_SCO_PACKET_LOOPBACK_ID 0x4
/*BEGIN: 0011453 kiran.kanneganti@lge.com 2010-11-26*/
/*ADD 0011453: Front Mic & Back Mic Separate LoopBack Support*/
#define BACK_MIC_PACKET_LOOPBACK_ID 0x5
/* END: 0011453 kiran.kanneganti@lge.com 2010-11-26 */
/*[START] jy0127.jang@lge.com MS910_AUDIO_CHANGE 2011-04-07*/
/*added speaker phone loopback*/
#define QTR_SPEAKERPHONE_PACKET_LOOPBACK_ID 0x7
/*[END] jy0127.jang@lge.com MS910_AUDIO_CHANGE 2011-04-07*/

/* Loopback actions*/
#define ACOUSTIC_OFF 0x00		//Packet Loopback
#define ACOUSTIC_ON 0x01		//packet loopback off
#define HEADSET_LB_PATH 0x02	//set headset path
#define HANDSET_LB_PATH 0x03	//set handset path
#define ACOUSTIC_LOOPBACK_ON 0x04//dsp loopback
#define ACOUSTIC_LOOPBACK_OFF 0x05//dsp loopback off
/*jy0127.jang@lge.com MS910_AUDIO_CHANGE 2011-04-07*/
/*added speaker phone loopback*/
#define SPEAKERPHONE_LB_PATH 0x08	//set speakerphone path
							 	
/*******************************************************************************
*  Structure for misc controls.  Add variables if needed.
*
********************************************************************************/
struct audio {
	struct mutex lock;
	struct msm_rpc_endpoint *ept;
	int opened;
	int path_set;
	int prev_path_set;
	int dsp_loopback_opened;
	int packet_loopback_opened;
/*BEGIN: 0011453 kiran.kanneganti@lge.com 2010-11-26*/
/*ADD 0011453: Front Mic & Back Mic Separate LoopBack Support*/	
	int use_back_mic;
/* END: 0011453 kiran.kanneganti@lge.com 2010-11-26 */	
};
static struct audio misc_audio;
/*******************************************************************************
*  Structure for loopback RPC .  Add variables if needed.
*
********************************************************************************/
struct rpc_snd_set_loopback_mode_args {
     uint32_t mode;
     uint32_t cb_func;
     uint32_t client_data;
	 int rx_vol;
};
struct snd_set_loopback_mode_msg {
    struct rpc_request_hdr hdr;
    struct rpc_snd_set_loopback_mode_args args;
};
struct snd_set_loopback_mode_msg lmsg;
/*******************************************************************************
*  Structure for Rx volume RPC .  Add variables if needed.
*
********************************************************************************/
struct rpc_snd_set_rx_vol_args {
 	 int rx_vol;
     int device;
     uint32_t cb_func;
     uint32_t client_data;
};
struct snd_set_rx_vol_msg {
    struct rpc_request_hdr hdr;
    struct rpc_snd_set_rx_vol_args args;
};
struct snd_set_rx_vol_msg rx_vol_msg;

static int current_rx_voice_vol = 100;
/*******************************************************************************
*	Function Name :  audio_misc_driver_test
*	Args : test function ID.
*	dependencies : TEST_MISC_DRVR macro 
*	Note: Exposed this function for kernal. Can test driver from any kernal event
********************************************************************************/
#if TEST_MISC_DRVR
#define TEST_RPC_CONNECT_FN 	1
#define TEST_RPC_LB_FN      	2
#define TEST_RPC_LB_FN_DN      	3
#define TEST_SET_RX_MIN_VOL_PROC    4
#define TEST_SET_RX_MAX_VOL_PROC    5
#if LOOPBACK_SUPPORT
static void start_packet_loopback(int device,struct audio *audio_for_rpc);
static void stop_packet_loopback(int device,struct audio *audio_for_rpc);
static void set_rx_voice_volume(int device,struct audio *audio_for_rpc);
#endif
int audio_misc_driver_test(unsigned int test_fn)
{
	struct audio *audio = &misc_audio;
	int rc = 0;
	MM_INFO("test function %d\n",test_fn);
	switch(test_fn)
	{
		case TEST_RPC_CONNECT_FN:
			MM_INFO("Testing RPC connect");
			if (audio->ept == NULL)
			{
				audio->ept = msm_rpc_connect_compatible(VOEM_IFPROG,VOEM_IFVERS, 0);
				if (IS_ERR(audio->ept))
				{
					rc = PTR_ERR(audio->ept);
					audio->ept = NULL;
					MM_ERR("failed to connect VOEM svc\n");					
				}
				else
					MM_INFO("Connected VOEM svc successfully\n");					
			}			
			break;
#if LOOPBACK_SUPPORT
		case TEST_RPC_LB_FN:
			MM_INFO("Testing RPC Loopback function");
			start_packet_loopback(QTR_HANDSET_PACKET_LOOPBACK_ID,audio);
			break;
		case TEST_RPC_LB_FN_DN:
			MM_INFO("Testing RPC Loopback disable function");
			stop_packet_loopback(QTR_HANDSET_PACKET_LOOPBACK_ID,audio);
			break;	
#endif
		case TEST_SET_RX_MIN_VOL_PROC:
			MM_INFO("Testing Minimum RX vol function");
			current_rx_voice_vol = 40;
			set_rx_voice_volume(QTR_HANDSET_PACKET_LOOPBACK_ID,audio);
			break;
		case TEST_SET_RX_MAX_VOL_PROC:
			MM_INFO("Testing Maximum RX vol function");
			current_rx_voice_vol = 100;
			set_rx_voice_volume(QTR_HANDSET_PACKET_LOOPBACK_ID,audio);
			break;
		default:
			MM_INFO("No such function\n");	
			break;
	}
}
EXPORT_SYMBOL(audio_misc_driver_test);
#endif

/*******************************************************************************
*	Function Name :  get_bryce_audio_device_data
*	Args : Tx/Rx device ID, parameter need to be read.
*	dependencies : None
*	Note: Can export & use from kernel space to read any audio device data.
*			Check caliculate_current_rx_vol for usage.
********************************************************************************/
void* get_bryce_audio_device_data (int iDevId, audio_parameter_type param)
{
	struct platform_device* pDev = NULL;
	struct snddev_icodec_data* pIcodec_dev_data = NULL;
	struct snddev_ecodec_data* pEcodec_dev_data = NULL;
	struct snddev_mi2s_data* pMi2s_dev_data = NULL;
	struct snddev_virtual_data* pVirtual_dev_data = NULL;
	void *result = NULL;	
	MSM7x30_CODEC_TYPE cdc_type = MAX_CDC_TYPE;
	
	GET_DEV_POINTER(pDev,iDevId);
	if( NULL == pDev)
		return NULL;
	MM_INFO("Audio device Addr %u \n",pDev);
	
	GET_CODEC_TYPE(cdc_type,pDev);		
	if( MAX_CDC_TYPE == cdc_type)
		return NULL;
	MM_INFO("Audio Codec type %d \n",cdc_type);
	
	switch (cdc_type)
	{
		case ICODEC:
			pIcodec_dev_data = (struct snddev_icodec_data*)pDev->dev.platform_data;
			break;
		case ECODEC:
			pEcodec_dev_data = (struct snddev_ecodec_data*)pDev->dev.platform_data;
			break;
		case MI2S:
			pMi2s_dev_data = (struct snddev_mi2s_data*)pDev->dev.platform_data;
			break;
		case VCODEC:
			pVirtual_dev_data = (struct snddev_virtual_data*)pDev->dev.platform_data;
			break;
		case MAX_CDC_TYPE:
			MM_INFO("Max codec case\n");
			break;
	}

	switch (param)
	{
		case RX_VOL_MIN:
			
			if( NULL != pIcodec_dev_data)
				GET_RX_MIN_VOL(result,pIcodec_dev_data,cdc_type)
			else if( NULL != pEcodec_dev_data)
				GET_RX_MIN_VOL(result,pEcodec_dev_data,cdc_type)			
			break;
			
		case RX_VOL_MAX:

			if( NULL != pIcodec_dev_data)
				GET_RX_MAX_VOL(result,pIcodec_dev_data,cdc_type)
			else if( NULL != pEcodec_dev_data)
				GET_RX_MAX_VOL(result,pEcodec_dev_data,cdc_type)			
			break;

		case MAX_PARAM:
			MM_INFO("Max Parameter case\n");
			break;
	}
	
	return result;
}

/*******************************************************************************
*	Function Name :  caliculate_current_rx_vol
*	Args : Device ID.
*	dependencies : None
*	
********************************************************************************/
int caliculate_current_rx_vol(int device)
{	
	s32 min_rx_vol_nb = -1100,max_rx_vol_nb = 400; //deafult values.
	int current_rx_vol_mb;
	void *rx_vol_ptr = NULL;

/* daniel.kang@lge.com ++ Nov 17 // 0011018: Remove Audience Code */
// Handset pack loopback is for audience
#if 0
	if( HANDSET_PACKET_LOOPBACK_ID == device)
	{
		rx_vol_ptr = get_bryce_audio_device_data(MSM_IEARPIECE_AUDIENCE_DEVICE,RX_VOL_MIN);
		if(NULL != rx_vol_ptr)
			min_rx_vol_nb = *(s32*)rx_vol_ptr;
		else
			MM_ERR("Error in gettign device data ::MSM_IEARPIECE_AUDIENCE_DEVICE:: \n");
		
		MM_INFO("Min Rx voice vol %d\n",min_rx_vol_nb);	
		
		rx_vol_ptr = NULL;
		rx_vol_ptr = get_bryce_audio_device_data(MSM_IEARPIECE_AUDIENCE_DEVICE,RX_VOL_MAX);
		if(NULL != rx_vol_ptr)
			max_rx_vol_nb = *(s32*)rx_vol_ptr;
		else
			MM_ERR("Error in gettign device data ::MSM_IEARPIECE_AUDIENCE_DEVICE:: \n");
		
		MM_INFO("Max Rx voice vol %d\n",max_rx_vol_nb);

		current_rx_vol_mb =  min_rx_vol_nb +
			((max_rx_vol_nb - min_rx_vol_nb) * current_rx_voice_vol)/100;

		MM_INFO("Current Rx voice vol in mb %d\n",current_rx_vol_mb);
		
	}
	else
#endif
/* daniel.kang@lge.com -- Nov 17 // 0011018: Remove Audience Code */
	if( HEADSET_PACKET_LOOPBACK_ID == device )
	{
/*BEGIN: 0012072 kiran.kanneganti@lge.com 2010-12-09*/
/*Mod 0012072: Use mono headset Rx volume incase of headset loopback*/	
		rx_vol_ptr = get_bryce_audio_device_data(MSM_IHS_MONO_RX_DEVICE,RX_VOL_MIN);
		if(NULL != rx_vol_ptr)
			min_rx_vol_nb = *(s32*)rx_vol_ptr;
		else
			MM_ERR("Error in gettign device data ::MSM_IHS_MONO_RX_DEVICE:: \n");
		
		MM_INFO("Min Rx voice vol %d\n",min_rx_vol_nb);	
		
		rx_vol_ptr = NULL;
		rx_vol_ptr = get_bryce_audio_device_data(MSM_IHS_MONO_RX_DEVICE,RX_VOL_MAX);
		if(NULL != rx_vol_ptr)
			max_rx_vol_nb = *(s32*)rx_vol_ptr;
		else
			MM_ERR("Error in gettign device data ::MSM_IHS_MONO_RX_DEVICE:: \n");
/*END: 0012072 kiran.kanneganti@lge.com 2010-12-09*/		
		MM_INFO("Max Rx voice vol %d\n",max_rx_vol_nb);	

		current_rx_vol_mb =  min_rx_vol_nb +
			((max_rx_vol_nb - min_rx_vol_nb) * current_rx_voice_vol)/100;
		
/*[START] jy0127.jang@lge.com MS910_AUDIO_CHANGE 2011-06-27*/
/*added acoustic volume when testmode volume level set 0*/
		if(current_rx_vol_mb==min_rx_vol_nb)
			current_rx_vol_mb = -5000;
/*[END] jy0127.jang@lge.com MS910_AUDIO_CHANGE 2011-06-27*/
/* BEGIN: 0010969 kiran.kanneganti@lge.com 2010-11-16 */
/* MOD 0010969: Calibration Data & functions changed for clear voice in loop back test */			
/*BEGIN: 0013991 kiran.kanneganti@lge.com 2011-01-13*/
/*MOD 0013991: Reduced Rx Volume to avoid noise.*/
//		current_rx_vol_mb += 1500;
/*END: 0013991 kiran.kanneganti@lge.com 2011-01-13*/
/* END: 0010969 kiran.kanneganti@lge.com 2010-11-16 */ 		
		MM_INFO("Current Rx voice vol in mb %d\n",current_rx_vol_mb);
	}
	else if(QTR_HANDSET_PACKET_LOOPBACK_ID == device)
	{
		rx_vol_ptr = get_bryce_audio_device_data(MSM_IEARPIECE_DEVICE,RX_VOL_MIN);
		if(NULL != rx_vol_ptr)
			min_rx_vol_nb = *(s32*)rx_vol_ptr;
		else
			MM_ERR("Error in gettign device data ::MSM_IEARPIECE_DEVICE:: \n");
		
		MM_INFO("Min Rx voice vol %d\n",min_rx_vol_nb);	
		
		rx_vol_ptr = NULL;
		rx_vol_ptr = get_bryce_audio_device_data(MSM_IEARPIECE_DEVICE,RX_VOL_MAX);
		if(NULL != rx_vol_ptr)
			max_rx_vol_nb = *(s32*)rx_vol_ptr;
		else
			MM_ERR("Error in gettign device data ::MSM_IEARPIECE_DEVICE:: \n");
		
		MM_INFO("Max Rx voice vol %d\n",max_rx_vol_nb);		

		current_rx_vol_mb =  min_rx_vol_nb +
			((max_rx_vol_nb - min_rx_vol_nb) * current_rx_voice_vol)/100;

/*[START] jy0127.jang@lge.com MS910_AUDIO_CHANGE 2011-06-27*/
/*added acoustic volume when testmode volume level set 0*/
		if(current_rx_vol_mb==min_rx_vol_nb)
			current_rx_vol_mb = -5000;
/*[END] jy0127.jang@lge.com MS910_AUDIO_CHANGE 2011-06-27*/
		
		MM_INFO("Current Rx voice vol in mb %d\n",current_rx_vol_mb);
	}
/*[START] jy0127.jang@lge.com MS910_AUDIO_CHANGE 2011-04-07*/
/*added speaker phone loopback*/
	else if(QTR_SPEAKERPHONE_PACKET_LOOPBACK_ID == device)
	{
		rx_vol_ptr = get_bryce_audio_device_data(MSM_IEARPIECE_DEVICE,RX_VOL_MIN);
		if(NULL != rx_vol_ptr)
			min_rx_vol_nb = *(s32*)rx_vol_ptr;
		else
			MM_ERR("Error in gettign device data ::MSM_ISPEAKER_PHONE_RX_DEVICE:: \n");
		
		MM_INFO("Min Rx voice vol %d\n",min_rx_vol_nb);	
		
		rx_vol_ptr = NULL;
		rx_vol_ptr = get_bryce_audio_device_data(MSM_IEARPIECE_DEVICE,RX_VOL_MAX);
		if(NULL != rx_vol_ptr)
			max_rx_vol_nb = *(s32*)rx_vol_ptr;
		else
			MM_ERR("Error in gettign device data ::MSM_ISPEAKER_PHONE_RX_DEVICE:: \n");
		
		MM_INFO("Max Rx voice vol %d\n",max_rx_vol_nb);		

		current_rx_vol_mb =  min_rx_vol_nb +
			((max_rx_vol_nb - min_rx_vol_nb) * current_rx_voice_vol)/100;
		
		MM_INFO("Current Rx voice vol in mb %d\n",current_rx_vol_mb);
	}	
/*[END] jy0127.jang@lge.com MS910_AUDIO_CHANGE 2011-04-07*/
	else
	{
		MM_ERR("No support for this device\n");
		current_rx_vol_mb = 100; //default rx vol
	}
	
	return current_rx_vol_mb;
}
#if LOOPBACK_SUPPORT

/*******************************************************************************
*	Function Name :  start_packet_loopback
*	Args : Device ID.
*	dependencies : Loopback path should be set before
*	
********************************************************************************/
static void start_packet_loopback(int device,struct audio *audio_for_rpc)
{
	int rc;
/* daniel.kang@lge.com ++ Nov 17 // 0011018: Remove Audience Code */
#if 0
	if ( HANDSET_PACKET_LOOPBACK_ID == device)
	{
		handset_loopback_for_testmode(0x01);
	}
	else
#endif
/* daniel.kang@lge.com -- Nov 17 // 0011018: Remove Audience Code */
	if ( HEADSET_PACKET_LOOPBACK_ID == device )
	{
		headset_loopback_for_testmode( 0x01);
	}
	else if(QTR_HANDSET_PACKET_LOOPBACK_ID == device)
	{
/*BEGIN: 0011453 kiran.kanneganti@lge.com 2010-11-26*/
/*ADD 0011453: Front Mic & Back Mic Separate LoopBack Support*/	
		if(audio_for_rpc->use_back_mic)
			QTR_handset_loopback_for_testmode(0x05);
		else
			QTR_handset_loopback_for_testmode(0x01);
/* END: 0011453 kiran.kanneganti@lge.com 2010-11-26 */			
	}
/*[START] jy0127.jang@lge.com MS910_AUDIO_CHANGE 2011-04-07*/
/*added speaker phone loopback*/
	else if(QTR_SPEAKERPHONE_PACKET_LOOPBACK_ID == device)
	{
		if(audio_for_rpc->use_back_mic)
			QTR_handset_loopback_for_testmode(0x05);
		else
			QTR_speakerphone_loopback_for_testmode(0x01);
	}
/*[END] jy0127.jang@lge.com MS910_AUDIO_CHANGE 2011-04-07*/
	else
	{
		MM_ERR("No support for this device\n");
		return;
	}
	if (audio_for_rpc->ept)
	{
		lmsg.args.mode = cpu_to_be32( 0x01 | (0x01 << device));
		lmsg.args.cb_func = CALLBACK_NULL;
		lmsg.args.client_data = 0;
		lmsg.args.rx_vol = cpu_to_be32( caliculate_current_rx_vol(device)); // added volume support in loopback. kiran.kanneganti@lge.com
		rc = msm_rpc_call(audio_for_rpc->ept,RPC_VOEM_PACKET_LOOPBACK_PROC,&lmsg, sizeof(lmsg), 5 * HZ);
		if (rc < 0){
			MM_ERR("failed to do loopback\n");
		}
		else
			MM_INFO("Loopback function called successfully\n");						
	}	
	else
		MM_ERR("End point not yet opened\n");
}
/*******************************************************************************
*	Function Name :  stop_packet_loopback
*	Args : Device ID.
*	dependencies : Loopback path should be set before
*	
********************************************************************************/
static void stop_packet_loopback(int device,struct audio *audio_for_rpc)
{
	int rc;
/* daniel.kang@lge.com ++ Nov 17 // 0011018: Remove Audience Code */
#if 0
	if ( HANDSET_PACKET_LOOPBACK_ID == device)
	{
		handset_loopback_for_testmode(0x00);
	}
	else
#endif
/* daniel.kang@lge.com -- Nov 17 // 0011018: Remove Audience Code */
	if ( HEADSET_PACKET_LOOPBACK_ID == device )
	{
		headset_loopback_for_testmode( 0x00);
	}
	else if( QTR_HANDSET_PACKET_LOOPBACK_ID == device)
	{
		QTR_handset_loopback_for_testmode(0x00);
	}
/*[START] jy0127.jang@lge.com MS910_AUDIO_CHANGE 2011-04-07*/
/*added speaker phone loopback*/
	else if( QTR_SPEAKERPHONE_PACKET_LOOPBACK_ID == device)
	{
		QTR_speakerphone_loopback_for_testmode(0x00);
	}
/*[END] jy0127.jang@lge.com MS910_AUDIO_CHANGE 2011-04-07*/
	else
	{
		MM_ERR("No support for this device\n");
		return;
	}
	if (audio_for_rpc->ept)
	{
		lmsg.args.mode = cpu_to_be32( 0x00 );
		lmsg.args.cb_func = CALLBACK_NULL;
		lmsg.args.client_data = 0;
		lmsg.args.rx_vol = cpu_to_be32(100);//No need. let it be default.
		rc = msm_rpc_call(audio_for_rpc->ept,RPC_VOEM_PACKET_LOOPBACK_PROC,&lmsg, sizeof(lmsg), 5 * HZ);
		if (rc < 0){
			MM_ERR("failed to do loopback\n");
		}
		else
			MM_INFO("Loopback function called successfully\n");						
	}	
	else
		MM_ERR("End point not yet opened\n");	
}

/*******************************************************************************
*	Function Name :  start_dsp_loopback
*	Args : Device ID.
*	dependencies : Loopback path should be set before
*	Note: For AUX PCM using packet loopback only.
********************************************************************************/
/*BEGIN: 0010850 kiran.kanneganti@lge.com 2010-11-15*/
/*ADD 0010850: Ext Codec PCM Loopback support*/
static void start_dsp_loopback(int device,struct audio *audio_for_rpc)
{
/* daniel.kang@lge.com ++ Nov 17 // 0011018: Remove Audience Code */
#if 0
	if ( HANDSET_PACKET_LOOPBACK_ID == device)
	{
		handset_loopback_for_testmode(0x03);
	}
	else
#endif
/* daniel.kang@lge.com -- Nov 17 // 0011018: Remove Audience Code */
	if ( HEADSET_PACKET_LOOPBACK_ID == device )
	{
		headset_loopback_for_testmode( 0x03);
	}
	else if( QTR_HANDSET_PACKET_LOOPBACK_ID == device)
	{
		QTR_handset_loopback_for_testmode(0x03);
	}
/*[START] jy0127.jang@lge.com MS910_AUDIO_CHANGE 2011-04-07*/
/*added speaker phone loopback*/
	else if( QTR_SPEAKERPHONE_PACKET_LOOPBACK_ID == device)
	{
		QTR_speakerphone_loopback_for_testmode(0x03);
	}
/*[END] jy0127.jang@lge.com MS910_AUDIO_CHANGE 2011-04-07*/
	else if(BT_SCO_PACKET_LOOPBACK_ID == device)
	{
		bt_sco_loopback_for_testmode(0x03);
	}
	else
	{
		MM_ERR("No support for this device\n");
		return;
	}	
}
/* END: 0010850 kiran.kanneganti@lge.com 2010-11-15 */
/*******************************************************************************
*	Function Name :  stop_dsp_loopback
*	Args : Device ID.
*	dependencies : Loopback path should be set before
*	Note: For AUX PCM using packet loopback only.
********************************************************************************/
/*BEGIN: 0010850 kiran.kanneganti@lge.com 2010-11-15*/
/*ADD 0010850: Ext Codec PCM Loopback support*/
static void stop_dsp_loopback(int device,struct audio *audio_for_rpc)
{
/* daniel.kang@lge.com ++ Nov 17 // 0011018: Remove Audience Code */
#if 0
	if ( HANDSET_PACKET_LOOPBACK_ID == device)
	{
		handset_loopback_for_testmode(0x02);
	}
	else
#endif
/* daniel.kang@lge.com -- Nov 17 // 0011018: Remove Audience Code */

	if ( HEADSET_PACKET_LOOPBACK_ID == device )
	{
		headset_loopback_for_testmode( 0x02);
	}
	else if( QTR_HANDSET_PACKET_LOOPBACK_ID == device)
	{
		QTR_handset_loopback_for_testmode(0x02);
	}
/*[START] jy0127.jang@lge.com MS910_AUDIO_CHANGE 2011-04-07*/
/*added speaker phone loopback*/
	else if( QTR_SPEAKERPHONE_PACKET_LOOPBACK_ID == device)
	{
		QTR_speakerphone_loopback_for_testmode(0x02);
	}
/*[END] jy0127.jang@lge.com MS910_AUDIO_CHANGE 2011-04-07*/
	else if(BT_SCO_PACKET_LOOPBACK_ID == device)
	{
		bt_sco_loopback_for_testmode(0x02);
	}
	else
	{
		MM_ERR("No support for this device\n");
		return;
	}	
}
#endif
/* END: 0010850 kiran.kanneganti@lge.com 2010-11-15 */
/*******************************************************************************
*	Function Name :  set_rx_voice_volume
*	Args : Device ID, rpc end point
*	dependencies : None
********************************************************************************/
static void set_rx_voice_volume(int device,struct audio *audio_for_rpc)
{
	int rc;
	if (audio_for_rpc->ept)
	{
		rx_vol_msg.args.rx_vol = cpu_to_be32(caliculate_current_rx_vol(device));
		rx_vol_msg.args.device = cpu_to_be32(device);
		rx_vol_msg.args.cb_func = CALLBACK_NULL;
		rx_vol_msg.args.client_data = 0;
		rc = msm_rpc_call(audio_for_rpc->ept,RPC_VOEM_RX_VOL_PROC,&rx_vol_msg, sizeof(rx_vol_msg), 5 * HZ);
		if (rc < 0){
			MM_ERR("set_rx_voice_volume failed\n");
		}
		else
			MM_INFO("set_rx_voice_volume rpc called successfully\n");						
	}
	else
		MM_ERR("End point not yet opened\n");
}
/*BEGIN: 0011452 kiran.kanneganti@lge.com 2010-11-26*/
/*ADD 0011452: Noice cancellation check support for testmode*/
/*******************************************************************************
*	Function Name :  lge_connect_disconnect_main_mic_path_inQTR
*	Args : on or off
*	dependencies : Adie should be configured before calling this
********************************************************************************/
#define ADIE_REG_TX_FRONT_END_LEFT 0x0D
#define ADIE_REG_TX_FRONT_END_RIGHT 0x0E
#define ADIE_MASK_TX_FRONT_END_LEFT 0xE1
#define ADIE_MASK_TX_FRONT_END_RIGHT 0xF0

#define ADIE_REG_CODEC_CHANNEL_CONTROL 0x83
#define ADIE_REG_CODEC_CHANNEL_GAIN_CONTROL 0x8A

void lge_connect_disconnect_main_mic_path_inQTR(unsigned char on)
{
	if( 1 == on )
	{
		//0xE1 --> enable left channel, 24 db mic amp gain, Connect Aux in to left
		lge_marimba_codec_reg_write(ADIE_REG_TX_FRONT_END_LEFT,ADIE_MASK_TX_FRONT_END_LEFT,0xE1);
		msleep(5);
		//0x04  --> Tx left channel enable
		lge_marimba_codec_reg_write(ADIE_REG_CODEC_CHANNEL_CONTROL,0x04,0x04);
		//0x40 --> apply left channel gain & unmute 
		lge_marimba_codec_reg_write(ADIE_REG_CODEC_CHANNEL_GAIN_CONTROL,0x50,0x40);
	}
	else
	{
		//0x00 --> disable left channel, 0 db mic amp gain, Disonnect Aux in to left
		lge_marimba_codec_reg_write(ADIE_REG_TX_FRONT_END_LEFT,ADIE_MASK_TX_FRONT_END_LEFT,0x00);
		msleep(5);
		//0x00 --> Tx left channel disable
		lge_marimba_codec_reg_write(ADIE_REG_CODEC_CHANNEL_CONTROL,0x04,0x00);
		//0x10 --> Do not apply left channel gain & mute
		lge_marimba_codec_reg_write(ADIE_REG_CODEC_CHANNEL_GAIN_CONTROL,0x50,0x10);		
	}
}
EXPORT_SYMBOL(lge_connect_disconnect_main_mic_path_inQTR);
/*******************************************************************************
*	Function Name :  lge_connect_disconnect_back_mic_path_inQTR
*	Args : on or off
*	dependencies : Adie should be configured before calling this
********************************************************************************/
void lge_connect_disconnect_back_mic_path_inQTR(unsigned char on)
{
	if( 1 == on )
	{
		//0xF0 --> enable right channel, 24 db mic amp gain, Connect Mic 1 to Right
		lge_marimba_codec_reg_write(ADIE_REG_TX_FRONT_END_RIGHT,ADIE_MASK_TX_FRONT_END_RIGHT,0xF0);
		msleep(5);
		//0x08  --> Tx Right channel enable
		lge_marimba_codec_reg_write(ADIE_REG_CODEC_CHANNEL_CONTROL,0x08,0x08);
		//0x80 --> apply right channel gain & unmute 
		lge_marimba_codec_reg_write(ADIE_REG_CODEC_CHANNEL_GAIN_CONTROL,0xA0,0x80);		
	}
	else
	{
		//0x00 --> disable right channel, 0 db mic amp gain, Disonnect Mic 1 to left
		lge_marimba_codec_reg_write(ADIE_REG_TX_FRONT_END_RIGHT,ADIE_MASK_TX_FRONT_END_RIGHT,0x00);
		msleep(5);
		//0x00  --> Tx Right channel disable
		lge_marimba_codec_reg_write(ADIE_REG_CODEC_CHANNEL_CONTROL,0x08,0x00);
		//0x00 --> apply right channel gain & unmute 
		lge_marimba_codec_reg_write(ADIE_REG_CODEC_CHANNEL_GAIN_CONTROL,0xA0,0x20);		
		
	}
}
EXPORT_SYMBOL(lge_connect_disconnect_back_mic_path_inQTR);
/* END: 0011452 kiran.kanneganti@lge.com 2010-11-26 */
/*******************************************************************************
*  Ioctl function. 
*
********************************************************************************/
static long audio_misc_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct audio *audio = file->private_data;
	int rc = -EINVAL;
	int dsp_packet_lb = 0;
	MM_DBG("ioctl cmd = %d\n", cmd);
	mutex_lock(&audio->lock);
	switch (cmd)
	{
/**************************************************************************/
		case AUDIO_ENABLE_SND_DEVICE:
//Get the device ID
			if (copy_from_user(&audio->path_set,(void *)arg,sizeof(int)))
				rc = -EFAULT;
			else
				rc = 0;	
#if LOOPBACK_SUPPORT
/*BEGIN: 0011453 kiran.kanneganti@lge.com 2010-11-26*/
/*ADD 0011453: Front Mic & Back Mic Separate LoopBack Support*/
			if( BACK_MIC_PACKET_LOOPBACK_ID == audio->path_set)
			{
				audio->use_back_mic = 1;
				audio->path_set = QTR_HANDSET_PACKET_LOOPBACK_ID;
			}
/* END: 0011453 kiran.kanneganti@lge.com 2010-11-26 */			
//Check if any loopback is set already.if so start that loopback			
			if(1 == audio->packet_loopback_opened)
			{
				stop_packet_loopback(audio->prev_path_set,audio);
				start_packet_loopback(audio->path_set,audio);
				audio->packet_loopback_opened = 1;
			}
			else if(1 == audio->dsp_loopback_opened)
			{
				stop_dsp_loopback(audio->prev_path_set,audio);
				start_dsp_loopback(audio->path_set,audio);
				audio->dsp_loopback_opened = 1;
			}
			else
				MM_DBG("Only device setting \n");
#endif			
			MM_DBG("Enabling loop back path %d\n", audio->path_set);
			break;
			
/**************************************************************************/			
		case AUDIO_DISABLE_SND_DEVICE:
//Store Previous path & close the path			
			audio->prev_path_set = audio->path_set;
			audio->path_set = -1;
			MM_DBG("Disabling loop back path %d\n", audio->path_set);
			rc = 0;	
			break;
			
/***************************************************************************/			
		case AUDIO_START_VOICE:
//Get the mode and deside packet or DSP loopback		
			if (copy_from_user(&dsp_packet_lb,(void *)arg,sizeof(int)))
				rc = -EFAULT;
			else
				rc = 0;
#if LOOPBACK_SUPPORT
//if no device info don't sart	loop back	. Make sure device setting is done before calling this.	
			if( -1 != audio->path_set)
			{
				if( ACOUSTIC_ON == dsp_packet_lb)
				{ 
					if( -1 == audio->packet_loopback_opened)
					{
						MM_DBG("starting packet loop back path %d\n", audio->path_set);	
						start_packet_loopback(audio->path_set,audio);
						audio->packet_loopback_opened = 1;
					}
					else
						MM_DBG("Packet Loopback already started path %d\n", audio->path_set);	
				}
				else
				{
					if( -1 == audio->dsp_loopback_opened)
					{
						MM_DBG("starting dsp loop back path %d\n", audio->path_set);	
						start_dsp_loopback(audio->path_set,audio);
						audio->dsp_loopback_opened = 1;
					}
					else
						MM_DBG("dsp Loopback already started path %d\n", audio->path_set);				
				}
			}
			else
			{
				MM_DBG("No device info to start actual loopback \n");
			}
#endif
			break;
/***************************************************************************************/			
		case AUDIO_STOP_VOICE:		
#if LOOPBACK_SUPPORT
//close the loopback. Hope only one is active at a time.
			if( 1 == audio->dsp_loopback_opened)
			{
				stop_dsp_loopback(audio->path_set,audio);			
				MM_DBG("DSP loop back Stopped %d\n", audio->path_set);
				audio->dsp_loopback_opened = -1;
			}
			else
				MM_DBG("DSP loop back not yet Started %d\n", audio->path_set);
			
			if( 1 == audio->packet_loopback_opened)
			{
				stop_packet_loopback(audio->path_set,audio);
				MM_DBG("PACKET loop back Stopped %d\n", audio->path_set);
				audio->packet_loopback_opened = -1;			
			}
			else
				MM_DBG("PACKET loop back not yet Started %d\n", audio->path_set);			
#endif
			audio->path_set = -1;
			audio->prev_path_set = -1;			
/*BEGIN: 0011453 kiran.kanneganti@lge.com 2010-11-26*/
/*ADD 0011453: Front Mic & Back Mic Separate LoopBack Support*/				
			audio->use_back_mic = 0;
/* END: 0011453 kiran.kanneganti@lge.com 2010-11-26 */			
			rc = 0;	
			break;
			
/*****************************************************************************************/	
	case AUDIO_SET_VOLUME:
//set rx volume.
			if (copy_from_user(&current_rx_voice_vol,(void *)arg,sizeof(int)))
				rc = -EFAULT;
			else
				rc = 0;	
			MM_DBG("Rx volume & loopback state %d %d\n", current_rx_voice_vol,audio->packet_loopback_opened);
			if( 1 == audio->packet_loopback_opened)
			{
				set_rx_voice_volume(audio->path_set,audio);
			}				
			break;

/*****************************************************************************************/			
		default:
			MM_DBG("Not supported action \n");
			rc =0;
	}
	
	mutex_unlock(&audio->lock);
	return rc;
}
/*******************************************************************************
*  close function. 
*
********************************************************************************/
static int audio_misc_close(struct inode *inode, struct file *file)
{
	struct audio *audio = file->private_data;
	int rc;
	MM_DBG("audio instance 0x%08x freeing\n", (int)audio);
	mutex_lock(&audio->lock);
	rc = msm_rpc_close(audio->ept);
	if (rc < 0)
		MM_ERR("msm_rpc_close failed\n");
	audio->ept = NULL;
	audio->opened = 0;
	mutex_unlock(&audio->lock);
	return 0;
}
/*******************************************************************************
*  close function. 
*
********************************************************************************/
static int audio_misc_open(struct inode *inode, struct file *file)
{
	struct audio *audio = &misc_audio;
	int rc = 0;
	MM_DBG("audio_misc_open 0x%08x\n", (int)audio);
	mutex_lock(&audio->lock);
	if (audio->opened)
		return -EPERM;
	if (audio->ept == NULL)
	{
		audio->ept = msm_rpc_connect_compatible(VOEM_IFPROG,VOEM_IFVERS, 0);
		if (IS_ERR(audio->ept))
		{
			rc = PTR_ERR(audio->ept);
			audio->ept = NULL;
			MM_ERR("failed to connect VOEM svc\n");
			goto err;
		}
	}
	audio->opened = 1;
	audio->path_set = -1;
	audio->prev_path_set = -1;
	audio->dsp_loopback_opened = -1;
	audio->packet_loopback_opened = -1;
/*BEGIN: 0011453 kiran.kanneganti@lge.com 2010-11-26*/
/*ADD 0011453: Front Mic & Back Mic Separate LoopBack Support*/	
	audio->use_back_mic = 0;
/* END: 0011453 kiran.kanneganti@lge.com 2010-11-26 */	
	file->private_data = audio;
err:
	mutex_unlock(&audio->lock);
	return rc;	
}
/*******************************************************************************
*  Init part
*
********************************************************************************/

static const struct file_operations audio_misc_fops = {
	.owner		= THIS_MODULE,
	.open		= audio_misc_open,
	.release	    = audio_misc_close,
	.unlocked_ioctl	= audio_misc_ioctl,
};

struct miscdevice misc_audio_ctrl = {
	.minor	= MISC_DYNAMIC_MINOR,
	.name	= "msm_audio_misc",
	.fops	= &audio_misc_fops,
};
static int __init audio_misc_init(void)
{
	int i;
	struct audio *audio = &misc_audio;
	struct platform_device **audio_dev_addr;
	mutex_init(&audio->lock);

	audio_dev_addr = lge_get_audio_device_data_address(&max_devices);
	MM_DBG("audio_misc_init device data addr %u\n", audio_dev_addr);
	for(i = 0; i < max_devices ; i++)
	{
		MM_DBG("audio_misc_init each device data addr %u\n", audio_dev_addr[i]);
		bryce_audio_devices[i]= audio_dev_addr[i];
	}

	return misc_register(&misc_audio_ctrl);
}

device_initcall(audio_misc_init);

MODULE_DESCRIPTION("MSM MISC driver");
MODULE_LICENSE("GPL v2");
