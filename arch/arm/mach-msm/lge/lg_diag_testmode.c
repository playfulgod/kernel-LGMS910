#include <linux/module.h>
#include <mach/lg_diagcmd.h>
#include <linux/input.h>
#include <linux/syscalls.h>
#include <linux/slab.h>

#include "lg_fw_diag_communication.h"
#include <mach/lg_diag_testmode.h>
/*BEGIN: 0011452 kiran.kanneganti@lge.com 2010-11-26*/
/*ADD 0011452: Noice cancellation check support for testmode*/
#include <mach/qdsp5v2/audio_def.h>
/* END: 0011452 kiran.kanneganti@lge.com 2010-11-26 */
#include <linux/delay.h>

#ifndef SKW_TEST
#include <linux/fcntl.h> 
#include <linux/fs.h>
#include <linux/uaccess.h>
#endif

#include <mach/lg_backup_items.h>

// BEGIN : munho.lee@lge.com 2011-01-15
// ADD: 0013541: 0014142: [Test_Mode] To remove Internal memory information in External memory test when SD-card is not exist 
#include <linux/gpio.h>
#define SYS_GPIO_SD_DET	55  /* SYS GPIO Number 55 */
// END : munho.lee@lge.com 2011-01-15
static struct diagcmd_dev *diagpdev;

/* 2011.4.23 jaeho.cho@lge.com for usb driver testmode */
int allow_usb_switch = 0;

extern PACK(void *) diagpkt_alloc (diagpkt_cmd_code_type code, unsigned int length);
extern PACK(void *) diagpkt_free (PACK(void *)pkt);
extern void send_to_arm9( void*	pReq, void	*pRsp);
extern testmode_user_table_entry_type testmode_mstr_tbl[TESTMODE_MSTR_TBL_SIZE];
extern int diag_event_log_start(void);
extern int diag_event_log_end(void);
extern void set_operation_mode(boolean isOnline);
extern struct input_dev* get_ats_input_dev(void);
extern unsigned int LGF_KeycodeTrans(word input);
extern void LGF_SendKey(word keycode);
#ifdef CONFIG_LGE_BOOTCOMPLETE_INFO
extern int boot_complete_info;
#endif
/* ==========================================================================
===========================================================================*/

struct statfs_local {
 __u32 f_type;
 __u32 f_bsize;
 __u32 f_blocks;
 __u32 f_bfree;
 __u32 f_bavail;
 __u32 f_files;
 __u32 f_ffree;
 __kernel_fsid_t f_fsid;
 __u32 f_namelen;
 __u32 f_frsize;
 __u32 f_spare[5];
};

/* ==========================================================================
===========================================================================*/


PACK (void *)LGF_TestMode (
        PACK (void	*)req_pkt_ptr,	/* pointer to request packet  */
        uint16		pkt_len )		      /* length of request packet   */
{
  DIAG_TEST_MODE_F_req_type *req_ptr = (DIAG_TEST_MODE_F_req_type *) req_pkt_ptr;
  DIAG_TEST_MODE_F_rsp_type *rsp_ptr;
  unsigned int rsp_len;
  testmode_func_type func_ptr= NULL;
  int nIndex = 0;

  diagpdev = diagcmd_get_dev();

  // DIAG_TEST_MODE_F_rsp_type union type is greater than the actual size, decrease it in case sensitive items
  switch(req_ptr->sub_cmd_code)
  {
    case TEST_MODE_FACTORY_RESET_CHECK_TEST:
    case TEST_MODE_FIRST_BOOT_COMPLETE_TEST:
      rsp_len = sizeof(DIAG_TEST_MODE_F_rsp_type) - sizeof(test_mode_rsp_type);
      break;

    case TEST_MODE_TEST_SCRIPT_MODE:
      rsp_len = sizeof(DIAG_TEST_MODE_F_rsp_type) - sizeof(test_mode_rsp_type) + sizeof(test_mode_req_test_script_mode_type);
      break;

    //0017509: [DV] REMOVE UNNECESSARY RESPONSE PACKET FOR EXTERNEL SOCKET ERASE 
    case TEST_MODE_EXT_SOCKET_TEST:
      if((req_ptr->test_mode_req.esm == EXTERNAL_SOCKET_ERASE) || (req_ptr->test_mode_req.esm == EXTERNAL_SOCKET_ERASE_SDCARD_ONLY) \
      	|| (req_ptr->test_mode_req.esm == EXTERNAL_SOCKET_ERASE_FAT_ONLY))
        rsp_len = sizeof(DIAG_TEST_MODE_F_rsp_type) - sizeof(test_mode_rsp_type);
      break;

    case TEST_MODE_MANUAL_MODE_TEST:
      if(req_ptr->test_mode_req.test_manual_mode == MANUAL_MODE_CHECK)
        rsp_len = sizeof(DIAG_TEST_MODE_F_rsp_type) - sizeof(test_mode_rsp_type) + sizeof(int); // add manual_test type
      else
        rsp_len = sizeof(DIAG_TEST_MODE_F_rsp_type) - sizeof(test_mode_rsp_type);
      break;

    default :
      rsp_len = sizeof(DIAG_TEST_MODE_F_rsp_type);
      break;
  }

  rsp_ptr = (DIAG_TEST_MODE_F_rsp_type *)diagpkt_alloc(DIAG_TEST_MODE_F, rsp_len);

  if (!rsp_ptr)
  	return 0;
  
  rsp_ptr->sub_cmd_code = req_ptr->sub_cmd_code;
  rsp_ptr->ret_stat_code = TEST_OK_S; // test ok

  for( nIndex = 0 ; nIndex < TESTMODE_MSTR_TBL_SIZE  ; nIndex++)
  {
    if( testmode_mstr_tbl[nIndex].cmd_code == req_ptr->sub_cmd_code)
    {
        if( testmode_mstr_tbl[nIndex].which_procesor == ARM11_PROCESSOR)
          func_ptr = testmode_mstr_tbl[nIndex].func_ptr;
      break;
    }
  }

  if( func_ptr != NULL)
    return func_ptr( &(req_ptr->test_mode_req), rsp_ptr);
  else
    send_to_arm9((void*)req_ptr, (void*)rsp_ptr);

  return (rsp_ptr);
}
EXPORT_SYMBOL(LGF_TestMode);

void* linux_app_handler(test_mode_req_type*	pReq, DIAG_TEST_MODE_F_rsp_type	*pRsp)
{
	diagpkt_free(pRsp);
  return 0;
}

void* not_supported_command_handler(test_mode_req_type*	pReq, DIAG_TEST_MODE_F_rsp_type	*pRsp)
{
	pRsp->ret_stat_code = TEST_NOT_SUPPORTED_S;
  return pRsp;
}

/* LCD QTEST */
PACK (void *)LGF_LcdQTest (
        PACK (void	*)req_pkt_ptr,	/* pointer to request packet  */
        uint16		pkt_len )		      /* length of request packet   */
{
	printk("[%s] LGF_LcdQTest\n", __func__ );

	/* Returns 0 for executing lg_diag_app */
	return 0;
}
EXPORT_SYMBOL(LGF_LcdQTest);
// BEGIN: 0010557  unchol.park@lge.com 2010-11-18
// 0010557 : [Testmode] added test_mode command for LTE test 
// add call function	
void* LGF_TestModeLteModeSelection(
		test_mode_req_type* pReq ,
		DIAG_TEST_MODE_F_rsp_type	*pRsp)
{
    pRsp->ret_stat_code = TEST_OK_S;

	if (diagpdev != NULL){
		update_diagcmd_state(diagpdev, "LTEMODESELETION", pReq->mode_seletion);
	}
	else
	{
		printk("\n[%s] error LteModeSelection", __func__ );
		pRsp->ret_stat_code = TEST_NOT_SUPPORTED_S;
	}
  return pRsp;
}

void* LGF_TestModeLteCall(
		test_mode_req_type* pReq ,
		DIAG_TEST_MODE_F_rsp_type	*pRsp)
{
    pRsp->ret_stat_code = TEST_OK_S;

	if (diagpdev != NULL){
		update_diagcmd_state(diagpdev, "LTECALL", pReq->lte_call);
	}
	else
	{
		printk("\n[%s] error LteCall", __func__ );
		pRsp->ret_stat_code = TEST_NOT_SUPPORTED_S;
	}
  return pRsp;
}
// BEGIN: 0010557  unchol.park@lge.com 2011-01-12
// 0010557 : [Testmode] added test_mode command for LTE test 
void* LGF_TestModeDetach(
		test_mode_req_type* pReq ,
		DIAG_TEST_MODE_F_rsp_type	*pRsp)
{
	DIAG_TEST_MODE_F_req_type req_ptr; 

	printk("\n[%s] First LteCallDetach", __func__ );

	if(pReq->lte_virtual_sim == 21)
	{
		printk("\n[%s] Second LteCallDetach", __func__ );
		pRsp->ret_stat_code = TEST_OK_S;
		
		if (diagpdev != NULL){
			update_diagcmd_state(diagpdev, "LTECALLDETACH", pReq->lte_virtual_sim);
		}
		else
		{
			printk("\n[%s] error LteCallDetach", __func__ );
			pRsp->ret_stat_code = TEST_NOT_SUPPORTED_S;
		}
		return pRsp;
	}

	req_ptr.sub_cmd_code = 44;
	req_ptr.test_mode_req.lte_virtual_sim = pReq->lte_virtual_sim;
	printk(KERN_INFO "%s, pReq->lte_virtual_sim : %d\n", __func__, pReq->lte_virtual_sim);

	send_to_arm9((void*)&req_ptr, (void*)pRsp);
	printk(KERN_INFO "%s, pRsp->ret_stat_code : %d\n", __func__, pRsp->ret_stat_code);

  	return pRsp;
}
// END: 0010557 unchol.park@lge.com 2011-01-12

// END: 0010557 unchol.park@lge.com 2010-11-18

/* 2011.4.23 jaeho.cho@lge.com for usb driver testmode */
void set_allow_usb_switch_mode(int allow)
{
    if(allow_usb_switch != allow)
    allow_usb_switch = allow;
}
EXPORT_SYMBOL(set_allow_usb_switch_mode);

int get_allow_usb_switch_mode(void)
{
    return allow_usb_switch;
}
EXPORT_SYMBOL(get_allow_usb_switch_mode);

/* [yk.kim@lge.com] 2011-01-04, change usb driver */
void* LGF_TestModeChangeUsbDriver(
		test_mode_req_type* pReq ,
		DIAG_TEST_MODE_F_rsp_type	*pRsp)
{
	pRsp->ret_stat_code = TEST_OK_S;

	extern int android_set_pid(const char *val, struct kernel_param *kp);

/* 2011.4.23 jaeho.cho@lge.com for usb driver testmode */
    set_allow_usb_switch_mode(1);

	if (pReq->change_usb_driver == CHANGE_MODEM)
		android_set_pid("61FE", NULL);
	else if (pReq->change_usb_driver == CHANGE_MASS)
		android_set_pid("630B", NULL);
	else
		pRsp->ret_stat_code = TEST_NOT_SUPPORTED_S;

	return pRsp;
}

