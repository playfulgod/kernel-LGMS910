/* Copyright (c) 2008-2011, Code Aurora Forum. All rights reserved.
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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/diagchar.h>
#include <linux/sched.h>
#ifdef CONFIG_DIAG_OVER_USB
#include <mach/usbdiag.h>
#endif
#include <asm/current.h>
#include <linux/timer.h>
#include <linux/kthread.h>
#include <mach/lg_diagcmd.h>
#include <linux/syscalls.h>
#include <linux/fcntl.h>
#include <linux/platform_device.h>
#include <linux/slab.h>

#include "diagchar_hdlc.h"
#include "diagfwd.h"
#include "diagmem.h"
#include "diagchar.h"
#include "lg_diag_kernel_service.h"
#include <mach/lg_diag_testmode.h>
#include <mach/lg_diagcmd.h>
#if 1
/* LGE_MERGE_S [sunmyoung.lee@lge.com] 2010-07-16. SMS UTS Test */
#include <mach/lg_diag_udm.h>
/* LGE_MERGE_E [sunmyoung.lee@lge.com] 2010-07-16. SMS UTS Test */

PACK (void *)LGF_WIFI (PACK (void	*)req_pkt_ptr, uint16		pkt_len );
#endif
PACK (void *)LGF_TestMode (PACK (void	*)req_pkt_ptr, uint16		pkt_len );
#if 1
PACK (void *)LGF_LcdQTest (PACK (void	*)req_pkt_ptr, uint16		pkt_len );
PACK (void *)LGF_KeyPress (PACK (void	*)req_pkt_ptr, uint16		pkt_len );
/* LGE_CHANGE_S [minjong.gong@lge.com] 2010-06-11. UTS Test */
PACK (void *)LGF_ScreenShot (PACK (void	*)req_pkt_ptr, uint16		pkt_len ); 
/* LGE_CHANGE_E [minjong.gong@lge.com] 2010-06-11. UTS Test */
#endif

/* BEGIN:0010147 [yk.kim@lge.com] 2010-10-22 */
/* MOD:0010147 [WMC] add feature vzw wmc (wireless mobile contol) */
#ifdef CONFIG_LGE_DIAG_WMC
PACK (void *)LGF_WMC (PACK (void	*)req_pkt_ptr, uint16		pkt_len );
#endif
/* END:0010147 [yk.kim@lge.com] 2010-10-22 */

#if 1
/* LGE_MERGE_S [sunmyoung.lee@lge.com] 2010-07-16. SMS UTS Test */
PACK (void *)LGF_Udm (PACK (void	*)req_pkt_ptr, uint16		pkt_len ); 
/* LGE_MERGE_E [sunmyoung.lee@lge.com] 2010-07-16. SMS UTS Test */
#endif
/* LGE_CHANGE_S [jihoon.lee@lge.com] 2010-02-07, LG_FW_MTC */
#if 1 // def LG_FW_MTC //defined (CONFIG_MACH_MSM7X27_THUNDERC) || defined (LG_FW_MTC)
PACK (void *)LGF_MTCProcess (PACK (void *)req_pkt_ptr, uint16	pkt_len );
#endif /*LG_FW_MTC*/
#if 1
/* LGE_CHANGE_E [jihoon.lee@lge.com] 2010-02-07, LG_FW_MTC */
void diagpkt_commit (PACK(void *)pkt);
/* LGE_CHANGE_S [jihoon.lee@lge.com] 2010-02-22, LG_FW_MTC */
#endif
/* LGE_CHANGE_E [jihoon.lee@lge.com] 2010-02-22, LG_FW_MTC */


#ifdef CONFIG_LGE_DIAGTEST
static unsigned int threshold_client_limit = 30;

/* delayed_rsp_id 0 represents no delay in the response. Any other number
    means that the diag packet has a delayed response. */
static uint16_t delayed_rsp_id = 1;
#ifndef DIAGPKT_MAX_DELAYED_RSP
#define DIAGPKT_MAX_DELAYED_RSP 0xFFFF
#endif
/* This macro gets the next delayed respose id. Once it reaches
 DIAGPKT_MAX_DELAYED_RSP, it stays at DIAGPKT_MAX_DELAYED_RSP */

#ifndef DIAGPKT_NEXT_DELAYED_RSP_ID
#define DIAGPKT_NEXT_DELAYED_RSP_ID(x) 				\
((x < DIAGPKT_MAX_DELAYED_RSP) ? x++ : DIAGPKT_MAX_DELAYED_RSP)
#endif

extern unsigned int diag_max_registration;
extern unsigned int diag_threshold_registration;
#endif /*CONFIG_LGE_DIAGTEST*/


static const diagpkt_user_table_entry_type registration_table[] =
{ /* subsys cmd low, subsys cmd code high, call back function */
	{DIAG_TEST_MODE_F, DIAG_TEST_MODE_F, LGF_TestMode},
	{DIAG_LCD_Q_TEST_F, DIAG_LCD_Q_TEST_F, LGF_LcdQTest},
	{DIAG_HS_KEY_F,DIAG_HS_KEY_F,LGF_KeyPress},
/* LGE_CHANGE_S [minjong.gong@lge.com] 2010-06-11. UTS Test */
	{DIAG_LGF_SCREEN_SHOT_F , DIAG_LGF_SCREEN_SHOT_F , LGF_ScreenShot },
/* LGE_CHANGE_E [minjong.gong@lge.com] 2010-06-11. UTS Test */
/* LGE_CHANGE_S [hyogook.lee@lge.com] 2010-11-05, LG_FW_MTC */
#if 1 // def LG_FW_MTC // defined (CONFIG_MACH_MSM7X27_THUNDERC) || defined (LG_FW_MTC)
	{DIAG_MTC_F 	 ,	DIAG_MTC_F	  , LGF_MTCProcess},
#endif /*LG_FW_MTC*/
/* LGE_CHANGE_E [hyogook.lee@lge.com] 2010-11-05, LG_FW_MTC */
	{DIAG_WIFI_MAC_ADDR, DIAG_WIFI_MAC_ADDR, LGF_WIFI},
#ifdef CONFIG_LGE_DIAG_WMC
    {DIAG_WMCSYNC_MAPPING_F, DIAG_WMCSYNC_MAPPING_F, LGF_WMC},
#endif
/* LGE_MERGE_S [sunmyoung.lee@lge.com] 2010-07-15. SMS UTS Test */
	{DIAG_UDM_SMS_MODE , DIAG_UDM_SMS_MODE , LGF_Udm },
/* LGE_MERGE_E [sunmyoung.lee@lge.com] 2010-07-15. SMS UTS Test */
};

/* This is the user dispatch table. */
static diagpkt_user_table_type *lg_diagpkt_user_table[DIAGPKT_USER_TBL_SIZE];

extern struct diagchar_dev *driver;
unsigned char read_buffer[READ_BUF_SIZE]; 
struct task_struct *lg_diag_thread;
static int num_bytes_read;

extern struct timer_list drain_timer;
extern int timer_in_progress;
extern spinlock_t diagchar_write_lock;
static 	void *buf_hdlc;
static unsigned int gPkt_commit_fail = 0;
extern int usb_diag_write(struct usb_diag_ch *ch, struct diag_request *d_req);

void* lg_diag_req_pkt_ptr;