// BEGIN: 0009352 chanha.park@lge.com 2010-09-27
// MOD: 0009352: [BRYCE][BT] Support BT Factory Testmode.
/* TEST_MODE_BLUETOOTH_TEST */
#ifndef LG_BTUI_TEST_MODE
void* LGF_TestModeBlueTooth(
        test_mode_req_type*	pReq,
        DIAG_TEST_MODE_F_rsp_type	*pRsp)
{
	printk(KERN_ERR "[_BTUI_] [%s:%d] BTSubCmd=<%d>\n", __func__, __LINE__, pReq->bt);

#if 0
	if (diagpdev != NULL){
		// Send DiagCommandObserver.java for Audio Loopback.
		if(pReq->bt==2) 
		{
			pRsp->ret_stat_code = TEST_OK_S;
			return pRsp;
		}
		else
		{			
			update_diagcmd_state(diagpdev, "BT_TEST_MODE", pReq->bt);
			// Send lg_diag_app for DUT mode set.
			return 0;
		}
	}
#endif
#if 0
	if (diagpdev != NULL)
	{
		update_diagcmd_state(diagpdev, "BT_TEST_MODE", pReq->bt);

		printk(KERN_ERR "[_BTUI_] [%s:%d] BTSubCmd=<%d>\n", __func__, __LINE__, pReq->bt);

		/* Set Test Mode */
		if(pReq->bt==1 || (pReq->bt>=11 && pReq->bt<=42)) 
		{			
			msleep(5900); //6sec timeout
		}
		/*Test Mode Check*/
		else if(pReq->bt==2) 
		{
			ssleep(1);
		}	
		/*Test Mode Release*/
		else if(pReq->bt==5)
		{
			ssleep(3);
		}
		/*Test Mode Not Supported*/
		else
		{
			pRsp->ret_stat_code = TEST_NOT_SUPPORTED_S;
			return pRsp;
		}

		pRsp->ret_stat_code = TEST_OK_S;
		return pRsp;
	}
	else
	{
		printk(KERN_ERR "[_BTUI_] [%s:%d] BTSubCmd=<%d> ERROR\n", __func__, __LINE__, pReq->bt);
		pRsp->ret_stat_code = TEST_FAIL_S;
		return pRsp;
	}  
#endif

	// 2011.04.30 sunhee.kang@lge.com BT TestMode merge from Gelato [START]
	if (diagpdev != NULL){
		update_diagcmd_state(diagpdev, "BT_TEST_MODE", pReq->bt);
		//if(pReq->bt==1) msleep(4900); //6sec timeout
		//else if(pReq->bt==2) msleep(4900);
		//else msleep(4900);
		printk(KERN_ERR "[_BTUI_] [%s:%d] BTSubCmd=<%d>\n", __func__, __LINE__, pReq->bt);
		msleep(6000);
		printk(KERN_ERR "[_BTUI_] [%s:%d] BTSubCmd=<%d>\n", __func__, __LINE__, pReq->bt);
		
		pRsp->ret_stat_code = TEST_OK_S;
	}
	else
	{
		printk(KERN_ERR "[_BTUI_] [%s:%d] BTSubCmd=<%d> ERROR\n", __func__, __LINE__, pReq->bt);
		pRsp->ret_stat_code = TEST_NOT_SUPPORTED_S;
	}
	return pRsp;
	// 2011.04.30 sunhee.kang@lge.com BT TestMode merge from Gelato [END]
}

#if 0//Not Use.
byte *pReq_valid_address(byte *pstr)
{
	int pcnt=0;
	byte value_pstr=0, *pstr_tmp;

	pstr_tmp = pstr;
	do
	{
		++pcnt;
		value_pstr = *(pstr_tmp++);
	}while(!('0'<=value_pstr && value_pstr<='9')&&!('a'<=value_pstr && value_pstr<='f')&&!('A'<=value_pstr && value_pstr<='F')&&(pcnt<BT_RW_CNT));

	return (--pstr_tmp);
	
}

byte g_bd_addr[BT_RW_CNT];
#endif

void* LGF_TestModeBlueTooth_RW(
        test_mode_req_type*	pReq,
        DIAG_TEST_MODE_F_rsp_type	*pRsp)
{
	return 0;
#if 0//Not Use
	byte *p_Req_addr;

	p_Req_addr = pReq_valid_address(pReq->bt_rw);
	
	if(!p_Req_addr)
	{
		pRsp->ret_stat_code = TEST_FAIL_S;
		return pRsp;
	}
	
	printk(KERN_ERR "[_BTUI_] [%s:%d] BTSubCmd=<%s>\n", __func__, __LINE__, p_Req_addr);

	if (diagpdev != NULL)
	{
		//250-83-0 bluetooth write
		if(strlen(p_Req_addr) > 0)
		{
			update_diagcmd_state(diagpdev, "BT_TEST_MODE_RW", p_Req_addr);
			memset((void*)g_bd_addr, 0x00, BT_RW_CNT);
			memcpy((void*)g_bd_addr, p_Req_addr, BT_RW_CNT);
			msleep(5900); //6sec timeout
		}
		//250-83-1 bluetooth read
		else
		{
			update_diagcmd_state(diagpdev, "BT_TEST_MODE_RW", 1);
			if(strlen(g_bd_addr)==0) {
				pRsp->ret_stat_code = TEST_FAIL_S;
				return pRsp;
			}
			memset((void*)pRsp->test_mode_rsp.read_bd_addr, 0x00, BT_RW_CNT);
			memcpy((void*)pRsp->test_mode_rsp.read_bd_addr, g_bd_addr, BT_RW_CNT);
		}
		pRsp->ret_stat_code = TEST_OK_S;
	}
	else 
	{
		printk(KERN_ERR "[_BTUI_] [%s:%d] BTSubCmd=<%d> ERROR\n", __func__, __LINE__, pReq->bt_rw);
		pRsp->ret_stat_code = TEST_NOT_SUPPORTED_S;
	}
  return pRsp;
#endif
}
#endif //LG_BTUI_TEST_MODE
// END: 0009352 chanha.park@lge.com 2010-09-27


void* LGF_TestPhotoSensor(
		test_mode_req_type* pReq ,
		DIAG_TEST_MODE_F_rsp_type	*pRsp)
{
	/* The photosensor isn't supported in VS910 model
	*/
	pRsp->ret_stat_code = TEST_NOT_SUPPORTED_S;

#if 0
	if (diagpdev != NULL){
		update_diagcmd_state(diagpdev, "ALC", pReq->motor);
	}
	else
	{
		printk("\n[%s] error MOTOR", __func__ );
		pRsp->ret_stat_code = TEST_NOT_SUPPORTED_S;
	}
#endif

  return pRsp;
}

void* LGF_TestMotor(
		test_mode_req_type* pReq ,
		DIAG_TEST_MODE_F_rsp_type	*pRsp)
{
	pRsp->ret_stat_code = TEST_OK_S;

	if (diagpdev != NULL){
		update_diagcmd_state(diagpdev, "MOTOR", pReq->motor);
	}
	else
	{
		printk("\n[%s] error MOTOR", __func__ );
		pRsp->ret_stat_code = TEST_NOT_SUPPORTED_S;
	}
  return pRsp;
}

void* LGF_TestAcoustic(
		test_mode_req_type* pReq ,
		DIAG_TEST_MODE_F_rsp_type	*pRsp)
{
    pRsp->ret_stat_code = TEST_OK_S;

	if (diagpdev != NULL){
		if(pReq->acoustic > ACOUSTIC_LOOPBACK_OFF)
			pRsp->ret_stat_code = TEST_NOT_SUPPORTED_S;
			
		update_diagcmd_state(diagpdev, "ACOUSTIC", pReq->acoustic);
	}
	else
	{
		printk("\n[%s] error ACOUSTIC", __func__ );
		pRsp->ret_stat_code = TEST_NOT_SUPPORTED_S;
	}
  return pRsp;
}

void* LGF_TestModeMP3 (
		test_mode_req_type* pReq ,
		DIAG_TEST_MODE_F_rsp_type	*pRsp)
{
	pRsp->ret_stat_code = TEST_OK_S;
	printk("\n[%s] diagpdev 0x%x", __func__, diagpdev );

	if (diagpdev != NULL){
		if(pReq->mp3_play == MP3_SAMPLE_FILE)
		{
			pRsp->ret_stat_code = TEST_NOT_SUPPORTED_S;
		}
		else
		{
			update_diagcmd_state(diagpdev, "MP3", pReq->mp3_play);
		}
	}
	else
	{
		printk("\n[%s] error MP3", __func__ );
		pRsp->ret_stat_code = TEST_NOT_SUPPORTED_S;
	}
  return pRsp;
}

void* LGF_TestModeSpeakerPhone(
		test_mode_req_type* pReq ,
		DIAG_TEST_MODE_F_rsp_type	*pRsp)
{
	pRsp->ret_stat_code = TEST_OK_S;

	if (diagpdev != NULL){
/*BEGIN: 0011452 kiran.kanneganti@lge.com 2010-11-26*/
/*ADD 0011452: Noice cancellation check support for testmode*/	
#ifdef SINGLE_MIC_PHONE		
		if((pReq->speaker_phone == NOMAL_Mic1) || (pReq->speaker_phone == NC_MODE_ON)
			|| (pReq->speaker_phone == ONLY_MIC2_ON_NC_ON) || (pReq->speaker_phone == ONLY_MIC1_ON_NC_ON)
		)
		{
			pRsp->ret_stat_code = TEST_NOT_SUPPORTED_S;
		}
#else
		if(pReq->speaker_phone == NOMAL_Mic1)
		{
			pRsp->ret_stat_code = TEST_NOT_SUPPORTED_S;
		}
		else if( ONLY_MIC1_ON_NC_ON == pReq->speaker_phone )
		{	
			lge_connect_disconnect_back_mic_path_inQTR(0);
			lge_connect_disconnect_main_mic_path_inQTR(1);
		}
		else if( ONLY_MIC2_ON_NC_ON == pReq->speaker_phone )
		{
			lge_connect_disconnect_back_mic_path_inQTR(1);
			lge_connect_disconnect_main_mic_path_inQTR(0);
		}		
		else if( NC_MODE_ON == pReq->speaker_phone )
		{
			lge_connect_disconnect_back_mic_path_inQTR(1);
			lge_connect_disconnect_main_mic_path_inQTR(1);
		}
#endif
/* END: 0011452 kiran.kanneganti@lge.com 2010-11-26 */
		else
		{
			update_diagcmd_state(diagpdev, "SPEAKERPHONE", pReq->speaker_phone);
		}
	}
	else
	{
		printk("\n[%s] error SPEAKERPHONE", __func__ );
		pRsp->ret_stat_code = TEST_NOT_SUPPORTED_S;
	}
  return pRsp;
}

void* LGT_TestModeVolumeLevel (
		test_mode_req_type* pReq ,
		DIAG_TEST_MODE_F_rsp_type *pRsp)
{
	pRsp->ret_stat_code = TEST_OK_S;

	if (diagpdev != NULL){
		update_diagcmd_state(diagpdev, "VOLUMELEVEL", pReq->volume_level);
	}
	else
	{
		printk("\n[%s] error VOLUMELEVEL", __func__ );
		pRsp->ret_stat_code = TEST_NOT_SUPPORTED_S;
	}
  return pRsp;
}

char key_buf[MAX_KEY_BUFF_SIZE];
boolean if_condition_is_on_key_buffering = FALSE;
int count_key_buf = 0;

boolean lgf_factor_key_test_rsp (char key_code)
{
    /* sanity check */
    if (count_key_buf>=MAX_KEY_BUFF_SIZE)
        return FALSE;

    key_buf[count_key_buf++] = key_code;
    return TRUE;
}
EXPORT_SYMBOL(lgf_factor_key_test_rsp);

void* LGT_TestModeKeyTest(test_mode_req_type* pReq, DIAG_TEST_MODE_F_rsp_type *pRsp)
{
  pRsp->ret_stat_code = TEST_OK_S;

  if(pReq->key_test_start){
	memset((void *)key_buf,0x00,MAX_KEY_BUFF_SIZE);
	count_key_buf=0;
	diag_event_log_start();
  }
  else
  {
	memcpy((void *)((DIAG_TEST_MODE_KEY_F_rsp_type *)pRsp)->key_pressed_buf, (void *)key_buf, MAX_KEY_BUFF_SIZE);
	memset((void *)key_buf,0x00,MAX_KEY_BUFF_SIZE);
	diag_event_log_end();
  }
  return pRsp;
}

void* LGF_TestCam(
		test_mode_req_type* pReq ,
		DIAG_TEST_MODE_F_rsp_type	*pRsp)
{
		pRsp->ret_stat_code = TEST_OK_S;

		switch(pReq->camera)
		{
			case CAM_TEST_SAVE_IMAGE:
			case CAM_TEST_FLASH_ON:
			case CAM_TEST_FLASH_OFF:
				pRsp->ret_stat_code = TEST_NOT_SUPPORTED_S;
				break;

			default:
				if (diagpdev != NULL){

					update_diagcmd_state(diagpdev, "CAMERA", pReq->camera);
				}
				else
				{
					printk("\n[%s] error CAMERA", __func__ );
					pRsp->ret_stat_code = TEST_NOT_SUPPORTED_S;
				}
				break;
		}
	  return pRsp;
}

/* sangwoo.kang	2010.09.03
					build error debugging */
#if 1 //def CONFIG_LGE_TOUCH_TEST
void* LGF_TestModeKeyData(	test_mode_req_type* pReq ,DIAG_TEST_MODE_F_rsp_type	*pRsp)
{

	pRsp->ret_stat_code = TEST_OK_S;

	LGF_SendKey(LGF_KeycodeTrans(pReq->key_data));

	return pRsp;
}

// BEGIN 0011665: eundeok.bae@lge.com FTM MODE FOR ONLY KERNEL BOOTING
// [KERNEL] Added source code For Sleep Mode Test, Test Mode V8.3 [250-42] 
uint8_t if_condition_is_on_air_plain_mode = 0;
extern void remote_set_operation_mode(int info);
extern void remote_set_ftm_boot(int info);
// END 0011665: eundeok.bae@lge.com FTM MODE FOR ONLY KERNEL BOOTING

// BEGIN 0012299: eundeok.bae@lge.com 2010-12-13 FTM MODE RESET
// [KERNEL] ADDED "FTM BOOT RESET" Function For Test Mode 
extern void remote_set_ftmboot_reset(uint32 info);
// END 0012299: eundeok.bae@lge.com 2010-12-13

void* LGF_PowerSaveMode(test_mode_req_type* pReq, DIAG_TEST_MODE_F_rsp_type* pRsp)
{
	pRsp->ret_stat_code = TEST_OK_S;
	pReq->sleep_mode = (pReq->sleep_mode & 0x00FF);     // 2011.06.21 biglake for power test after cal

#if 1 //def LG_FW_COMPILE_ERROR
	switch(pReq->sleep_mode){
		case SLEEP_MODE_ON:
			LGF_SendKey(KEY_END);
			break;
		case AIR_PLAIN_MODE_ON:
// BEGIN 0011665: eundeok.bae@lge.com FTM MODE FOR ONLY KERNEL BOOTING
// [KERNEL] Added source code For Sleep Mode Test, Test Mode V8.3 [250-42] 
// LGE_CHANGE [dojip.kim@lge.com] 2010-09-28, ftm boot 
      remote_set_ftm_boot(0);
      if_condition_is_on_air_plain_mode = 1;
      remote_set_operation_mode(0);
// END 0011665: eundeok.bae@lge.com FTM MODE FOR ONLY KERNEL BOOTING       
			break;
// BEGIN 0011665: eundeok.bae@lge.com FTM MODE FOR ONLY KERNEL BOOTING
// [KERNEL] Added source code For Sleep Mode Test, Test Mode V8.3 [250-42] 			
// LGE_CHANGE [dojip.kim@lge.com] 2010-09-28, ftm boot 
#if 0 
	case FTM_BOOT_ON: /* kernel mode */
		remote_set_ftm_boot(1);      // Writing file on the EFS
		
// BEGIN 0012299: eundeok.bae@lge.com 2010-12-13 FTM MODE RESET
// [KERNEL] ADDED "FTM BOOT RESET" Function For Test Mode 		
		remote_set_ftmboot_reset(1); // RESET
// END 0012299: eundeok.bae@lge.com 2010-12-13

		break;
#endif
// END 0011665: eundeok.bae@lge.com FTM MODE FOR ONLY KERNEL BOOTING 
	case AIR_PLAIN_MODE_OFF:
// BEGIN 0011665: eundeok.bae@lge.com FTM MODE FOR ONLY KERNEL BOOTING
// [KERNEL] Added source code For Sleep Mode Test, Test Mode V8.3 [250-42] 
// LGE_CHANGE [dojip.kim@lge.com] 2010-09-28, ftm boot 
	  remote_set_ftm_boot(0);
	  if_condition_is_on_air_plain_mode = 0;
	  remote_set_operation_mode(1);
// END 0011665: eundeok.bae@lge.com FTM MODE FOR ONLY KERNEL BOOTING	   
		break;

		default:
			pRsp->ret_stat_code = TEST_NOT_SUPPORTED_S;
	}
#endif /* LG_FW_COMPILE_ERROR */
	return pRsp;
}
#endif /* CONFIG_LGE_TOUCH_TEST */

char external_memory_copy_test(void)
{
	char return_value = 1;
	char *src = (void *)0;
	char *dest = (void *)0;
	off_t fd_offset;
	int fd;
	mm_segment_t old_fs=get_fs();
    set_fs(get_ds());

// BEGIN : munho.lee@lge.com 2010-12-30
// MOD: 0013315: [SD-card] SD-card drectory path is changed in the testmode
	if ( (fd = sys_open((const char __user *) "/sdcard/_ExternalSD/SDTest.txt", O_CREAT | O_RDWR, 0) ) < 0 )
/*
	if ( (fd = sys_open((const char __user *) "/sdcard/SDTest.txt", O_CREAT | O_RDWR, 0) ) < 0 )
*/	
// END : munho.lee@lge.com 2010-12-30
	{
		printk(KERN_ERR "[ATCMD_EMT] Can not access SD card\n");
		goto file_fail;
	}

	if ( (src = kmalloc(10, GFP_KERNEL)) )
	{
		sprintf(src,"TEST");
		if ((sys_write(fd, (const char __user *) src, 5)) < 0)
		{
			printk(KERN_ERR "[ATCMD_EMT] Can not write SD card \n");
			goto file_fail;
		}
		fd_offset = sys_lseek(fd, 0, 0);
	}
	if ( (dest = kmalloc(10, GFP_KERNEL)) )
	{
		if ((sys_read(fd, (char __user *) dest, 5)) < 0)
		{
			printk(KERN_ERR "[ATCMD_EMT]Can not read SD card \n");
			goto file_fail;
		}
		if ((memcmp(src, dest, 4)) == 0)
			return_value = 0;
		else
			return_value = 1;
	}

	kfree(src);
	kfree(dest);
file_fail:
	sys_close(fd);
    set_fs(old_fs);
// BEGIN : munho.lee@lge.com 2010-12-30
// MOD: 0013315: [SD-card] SD-card drectory path is changed in the testmode
	sys_unlink((const char __user *)"/sdcard/_ExternalSD/SDTest.txt");
/*
	sys_unlink((const char __user *)"/sdcard/SDTest.txt");
*/
// END : munho.lee@lge.com 2010-12-30

	return return_value;
}