/* LGE_CHANGES_S, [dongp.kim@lge.com], 2010-01-10, <LGE_FACTORY_TEST_MODE for WLAN RF Test > */
wlan_status lg_diag_req_wlan_status={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
/* LGE_CHANGES_E, [dongp.kim@lge.com], 2010-01-10, <LGE_FACTORY_TEST_MODE for WLAN RF Test > */

#ifdef LG_FW_COMPILE_ERROR
/* LGE_MERGE_S [sunmyoung.lee@lge.com] 2010-07-15. SMS UTS Test */
udm_sms_status_new lg_diag_req_udm_sms_status_new = {0};
/* LGE_MERGE_E [sunmyoung.lee@lge.com] 2010-07-15. SMS UTS Test */
#endif /* LG_FW_COMPILE_ERROR */

uint16 lg_diag_req_pkt_length;
uint16 lg_diag_rsp_pkt_length;
char lg_diag_cmd_line[LG_DIAG_CMD_LINE_LEN];
/* LGE_CHANGE_S [jihoon.lee@lge.com] 2010-02-22, LG_FW_MTC */
// enable to send more than maximum packet size limitation
#if 1 //def LG_FW_MTC //defined (CONFIG_MACH_MSM7X27_THUNDERC) || defined (LG_FW_MTC)
extern unsigned char g_diag_mtc_check;
extern unsigned char g_diag_mtc_capture_rsp_num;
extern void lg_diag_set_enc_param(void *, void *);
#endif /*LG_FW_MTC*/
/* LGE_CHANGE_E [jihoon.lee@lge.com] 2010-02-22, LG_FW_MTC */

#ifdef CONFIG_LGE_DIAG_WMC
extern void* lg_diag_wmc_req_pkt_ptr;
extern uint16 lg_diag_wmc_req_pkt_length;
extern uint16 lg_diag_wmc_rsp_pkt_length;
#endif /*CONFIG_LGE_DIAG_WMC*/
/* jihye.ahn  2010-11-13  for capturing video preview */
int blt_mode_enable(void);
int blt_mode_disable(void);

/*===========================================================================
FUNCTION DIAGPKT_ALLOC

DESCRIPTION
  This function is from diagpkt_alloc() in diag_lsm_pkt.c

===========================================================================*/
PACK(void *)
diagpkt_alloc (diagpkt_cmd_code_type code, unsigned int length)
{
    diagpkt_lsm_rsp_type *item = NULL;
    diagpkt_hdr_type *pkt = NULL;
    PACK(uint16 *)pattern = NULL;    /* Overrun pattern. */
    unsigned char *p;
    diag_data* pdiag_data = NULL;
     unsigned int size = 0;

#ifndef CONFIG_LGE_DIAGTEST
	#ifdef FEATURE_WINMOB
     if(NULL == ghWinDiag)
    #else
	 if(0 == fd)
	#endif
      {
         return NULL;
      }
#endif
   size = DIAG_REST_OF_DATA_POS + FPOS (diagpkt_lsm_rsp_type, rsp.pkt) + length + sizeof (uint16);
   
    /*-----------------------------------------------
      Try to allocate a buffer.  Size of buffer must
      include space for overhead and CRC at the end.
    -----------------------------------------------*/
#ifdef CONFIG_LGE_DIAGTEST
#ifdef CONFIG_LGE_DIAG_COMPLEMENT
      // allocate memory and set initially 0.
      pdiag_data = (diag_data*)kzalloc (size, GFP_KERNEL);
#else
      pdiag_data = (diag_data*)kmalloc (size, GFP_KERNEL);
#endif
#else
      pdiag_data = (diag_data*)DiagSvc_Malloc (size, PKT_SVC_ID);
#endif /*CONFIG_LGE_DIAGTEST*/
      if(NULL == pdiag_data)
      {
         /* Alloc not successful.  Return NULL. DiagSvc_Malloc() allocates memory
	  from client's heap using a malloc call if the pre-malloced buffers are not available.
	  So if this fails, it means that the client is out of heap. */
         return NULL;
      }
      /* Fill in the fact that this is a response */
      pdiag_data->diag_data_type = DIAG_DATA_TYPE_RESPONSE;
      // WM7 prototyping: advance the pointer now
      item = (diagpkt_lsm_rsp_type*)((byte*)(pdiag_data)+DIAG_REST_OF_DATA_POS);
	
    /* This pattern is written to verify pointers elsewhere in this
       service  are valid. */
    item->rsp.pattern = DIAGPKT_HDR_PATTERN;    /* Sanity check pattern */
    
    /* length ==  size unless packet is resized later */
    item->rsp.size = length;
    item->rsp.length = length;

    pattern = (PACK(uint16 *)) & item->rsp.pkt[length];

    /* We need this to meet alignment requirements - MATS */
    p = (unsigned char *) pattern;
    p[0] = (DIAGPKT_OVERRUN_PATTERN >> 8) & 0xff;
    p[1] = (DIAGPKT_OVERRUN_PATTERN >> 0) & 0xff;

    pkt = (diagpkt_hdr_type *) & item->rsp.pkt;

    if (pkt)
    {
        pkt->command_code = code;
    }
    return (PACK(void *)) pkt;
}               /* diagpkt_alloc */
EXPORT_SYMBOL(diagpkt_alloc);


/*=========================================================================
FUNCTION DIAGPKT_FREE

DESCRIPTION
  This function is from diagpkt_free in diag_lsm_pkt.c

===========================================================================*/
void
diagpkt_free(PACK(void *)pkt)
{
  if (pkt)
  {
    byte *item = (byte*)DIAGPKT_PKT2LSMITEM(pkt);
    item -= DIAG_REST_OF_DATA_POS;
#ifdef CONFIG_LGE_DIAGTEST
    kfree ((void *)item);
#else
    DiagSvc_Free ((void *)item,PKT_SVC_ID);
#endif
  }
 return;
}
EXPORT_SYMBOL(diagpkt_free);

static ssize_t read_cmd_pkt(struct device *dev, struct device_attribute *attr,
		char *buf)
{
  memcpy(buf, lg_diag_req_pkt_ptr, lg_diag_req_pkt_length);
  
	return lg_diag_req_pkt_length;
}

static ssize_t write_cmd_pkt(struct device *dev,
						 struct device_attribute *attr,
						 const char *buf, size_t size)
{
  void* rsp_pkt_ptr;
#ifdef LG_DIAG_DEBUG
  int i;
#endif

	printk(KERN_ERR "\n LG_FW : print received packet :len(%d) \n",lg_diag_rsp_pkt_length);
	rsp_pkt_ptr = (DIAG_TEST_MODE_F_rsp_type *)diagpkt_alloc(DIAG_TEST_MODE_F, lg_diag_rsp_pkt_length);
  memcpy(rsp_pkt_ptr, buf, lg_diag_rsp_pkt_length);

#ifdef LG_DIAG_DEBUG
	for (i=0;i<lg_diag_rsp_pkt_length;i++) {
			printk(KERN_ERR "0x%x ",*((unsigned char*)(rsp_pkt_ptr + i)));
	}
	printk(KERN_ERR "\n");
#endif
  diagpkt_commit(rsp_pkt_ptr);
	return size;
}

static ssize_t read_cmd_pkt_length(struct device *dev, struct device_attribute *attr,
	char *buf)
{
  int read_len = 2;
  
  memcpy(buf, &lg_diag_req_pkt_length, read_len);
  return read_len;
}

static ssize_t write_cmd_pkt_length(struct device *dev,
					 struct device_attribute *attr,
					 const char *buf, size_t size)
{
  int write_len = 2;

  memcpy((void*)&lg_diag_rsp_pkt_length, buf, write_len);
  printk( KERN_DEBUG "LG_FW : write_cmd_pkt_length = %d\n",lg_diag_rsp_pkt_length);  
  return write_len;
}
/* LGE_CHANGES_S, [dongp.kim@lge.com], 2010-01-10, <LGE_FACTORY_TEST_MODE for WLAN RF Test > */

#if 1    //LGE_CHANG, [dongp.kim@lge.com], 2010-10-09, deleting #ifdef LG_FW_WLAN_TEST for enabling Wi-Fi Testmode menus
static ssize_t read_wlan_status(struct device *dev, struct device_attribute *attr,
		char *buf)
{
  int wlan_status_length = sizeof(wlan_status);
  memcpy(buf, &lg_diag_req_wlan_status, wlan_status_length);

  printk( KERN_DEBUG "LG_FW [KERNEL]: read_wlan_status(wlan_status)= %d\n",lg_diag_req_wlan_status.wlan_status);
  printk( KERN_DEBUG "LG_FW [KERNEL]: read_wlan_status(g_wlan_status) = %d\n",lg_diag_req_wlan_status.g_wlan_status);
  printk( KERN_DEBUG "LG_FW [KERNEL]: read_wlan_status(rx_channel) = %d\n",lg_diag_req_wlan_status.rx_channel);
  printk( KERN_DEBUG "LG_FW [KERNEL]: read_wlan_status(rx_per) = %d\n",lg_diag_req_wlan_status.rx_per);
  printk( KERN_DEBUG "LG_FW [KERNEL]: read_wlan_status(tx_channel) = %d\n",lg_diag_req_wlan_status.tx_channel);
  printk( KERN_DEBUG "LG_FW [KERNEL]: read_wlan_status(goodFrames) = %ld\n",lg_diag_req_wlan_status.goodFrames);
  printk( KERN_DEBUG "LG_FW [KERNEL]: read_wlan_status(badFrames) = %d\n",lg_diag_req_wlan_status.badFrames);
  printk( KERN_DEBUG "LG_FW [KERNEL]: read_wlan_status(rxFrames) = %d\n",lg_diag_req_wlan_status.rxFrames);
  printk( KERN_DEBUG "LG_FW [KERNEL]: read_wlan_status(wlan_data_rate) = %d\n",lg_diag_req_wlan_status.wlan_data_rate);
  printk( KERN_DEBUG "LG_FW [KERNEL]: read_wlan_status(wlan_payload) = %d\n",lg_diag_req_wlan_status.wlan_payload);
  printk( KERN_DEBUG "LG_FW [KERNEL]: read_wlan_status(wlan_data_rate_recent) = %d\n",lg_diag_req_wlan_status.wlan_data_rate_recent);
  
  return wlan_status_length;
}
static ssize_t write_wlan_status(struct device *dev,
					 struct device_attribute *attr,
					 const char *buf, size_t size)
{

  int wlan_status_length = sizeof(wlan_status);
  memcpy(&lg_diag_req_wlan_status, buf, wlan_status_length);

  printk( KERN_DEBUG "LG_FW [KERNEL]: write_wlan_status(wlan_status)= %d\n",lg_diag_req_wlan_status.wlan_status);
  printk( KERN_DEBUG "LG_FW [KERNEL]: write_wlan_status(g_wlan_status) = %d\n",lg_diag_req_wlan_status.g_wlan_status);
  printk( KERN_DEBUG "LG_FW [KERNEL]: write_wlan_status(rx_channel) = %d\n",lg_diag_req_wlan_status.rx_channel);
  printk( KERN_DEBUG "LG_FW [KERNEL]: write_wlan_status(rx_per) = %d\n",lg_diag_req_wlan_status.rx_per);
  printk( KERN_DEBUG "LG_FW [KERNEL]: write_wlan_status(tx_channel) = %d\n",lg_diag_req_wlan_status.tx_channel);
  printk( KERN_DEBUG "LG_FW [KERNEL]: write_wlan_status(goodFrames) = %ld\n",lg_diag_req_wlan_status.goodFrames);
  printk( KERN_DEBUG "LG_FW [KERNEL]: write_wlan_status(badFrames) = %d\n",lg_diag_req_wlan_status.badFrames);
  printk( KERN_DEBUG "LG_FW [KERNEL]: write_wlan_status(rxFrames) = %d\n",lg_diag_req_wlan_status.rxFrames);
  printk( KERN_DEBUG "LG_FW [KERNEL]: write_wlan_status(wlan_data_rate) = %d\n",lg_diag_req_wlan_status.wlan_data_rate);
  printk( KERN_DEBUG "LG_FW [KERNEL]: write_wlan_status(wlan_payload) = %d\n",lg_diag_req_wlan_status.wlan_payload);
  printk( KERN_DEBUG "LG_FW [KERNEL]: write_wlan_status(wlan_data_rate_recent) = %d\n",lg_diag_req_wlan_status.wlan_data_rate_recent);
  printk( KERN_DEBUG "LG_FW [KERNEL]: SIZEOF = %d\n", sizeof(wlan_status));
  
  return size;
}
#endif

#ifdef CONFIG_LGE_DIAG_WMC
static ssize_t read_wmc_cmd_pkt(struct device *dev, struct device_attribute *attr,
		char *buf)
{

  printk(KERN_INFO "%s, attr_name : %s, length : %d\n", __func__, attr->attr.name, lg_diag_wmc_req_pkt_length);
  
  memcpy(buf, lg_diag_wmc_req_pkt_ptr, lg_diag_wmc_req_pkt_length);
  
  return lg_diag_wmc_req_pkt_length;
}

static ssize_t write_wmc_cmd_pkt(struct device *dev,
						 struct device_attribute *attr,
						 const char *buf, size_t size)
{
  void* rsp_pkt_ptr;
#ifdef LG_DIAG_DEBUG
  int i;
#endif

	printk(KERN_ERR "\n LG_FW : print received packet :len(%d) \n",lg_diag_wmc_rsp_pkt_length);
	rsp_pkt_ptr = (void *)diagpkt_alloc(DIAG_WMCSYNC_MAPPING_F, lg_diag_wmc_rsp_pkt_length);
  memcpy(rsp_pkt_ptr, buf, lg_diag_wmc_rsp_pkt_length);

#ifdef LG_DIAG_DEBUG
	for (i=0;i<lg_diag_wmc_rsp_pkt_length;i++) {
			printk(KERN_ERR "0x%x ",*((unsigned char*)(rsp_pkt_ptr + i)));
	}
	printk(KERN_ERR "\n");
#endif
  diagpkt_commit(rsp_pkt_ptr);
	return size;
}

static ssize_t read_wmc_cmd_pkt_length(struct device *dev, struct device_attribute *attr,
	char *buf)
{
  int read_len = 2;

  printk(KERN_INFO "%s, attr_name : %s, length : %d\n", __func__, attr->attr.name, lg_diag_wmc_req_pkt_length);
  
  memcpy(buf, &lg_diag_wmc_req_pkt_length, read_len);
  return read_len;
}

static ssize_t write_wmc_cmd_pkt_length(struct device *dev,
					 struct device_attribute *attr,
					 const char *buf, size_t size)
{
  int write_len = 2;

  printk(KERN_INFO "%s, attr_name : %s\n", __func__, attr->attr.name);
 
  memcpy((void*)&lg_diag_wmc_rsp_pkt_length, buf, write_len);
  printk( KERN_DEBUG "LG_FW : write_cmd_pkt_length = %d\n",lg_diag_wmc_rsp_pkt_length);  
  return write_len;
}
#endif /*CONFIG_LGE_DIAG_WMC*/

/* LGE_MERGE_S [sunmyoung.lee@lge.com] 2010-07-15. SMS UTS Test */
#if 1
static ssize_t read_sms_status_new(struct device *dev, struct device_attribute *attr,
	char *buf)
{
#ifdef LG_FW_COMPILE_ERROR
  int udm_sms_statu_len = sizeof(udm_sms_status_new);
  
  memcpy(buf, &lg_diag_req_udm_sms_status_new, udm_sms_statu_len);
  return udm_sms_statu_len;
#else
	return 0;
#endif /* LG_FW_COMPILE_ERROR */
}

static ssize_t write_sms_status_new(struct device *dev,
					 struct device_attribute *attr,
					 const char *buf, size_t size)
{
#ifdef LG_FW_COMPILE_ERROR
  int udm_sms_statu_len = sizeof(udm_sms_status_new);

  memcpy((void*)&lg_diag_req_udm_sms_status_new, buf, udm_sms_statu_len);
//  printk( KERN_DEBUG "LG_FW : write_cmd_pkt_length = %d\n",lg_diag_rsp_pkt_length);  
  return udm_sms_statu_len;
#else
	return 0;
#endif /* LG_FW_COMPILE_ERROR */
}
#endif
/* LGE_MERGE_E [sunmyoung.lee@lge.com] 2010-07-15. SMS UTS Test */

/* LGE_CHANGES_E, [dongp.kim@lge.com], 2010-01-10, <LGE_FACTORY_TEST_MODE for WLAN RF Test > */
static DEVICE_ATTR(cmd_pkt, S_IRUGO | S_IWUSR,read_cmd_pkt, write_cmd_pkt);
static DEVICE_ATTR(length, S_IRUGO | S_IWUSR,read_cmd_pkt_length, write_cmd_pkt_length);
/* LGE_CHANGES_S, [dongp.kim@lge.com], 2010-01-10, <LGE_FACTORY_TEST_MODE for WLAN RF Test > */
static DEVICE_ATTR(wlan_status, S_IRUGO | S_IWUSR,read_wlan_status, write_wlan_status);
/* LGE_CHANGES_E, [dongp.kim@lge.com], 2010-01-10, <LGE_FACTORY_TEST_MODE for WLAN RF Test > */

#ifdef CONFIG_LGE_DIAG_WMC
static DEVICE_ATTR(wmc_cmd_pkt, S_IRUGO | S_IWUSR,read_wmc_cmd_pkt, write_wmc_cmd_pkt);
static DEVICE_ATTR(wmc_length, S_IRUGO | S_IWUSR,read_wmc_cmd_pkt_length, write_wmc_cmd_pkt_length);
#endif /*CONFIG_LGE_DIAG_WMC*/

/* LGE_MERGE_S [sunmyoung.lee@lge.com] 2010-07-15. SMS UTS Test */
#if 1
static DEVICE_ATTR(get_sms, S_IRUGO | S_IWUSR,read_sms_status_new, write_sms_status_new);
static DEVICE_ATTR(set_sms, S_IRUGO | S_IWUSR,read_sms_status_new, write_sms_status_new);
static DEVICE_ATTR(sms_status, S_IRUGO | S_IWUSR,read_sms_status_new, write_sms_status_new);
static DEVICE_ATTR(rsp_get_sms, S_IRUGO | S_IWUSR,read_sms_status_new, write_sms_status_new);
static DEVICE_ATTR(rsp_set_sms, S_IRUGO | S_IWUSR,read_sms_status_new, write_sms_status_new);
static DEVICE_ATTR(rsp_sms_status, S_IRUGO | S_IWUSR,read_sms_status_new, write_sms_status_new);
#endif
/* LGE_MERGE_E [sunmyoung.lee@lge.com] 2010-07-15. SMS UTS Test */

int lg_diag_create_file(struct platform_device *pdev)
{
  int ret;

	ret = device_create_file(&pdev->dev, &dev_attr_cmd_pkt);
	if (ret) {
		printk( KERN_DEBUG "LG_FW : diag device file create fail\n");
		device_remove_file(&pdev->dev, &dev_attr_cmd_pkt);
		return ret;
	}
	
	ret = device_create_file(&pdev->dev, &dev_attr_length);
	if (ret) {
		printk( KERN_DEBUG "LG_FW : diag device file2 create fail\n");
		device_remove_file(&pdev->dev, &dev_attr_length);
		return ret;
	}
/* LGE_CHANGES_S, [dongp.kim@lge.com], 2010-01-10, <LGE_FACTORY_TEST_MODE for WLAN RF Test > */
#if 1    //LGE_CHANG, [dongp.kim@lge.com], 2010-10-09, deleting #ifdef LG_FW_WLAN_TEST for enabling Wi-Fi Testmode menus
	ret = device_create_file(&pdev->dev, &dev_attr_wlan_status);
	if (ret) {
		printk( KERN_DEBUG "LG_FW : diag device file3 create fail\n");
		device_remove_file(&pdev->dev, &dev_attr_wlan_status);
		return ret;
	}
#endif
/* LGE_CHANGES_E, [dongp.kim@lge.com], 2010-01-10, <LGE_FACTORY_TEST_MODE for WLAN RF Test > */	
#ifdef CONFIG_LGE_DIAG_WMC
	ret = device_create_file(&pdev->dev, &dev_attr_wmc_cmd_pkt);
	if (ret) {
		printk( KERN_DEBUG "LG_FW : diag device file4 create fail\n");
		device_remove_file(&pdev->dev, &dev_attr_wmc_cmd_pkt);
		return ret;
	}

	ret = device_create_file(&pdev->dev, &dev_attr_wmc_length);
	if (ret) {
		printk( KERN_DEBUG "LG_FW : diag device file5 create fail\n");
		device_remove_file(&pdev->dev, &dev_attr_wmc_length);
		return ret;
	}
#endif /*CONFIG_LGE_DIAG_WMC*/

/* LGE_MERGE_S [sunmyoung.lee@lge.com] 2010-07-15. SMS UTS Test */
#if 1
        ret = device_create_file(&pdev->dev, &dev_attr_sms_status);
	  if (ret) {
		  printk( KERN_DEBUG "LG_FW : diag device file3 create fail\n");
		  device_remove_file(&pdev->dev, &dev_attr_sms_status);
		  return ret;
	  }
  
	  ret = device_create_file(&pdev->dev, &dev_attr_get_sms);
	  if (ret) {
		  printk( KERN_DEBUG "LG_FW : diag device file3 create fail\n");
		  device_remove_file(&pdev->dev, &dev_attr_get_sms);
		  return ret;
	  }
  
	  ret = device_create_file(&pdev->dev, &dev_attr_set_sms);
	  if (ret) {
		  printk( KERN_DEBUG "LG_FW : diag device file3 create fail\n");
		  device_remove_file(&pdev->dev, &dev_attr_set_sms);
		  return ret;
	  }
  
	  ret = device_create_file(&pdev->dev, &dev_attr_rsp_sms_status);
	  if (ret) {
		  printk( KERN_DEBUG "LG_FW : diag device file3 create fail\n");
		  device_remove_file(&pdev->dev, &dev_attr_rsp_sms_status);
		  return ret;
	  }
  
	  ret = device_create_file(&pdev->dev, &dev_attr_rsp_get_sms);
	  if (ret) {
		  printk( KERN_DEBUG "LG_FW : diag device file3 create fail\n");
		  device_remove_file(&pdev->dev, &dev_attr_rsp_get_sms);
		  return ret;
	  }
  
	  ret = device_create_file(&pdev->dev, &dev_attr_rsp_set_sms);
	  if (ret) {
		  printk( KERN_DEBUG "LG_FW : diag device file3 create fail\n");
		  device_remove_file(&pdev->dev, &dev_attr_rsp_set_sms);
		  return ret;
	  }
#endif
/* LGE_MERGE_E [sunmyoung.lee@lge.com] 2010-07-15. SMS UTS Test */

  return ret;
}
EXPORT_SYMBOL(lg_diag_create_file);

int lg_diag_remove_file(struct platform_device *pdev)
{
	device_remove_file(&pdev->dev, &dev_attr_cmd_pkt);

#if 1    //LGE_CHANG, [dongp.kim@lge.com], 2010-10-09, deleting #ifdef LG_FW_WLAN_TEST for enabling Wi-Fi Testmode menus
/* LGE_CHANGES_S [dongp.kim@lge.com], 2010-01-10, <LGE_FACTORY_TEST_MODE for WLAN RF Test > */
	device_remove_file(&pdev->dev, &dev_attr_wlan_status);
/* LGE_CHANGES_E, [dongp.kim@lge.com], 2010-01-10, <LGE_FACTORY_TEST_MODE for WLAN RF Test > */

/* LGE_MERGE_S [sunmyoung.lee@lge.com] 2010-07-15. SMS UTS Test */
	device_remove_file(&pdev->dev, &dev_attr_sms_status);
	device_remove_file(&pdev->dev, &dev_attr_get_sms);
	device_remove_file(&pdev->dev, &dev_attr_set_sms);
	device_remove_file(&pdev->dev, &dev_attr_rsp_sms_status);
	device_remove_file(&pdev->dev, &dev_attr_rsp_get_sms);
	device_remove_file(&pdev->dev, &dev_attr_rsp_set_sms);
/* LGE_MERGE_E [sunmyoung.lee@lge.com] 2010-07-15. SMS UTS Test */
#endif /* LG_FW_WLAN_TEST */
	device_remove_file(&pdev->dev, &dev_attr_length);

#ifdef CONFIG_LGE_DIAG_WMC
	device_remove_file(&pdev->dev, &dev_attr_wmc_cmd_pkt);
	device_remove_file(&pdev->dev, &dev_attr_wmc_length);
#endif /*CONFIG_LGE_DIAG_WMC*/
  return 0;
}
EXPORT_SYMBOL(lg_diag_remove_file);

static int lg_diag_app_execute(void)
{
	int ret;
	char cmdstr[100];
	int fd;
	char *envp[] = {
		"HOME=/",
		"TERM=linux",
		NULL,
	};

char *argv[] = {
	"sh",
	"-c",
	cmdstr,
	NULL,
};	

// BEGIN: eternalblue@lge.com.2009-10-23
// 0001794: [ARM9] ATS AT CMD added 
	if ( (fd = sys_open((const char __user *) "/system/bin/lg_diag_app", O_RDONLY ,0) ) < 0 )
	{
			printk("\n can not open /system/bin/lg_diag - execute /system/bin/lg_diag_app\n");
			sprintf(cmdstr, "/system/bin/lg_diag_app\n");
	}
	else
	{
		printk("\n execute /system/bin/lg_diag_app\n");
		sprintf(cmdstr, "/system/bin/lg_diag_app\n");
		sys_close(fd);
	}
// END: eternalblue@lge.com.2009-10-23

  printk(KERN_INFO "execute - %s", cmdstr);
	if ((ret = call_usermodehelper("/system/bin/sh", argv, envp, UMH_WAIT_PROC)) != 0) {
		printk(KERN_ERR "LG_DIAG failed to run \": %i\n",
		       ret);
	}
	else
		printk(KERN_INFO "LG_DIAG execute ok");
	return ret;
}
/* LGE_S jihye.ahn 2010-11-13  for capturing video preview */
int blt_mode_enable(void)
{
	int ret;
	char cmdstr[100];
	int fd;
	char *envp[] = {
		"HOME=/",
		"TERM=linux",
		NULL,
	};

char *argv[] = {
	  "sh",
        "-c",
        cmdstr,
        NULL,	
};	

// BEGIN: eternalblue@lge.com.2009-10-23
// 0001794: [ARM9] ATS AT CMD added 
	if ( (fd = sys_open((const char __user *) "/system/bin/enablebltmode", O_RDONLY ,0) ) < 0 )
	{
			//printk("\n can not open /system/bin/lg_diag - execute /system/bin/lg_diag_app\n");
			sprintf(cmdstr, "/system/bin/enablebltmode\n");
	}
	else
	{
		printk("\n execute /system/bin/enablebltmode\n");
		sprintf(cmdstr, "/system/bin/enablebltmode\n");
		sys_close(fd);
	}
// END: eternalblue@lge.com.2009-10-23

  printk(KERN_INFO "execute - %s", cmdstr);
	if ((ret = call_usermodehelper("/system/bin/sh", argv, envp, UMH_WAIT_PROC)) != 0) {
		printk(KERN_ERR "jihye.ahn          LG_DIAG failed to run \": %i\n",
		       ret);
	}
	else
		printk(KERN_INFO "jihye.ahn          LG_DIAG execute ok");
	return ret;
}
EXPORT_SYMBOL(blt_mode_enable);
int blt_mode_disable(void)
{
	int ret;
	char cmdstr[100];
	int fd;
	char *envp[] = {
		"HOME=/",
		"TERM=linux",
		NULL,
	};

char *argv[] = {
	  "sh",
        "-c",
        cmdstr,
        NULL,	
};	

// BEGIN: eternalblue@lge.com.2009-10-23
// 0001794: [ARM9] ATS AT CMD added 
	if ( (fd = sys_open((const char __user *) "/system/bin/disablebltmode", O_RDONLY ,0) ) < 0 )
	{
			//printk("\n can not open /system/bin/lg_diag - execute /system/bin/lg_diag_app\n");
			sprintf(cmdstr, "/system/bin/disablebltmode\n");
	}
	else
	{
		printk("\n execute /system/bin/disablebltmode\n");
		sprintf(cmdstr, "/system/bin/disablebltmode\n");
		sys_close(fd);
	}
// END: eternalblue@lge.com.2009-10-23

  printk(KERN_INFO "execute - %s", cmdstr);
	if ((ret = call_usermodehelper("/system/bin/sh", argv, envp, UMH_WAIT_PROC)) != 0) {
		printk(KERN_ERR "jihye.ahn          LG_DIAG failed to run \": %i\n",
		       ret);
	}
	else
		printk(KERN_INFO "jihye.ahn          LG_DIAG execute ok");
	return ret;
}
EXPORT_SYMBOL(blt_mode_disable);
/* LGE_E jihye.ahn 2010-11-13  for capturing video preview */

/*=========================================================================
FUNCTION DIAGCHAR_OPEN

DESCRIPTION
  This function is from diagchar_open in diagchar_core.c

===========================================================================*/
#ifdef CONFIG_LGE_DIAGTEST
static int diagchar_open(void)
#else
static int diagchar_open(struct inode *inode, struct file *file)
#endif
{
	int i = 0;

	if (driver) {
		mutex_lock(&driver->diagchar_mutex);

		for (i = 0; i < driver->num_clients; i++)
			if (driver->client_map[i].pid == 0)
				break;

		if (i < driver->num_clients) {
			driver->client_map[i].pid = current->tgid;
			strncpy(driver->client_map[i].name, current->comm, 20);
			driver->client_map[i].name[19] = '\0';
#ifdef LG_DIAG_DEBUG
			printk(KERN_DEBUG "LG_FW : client_map id = 0x%x\n", driver->client_map[i]);
#endif
		} else {
			if (i < threshold_client_limit) {
				driver->num_clients++;
				driver->client_map = krealloc(driver->client_map
					, (driver->num_clients) * sizeof(struct
						 diag_client_map), GFP_KERNEL);
				driver->client_map[i].pid = current->tgid;
				strncpy(driver->client_map[i].name,
					current->comm, 20);
				driver->client_map[i].name[19] = '\0';
			} else {
				mutex_unlock(&driver->diagchar_mutex);
				if (driver->alert_count == 0 ||
						 driver->alert_count == 10) {
					printk(KERN_ALERT "Max client limit for"
						 "DIAG driver reached\n");
					printk(KERN_INFO "Cannot open handle %s"
					   " %d", current->comm, current->tgid);
				for (i = 0; i < driver->num_clients; i++)
					printk(KERN_INFO "%d) %s PID=%d"
					, i, driver->client_map[i].name,
					 driver->client_map[i].pid);
					driver->alert_count = 0;
				}
				driver->alert_count++;
				return -ENOMEM;
			}
		}
		driver->data_ready[i] |= MSG_MASKS_TYPE;
		driver->data_ready[i] |= EVENT_MASKS_TYPE;
		driver->data_ready[i] |= LOG_MASKS_TYPE;

		if (driver->ref_count == 0)
			diagmem_init(driver);
		driver->ref_count++;
		mutex_unlock(&driver->diagchar_mutex);
		return 0;
	}
	return -ENOMEM;
}

/*=========================================================================
FUNCTION DIAGCHAR_CLOSE

DESCRIPTION
  This function is from diagchar_close in diagchar_core.c

===========================================================================*/
#ifdef CONFIG_LGE_DIAGTEST
static int diagchar_close(void)
#else
static int diagchar_close(struct inode *inode, struct file *file)
#endif
{
	int i = 0;
#ifdef CONFIG_DIAG_OVER_USB
	/* If the SD logging process exits, change logging to USB mode */
	if (driver->logging_process_id == current->tgid) {
		driver->logging_mode = USB_MODE;
		diagfwd_connect();
	}
#endif /* DIAG over USB */
	/* Delete the pkt response table entry for the exiting process */
	for (i = 0; i < diag_max_registration; i++)
			if (driver->table[i].process_id == current->tgid)
					driver->table[i].process_id = 0;

			if (driver) {
				mutex_lock(&driver->diagchar_mutex);
				driver->ref_count--;
				/* On Client exit, try to destroy all 3 pools */
				diagmem_exit(driver, POOL_TYPE_COPY);
				diagmem_exit(driver, POOL_TYPE_HDLC);
				diagmem_exit(driver, POOL_TYPE_WRITE_STRUCT);
				for (i = 0; i < driver->num_clients; i++)
					if (driver->client_map[i].pid ==
					     current->tgid) {
						driver->client_map[i].pid = 0;
						break;
					}
		mutex_unlock(&driver->diagchar_mutex);
		return 0;
	}
	return -ENOMEM;
}

/*=========================================================================
FUNCTION DIAGCHAR_IOCTL

DESCRIPTION
  This function is from diagchar_ioctl in diagchar_core.c

===========================================================================*/
#ifdef CONFIG_LGE_DIAGTEST
static int diagchar_ioctl(unsigned int iocmd, unsigned long ioarg)
#else
static int diagchar_ioctl(struct inode *inode, struct file *filp,
			   unsigned int iocmd, unsigned long ioarg)
#endif
{
	int i, j, count_entries = 0, temp;
	int success = -1;

	if (iocmd == DIAG_IOCTL_COMMAND_REG) {
		struct bindpkt_params_per_process *pkt_params =
			 (struct bindpkt_params_per_process *) ioarg;

		for (i = 0; i < diag_max_registration; i++) {
			if (driver->table[i].process_id == 0) {
				success = 1;
				driver->table[i].cmd_code =
					pkt_params->params->cmd_code;
				driver->table[i].subsys_id =
					pkt_params->params->subsys_id;
				driver->table[i].cmd_code_lo =
					pkt_params->params->cmd_code_lo;
				driver->table[i].cmd_code_hi =
					pkt_params->params->cmd_code_hi;
				driver->table[i].process_id = current->tgid;
				count_entries++;
				if (pkt_params->count > count_entries)
					pkt_params->params++;
				else
					return success;
			}
		}
		if (i < diag_threshold_registration) {
			/* Increase table size by amount required */
			diag_max_registration += pkt_params->count -
							 count_entries;
			/* Make sure size doesnt go beyond threshold */
			if (diag_max_registration > diag_threshold_registration)
				diag_max_registration =
						 diag_threshold_registration;
			driver->table = krealloc(driver->table,
					 diag_max_registration*sizeof(struct
					 diag_master_table), GFP_KERNEL);
			for (j = i; j < diag_max_registration; j++) {
				success = 1;
				driver->table[j].cmd_code = pkt_params->
							params->cmd_code;
				driver->table[j].subsys_id = pkt_params->
							params->subsys_id;
				driver->table[j].cmd_code_lo = pkt_params->
							params->cmd_code_lo;
				driver->table[j].cmd_code_hi = pkt_params->
							params->cmd_code_hi;
				driver->table[j].process_id = current->tgid;
				count_entries++;
				if (pkt_params->count > count_entries)
					pkt_params->params++;
				else
					return success;
			}
		} else
			pr_err("Max size reached, Pkt Registration failed for"
						" Process %d", current->tgid);

		success = 0;
	} else if (iocmd == DIAG_IOCTL_GET_DELAYED_RSP_ID) {
		struct diagpkt_delay_params *delay_params =
					(struct diagpkt_delay_params *) ioarg;

		if ((delay_params->rsp_ptr) &&
		 (delay_params->size == sizeof(delayed_rsp_id)) &&
				 (delay_params->num_bytes_ptr)) {
			*((uint16_t *)delay_params->rsp_ptr) =
				DIAGPKT_NEXT_DELAYED_RSP_ID(delayed_rsp_id);
			*(delay_params->num_bytes_ptr) = sizeof(delayed_rsp_id);
			success = 0;
		}
	} else if (iocmd == DIAG_IOCTL_LSM_DEINIT) {
		for (i = 0; i < driver->num_clients; i++)
			if (driver->client_map[i].pid == current->tgid)
				break;
		if (i == -1)
			return -EINVAL;
		driver->data_ready[i] |= DEINIT_TYPE;
		wake_up_interruptible(&driver->wait_q);
		success = 1;
	} else if (iocmd == DIAG_IOCTL_SWITCH_LOGGING) {
		mutex_lock(&driver->diagchar_mutex);
		temp = driver->logging_mode;
		driver->logging_mode = (int)ioarg;
		driver->logging_process_id = current->tgid;
		mutex_unlock(&driver->diagchar_mutex);
		if (temp == MEMORY_DEVICE_MODE && driver->logging_mode
							== NO_LOGGING_MODE) {
			driver->in_busy_1 = 1;
			driver->in_busy_2 = 1;
			driver->in_busy_qdsp_1 = 1;
			driver->in_busy_qdsp_2 = 1;
		} else if (temp == NO_LOGGING_MODE && driver->logging_mode
							== MEMORY_DEVICE_MODE) {
			driver->in_busy_1 = 0;
			driver->in_busy_2 = 0;
			driver->in_busy_qdsp_1 = 0;
			driver->in_busy_qdsp_2 = 0;
			/* Poll SMD channels to check for data*/
			if (driver->ch)
				queue_work(driver->diag_wq,
					&(driver->diag_read_smd_work));
			if (driver->chqdsp)
				queue_work(driver->diag_wq,
					&(driver->diag_read_smd_qdsp_work));
		}
#ifdef CONFIG_DIAG_OVER_USB
		else if (temp == USB_MODE && driver->logging_mode
							 == NO_LOGGING_MODE)
			diagfwd_disconnect();
		else if (temp == NO_LOGGING_MODE && driver->logging_mode
								== USB_MODE)
			diagfwd_connect();
		else if (temp == USB_MODE && driver->logging_mode
							== MEMORY_DEVICE_MODE) {
			diagfwd_disconnect();
			driver->in_busy_1 = 0;
			driver->in_busy_2 = 0;
			driver->in_busy_qdsp_2 = 0;
			driver->in_busy_qdsp_2 = 0;
			/* Poll SMD channels to check for data*/
			if (driver->ch)
				queue_work(driver->diag_wq,
					 &(driver->diag_read_smd_work));
			if (driver->chqdsp)
				queue_work(driver->diag_wq,
					&(driver->diag_read_smd_qdsp_work));
		} else if (temp == MEMORY_DEVICE_MODE && driver->logging_mode
								== USB_MODE)
			diagfwd_connect();
#endif /* DIAG over USB */
		success = 1;
	}

	return success;
}

/*=========================================================================
FUNCTION DIAGCHAR_READ

DESCRIPTION
  This function is from diagchar_read in diagchar_core.c

===========================================================================*/
#ifdef CONFIG_LGE_DIAGTEST
static int diagchar_read(char *buf, int count )
#else
static int diagchar_read(struct file *file, char __user *buf, size_t count,
			  loff_t *ppos)
#endif
{
	int index = -1, i = 0, ret = 0;
	int num_data = 0, data_type;
	for (i = 0; i < driver->num_clients; i++)
		if (driver->client_map[i].pid == current->tgid)
			index = i;

	if (index == -1) {
		printk(KERN_ALERT "\n Client PID not found in table");
		return -EINVAL;
	}

	wait_event_interruptible(driver->wait_q,
				  driver->data_ready[index]);
	mutex_lock(&driver->diagchar_mutex);

#ifdef LG_DIAG_DEBUG
    printk(KERN_DEBUG "LG_FW : diagchar_read	data_ready\n");
#endif

	if ((driver->data_ready[index] & MEMORY_DEVICE_LOG_TYPE) && (driver->
					logging_mode == MEMORY_DEVICE_MODE)) {
		/*Copy the type of data being passed*/
		data_type = driver->data_ready[index] & MEMORY_DEVICE_LOG_TYPE;
#ifdef CONFIG_LGE_DIAGTEST
		memcpy((void *)buf, (void *)&data_type, 4);
		ret += 4;
#else
		COPY_USER_SPACE_OR_EXIT(buf, data_type, 4);
#endif
		/* place holder for number of data field */
		ret += 4;

		for (i = 0; i < driver->poolsize_write_struct; i++) {
			if (driver->buf_tbl[i].length > 0) {
#ifdef DIAG_DEBUG
				printk(KERN_INFO "\n WRITING the buf address "
				       "and length is %x , %d\n", (unsigned int)
					(driver->buf_tbl[i].buf),
					driver->buf_tbl[i].length);
#endif
				num_data++;
				/* Copy the length of data being passed */
#ifdef CONFIG_LGE_DIAGTEST
				memcpy((void *)(buf+ret), (void *)&(driver->
						buf_tbl[i].length), 4);
#else
				if (copy_to_user(buf+ret, (void *)&(driver->
						buf_tbl[i].length), 4)) {
						num_data--;
						goto drop;
				}
#endif
				ret += 4;

				/* Copy the actual data being passed */
#ifdef CONFIG_LGE_DIAGTEST
				memcpy((void *)(buf+ret), (void *)driver->
				buf_tbl[i].buf, driver->buf_tbl[i].length);
#else
				if (copy_to_user(buf+ret, (void *)driver->
				buf_tbl[i].buf, driver->buf_tbl[i].length)) {
					ret -= 4;
					num_data--;
					goto drop;
				}
#endif
				ret += driver->buf_tbl[i].length;
drop:
#ifdef DIAG_DEBUG
				printk(KERN_INFO "\n DEQUEUE buf address and"
				       " length is %x,%d\n", (unsigned int)
				       (driver->buf_tbl[i].buf), driver->
				       buf_tbl[i].length);
#endif
				diagmem_free(driver, (unsigned char *)
				(driver->buf_tbl[i].buf), POOL_TYPE_HDLC);
				driver->buf_tbl[i].length = 0;
				driver->buf_tbl[i].buf = 0;
			}
		}

		/* copy modem data */
		if (driver->in_busy_1 == 1) {
			num_data++;
			/*Copy the length of data being passed*/
#ifdef CONFIG_LGE_DIAGTEST
			memcpy((void *)(buf+ret),
					 (void *)&(driver->write_ptr_1->length), 4);
			ret += 4;
#else
			COPY_USER_SPACE_OR_EXIT(buf+ret,
					 (driver->write_ptr_1->length), 4);
#endif
			/*Copy the actual data being passed*/
#ifdef CONFIG_LGE_DIAGTEST
			memcpy((void *)(buf+ret),
					(void *)&(*(driver->buf_in_1)),
					 driver->write_ptr_1->length);
			ret += driver->write_ptr_1->length;
#else
			COPY_USER_SPACE_OR_EXIT(buf+ret,
					*(driver->buf_in_1),
					 driver->write_ptr_1->length);
#endif
			driver->in_busy_1 = 0;
		}
		if (driver->in_busy_2 == 1) {
			num_data++;
			/*Copy the length of data being passed*/
#ifdef CONFIG_LGE_DIAGTEST
			memcpy((void *)(buf+ret),
					 (void *)&(driver->write_ptr_2->length), 4);
			ret += 4;
#else
			COPY_USER_SPACE_OR_EXIT(buf+ret,
					 (driver->write_ptr_2->length), 4);
#endif
			/*Copy the actual data being passed*/
#ifdef CONFIG_LGE_DIAGTEST
			memcpy((void *)(buf+ret),
					 (void *)&(*(driver->buf_in_2)),
					 driver->write_ptr_2->length);
			ret += driver->write_ptr_2->length;
#else
			COPY_USER_SPACE_OR_EXIT(buf+ret,
					 *(driver->buf_in_2),
					 driver->write_ptr_2->length);
#endif
			driver->in_busy_2 = 0;
		}

		/* copy q6 data */
		if (driver->in_busy_qdsp_1 == 1) {
			num_data++;
			/*Copy the length of data being passed*/
#ifdef CONFIG_LGE_DIAGTEST
			memcpy((void *)(buf+ret),
				 (void *)&(driver->write_ptr_qdsp_1->length), 4);
			ret += 4;
#else
			COPY_USER_SPACE_OR_EXIT(buf+ret,
				 (driver->write_ptr_qdsp_1->length), 4);
#endif
			/*Copy the actual data being passed*/
#ifdef CONFIG_LGE_DIAGTEST
			memcpy((void *)(buf+ret), (void *)&(*(driver->
							buf_in_qdsp_1)),
					 driver->write_ptr_qdsp_1->length);
			ret += driver->write_ptr_qdsp_1->length;
#else
			COPY_USER_SPACE_OR_EXIT(buf+ret, *(driver->
							buf_in_qdsp_1),
					 driver->write_ptr_qdsp_1->length);
#endif
			driver->in_busy_qdsp_1 = 0;
		}
		if (driver->in_busy_qdsp_2 == 1) {
			num_data++;
			/*Copy the length of data being passed*/
#ifdef CONFIG_LGE_DIAGTEST
			memcpy((void *)(buf+ret),
				 (void *)&(driver->write_ptr_qdsp_2->length), 4);
			ret += 4;
#else
			COPY_USER_SPACE_OR_EXIT(buf+ret,
				 (driver->write_ptr_qdsp_2->length), 4);
#endif
			/*Copy the actual data being passed*/
#ifdef CONFIG_LGE_DIAGTEST
			memcpy((void *)(buf+ret), (void *)&(*(driver->
				buf_in_qdsp_2)), driver->
					write_ptr_qdsp_2->length);
			ret += driver->write_ptr_qdsp_2->length;
#else
			COPY_USER_SPACE_OR_EXIT(buf+ret, *(driver->
				buf_in_qdsp_2), driver->
					write_ptr_qdsp_2->length);
#endif
			driver->in_busy_qdsp_2 = 0;
		}

		/* copy number of data fields */
#ifdef CONFIG_LGE_DIAGTEST
		memcpy((void *)(buf+4), (void *)&num_data, 4);
		ret += 4;
#else
		COPY_USER_SPACE_OR_EXIT(buf+4, num_data, 4);
#endif
		ret -= 4;
		driver->data_ready[index] ^= MEMORY_DEVICE_LOG_TYPE;
		if (driver->ch)
			queue_work(driver->diag_wq,
					 &(driver->diag_read_smd_work));
		if (driver->chqdsp)
			queue_work(driver->diag_wq,
					 &(driver->diag_read_smd_qdsp_work));
		APPEND_DEBUG('n');
		goto exit;
	} else if (driver->data_ready[index] & MEMORY_DEVICE_LOG_TYPE) {
		/* In case, the thread wakes up and the logging mode is
		not memory device any more, the condition needs to be cleared */
		driver->data_ready[index] ^= MEMORY_DEVICE_LOG_TYPE;
	}

	if (driver->data_ready[index] & DEINIT_TYPE) {
		/*Copy the type of data being passed*/
		data_type = driver->data_ready[index] & DEINIT_TYPE;
#ifdef CONFIG_LGE_DIAGTEST
		memcpy((void *)buf, (void *)&data_type, 4);
		ret += 4;
#else
		COPY_USER_SPACE_OR_EXIT(buf, data_type, 4);
#endif
		driver->data_ready[index] ^= DEINIT_TYPE;
		goto exit;
	}

	if (driver->data_ready[index] & MSG_MASKS_TYPE) {
		/*Copy the type of data being passed*/
		data_type = driver->data_ready[index] & MSG_MASKS_TYPE;
#ifdef CONFIG_LGE_DIAGTEST
		memcpy((void *)buf, (void *)&data_type, 4);
		ret += 4;
		memcpy((void *)(buf+4), (void *)&(*(driver->msg_masks)),
							 MSG_MASK_SIZE);
		ret += MSG_MASK_SIZE;
#else
		COPY_USER_SPACE_OR_EXIT(buf, data_type, 4);
		COPY_USER_SPACE_OR_EXIT(buf+4, *(driver->msg_masks),
							 MSG_MASK_SIZE);
#endif
		driver->data_ready[index] ^= MSG_MASKS_TYPE;
		goto exit;
	}

	if (driver->data_ready[index] & EVENT_MASKS_TYPE) {
		/*Copy the type of data being passed*/
		data_type = driver->data_ready[index] & EVENT_MASKS_TYPE;
#ifdef CONFIG_LGE_DIAGTEST
		memcpy((void *)buf, (void *)&data_type, 4);
		ret += 4;
		memcpy((void *)(buf+4), (void *)&(*(driver->event_masks)),
							 EVENT_MASK_SIZE);
		ret += EVENT_MASK_SIZE;
#else
		COPY_USER_SPACE_OR_EXIT(buf, data_type, 4);
		COPY_USER_SPACE_OR_EXIT(buf+4, *(driver->event_masks),
							 EVENT_MASK_SIZE);
#endif
		driver->data_ready[index] ^= EVENT_MASKS_TYPE;
		goto exit;
	}

	if (driver->data_ready[index] & LOG_MASKS_TYPE) {
		/*Copy the type of data being passed*/
		data_type = driver->data_ready[index] & LOG_MASKS_TYPE;
#ifdef CONFIG_LGE_DIAGTEST
		memcpy((void *)buf, (void *)&data_type, 4);
		ret += 4;
		memcpy((void *)(buf+4), (void *)&(*(driver->log_masks)),
							 LOG_MASK_SIZE);
		ret += LOG_MASK_SIZE;
#else
		COPY_USER_SPACE_OR_EXIT(buf, data_type, 4);
		COPY_USER_SPACE_OR_EXIT(buf+4, *(driver->log_masks),
							 LOG_MASK_SIZE);
#endif
		driver->data_ready[index] ^= LOG_MASKS_TYPE;
		goto exit;
	}

	if (driver->data_ready[index] & PKT_TYPE) {
		/*Copy the type of data being passed*/
		data_type = driver->data_ready[index] & PKT_TYPE;
#ifdef CONFIG_LGE_DIAGTEST
		memcpy((void *)buf, (void *)&data_type, 4);
		ret += 4;
		memcpy((void *)(buf+4), (void *)&(*(driver->pkt_buf)),
							 driver->pkt_length);
		ret += driver->pkt_length;
#else
		COPY_USER_SPACE_OR_EXIT(buf, data_type, 4);
		COPY_USER_SPACE_OR_EXIT(buf+4, *(driver->pkt_buf),
							 driver->pkt_length);
#endif
		driver->data_ready[index] ^= PKT_TYPE;
		goto exit;
	}

exit:
	mutex_unlock(&driver->diagchar_mutex);
	return ret;
}

/*=========================================================================
FUNCTION DIAGCHAR_WRITE

DESCRIPTION
  This function is from diagchar_write in diagchar_core.c

===========================================================================*/
#ifdef CONFIG_LGE_DIAGTEST
static int diagchar_write( const char *buf, size_t count)
#else
static int diagchar_write(struct file *file, const char __user *buf,
			      size_t count, loff_t *ppos)
#endif
{
	int err, ret = 0, pkt_type;
#if defined (DIAG_DEBUG) || defined (LG_DIAG_DEBUG)
	int length = 0, i;
#endif
	struct diag_send_desc_type send = { NULL, NULL, DIAG_STATE_START, 0 };
	struct diag_hdlc_dest_type enc = { NULL, NULL, 0 };
	void *buf_copy = NULL;
	int payload_size;
#ifdef CONFIG_DIAG_OVER_USB
	if (((driver->logging_mode == USB_MODE) && (!driver->usb_connected)) ||
				(driver->logging_mode == NO_LOGGING_MODE)) {
		/*Drop the diag payload */
#ifdef CONFIG_LGE_DIAG_COMPLEMENT
		printk(KERN_ERR "%s, [-EIO] logging_mode : %d, usb_connected : %d\n", __func__, driver->logging_mode, driver->usb_connected);
#endif
		return -EIO;
	}
#endif /* DIAG over USB */
	/* Get the packet type F3/log/event/Pkt response */
#ifdef CONFIG_LGE_DIAGTEST
	memcpy((&pkt_type), buf, 4);
#else
	err = copy_from_user((&pkt_type), buf, 4);
#endif
	/*First 4 bytes indicate the type of payload - ignore these */
	payload_size = count - 4;

	if (pkt_type == MEMORY_DEVICE_LOG_TYPE) {
		if (!mask_request_validate((unsigned char *)buf)) {
			printk(KERN_ALERT "mask request Invalid ..cannot send to modem \n");
			return -EFAULT;
		}
		buf = buf + 4;
#ifdef DIAG_DEBUG
		printk(KERN_INFO "\n I got the masks: %d\n", payload_size);
		for (i = 0; i < payload_size; i++)
			printk(KERN_DEBUG "\t %x", *(((unsigned char *)buf)+i));
#endif
		diag_process_hdlc((void *)buf, payload_size);
		return 0;
	}

	buf_copy = diagmem_alloc(driver, payload_size, POOL_TYPE_COPY);
	if (!buf_copy) {
		driver->dropped_count++;
		return -ENOMEM;
	}

#ifdef CONFIG_LGE_DIAGTEST
	memcpy(buf_copy, buf + 4, payload_size);
#else
	err = copy_from_user(buf_copy, buf + 4, payload_size);
	if (err) {
		printk(KERN_INFO "diagchar : copy_from_user failed\n");
		ret = -EFAULT;
		goto fail_free_copy;
	}
#endif
#if defined (DIAG_DEBUG) || defined (LG_DIAG_DEBUG)
	printk(KERN_DEBUG "data is -->\n");
	for (i = 0; i < payload_size; i++)
		printk(KERN_DEBUG "\t %x \t", *(((unsigned char *)buf_copy)+i));
#endif
	send.state = DIAG_STATE_START;
	send.pkt = buf_copy;
	send.last = (void *)(buf_copy + payload_size - 1);
	send.terminate = 1;
#if defined (DIAG_DEBUG) || defined (LG_DIAG_DEBUG)
	printk(KERN_INFO "\n Already used bytes in buffer %d, and"
	" incoming payload size is %d\n", driver->used, payload_size);
	printk(KERN_DEBUG "hdlc encoded data is -->\n");
	for (i = 0; i < payload_size + 8; i++) {
		printk(KERN_DEBUG "\t %x \t", *(((unsigned char *)buf_hdlc)+i));
		if (*(((unsigned char *)buf_hdlc)+i) != 0x7e)
			length++;
	}
#endif
	mutex_lock(&driver->diagchar_mutex);
	if (!buf_hdlc)
		buf_hdlc = diagmem_alloc(driver, HDLC_OUT_BUF_SIZE,
						 POOL_TYPE_HDLC);
	if (!buf_hdlc) {
#ifdef CONFIG_LGE_DIAG_COMPLEMENT
		printk(KERN_ERR "%s, buf_hdlc allocation failed\n", __func__);
#endif
		ret = -ENOMEM;
		goto fail_free_hdlc;
	}
	if (HDLC_OUT_BUF_SIZE - driver->used <= (2*payload_size) + 3) {
		err = diag_device_write(buf_hdlc, APPS_DATA, NULL);
		if (err) {
#ifdef CONFIG_LGE_DIAG_COMPLEMENT
			printk(KERN_ERR "%s, diag_device_write error : %d\n", __func__, err);
#endif
			/*Free the buffer right away if write failed */
			diagmem_free(driver, buf_hdlc, POOL_TYPE_HDLC);
			diagmem_free(driver, (unsigned char *)driver->
				 write_ptr_svc, POOL_TYPE_WRITE_STRUCT);
			ret = -EIO;
			goto fail_free_hdlc;
		}

		buf_hdlc = NULL;
#if defined (DIAG_DEBUG) || defined (LG_DIAG_DEBUG)
		printk(KERN_INFO "\n size written is %d\n", driver->used);
#endif
		driver->used = 0;
		buf_hdlc = diagmem_alloc(driver, HDLC_OUT_BUF_SIZE,
							 POOL_TYPE_HDLC);
		if (!buf_hdlc) {
#ifdef CONFIG_LGE_DIAG_COMPLEMENT
			printk(KERN_ERR "%s, buf_hdlc allocation failed\n", __func__);
#endif
			ret = -ENOMEM;
			goto fail_free_hdlc;
		}
	}

	enc.dest = buf_hdlc + driver->used;
// enc.dest_last has been changed from payload_size+7 to 2*payload_size+3, so keep up with it until any problem is detected.
// support screen capture, In that case, it has too many 'ESC_CHAR'
#if 0//def CONFIG_LGE_DIAGTEST
	enc.dest_last = (void *)(buf_hdlc + HDLC_OUT_BUF_SIZE -1);
#else
	enc.dest_last = (void *)(buf_hdlc + driver->used + 2*payload_size + 3);
#endif
	diag_hdlc_encode(&send, &enc);

#ifdef LG_DIAG_DEBUG
		printk(KERN_INFO "\n LG_FW : 2 Already used bytes in buffer %d, and"
		" incoming payload size is %d \n", driver->used, payload_size);
		printk(KERN_DEBUG "LG_FW : hdlc encoded data is --> \n");
		for (i = 0; i < payload_size + 8; i++) {
			printk(KERN_DEBUG "\t %x \t", *(((unsigned char *)buf_hdlc)+i));
			if (*(((unsigned char *)buf_hdlc)+i) != 0x7e)
				length++;
		}
#endif

	/* This is to check if after HDLC encoding, we are still within the
	 limits of aggregation buffer. If not, we write out the current buffer
	and start aggregation in a newly allocated buffer */
	if ((unsigned int) enc.dest >=
		 (unsigned int)(buf_hdlc + HDLC_OUT_BUF_SIZE)) {
		err = diag_device_write(buf_hdlc, APPS_DATA, NULL);
		if (err) {
#ifdef CONFIG_LGE_DIAG_COMPLEMENT
			printk(KERN_ERR "%s, usb_diag_write error : %d\n", __func__, err);
#endif
			/*Free the buffer right away if write failed */
			diagmem_free(driver, buf_hdlc, POOL_TYPE_HDLC);
			diagmem_free(driver, (unsigned char *)driver->
				 write_ptr_svc, POOL_TYPE_WRITE_STRUCT);
			ret = -EIO;
			goto fail_free_hdlc;
		}
		buf_hdlc = NULL;
#if defined (DIAG_DEBUG) || defined (LG_DIAG_DEBUG)
		printk(KERN_INFO "\n size written is %d\n", driver->used);
#endif
		driver->used = 0;
		buf_hdlc = diagmem_alloc(driver, HDLC_OUT_BUF_SIZE,
							 POOL_TYPE_HDLC);
		if (!buf_hdlc) {
#ifdef CONFIG_LGE_DIAG_COMPLEMENT
			printk(KERN_ERR "%s, buf_hdlc allocation failed\n", __func__);
#endif
			ret = -ENOMEM;
			goto fail_free_hdlc;
		}
		enc.dest = buf_hdlc + driver->used;
		enc.dest_last = (void *)(buf_hdlc + driver->used +
							 (2*payload_size) + 3);
		diag_hdlc_encode(&send, &enc);
#ifdef LG_DIAG_DEBUG
			printk(KERN_INFO "\n LG_FW : 3 Already used bytes in buffer %d, and"
			" incoming payload size is %d \n", driver->used, payload_size);
			printk(KERN_DEBUG "LG_FW : hdlc encoded data is --> \n");
			for (i = 0; i < payload_size + 8; i++) {
				printk(KERN_DEBUG "\t %x \t", *(((unsigned char *)buf_hdlc)+i));
				if (*(((unsigned char *)buf_hdlc)+i) != 0x7e)
					length++;
			}
#endif
	}

	driver->used = (uint32_t) enc.dest - (uint32_t) buf_hdlc;
	if (pkt_type == DATA_TYPE_RESPONSE) {
		err = diag_device_write(buf_hdlc, APPS_DATA, NULL);
		if (err) {
#ifdef CONFIG_LGE_DIAG_COMPLEMENT
			printk(KERN_ERR "%s, usb_diag_write error : %d\n", __func__, err);
#endif
			/*Free the buffer right away if write failed */
			diagmem_free(driver, buf_hdlc, POOL_TYPE_HDLC);
			diagmem_free(driver, (unsigned char *)driver->
				 write_ptr_svc, POOL_TYPE_WRITE_STRUCT);
			ret = -EIO;
			goto fail_free_hdlc;
		}
		buf_hdlc = NULL;
#if defined (DIAG_DEBUG) || defined (LG_DIAG_DEBUG)
		printk(KERN_INFO "\n size written is %d\n", driver->used);
#endif
		driver->used = 0;
	}

	mutex_unlock(&driver->diagchar_mutex);
	diagmem_free(driver, buf_copy, POOL_TYPE_COPY);
	if (!timer_in_progress)	{
		timer_in_progress = 1;
		ret = mod_timer(&drain_timer, jiffies + msecs_to_jiffies(500));
	}
	return 0;

fail_free_hdlc:
	buf_hdlc = NULL;
	driver->used = 0;
	diagmem_free(driver, buf_copy, POOL_TYPE_COPY);
	mutex_unlock(&driver->diagchar_mutex);
	return ret;

fail_free_copy:
	diagmem_free(driver, buf_copy, POOL_TYPE_COPY);
	return ret;
}

static void diagpkt_user_tbl_init (void)
{
	int i = 0;
	static boolean initialized = FALSE;

	if (!initialized)
	{
		 for (i = 0; (i < DIAGPKT_USER_TBL_SIZE); i++)
		 {
			lg_diagpkt_user_table[i] = NULL;
		 }
     initialized = TRUE;
	}
}

void 	diagpkt_tbl_reg (const diagpkt_user_table_type * tbl_ptr)
{
		int i = 0;
    //int mem_alloc_count = 0;
		word num_entries = tbl_ptr->count;
		bindpkt_params *bind_req = (bindpkt_params*)kmalloc(sizeof(bindpkt_params) * num_entries, GFP_KERNEL);
		bindpkt_params_per_process bind_req_send;

		if(NULL != bind_req)
		{
			/* Make sure this is initialized */
				 diagpkt_user_tbl_init ();

				 for (i = 0; i < DIAGPKT_USER_TBL_SIZE; i++)
				 {
					 if (lg_diagpkt_user_table[i] == NULL)
					 {
							 lg_diagpkt_user_table[i] = (diagpkt_user_table_type *)
																kmalloc(sizeof(diagpkt_user_table_type), GFP_KERNEL);
						if (NULL == lg_diagpkt_user_table[i])
						{
							printk(KERN_ERR "LG_FW : diagpkt_tbl_reg: malloc failed.");
							kfree (bind_req);
							return;
						}
						 memcpy(lg_diagpkt_user_table[i], tbl_ptr, sizeof(diagpkt_user_table_type));
						 break;
					 }
				 }
				 bind_req_send.count = num_entries;
			   //sprintk(bind_req_send.sync_obj_name, "%s%d", DIAG_LSM_PKT_EVENT_PREFIX, gdwClientID);

				for (i = 0; i < num_entries; i++)
				{
          bind_req[i].cmd_code = tbl_ptr->cmd_code;
          bind_req[i].subsys_id = tbl_ptr->subsysid;
          bind_req[i].cmd_code_lo = tbl_ptr->user_table[i].cmd_code_lo;
          bind_req[i].cmd_code_hi = tbl_ptr->user_table[i].cmd_code_hi;
          bind_req[i].proc_id = tbl_ptr->proc_id;
          bind_req[i].event_id = 0;
          bind_req[i].log_code = 0;
          //bind_req[i].client_id = gdwClientID;
		 #ifdef LG_DIAG_DEBUG
          printk(KERN_ERR "\n LG_FW : params are %d \t%d \t%d \t%d \t%d \t \n", bind_req[i].cmd_code, bind_req[i].subsys_id, 
		        bind_req[i].cmd_code_lo, bind_req[i].cmd_code_hi, bind_req[i].proc_id	);
		 #endif
				}
				bind_req_send.params = bind_req;
			if(diagchar_ioctl(DIAG_IOCTL_COMMAND_REG, (unsigned long)&bind_req_send))
			{
					printk(KERN_ERR "LG_FW :  diagpkt_tbl_reg: DeviceIOControl failed. \n");
			}
			kfree (bind_req);
		} /* if(NULL != bind_req) */	
}

/*===========================================================================

FUNCTION DIAGPKT_COMMIT

DESCRIPTION
  This function is from diagpkt_commit in diag_lsm_pkt.c
===========================================================================*/
void diagpkt_commit (PACK(void *)pkt)
{
  unsigned int length = 0;
  unsigned char *temp = NULL;
  int type = DIAG_DATA_TYPE_RESPONSE;
#ifdef LG_DIAG_DEBUG
  int i;
#endif

  if (pkt)
  {
    diagpkt_lsm_rsp_type *item = DIAGPKT_PKT2LSMITEM (pkt);
    item->rsp_func = NULL;
    item->rsp_func_param = NULL;
    /* end mobile-view */
#if defined (PKT_RESPONSE_DEBUG) || defined (LG_DIAG_DEBUG)
    printk(KERN_ERR "\n printing buffer at top \n");
    for(i=0;i<item->rsp.length;i++)
      printk(KERN_ERR "0x%x ", ((unsigned char*)(pkt))[i]);      
#endif

    length = DIAG_REST_OF_DATA_POS + FPOS(diagpkt_lsm_rsp_type, rsp.pkt) + item->rsp.length + sizeof(uint16);
#ifdef LG_DIAG_DEBUG
    printk(KERN_INFO "%s, DATA_POS : %d, FPOS : %d\n", __func__, DIAG_REST_OF_DATA_POS, FPOS(diagpkt_lsm_rsp_type, rsp.pkt));
    printk(KERN_INFO "%s, item->rsp.length : %d, sizeof(uint16) : %d, length : %d\n", __func__, item->rsp.length, sizeof(uint16), length);
#endif

    if (item->rsp.length > 0)
    {
#ifdef CONFIG_LGE_DIAGTEST
      temp =  (unsigned char*) kmalloc((int)DIAG_REST_OF_DATA_POS + (int)(item->rsp.length), GFP_KERNEL);
#else
      temp =  (unsigned char*) DiagSvc_Malloc((int)DIAG_REST_OF_DATA_POS + (int)(item->rsp.length), PKT_SVC_ID);
#endif

#ifdef CONFIG_LGE_DIAG_COMPLEMENT
      if(temp == NULL)
      {
        printk(KERN_ERR "%s, kmalloc failed\n",__func__);
        goto commit_error;
      }
#endif

      memcpy(temp, (unsigned char*)&type, DIAG_REST_OF_DATA_POS);
      memcpy(temp+4, pkt, item->rsp.length);
      
#if defined (PKT_RESPONSE_DEBUG) || defined (LG_DIAG_DEBUG)
      printk(KERN_ERR "\n LG_FW : printing buffer %d \n",(int)(item->rsp.length + DIAG_REST_OF_DATA_POS));
      
      for(i=0; i < (int)(item->rsp.length + DIAG_REST_OF_DATA_POS) ;i++)
      		printk(KERN_ERR "0x%x ", ((unsigned char*)(temp))[i]);      
        
      printk(KERN_ERR "\n");
#endif

#ifdef CONFIG_LGE_DIAGTEST
      if(diagchar_write((const void*) temp, item->rsp.length + DIAG_REST_OF_DATA_POS)) /*TODO: Check the Numberofbyteswritten against number of bytes we wanted to write?*/
#else
      if(write(fd, (const void*) temp, item->rsp.length + DIAG_REST_OF_DATA_POS)) /*TODO: Check the Numberofbyteswritten against number of bytes we wanted to write?*/
#endif
      {
        printk(KERN_ERR "\n Diag_LSM:Diag_LSM_Pkt: WriteFile Failed in diagpkt_commit \n");
#ifdef CONFIG_LGE_DIAG_COMPLEMENT
        goto commit_error;
#else
        gPkt_commit_fail++;
#endif
      }

#ifdef CONFIG_LGE_DIAGTEST
      kfree(temp);
      diagpkt_free(pkt);
      printk(KERN_INFO "%s completed\n", __func__);
#else
      DiagSvc_Free(temp, PKT_SVC_ID);
#endif
      return;
    }

  } /* end if (pkt)*/

#ifdef CONFIG_LGE_DIAG_COMPLEMENT
  commit_error:
    gPkt_commit_fail++;
    printk(KERN_ERR "%s, fail cnt : %d\n", __func__, gPkt_commit_fail);
    if(temp)
      kfree(temp);
    if(pkt)
      diagpkt_free(pkt);
#endif
    return;
}               /* diagpkt_commit */


/*===========================================================================

FUNCTION DIAGPKT_GET_CMD_CODE

DESCRIPTION
  This function is from diagpkt_get_cmd_code in diag_lsm_pkt.c

===========================================================================*/
diagpkt_cmd_code_type
diagpkt_get_cmd_code (PACK(void *)ptr)
{
	diagpkt_cmd_code_type cmd_code = 0;
	if(ptr)
	{
		/* Diag command codes are the first byte */
        return *((diagpkt_cmd_code_type *) ptr);
	}
	return cmd_code;
}               /* diag_get_cmd_code */


/*===========================================================================

FUNCTION DIAGPKT_SUBSYS_GET_ID

DESCRIPTION
  This function is from diagpkt_subsys_get_id in diag_lsm_pkt.c

===========================================================================*/
diagpkt_subsys_id_type
diagpkt_subsys_get_id (PACK(void *)ptr)
{
	diagpkt_subsys_id_type id = 0;
	if (ptr)
	{
		diagpkt_subsys_hdr_type *pkt_ptr = (void *) ptr;

        if ((pkt_ptr->command_code == DIAG_SUBSYS_CMD_F) || (pkt_ptr->command_code
                      == DIAG_SUBSYS_CMD_VER_2_F)) 
        {
		    id = (diagpkt_subsys_id_type) pkt_ptr->subsys_id;
        } 
        else 
        {
		    id = 0xFF;
        }
	}
    return id;
}               /* diagpkt_subsys_get_id */

/*===========================================================================

FUNCTION DIAGPKT_SUBSYS_GET_CMD_CODE

DESCRIPTION
  This function is from diagpkt_subsys_get_cmd_code in diag_lsm_pkt.c
===========================================================================*/
diagpkt_subsys_cmd_code_type
diagpkt_subsys_get_cmd_code (PACK(void *)ptr)
{
	diagpkt_subsys_cmd_code_type code = 0;
	if(ptr)
	{
		diagpkt_subsys_hdr_type *pkt_ptr = (void *) ptr;

        if ((pkt_ptr->command_code == DIAG_SUBSYS_CMD_F) || (pkt_ptr->command_code
            == DIAG_SUBSYS_CMD_VER_2_F)) 
        {
		    code = pkt_ptr->subsys_cmd_code;
        } 
        else 
        {
            code = 0xFFFF;
		}
	}
	return code;
}               /* diagpkt_subsys_get_cmd_code */

void diagpkt_process_request (void *req_pkt, uint16 pkt_len,
							 diag_cmd_rsp rsp_func, void *rsp_func_param)
{
	uint16 packet_id;     /* Command code for std or subsystem */
    uint8 subsys_id = DIAGPKT_NO_SUBSYS_ID;
    const diagpkt_user_table_type *user_tbl_entry = NULL;
    const diagpkt_user_table_entry_type *tbl_entry = NULL;
    int tbl_entry_count = 0;
    int i,j;
    void *rsp_pkt = NULL;
    boolean found = FALSE;
    uint16 cmd_code = 0xFF;
	#ifdef LG_DIAG_DEBUG
	printk(KERN_ERR "\n LG_FW : print received packet \n");
	for (i=0;i<pkt_len;i++) {
			printk(KERN_ERR "0x%x ",*((unsigned char*)(req_pkt + i)));
	}
	printk(KERN_ERR "\n");
	#endif
    packet_id = diagpkt_get_cmd_code (req_pkt);

    if ( packet_id == DIAG_SUBSYS_CMD_VER_2_F )
    {
		  cmd_code = packet_id;
    }
    
	if ((packet_id == DIAG_SUBSYS_CMD_F) || ( packet_id == DIAG_SUBSYS_CMD_VER_2_F ))
    {
		  subsys_id = diagpkt_subsys_get_id (req_pkt);
      packet_id = diagpkt_subsys_get_cmd_code (req_pkt);
    }

 /* Search the dispatch table for a matching subsystem ID.  If the
     subsystem ID matches, search that table for an entry for the given
     command code. */

  for (i = 0; !found && i < DIAGPKT_USER_TBL_SIZE; i++)
  {
    user_tbl_entry = lg_diagpkt_user_table[i];
    
    if (user_tbl_entry != NULL && user_tbl_entry->subsysid == subsys_id && user_tbl_entry->cmd_code == cmd_code)
    {
      tbl_entry = user_tbl_entry->user_table;
      
      tbl_entry_count = (tbl_entry) ? user_tbl_entry->count : 0;
      
      for (j = 0; (tbl_entry!=NULL) && !found && j < tbl_entry_count; j++)
      {
#ifdef LG_DIAG_DEBUG
        printk(KERN_DEBUG "packet_id : %d, lo : %d, hi : %d\n",packet_id, tbl_entry->cmd_code_lo, tbl_entry->cmd_code_hi);
#endif
        if (packet_id >= tbl_entry->cmd_code_lo && 
        packet_id <= tbl_entry->cmd_code_hi)
        {
          /* If the entry has no func, ignore it. */
          if (tbl_entry->func_ptr)
          {
            found = TRUE;
            rsp_pkt = (void *) (*tbl_entry->func_ptr) (req_pkt, pkt_len);
            if (rsp_pkt)
            {
#ifdef LG_DIAG_DEBUG
              printk(KERN_ERR " LG_FW : diagpkt_process_request: about to call diagpkt_commit.[%d]\n", g_diag_mtc_capture_rsp_num);
#endif
              /* The most common case: response is returned.  Go ahead and commit it here. */
#if 1 // def LG_FW_MTC //MTC
			diagpkt_commit (rsp_pkt);
            } /* endif if (rsp_pkt) */
            else
            {
if(g_diag_mtc_check == 0)
{
              switch(packet_id)
              {
/* BEGIN:0010147 [yk.kim@lge.com] 2010-10-22 */
/* MOD:0010147 [WMC] add feature vzw wmc (wireless mobile contol) */
#ifdef CONFIG_LGE_DIAG_WMC
                case DIAG_WMCSYNC_MAPPING_F:
                  break;
#endif
/* END:0010147 [yk.kim@lge.com] 2010-10-22 */
                default:
#ifdef LG_DIAG_DEBUG
              printk(KERN_ERR " LG_FW : diagpkt_process_request: about to execute lg_diag_app_execute.\n");
#endif                
                  lg_diag_req_pkt_ptr = req_pkt;
                  lg_diag_req_pkt_length = pkt_len;
                  lg_diag_app_execute();
                  break;
              }
}
#endif
            } /* endif if (rsp_pkt) */
          } /* endif if (tbl_entry->func_ptr) */
        } /* endif if (packet_id >= tbl_entry->cmd_code_lo && packet_id <= tbl_entry->cmd_code_hi)*/
        tbl_entry++;
      } /* for (j = 0; (tbl_entry!=NULL) && !found && j < tbl_entry_count; j++) */
    } /* endif if (user_tbl_entry != NULL && user_tbl_entry->subsysid == subsys_id
    && user_tbl_entry->cmd_code == cmd_code)*/
  } /*  for (i = 0; !found && i < DIAGPKT_USER_TBL_SIZE; i++) */

  /* Assume that rsp and rsp_pkt are NULL if !found */

  if (!found)
  {
//      ERR_FATAL("Diag_LSM: diagpkt_process_request: Did not find match in user table",0,0,0);
	       printk(KERN_ERR "LG_FW : diagpkt_process_request: Did not find match in user table \n");
  }
  
/* BEGIN: 0014110 jihoon.lee@lge.com 20110115 */
/* MOD 0014110: [FACTORY RESET] stability */
/* exception handling, merge from VS740 */
  //jihoon.lee - clear req_pkt so that current diag events do not affect the post, if it is not press and release events will be odd.
  //for example, send key followed by number of keys will be missed.
  //LG_FW khlee bug fix
  memset(req_pkt , 0 , 	pkt_len);
/* END: 0014110 jihoon.lee@lge.com 20110115 */
  return;
}               /* diagpkt_process_request */

static void process_diag_payload(void) 
{
	int type = *(int *)read_buffer;
	unsigned char* ptr = read_buffer+4;

  if(type == PKT_TYPE)
  	diagpkt_process_request((void*)ptr, (uint16)num_bytes_read-4, NULL, NULL);
}

static int CreateWaitThread(void* param)
{
		if(diagchar_open() != 0)
		{
#ifdef LG_DIAG_DEBUG
			printk(KERN_INFO "\n LG_FW :	size written is %d \n", driver->used);
#endif
      kthread_stop(lg_diag_thread);
			return 0; 	 
		}

		DIAGPKT_DISPATCH_TABLE_REGISTER(DIAGPKT_NO_SUBSYS_ID, registration_table);

		do{
			num_bytes_read = diagchar_read(read_buffer, READ_BUF_SIZE);
#ifdef LG_DIAG_DEBUG
			printk(KERN_DEBUG "LG_FW : CreateWaitThread, diagchar_read %d byte",num_bytes_read);
#endif
			if(*(int *)read_buffer == DEINIT_TYPE)
				break;
			process_diag_payload();
		}while(1);

		return 0;
}

void lgfw_diag_kernel_service_init(int driver_ptr)
{
  driver = (struct diagchar_dev*)driver_ptr;

	lg_diag_thread = kthread_run(CreateWaitThread, NULL, "kthread_lg_diag");
	if (IS_ERR(lg_diag_thread)) {
		lg_diag_thread = NULL;
		printk(KERN_ERR "LG_FW : %s: ts kthread run was failed!\n", __FUNCTION__);
		return;
	}
}
EXPORT_SYMBOL(lgfw_diag_kernel_service_init);