void* LGF_ExternalSocketMemory(	test_mode_req_type* pReq ,DIAG_TEST_MODE_F_rsp_type	*pRsp)
{
    struct statfs_local  sf;
    pRsp->ret_stat_code = TEST_OK_S;

    printk(KERN_ERR "khlee debug %d \n", pReq->esm);

    switch( pReq->esm){
	case EXTERNAL_SOCKET_MEMORY_CHECK:
// BEGIN : munho.lee@lge.com 2011-01-15
// ADD: 0013541: 0014142: [Test_Mode] To remove Internal memory information in External memory test when SD-card is not exist 	
		if(gpio_get_value(SYS_GPIO_SD_DET))
		{
			pRsp->test_mode_rsp.memory_check = TEST_FAIL_S;
			break;
		}		
// END : munho.lee@lge.com 2011-01-15		
        pRsp->test_mode_rsp.memory_check = external_memory_copy_test();
        break;

	case EXTERNAL_FLASH_MEMORY_SIZE:
// BEGIN : munho.lee@lge.com 2010-12-30
// MOD: 0013315: [SD-card] SD-card drectory path is changed in the testmode
// BEGIN : munho.lee@lge.com 2011-01-15
// ADD: 0013541: 0014142: [Test_Mode] To remove Internal memory information in External memory test when SD-card is not exist 
		if(gpio_get_value(SYS_GPIO_SD_DET))
		{
			pRsp->test_mode_rsp.socket_memory_size = 0;
			break;
		}
// END : munho.lee@lge.com 2011-01-15		
   		if (sys_statfs("/sdcard/_ExternalSD", (struct statfs *)&sf) != 0)		
/*
        if (sys_statfs("/sdcard", (struct statfs *)&sf) != 0)
*/       
// END : munho.lee@lge.com 2010-12-30
        {
			printk(KERN_ERR "[Testmode]can not get sdcard infomation \n");
			pRsp->ret_stat_code = TEST_FAIL_S;
			break;
        }

// BEGIN : munho.lee@lge.com 2011-01-15
// MOD: 0013541: 0014142: [Test_Mode] To remove Internal memory information in External memory test when SD-card is not exist 
//		pRsp->test_mode_rsp.socket_memory_size = ((long long)sf.f_blocks * (long long)sf.f_bsize);  // needs byte
		pRsp->test_mode_rsp.socket_memory_size = ((long long)sf.f_blocks * (long long)sf.f_bsize) >> 20; // needs Mb.
// END : munho.lee@lge.com 2011-01-15
        break;

	case EXTERNAL_SOCKET_ERASE:

        if (diagpdev != NULL){
// BEGIN : munho.lee@lge.com 2010-11-27
// MOD : 0011477: [SD-card] Diag test mode			
// BEGIN : munho.lee@lge.com 2011-01-15
// ADD: 0013541: 0014142: [Test_Mode] To remove Internal memory information in External memory test when SD-card is not exist 	

/*  remove  //START : munho.lee@lge.com 2011-02-24 0016976: [Testmode] Without external memory internal memory can be formatted via diag command. 
			if(gpio_get_value(SYS_GPIO_SD_DET))
			{
				pRsp->ret_stat_code = TEST_FAIL_S;
				break;
			}
*/			// END : munho.lee@lge.com 2011-02-24
// END : munho.lee@lge.com 2011-01-15
			update_diagcmd_state(diagpdev, "MMCFORMAT", 1);
//			update_diagcmd_state(diagpdev, "FACTORY_RESET", 3);
// END : munho.lee@lge.com 2010-11-27			
			msleep(5000);
			pRsp->ret_stat_code = TEST_OK_S;
        }
        else 
        {
			printk("\n[%s] error FACTORY_RESET", __func__ );
			pRsp->ret_stat_code = TEST_NOT_SUPPORTED_S;
        }
        break;

	case EXTERNAL_FLASH_MEMORY_USED_SIZE:
// BEGIN : munho.lee@lge.com 2010-12-30
// MOD: 0013315: [SD-card] SD-card drectory path is changed in the testmode
// BEGIN : munho.lee@lge.com 2011-01-15
// ADD: 0013541: 0014142: [Test_Mode] To remove Internal memory information in External memory test when SD-card is not exist	

		if(gpio_get_value(SYS_GPIO_SD_DET))
		{
			pRsp->test_mode_rsp.socket_memory_usedsize = 0;
			break;
		}		
// END : munho.lee@lge.com 2011-01-15
		if (sys_statfs("/sdcard/_ExternalSD", (struct statfs *)&sf) != 0)		
/*
		if (sys_statfs("/sdcard", (struct statfs *)&sf) != 0)
*/			
// END : munho.lee@lge.com 2010-12-30
		{
			printk(KERN_ERR "[Testmode]can not get sdcard information \n");
			pRsp->ret_stat_code = TEST_FAIL_S;
			break;
		}
// BEGIN : munho.lee@lge.com 2011-01-15
// MOD: 0013541: 0014142: [Test_Mode] To remove Internal memory information in External memory test when SD-card is not exist	
		pRsp->test_mode_rsp.socket_memory_usedsize = ((long long)(sf.f_blocks - (long long)sf.f_bfree) * sf.f_bsize); // needs byte
// END : munho.lee@lge.com 2011-01-15
/*
		pRsp->test_mode_rsp.socket_memory_usedsize = ((long long)(sf.f_blocks - (long long)sf.f_bfree) * sf.f_bsize) >> 20;
*/		
		break;

	case EXTERNAL_SOCKET_ERASE_SDCARD_ONLY: /*0xE*/
		if (diagpdev != NULL){
			update_diagcmd_state(diagpdev, "MMCFORMAT", EXTERNAL_SOCKET_ERASE_SDCARD_ONLY);		
			msleep(5000);
			pRsp->ret_stat_code = TEST_OK_S;
		}
		else 
		{
			printk("\n[%s] error EXTERNAL_SOCKET_ERASE_SDCARD_ONLY", __func__ );
			pRsp->ret_stat_code = TEST_FAIL_S;
		}
		break;
	case EXTERNAL_SOCKET_ERASE_FAT_ONLY: /*0xF*/
		if (diagpdev != NULL){
			update_diagcmd_state(diagpdev, "MMCFORMAT", EXTERNAL_SOCKET_ERASE_FAT_ONLY);		
			msleep(5000);
			pRsp->ret_stat_code = TEST_OK_S;
		}
		else 
		{
			printk("\n[%s] error EXTERNAL_SOCKET_ERASE_FAT_ONLY", __func__ );
			pRsp->ret_stat_code = TEST_FAIL_S;
		}
		break;

	default:
        pRsp->ret_stat_code = TEST_NOT_SUPPORTED_S;
        break;
	}

    return pRsp;
}

/* BEGIN: 0015566 jihoon.lee@lge.com 20110207 */
/* ADD 0015566: [Kernel] charging mode check command */
#ifdef CONFIG_LGE_CHARGING_MODE_INFO
// this will be set if "/dev/chg_logo" file is written from the chargerlogo, dependant to models
static int first_booting_chg_mode_status = -1;
void set_first_booting_chg_mode_status(int status)
{
	first_booting_chg_mode_status = status;
	printk("%s, status : %d\n", __func__, first_booting_chg_mode_status);
}

int get_first_booting_chg_mode_status(void)
{
	printk("%s, status : %d\n", __func__, first_booting_chg_mode_status);
	return first_booting_chg_mode_status;
}
#endif
/* END: 0015566 jihoon.lee@lge.com 20110207 */

#ifdef CONFIG_LGE_BOOTCOMPLETE_INFO
void * LGF_TestModeFboot (	test_mode_req_type* pReq ,DIAG_TEST_MODE_F_rsp_type	*pRsp)
{

	switch( pReq->fboot)
	{
		case FIRST_BOOTING_COMPLETE_CHECK:
			printk("[Testmode] First Boot info ?? ====> %d \n", boot_complete_info);
			if (boot_complete_info)
				pRsp->ret_stat_code = TEST_OK_S;
			else
				pRsp->ret_stat_code = TEST_FAIL_S;
			break;
/* BEGIN: 0015566 jihoon.lee@lge.com 20110207 */
/* ADD 0015566: [Kernel] charging mode check command */
/*
  * chg_status 0 : in the charging mode
  * chg_status 1 : normal boot mode
  */
#ifdef CONFIG_LGE_CHARGING_MODE_INFO
		case FIRST_BOOTING_CHG_MODE_CHECK:
			if(get_first_booting_chg_mode_status() == 1)
				pRsp->ret_stat_code = FIRST_BOOTING_IN_CHG_MODE;
			else
				pRsp->ret_stat_code = FIRST_BOOTING_NOT_IN_CHG_MODE;
			break;
#endif
/* END: 0015566 jihoon.lee@lge.com 20110207 */
	    default:
			pRsp->ret_stat_code = TEST_NOT_SUPPORTED_S;
			break;
	}

    return pRsp;
}
#endif

// BEGIN : munho.lee@lge.com 2010-11-30
// ADD: 0011620: [Test_mode] Memory format test 
void* LGF_MemoryFormatTest(	test_mode_req_type* pReq ,DIAG_TEST_MODE_F_rsp_type	*pRsp)
{
  struct statfs_local  sf;
  unsigned int remained = 0;
  pRsp->ret_stat_code = TEST_OK_S;
  if (sys_statfs("/data", (struct statfs *)&sf) != 0)
  {
    printk(KERN_ERR "[Testmode]can not get sdcard infomation \n");
    pRsp->ret_stat_code = TEST_FAIL_S;
  }
  else
  {	
    switch(pReq->memory_format)
    {
      case MEMORY_TOTAL_SIZE_TEST:
		  break;	
      case MEMORY_FORMAT_MEMORY_TEST:
	  	/*
		  For code of format memory
		*/		  
		  pRsp->ret_stat_code = TEST_OK_S;
          break;

      default :
          pRsp->ret_stat_code = TEST_NOT_SUPPORTED_S;
          break;
    }
  }
  return pRsp;
}
// END : munho.lee@lge.com 2010-11-30

void* LGF_MemoryVolumeCheck(	test_mode_req_type* pReq ,DIAG_TEST_MODE_F_rsp_type	*pRsp)
{
  struct statfs_local  sf;
  unsigned int total = 0;
  unsigned int used = 0;
  unsigned int remained = 0;
  pRsp->ret_stat_code = TEST_OK_S;

  if (sys_statfs("/data", (struct statfs *)&sf) != 0)
  {
    printk(KERN_ERR "[Testmode]can not get sdcard infomation \n");
    pRsp->ret_stat_code = TEST_FAIL_S;
  }
  else
  {

    total = (sf.f_blocks * sf.f_bsize) >> 20;
    remained = (sf.f_bavail * sf.f_bsize) >> 20;
    used = total - remained;

    switch(pReq->mem_capa)
    {
      case MEMORY_TOTAL_CAPA_TEST:
          pRsp->test_mode_rsp.mem_capa = total;
          break;

      case MEMORY_USED_CAPA_TEST:
          pRsp->test_mode_rsp.mem_capa = used;
          break;

      case MEMORY_REMAIN_CAPA_TEST:
          pRsp->test_mode_rsp.mem_capa = remained;
          break;

      default :
          pRsp->ret_stat_code = TEST_NOT_SUPPORTED_S;
          break;
    }
  }
  return pRsp;
}

void* LGF_TestModeManual(test_mode_req_type* pReq ,DIAG_TEST_MODE_F_rsp_type	*pRsp)
{
  pRsp->ret_stat_code = TEST_OK_S;
  pRsp->test_mode_rsp.manual_test = TRUE;
  return pRsp;
}

#ifndef SKW_TEST
static unsigned char test_mode_factory_reset_status = FACTORY_RESET_START;
#define BUF_PAGE_SIZE 2048
// BEGIN: 0010090 sehyuny.kim@lge.com 2010-10-21
// MOD 0010090: [FactoryReset] Enable Recovery mode FactoryReset

#define FACTORY_RESET_STR       "FACT_RESET_"
#define FACTORY_RESET_STR_SIZE	11
#define FACTORY_RESET_BLK 1 // read / write on the first block

#define MSLEEP_CNT 100

typedef struct MmcPartition MmcPartition;

struct MmcPartition {
    char *device_index;
    char *filesystem;
    char *name;
    unsigned dstatus;
    unsigned dtype ;
    unsigned dfirstsec;
    unsigned dsize;
};

extern int lge_erase_block(int secnum, size_t size);
extern int lge_write_block(int secnum, unsigned char *buf, size_t size);
extern int lge_read_block(int secnum, unsigned char *buf, size_t size);
extern int lge_mmc_scan_partitions(void);
extern const MmcPartition *lge_mmc_find_partition_by_name(const char *name);

// END: 0010090 sehyuny.kim@lge.com 2010-10-21
#endif

// BEGIN: 0011396 sehyuny.kim@lge.com 2010-11-25
// MOD 0011396: [Modem] it provide HW revision api service in kernel

byte CheckHWRev(void)
{
/* BEGIN: 0014656 jihoon.lee@lge.com 20110124 */
/* MOD 0014656: [LG RAPI] OEM RAPI PACKET MISMATCH KERNEL CRASH FIX */
	// request packet of send_to_arm9 should be DIAG_TEST_MODE_F_req_type
	//test_mode_req_type Req;
	DIAG_TEST_MODE_F_req_type Req;
	DIAG_TEST_MODE_F_rsp_type Rsp;

	/*
	char *pReq = (char *) &Req;
	char *pRsp = (char *) &Rsp;

	pReq[0] = 250;
	pReq[1] = 0;
	pReq[2] = 0;
	pReq[3] = 8;

	send_to_arm9(pReq , pRsp);
	printk("CheckHWRev> 0x%x 0x%x 0x%x 0x%x 0x%x\n", pRsp[0],pRsp[1],pRsp[2],pRsp[3],pRsp[4]);
	*/

	Req.sub_cmd_code = TEST_MODE_VERSION;
	Req.test_mode_req.version = VER_HW;

	send_to_arm9((void*)&Req, (void*)&Rsp);
	/*
	 * previous kmsg : CheckHWRev> 0xfa 0x0 0x0 0x0 0x44
	 * current kmsg : CheckHWRev> 0xfa 0x0 0x0 0x44
	 * last packet matches to the previous, so this modification seems to be working fine
	*/
	printk("CheckHWRev> 0x%x 0x%x 0x%x 0x%x\n", Rsp.xx_header.opaque_header, \
		Rsp.sub_cmd_code, Rsp.ret_stat_code, Rsp.test_mode_rsp.str_buf[0]);

	return Rsp.test_mode_rsp.str_buf[0];
/* END: 0014656 jihoon.lee@lge.com 2011024 */
	
}

/* BEGIN: 0015983 jihoon.lee@lge.com 20110214 */
/* MOD 0015983: [MANUFACTURE] HW VERSION DISPLAY */
#ifdef CONFIG_LGE_PCB_VERSION
void CheckHWRevStr(char *buf, int str_size)
{
	DIAG_TEST_MODE_F_req_type Req;
	DIAG_TEST_MODE_F_rsp_type Rsp;

	Req.sub_cmd_code = TEST_MODE_VERSION;
	Req.test_mode_req.version = VER_HW;

	send_to_arm9((void*)&Req, (void*)&Rsp);
       memcpy(buf, Rsp.test_mode_rsp.str_buf, (str_size <= sizeof(Rsp.test_mode_rsp.str_buf) ? str_size: sizeof(Rsp.test_mode_rsp.str_buf)));
}
#endif
/* END: 0015983 jihoon.lee@lge.com 20110214 */

//EXPORT_SYMBOL(CheckHWRev);
// BEGIN : munho.lee@lge.com 2010-12-10
// ADD: 0012164: [Hidden_menu] For gettng PCB version. 
EXPORT_SYMBOL(CheckHWRev);
// END : munho.lee@lge.com 2010-12-10

// END: 0011396 sehyuny.kim@lge.com 2010-11-25
// BEGIN: 0010090 sehyuny.kim@lge.com 2010-10-21
// MOD 0010090: [FactoryReset] Enable Recovery mode FactoryReset


void* LGF_TestModeFactoryReset(
		test_mode_req_type* pReq ,
		DIAG_TEST_MODE_F_rsp_type	*pRsp)
{
  unsigned char pbuf[50]; //no need to have huge size, this is only for the flag
  const MmcPartition *pMisc_part; 
  unsigned char startStatus = FACTORY_RESET_NA; 
  int mtd_op_result = 0;
  unsigned long factoryreset_bytes_pos_in_emmc = 0;
/* BEGIN: 0014656 jihoon.lee@lge.com 20110124 */
/* MOD 0014656: [LG RAPI] OEM RAPI PACKET MISMATCH KERNEL CRASH FIX */
  DIAG_TEST_MODE_F_req_type req_ptr;

  req_ptr.sub_cmd_code = TEST_MODE_FACTORY_RESET_CHECK_TEST;
  req_ptr.test_mode_req.factory_reset = pReq->factory_reset;
/* END: 0014656 jihoon.lee@lge.com 2011024 */
  
/* BEGIN: 0014110 jihoon.lee@lge.com 20110115 */
/* MOD 0014110: [FACTORY RESET] stability */
/* handle operation or rpc failure as well */
  pRsp->ret_stat_code = TEST_FAIL_S;
/* END: 0014110 jihoon.lee@lge.com 20110115 */
  
  lge_mmc_scan_partitions();
  pMisc_part = lge_mmc_find_partition_by_name("misc");
  factoryreset_bytes_pos_in_emmc = (pMisc_part->dfirstsec*512)+PTN_FRST_PERSIST_POSITION_IN_MISC_PARTITION;
  
  printk("LGF_TestModeFactoryReset> mmc info sec : 0x%x, size : 0x%x type : 0x%x frst sec: 0x%lx\n", pMisc_part->dfirstsec, pMisc_part->dsize, pMisc_part->dtype, factoryreset_bytes_pos_in_emmc);

/* BEGIN: 0013861 jihoon.lee@lge.com 20110111 */
/* MOD 0013861: [FACTORY RESET] emmc_direct_access factory reset flag access */
/* add carriage return and change flag size for the platform access */
/* END: 0013861 jihoon.lee@lge.com 20110111 */
  switch(pReq->factory_reset)
  {
    case FACTORY_RESET_CHECK :
#if 1  // def CONFIG_LGE_MTD_DIRECT_ACCESS
/* BEGIN: 0014110 jihoon.lee@lge.com 20110115 */
/* MOD 0014110: [FACTORY RESET] stability */
/* handle operation or rpc failure as well */
      memset((void*)pbuf, 0, sizeof(pbuf));
      mtd_op_result = lge_read_block(factoryreset_bytes_pos_in_emmc, pbuf, FACTORY_RESET_STR_SIZE+2);

      if( mtd_op_result != (FACTORY_RESET_STR_SIZE+2) )
      {
        printk(KERN_ERR "[Testmode]lge_read_block, read data  = %d \n", mtd_op_result);
        pRsp->ret_stat_code = TEST_FAIL_S;
        break;
      }
      else
      {
        //printk(KERN_INFO "\n[Testmode]factory reset memcmp\n");
        if(memcmp(pbuf, FACTORY_RESET_STR, FACTORY_RESET_STR_SIZE) == 0) // tag read sucess
        {
          startStatus = pbuf[FACTORY_RESET_STR_SIZE] - '0';
          printk(KERN_INFO "[Testmode]factory reset backup status = %d \n", startStatus);
        }
        else
        {
          // if the flag storage is erased this will be called, start from the initial state
          printk(KERN_ERR "[Testmode] tag read failed :  %s \n", pbuf);
        }
      }  
/* END: 0014110 jihoon.lee@lge.com 20110115 */

      test_mode_factory_reset_status = FACTORY_RESET_INITIAL;
      memset((void *)pbuf, 0, sizeof(pbuf));
      sprintf(pbuf, "%s%d\n",FACTORY_RESET_STR, test_mode_factory_reset_status);
      printk(KERN_INFO "[Testmode]factory reset status = %d\n", test_mode_factory_reset_status);

      mtd_op_result = lge_erase_block(factoryreset_bytes_pos_in_emmc, FACTORY_RESET_STR_SIZE+2);	
/* BEGIN: 0014110 jihoon.lee@lge.com 20110115 */
/* MOD 0014110: [FACTORY RESET] stability */
/* handle operation or rpc failure as well */
      if(mtd_op_result!= (FACTORY_RESET_STR_SIZE+2))
      {
        printk(KERN_ERR "[Testmode]lge_erase_block, error num = %d \n", mtd_op_result);
        pRsp->ret_stat_code = TEST_FAIL_S;
        break;
      }
      else
      {
        mtd_op_result = lge_write_block(factoryreset_bytes_pos_in_emmc, pbuf, FACTORY_RESET_STR_SIZE+2);
        if(mtd_op_result!=(FACTORY_RESET_STR_SIZE+2))
        {
          printk(KERN_ERR "[Testmode]lge_write_block, error num = %d \n", mtd_op_result);
          pRsp->ret_stat_code = TEST_FAIL_S;
          break;
        }
      }
/* END: 0014110 jihoon.lee@lge.com 20110115 */

/* BEGIN: 0014656 jihoon.lee@lge.com 20110124 */
/* MOD 0014656: [LG RAPI] OEM RAPI PACKET MISMATCH KERNEL CRASH FIX */
      //send_to_arm9((void*)(((byte*)pReq) -sizeof(diagpkt_header_type) - sizeof(word)) , pRsp);
      send_to_arm9((void*)&req_ptr, (void*)pRsp);
/* END: 0014656 jihoon.lee@lge.com 2011024 */

/* BEGIN: 0014110 jihoon.lee@lge.com 20110115 */
/* MOD 0014110: [FACTORY RESET] stability */
/* handle operation or rpc failure as well */
      if(pRsp->ret_stat_code != TEST_OK_S)
      {
        printk(KERN_ERR "[Testmode]send_to_arm9 response : %d\n", pRsp->ret_stat_code);
        pRsp->ret_stat_code = TEST_FAIL_S;
        break;
      }
/* END: 0014110 jihoon.lee@lge.com 20110115 */

      /*LG_FW khlee 2010.03.04 -If we start at 5, we have to go to APP reset state(3) directly */
      if( startStatus == FACTORY_RESET_COLD_BOOT_END)
        test_mode_factory_reset_status = FACTORY_RESET_COLD_BOOT_START;
      else
        test_mode_factory_reset_status = FACTORY_RESET_ARM9_END;

      memset((void *)pbuf, 0, sizeof(pbuf));
      sprintf(pbuf, "%s%d\n",FACTORY_RESET_STR, test_mode_factory_reset_status);
      printk(KERN_INFO "[Testmode]factory reset status = %d\n", test_mode_factory_reset_status);

      mtd_op_result = lge_erase_block(factoryreset_bytes_pos_in_emmc, FACTORY_RESET_STR_SIZE+2);
/* BEGIN: 0014110 jihoon.lee@lge.com 20110115 */
/* MOD 0014110: [FACTORY RESET] stability */
/* handle operation or rpc failure as well */
      if(mtd_op_result!=(FACTORY_RESET_STR_SIZE+2))
      {
        printk(KERN_ERR "[Testmode]lge_erase_block, error num = %d \n", mtd_op_result);
        pRsp->ret_stat_code = TEST_FAIL_S;
        break;
      }
      else
      {
         mtd_op_result = lge_write_block(factoryreset_bytes_pos_in_emmc, pbuf, FACTORY_RESET_STR_SIZE+2);
         if(mtd_op_result!=(FACTORY_RESET_STR_SIZE+2))
         {
          printk(KERN_ERR "[Testmode]lge_write_block, error num = %d \n", mtd_op_result);
          pRsp->ret_stat_code = TEST_FAIL_S;
          break;
         }
      }
/* END: 0014110 jihoon.lee@lge.com 20110115 */

#else /**/
      //send_to_arm9((void*)(((byte*)pReq) -sizeof(diagpkt_header_type) - sizeof(word)) , pRsp);
      send_to_arm9((void*)&req_ptr, (void*)pRsp);
#endif /*CONFIG_LGE_MTD_DIRECT_ACCESS*/

      printk(KERN_INFO "%s, factory reset check completed \n", __func__);
      pRsp->ret_stat_code = TEST_OK_S;
      break;

    case FACTORY_RESET_COMPLETE_CHECK:
#if 1//def CONFIG_LGE_MTD_DIRECT_ACCESS
      pRsp->ret_stat_code = TEST_NOT_SUPPORTED_S;
      printk(KERN_ERR "[Testmode]not supported\n");
#else
      printk(KERN_INFO "[Testmode]send_to_arm9 start\n");
      //send_to_arm9((void*)(((byte*)pReq) -sizeof(diagpkt_header_type) - sizeof(word)) , pRsp);
      send_to_arm9((void*)&req_ptr, (void*)pRsp);
      printk(KERN_INFO "[Testmode]send_to_arm9 end\n");

      pRsp->ret_stat_code = TEST_OK_S;
#endif /*CONFIG_LGE_MTD_DIRECT_ACCESS*/
      break;

    case FACTORY_RESET_STATUS_CHECK:
#if 1 // def CONFIG_LGE_MTD_DIRECT_ACCESS
      memset((void*)pbuf, 0, sizeof(pbuf));
      mtd_op_result = lge_read_block(factoryreset_bytes_pos_in_emmc, pbuf, FACTORY_RESET_STR_SIZE+2 );
/* BEGIN: 0014110 jihoon.lee@lge.com 20110115 */
/* MOD 0014110: [FACTORY RESET] stability */
/* handle operation or rpc failure as well */
      if(mtd_op_result!=(FACTORY_RESET_STR_SIZE+2))
      {
      	 printk(KERN_ERR "[Testmode]lge_read_block, error num = %d \n", mtd_op_result);
      	 pRsp->ret_stat_code = TEST_FAIL_S;
      	 break;
      }
      else
      {
      	 if(memcmp(pbuf, FACTORY_RESET_STR, FACTORY_RESET_STR_SIZE) == 0) // tag read sucess
      	 {
      	   test_mode_factory_reset_status = pbuf[FACTORY_RESET_STR_SIZE] - '0';
      	   printk(KERN_INFO "[Testmode]factory reset status = %d \n", test_mode_factory_reset_status);
      	   pRsp->ret_stat_code = test_mode_factory_reset_status;
      	 }
      	 else
      	 {
      	   printk(KERN_ERR "[Testmode]factory reset tag fail, set initial state\n");
      	   test_mode_factory_reset_status = FACTORY_RESET_START;
      	   pRsp->ret_stat_code = test_mode_factory_reset_status;
      	   break;
      	 }
      }  
/* END: 0014110 jihoon.lee@lge.com 20110115 */
#endif /*CONFIG_LGE_MTD_DIRECT_ACCESS*/

      break;

    case FACTORY_RESET_COLD_BOOT:
// remove requesting sync to CP as all sync will be guaranteed on their own.

#if 1 // def CONFIG_LGE_MTD_DIRECT_ACCESS
      test_mode_factory_reset_status = FACTORY_RESET_COLD_BOOT_START;
      memset((void *)pbuf, 0, sizeof(pbuf));
      sprintf(pbuf, "%s%d",FACTORY_RESET_STR, test_mode_factory_reset_status);
      printk(KERN_INFO "[Testmode]factory reset status = %d\n", test_mode_factory_reset_status);
      mtd_op_result = lge_erase_block(factoryreset_bytes_pos_in_emmc,  FACTORY_RESET_STR_SIZE+2);
/* BEGIN: 0014110 jihoon.lee@lge.com 20110115 */
/* MOD 0014110: [FACTORY RESET] stability */
/* handle operation or rpc failure as well */
      if(mtd_op_result!=( FACTORY_RESET_STR_SIZE+2))
      {
        printk(KERN_ERR "[Testmode]lge_erase_block, error num = %d \n", mtd_op_result);
        pRsp->ret_stat_code = TEST_FAIL_S;
        break;
      }
      else
      {
        mtd_op_result = lge_write_block(factoryreset_bytes_pos_in_emmc, pbuf,  FACTORY_RESET_STR_SIZE+2);
        if(mtd_op_result!=(FACTORY_RESET_STR_SIZE+2))
        {
          printk(KERN_ERR "[Testmode]lge_write_block, error num = %d \n", mtd_op_result);
          pRsp->ret_stat_code = TEST_FAIL_S;
        }
      }
/* END: 0014110 jihoon.lee@lge.com 20110115 */
#endif /*CONFIG_LGE_MTD_DIRECT_ACCESS*/
      pRsp->ret_stat_code = TEST_OK_S;
      break;

    case FACTORY_RESET_ERASE_USERDATA:
#if 1 // def CONFIG_LGE_MTD_DIRECT_ACCESS
      test_mode_factory_reset_status = FACTORY_RESET_COLD_BOOT_START;
      memset((void *)pbuf, 0, sizeof(pbuf));
      sprintf(pbuf, "%s%d",FACTORY_RESET_STR, test_mode_factory_reset_status);
      printk(KERN_INFO "[Testmode-erase userdata]factory reset status = %d\n", test_mode_factory_reset_status);
      mtd_op_result = lge_erase_block(factoryreset_bytes_pos_in_emmc , FACTORY_RESET_STR_SIZE+2);
/* BEGIN: 0014110 jihoon.lee@lge.com 20110115 */
/* MOD 0014110: [FACTORY RESET] stability */
/* handle operation or rpc failure as well */
      if(mtd_op_result!=(FACTORY_RESET_STR_SIZE+2))
      {
        printk(KERN_ERR "[Testmode]lge_erase_block, error num = %d \n", mtd_op_result);
        pRsp->ret_stat_code = TEST_FAIL_S;
        break;
      }
      else
      {
        mtd_op_result = lge_write_block(factoryreset_bytes_pos_in_emmc, pbuf, FACTORY_RESET_STR_SIZE+2);
        if(mtd_op_result!=(FACTORY_RESET_STR_SIZE+2))
        {
          printk(KERN_ERR "[Testmode]lge_write_block, error num = %d \n", mtd_op_result);
          pRsp->ret_stat_code = TEST_FAIL_S;
          break;
        }
      }
/* END: 0014110 jihoon.lee@lge.com 20110115 */
#endif /*CONFIG_LGE_MTD_DIRECT_ACCESS*/
    pRsp->ret_stat_code = TEST_OK_S;
    break;

     default:
        pRsp->ret_stat_code = TEST_NOT_SUPPORTED_S;
        break;
    }
  return pRsp;
}

/* sangwoo.kang	10.09.03
				build error debug */
#if 0 // def CONFIG_LGE_MTD_DIRECT_ACCESS
int factory_reset_check(void)
{
  unsigned char pbuf[BUF_PAGE_SIZE];
  int mtd_op_result = 0;
  const MmcPartition *pMisc_part; 
  unsigned long factoryreset_bytes_pos_in_emmc = 0;
  
  lge_mmc_scan_partitions();
  pMisc_part = lge_mmc_find_partition_by_name("misc");
  factoryreset_bytes_pos_in_emmc = (pMisc_part->dfirstsec*512)+PTN_FRST_PERSIST_POSITION_IN_MISC_PARTITION;

  printk("factory_reset_check> mmc info sec : 0x%x, size : 0x%x type : 0x%x frst sec: 0x%x\n", pMisc_part->dfirstsec, pMisc_part->dsize, pMisc_part->dtype, factoryreset_bytes_pos_in_emmc);

  // check_staus
  memset((void*)pbuf, 0, sizeof(pbuf));
  mtd_op_result = lge_read_block(factoryreset_bytes_pos_in_emmc, pbuf, FACTORY_RESET_STR_SIZE+1 );
  if(mtd_op_result!=(FACTORY_RESET_STR_SIZE+1))
  {
 	 printk(KERN_ERR "factory_reset_check : lge_read_block, error num = %d \n", mtd_op_result);
  }
  else
  {
  	if(memcmp(pbuf, FACTORY_RESET_STR, FACTORY_RESET_STR_SIZE) == 0) // tag read sucess
   {
  	 test_mode_factory_reset_status = pbuf[FACTORY_RESET_STR_SIZE] - '0';
  	 printk(KERN_INFO "factory_reset_check : status = %d \n", test_mode_factory_reset_status);
   }
  	else
   {
  	 printk(KERN_ERR "factory_reset_check : tag fail\n");
  	 test_mode_factory_reset_status = FACTORY_RESET_START;
   }
  		
  }  

  // if status is cold boot start then mark it end
  if(test_mode_factory_reset_status == FACTORY_RESET_COLD_BOOT_START ||
      test_mode_factory_reset_status == 4 || /* 4 : temp value between factory reset operation and reboot */
      test_mode_factory_reset_status == 6 /* 6 : value to indicate to disable usb debug setting for ui factory reset*/)
  {
		memset((void *)pbuf, 0, sizeof(pbuf));
		test_mode_factory_reset_status = FACTORY_RESET_COLD_BOOT_END;

		diagpdev = diagcmd_get_dev();
    if (diagpdev != NULL){
        update_diagcmd_state(diagpdev, "ADBSET", 0);
    }

		sprintf(pbuf, "%s%d",FACTORY_RESET_STR, test_mode_factory_reset_status);
		printk(KERN_INFO "factory_reset_check : status = %d\n", test_mode_factory_reset_status);
		mtd_op_result = lge_erase_block(factoryreset_bytes_pos_in_emmc, FACTORY_RESET_STR_SIZE+1);
		if(mtd_op_result!=(FACTORY_RESET_STR_SIZE+1))
		{
			printk(KERN_ERR "factory_reset_check : lge_erase_block, error num = %d \n", mtd_op_result);
		}
		else
		{
			mtd_op_result = lge_write_block(factoryreset_bytes_pos_in_emmc, pbuf, FACTORY_RESET_STR_SIZE+1);
			if(mtd_op_result!=(FACTORY_RESET_STR_SIZE+1))
			{
				printk(KERN_ERR "factory_reset_check : lge_write_block, error num = %d \n", mtd_op_result);
			}
		}
	}
  return 0;
}

//EXPORT_SYMBOL(factory_reset_check);
#endif /* CONFIG_LGE_MTD_DIRECT_ACCESS */


void* LGF_TestScriptItemSet(	test_mode_req_type* pReq ,DIAG_TEST_MODE_F_rsp_type	*pRsp)
{
// BEGIN: 0009720 sehyuny.kim@lge.com 2010-10-06
// MOD 0009720: [Modem] It add RF X-Backup feature
  int mtd_op_result = 0;

  const MmcPartition *pMisc_part; 
  unsigned long factoryreset_bytes_pos_in_emmc = 0;
/* BEGIN: 0014656 jihoon.lee@lge.com 20110124 */
/* MOD 0014656: [LG RAPI] OEM RAPI PACKET MISMATCH KERNEL CRASH FIX */
  DIAG_TEST_MODE_F_req_type req_ptr;

  req_ptr.sub_cmd_code = TEST_MODE_TEST_SCRIPT_MODE;
  req_ptr.test_mode_req.test_mode_test_scr_mode = pReq->test_mode_test_scr_mode;
/* END: 0014656 jihoon.lee@lge.com 2011024 */

  lge_mmc_scan_partitions();
  pMisc_part = lge_mmc_find_partition_by_name("misc");
  factoryreset_bytes_pos_in_emmc = (pMisc_part->dfirstsec*512)+PTN_FRST_PERSIST_POSITION_IN_MISC_PARTITION;

  printk("LGF_TestScriptItemSet> mmc info sec : 0x%x, size : 0x%x type : 0x%x frst sec: 0x%lx\n", pMisc_part->dfirstsec, pMisc_part->dsize, pMisc_part->dtype, factoryreset_bytes_pos_in_emmc);

  switch(pReq->test_mode_test_scr_mode)
  {
    case TEST_SCRIPT_ITEM_SET:
  #if 1 // def CONFIG_LGE_MTD_DIRECT_ACCESS
      mtd_op_result = lge_erase_block(factoryreset_bytes_pos_in_emmc, (FACTORY_RESET_STR_SIZE+1) );
/* BEGIN: 0014110 jihoon.lee@lge.com 20110115 */
/* MOD 0014110: [FACTORY RESET] stability */
/* handle operation or rpc failure as well */
      if(mtd_op_result!=(FACTORY_RESET_STR_SIZE+1))
      {
      	 printk(KERN_ERR "[Testmode]lge_erase_block, error num = %d \n", mtd_op_result);
      	 pRsp->ret_stat_code = TEST_FAIL_S;
      	 break;
/* END: 0014110 jihoon.lee@lge.com 20110115 */
      } else
  #endif /*CONFIG_LGE_MTD_DIRECT_ACCESS*/
      // LG_FW khlee 2010.03.16 - They want to ACL on state in test script state.
      {
      	 update_diagcmd_state(diagpdev, "ALC", 1);
/* BEGIN: 0014656 jihoon.lee@lge.com 20110124 */
/* MOD 0014656: [LG RAPI] OEM RAPI PACKET MISMATCH KERNEL CRASH FIX */
      	 //send_to_arm9((void*)(((byte*)pReq) -sizeof(diagpkt_header_type) - sizeof(word)) , pRsp);
      	 send_to_arm9((void*)&req_ptr, (void*)pRsp);
        printk(KERN_INFO "%s, result : %s\n", __func__, pRsp->ret_stat_code==TEST_OK_S?"OK":"FALSE");
/* END: 0014656 jihoon.lee@lge.com 2011024 */
      }
      break;
  /*			
  	case CAL_DATA_BACKUP:
  	case CAL_DATA_RESTORE:
  	case CAL_DATA_ERASE:
  	case CAL_DATA_INFO:
  		diagpkt_free(pRsp);
  		return 0;			
  		break;
  */			
    default:
/* BEGIN: 0014656 jihoon.lee@lge.com 20110124 */
/* MOD 0014656: [LG RAPI] OEM RAPI PACKET MISMATCH KERNEL CRASH FIX */
      //send_to_arm9((void*)(((byte*)pReq) -sizeof(diagpkt_header_type) - sizeof(word)) , pRsp);
      send_to_arm9((void*)&req_ptr, (void*)pRsp);
      printk(KERN_INFO "%s, cmd : %d, result : %s\n", __func__, pReq->test_mode_test_scr_mode, \
	  										pRsp->ret_stat_code==TEST_OK_S?"OK":"FALSE");
      if(pReq->test_mode_test_scr_mode == TEST_SCRIPT_MODE_CHECK)
      {
        switch(pRsp->test_mode_rsp.test_mode_test_scr_mode)
        {
          case 0:
            printk(KERN_INFO "%s, mode : %s\n", __func__, "USER SCRIPT");
            break;
          case 1:
            printk(KERN_INFO "%s, mode : %s\n", __func__, "TEST SCRIPT");
            break;
          default:
            printk(KERN_INFO "%s, mode : %s, returned %d\n", __func__, "NO PRL", pRsp->test_mode_rsp.test_mode_test_scr_mode);
            break;
        }
      }
/* END: 0014656 jihoon.lee@lge.com 2011024 */
      break;
  }  
        

// END: 0009720 sehyuny.kim@lge.com 2010-10-06
  return pRsp;
}

// BEGIN: 0011366 sehyuny.kim@lge.com 2010-11-25
// MOD 0011366: [Testmode] Fix some testmode command related to firmware
extern int db_integrity_ready;
extern int fpri_crc_ready;
extern int file_crc_ready;
extern int code_partition_crc_ready;
extern int total_crc_ready;
extern int db_dump_ready;	
extern int db_copy_ready;	

typedef struct {
	char ret[32];
} testmode_rsp_from_diag_type;

extern testmode_rsp_from_diag_type integrity_ret;
void* LGF_TestModeDBIntegrityCheck(    test_mode_req_type* pReq ,DIAG_TEST_MODE_F_rsp_type	   *pRsp)
{
 
	printk(KERN_ERR "[_DBCHECK_] [%s:%d] DBCHECKSubCmd=<%d>\n", __func__, __LINE__, pReq->bt);
	memset(integrity_ret.ret, 0, 32);
	if (diagpdev != NULL){
			update_diagcmd_state(diagpdev, "DBCHECK", pReq->db_check);
			switch(pReq->db_check)
			{
				case DB_INTEGRITY_CHECK:
					while ( !db_integrity_ready )
						msleep(10);
					db_integrity_ready = 0;

					msleep(100); // wait until the return value is written to the file

					{
						unsigned long crc_val;
						//crc_val = simple_strtoul(integrity_ret.ret+1,NULL,10);
						crc_val = simple_strtoul(integrity_ret.ret+1,NULL,16);
						sprintf(pRsp->test_mode_rsp.str_buf, "0x%08X", crc_val);
						
						printk(KERN_INFO "%s\n", integrity_ret.ret);
						printk(KERN_INFO "%ld\n", crc_val);
						printk(KERN_INFO "%s\n", pRsp->test_mode_rsp.str_buf);
					}

					/* MANUFACTURE requested not to check the status, just return CRC
					if ( integrity_ret.ret[0] == '0' )
						pRsp->ret_stat_code = TEST_OK_S;
					else
						pRsp->ret_stat_code = TEST_FAIL_S;
					*/
					pRsp->ret_stat_code = TEST_OK_S;
					break;
					
				case FPRI_CRC_CHECK:
					while ( !fpri_crc_ready )
						msleep(10);
					fpri_crc_ready = 0;

					msleep(100); // wait until the return value is written to the file

					{
						unsigned long crc_val;
						crc_val = simple_strtoul(integrity_ret.ret+1,NULL,10);
						sprintf(pRsp->test_mode_rsp.str_buf, "0x%08X", crc_val);
						
						printk(KERN_INFO "%s\n", integrity_ret.ret);
						printk(KERN_INFO "%ld\n", crc_val);
						printk(KERN_INFO "%s\n", pRsp->test_mode_rsp.str_buf);
					}

					/* MANUFACTURE requested not to check the status, just return CRC
					if ( integrity_ret.ret[0] == '0' )
						pRsp->ret_stat_code = TEST_OK_S;
					else
						pRsp->ret_stat_code = TEST_FAIL_S;
					*/
					
					/*
					if ( integrity_ret.ret[0] == '0' )
					{
						unsigned long crc_val;
						pRsp->ret_stat_code = TEST_OK_S;
						memcpy(pRsp->test_mode_rsp.str_buf,integrity_ret.ret, 1);
						
						crc_val = simple_strtoul(integrity_ret.ret+1,NULL,10);
						sprintf(pRsp->test_mode_rsp.str_buf + 1, "%08x", crc_val);
						
					} else {
						pRsp->ret_stat_code = TEST_FAIL_S;
					}
					*/
					pRsp->ret_stat_code = TEST_OK_S;
					break;
					
				case FILE_CRC_CHECK:
				{
					while ( !file_crc_ready )
						msleep(10);
					file_crc_ready = 0;

					msleep(100); // wait until the return value is written to the file

					{
						unsigned long crc_val;
						crc_val = simple_strtoul(integrity_ret.ret+1,NULL,10);
						sprintf(pRsp->test_mode_rsp.str_buf, "0x%08X", crc_val);
						
						printk(KERN_INFO "%s\n", integrity_ret.ret);
						printk(KERN_INFO "%ld\n", crc_val);
						printk(KERN_INFO "%s\n", pRsp->test_mode_rsp.str_buf);
					}

					/* MANUFACTURE requested not to check the status, just return CRC
					if ( integrity_ret.ret[0] == '0' )
						pRsp->ret_stat_code = TEST_OK_S;
					else
						pRsp->ret_stat_code = TEST_FAIL_S;
					*/
					pRsp->ret_stat_code = TEST_OK_S;
					break;
					
				/*
					int mtd_op_result = 0;
					char sec_buf[512];
					const MmcPartition *pSystem_part; 
					unsigned long system_bytes_pos_in_emmc = 0;
					unsigned long system_sec_remained = 0;
					
					printk(KERN_INFO"FILE_CRC_CHECK read block1\n");
					
					lge_mmc_scan_partitions();
					
					pSystem_part = lge_mmc_find_partition_by_name("system");
					if ( pSystem_part == NULL )
					{
					
						printk(KERN_INFO"NO System\n");
						return 0;
					}
					system_bytes_pos_in_emmc = (pSystem_part->dfirstsec*512);
					system_sec_remained = pSystem_part->dsize;
					memset(sec_buf, 0 , 512);

					do 
					{
						mtd_op_result = lge_read_block(system_bytes_pos_in_emmc, sec_buf, 512);
						system_bytes_pos_in_emmc += mtd_op_result;
						system_sec_remained -= 1;
						printk(KERN_INFO"FILE_CRC_CHECK> system_sec_remained %d \n", system_sec_remained);
						
					} while ( mtd_op_result != 0 && system_sec_remained != 0 );
				*/	
#if 0					
					while ( !file_crc_ready )
						msleep(10);
					file_crc_ready = 0;
#else
//					pRsp->ret_stat_code = TEST_OK_S;

#endif					
					break;
				}
				case CODE_PARTITION_CRC_CHECK:
#if 0					
					while ( !code_partition_crc_ready )
						msleep(10);
					code_partition_crc_ready = 0;
#else
					pRsp->ret_stat_code = TEST_NOT_SUPPORTED_S;
					
#endif					
					break;
				case TOTAL_CRC_CHECK:
#if 0 					
					while ( !total_crc_ready )
						msleep(10);
					total_crc_ready = 0;
#else
					pRsp->ret_stat_code = TEST_NOT_SUPPORTED_S;
										
#endif					

					break;

// hojung7.kim@lge.com Add  (MS910)

				case DB_DUMP:
#if 0 					
					while ( !total_crc_ready )
									msleep(10);
					total_crc_ready = 0;
#else
					//apply LS700 codes
					//pRsp->ret_stat_code = TEST_NOT_SUPPORTED_S;
					msleep(100); // wait until the return value is written to the file
					pRsp->ret_stat_code = TEST_OK_S;
													
#endif					
			
					break;

				case DB_COPY:
#if 0 					
					while ( !total_crc_ready )
						msleep(10);
					total_crc_ready = 0;
#else
					//apply LS700 codes
					//pRsp->ret_stat_code = TEST_NOT_SUPPORTED_S;
					msleep(100); // wait until the return value is written to the file
					pRsp->ret_stat_code = TEST_OK_S;
										
#endif					

					break;
// hojung7.kim@lge.com Add  (MS910)

			}
	}
	else
	{
			printk("\n[%s] error DBCHECK", __func__ );
			pRsp->ret_stat_code = TEST_NOT_SUPPORTED_S;
	}
	printk(KERN_ERR "[_DBCHECK_] [%s:%d] DBCHECK Result=<%s>\n", __func__, __LINE__, integrity_ret.ret);

	return pRsp;
}
 
// END: 0010090 sehyuny.kim@lge.com 2010-10-21


//LGE_FOTA_ID_CHECK
extern int fota_cmd_ret_ready;
extern char fota_cmd_ret[100];

void* LGF_TestModeFOTAIDCheck(    test_mode_req_type* pReq ,DIAG_TEST_MODE_F_rsp_type	   *pRsp)
{
	int wait_cnt = 0;
	
	printk(KERN_INFO "%s, cmd : %d\n", __func__, pReq->fota_check_cmd);
	
	memset(fota_cmd_ret, 0, sizeof(fota_cmd_ret));
	
	switch(pReq->fota_check_cmd)
	{
		case FOTA_ID_CHECK_CMD:
			if (diagpdev != NULL){
				update_diagcmd_state(diagpdev, "FOTAIDCHECK", pReq->fota_check_cmd);
			
				while ( !fota_cmd_ret_ready )
				{
					wait_cnt++;
					msleep(100);

					if(wait_cnt > 100)
					{
						printk(KERN_ERR "%s, timeout cnt : %d", __func__, wait_cnt);
						pRsp->ret_stat_code = TEST_FAIL_S;
						return pRsp;
					}					
				}
				fota_cmd_ret_ready = 0;

				msleep(100); // wait until the return value is written to the file

				printk(KERN_INFO "%s, ret : %s\n", __func__, fota_cmd_ret);

				if(strcmp(fota_cmd_ret, "TRUE") == 0)
				{
					pRsp->ret_stat_code = TEST_OK_S;
				}
				else
				{
					pRsp->ret_stat_code = TEST_FAIL_S;
				}
			}
			else
			{
				printk(KERN_ERR "%s, diagpdev is not ready", __func__ );
				pRsp->ret_stat_code = TEST_FAIL_S;
			}
			break;

		default:
			printk(KERN_INFO "%s, unknown command : %d\n", __func__, pReq->fota_check_cmd);
			pRsp->ret_stat_code = TEST_NOT_SUPPORTED_S;
			break;
	}

	return pRsp;
}



/*  USAGE
  *    1. If you want to handle at ARM9 side, you have to insert fun_ptr as NULL and mark ARM9_PROCESSOR
  *    2. If you want to handle at ARM11 side , you have to insert fun_ptr as you want and mark AMR11_PROCESSOR.
  */

testmode_user_table_entry_type testmode_mstr_tbl[TESTMODE_MSTR_TBL_SIZE] =
{
/*    sub_command                                              fun_ptr                      which procesor              */
	/* 0 ~ 5 */
	{ TEST_MODE_VERSION,                  NULL,                     ARM9_PROCESSOR},
	{ TEST_MODE_LCD,                      linux_app_handler,        ARM11_PROCESSOR},
	{ TEST_MODE_MOTOR,                    LGF_TestMotor,            ARM11_PROCESSOR},
	{ TEST_MODE_ACOUSTIC,                 LGF_TestAcoustic,         ARM11_PROCESSOR},
	/* 5 ~ 10 */
	{ TEST_MODE_CAM,                      LGF_TestCam,              ARM11_PROCESSOR},
	/* 11 ~ 15 */

	/* 16 ~ 20 */

	/* 21 ~ 25 */
	{ TEST_MODE_KEY_TEST,                 LGT_TestModeKeyTest,      ARM11_PROCESSOR},
	{ TEST_MODE_EXT_SOCKET_TEST,		  linux_app_handler,		ARM11_PROCESSOR},
//	{ TEST_MODE_EXT_SOCKET_TEST,		  LGF_ExternalSocketMemory, ARM11_PROCESSOR},
#ifndef LG_BTUI_TEST_MODE
	{ TEST_MODE_BLUETOOTH_TEST,           LGF_TestModeBlueTooth,    ARM11_PROCESSOR},
#endif //LG_BTUI_TEST_MODE
	/* 26 ~ 30 */
	{ TEST_MODE_MP3_TEST,                 LGF_TestModeMP3,          ARM11_PROCESSOR},
	/* 31 ~ 35 */
	{ TEST_MODE_ACCEL_SENSOR_TEST,        linux_app_handler,        ARM11_PROCESSOR},
	{ TEST_MODE_WIFI_TEST,                linux_app_handler,        ARM11_PROCESSOR},
	/* 36 ~ 40 */
	{ TEST_MODE_KEY_DATA_TEST,            LGF_TestModeKeyData,        ARM11_PROCESSOR},
// BEGIN : munho.lee@lge.com 2010-11-30
// ADD: 0011620: [Test_mode] Memory format test 
	{ TEST_MODE_FORMAT_MEMORY_TEST,		  LGF_MemoryFormatTest,		ARM11_PROCESSOR},
// END : munho.lee@lge.com 2010-11-30
	/* 41 ~ 45 */
	{ TEST_MODE_MEMORY_CAPA_TEST,         LGF_MemoryVolumeCheck,    ARM11_PROCESSOR},
#if 1 //def CONFIG_LGE_TOUCH_TEST
	{ TEST_MODE_SLEEP_MODE_TEST,          LGF_PowerSaveMode,        ARM11_PROCESSOR},
#endif /* CONFIG_LGE_TOUCH_TEST */
	{ TEST_MODE_SPEAKER_PHONE_TEST,       LGF_TestModeSpeakerPhone, ARM11_PROCESSOR},
// BEGIN: 0010557  unchol.park@lge.com 2010-11-18
// 0010557 : [Testmode] added test_mode command for LTE test 
// add call function
// BEGIN: 0010557  unchol.park@lge.com 2011-01-12
// 0010557 : [Testmode] added test_mode command for LTE test 
	{ TEST_MODE_VIRTUAL_SIM_TEST,         LGF_TestModeDetach, 		ARM11_PROCESSOR},
// END: 0010557 unchol.park@lge.com 2011-01-12
// END: 0010557 unchol.park@lge.com 2010-11-18
	{ TEST_MODE_PHOTO_SENSER_TEST,        LGF_TestPhotoSensor,        ARM11_PROCESSOR},

	/* 46 ~ 50 */
	{ TEST_MODE_MRD_USB_TEST,             NULL,                     ARM9_PROCESSOR },
	{ TEST_MODE_PROXIMITY_SENSOR_TEST,    linux_app_handler,        ARM11_PROCESSOR },
	{ TEST_MODE_TEST_SCRIPT_MODE,         LGF_TestScriptItemSet,    ARM11_PROCESSOR },
	
	{ TEST_MODE_FACTORY_RESET_CHECK_TEST, LGF_TestModeFactoryReset, ARM11_PROCESSOR },//
	/* 51 ~	*/
	{ TEST_MODE_VOLUME_TEST,              LGT_TestModeVolumeLevel,  ARM11_PROCESSOR},
#ifdef CONFIG_LGE_BOOTCOMPLETE_INFO
	{ TEST_MODE_FIRST_BOOT_COMPLETE_TEST,  LGF_TestModeFboot,        ARM11_PROCESSOR},
#endif
// 0010557 : [Testmode] added test_mode command for LTE test
// add call function 2010-11-18
	{ TEST_MODE_MAX_CURRENT_CHECK,     	 NULL,							ARM9_PROCESSOR},
	{ TEST_MODE_CHANGE_RFCALMODE,     	 NULL,  						ARM9_PROCESSOR},
	{ TEST_MODE_SELECT_MIMO_ANT,       	 NULL,  						ARM9_PROCESSOR},
	{ TEST_MODE_LTE_MODE_SELECTION, 	LGF_TestModeLteModeSelection,	ARM11_PROCESSOR},
	{ TEST_MODE_LTE_CALL, 				LGF_TestModeLteCall,			ARM11_PROCESSOR},
	/* [yk.kim@lge.com] 2011-01-04, change usb driver */
	{ TEST_MODE_CHANGE_USB_DRIVER, LGF_TestModeChangeUsbDriver, ARM11_PROCESSOR},
// BEGIN: 0010557  unchol.park@lge.com 2011-01-24
// 0010557 : [Testmode] added test_mode command for LTE test
	{ TEST_MODE_GET_HKADC_VALUE, 		NULL, 							ARM9_PROCESSOR},
// END: 0010557 unchol.park@lge.com 2011-01-24
// add call function 2010-11-18
// END: 0010557 unchol.park@lge.com 2010-11-05
//	{ TEST_MODE_LED_TEST, 				linux_app_handler,				ARM11_PROCESSOR},
	{ TEST_MODE_LED_TEST, 				NULL,				ARM9_PROCESSOR},
       /*70~	*/
	{ TEST_MODE_PID_TEST,             	 NULL,  						ARM9_PROCESSOR},
	{ TEST_MODE_SW_VERSION, 		NULL, 						ARM9_PROCESSOR},
	{ TEST_MODE_IME_TEST, 		NULL, 						ARM9_PROCESSOR},
	{ TEST_MODE_IMPL_TEST, 		NULL, 						ARM9_PROCESSOR},
	{ TEST_MODE_SIM_LOCK_TYPE_TEST, 		NULL, 						ARM9_PROCESSOR},
	{ TEST_MODE_UNLOCK_CODE_TEST, 		NULL, 						ARM9_PROCESSOR},
	{ TEST_MODE_IDDE_TEST, 		NULL, 						ARM9_PROCESSOR},
	{ TEST_MODE_FULL_SIGNATURE_TEST, 		NULL, 						ARM9_PROCESSOR},
	{ TEST_MODE_NT_CODE_TEST, 		NULL, 						ARM9_PROCESSOR},
	{ TEST_MODE_SIM_ID_TEST, 		NULL, 						ARM9_PROCESSOR},
	/*80~	*/
	{ TEST_MODE_CAL_CHECK, 			NULL,						ARM9_PROCESSOR},
#ifndef LG_BTUI_TEST_MODE
	{ TEST_MODE_BLUETOOTH_TEST_RW			 ,	LGF_TestModeBlueTooth_RW	   , ARM11_PROCESSOR},
#endif //LG_BTUI_TEST_MODE
	{ TEST_MODE_SKIP_WELCOM_TEST, 			NULL,						ARM9_PROCESSOR},
#if 1     //LGE_CHANGE, [dongp.kim@lge.com], 2010-10-09, adding Wi-Fi MAC read/write for Testmode menus 
	{ TEST_MODE_MAC_READ_WRITE,    linux_app_handler,        ARM11_PROCESSOR },
#endif //LG_FW_WLAN_TEST 
// BEGIN: 0011366 sehyuny.kim@lge.com 2010-11-25
// MOD 0011366: [Testmode] Fix some testmode command related to firmware
	{ TEST_MODE_DB_INTEGRITY_CHECK,         LGF_TestModeDBIntegrityCheck,   ARM11_PROCESSOR},
    { TEST_MODE_NVCRC_CHECK, 		NULL,	ARM9_PROCESSOR},
// END: 0011366 sehyuny.kim@lge.com 2010-11-25
	{ TEST_MODE_RELEASE_CURRENT_LIMIT,		NULL,								ARM9_PROCESSOR},
/* BEGIN: 0015893 jihoon.lee@lge.com 20110212 */
/* ADD 0015893: [MANUFACTURE] WDC LIFETIMER CLNR LAUNCH_KIT */
    { TEST_MODE_RESET_PRODUCTION, 		NULL,								ARM9_PROCESSOR},
/* END: 0015893 jihoon.lee@lge.com 20110212 */

    //LGE_FOTA_ID_CHECK
    { TEST_MODE_FOTA_ID_CHECK, 		LGF_TestModeFOTAIDCheck,					ARM11_PROCESSOR},
};

