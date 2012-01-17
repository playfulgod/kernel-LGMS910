/*****************************************************************************/
/* Copyright (c) 2009 NXP Semiconductors BV                                  */
/*                                                                           */
/* This program is free software; you can redistribute it and/or modify      */
/* it under the terms of the GNU General Public License as published by      */
/* the Free Software Foundation, using version 2 of the License.             */
/*                                                                           */
/* This program is distributed in the hope that it will be useful,           */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of            */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the              */
/* GNU General Public License for more details.                              */
/*                                                                           */
/* You should have received a copy of the GNU General Public License         */
/* along with this program; if not, write to the Free Software               */
/* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307       */
/* USA.                                                                      */
/*                                                                           */
/*****************************************************************************/

#define _tx_c_

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/cdev.h>  
#include <linux/fs.h>
#include <linux/ioctl.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>
#include <asm/uaccess.h>
#include <mach/gpio.h>
/* 	jaeseong.gim 	10.08.24
 * 	we do not use following header file
 */
#ifndef CONFIG_MACH_LGE_BRYCE
#include <mach/display.h>
#endif

/* HDMI DevLib */
#include "tmNxCompId.h"
#include "tmdlHdmiTx_Types.h"
#include "tmdlHdmiTx_Functions.h"

/* local */
#include "tda998x_version.h"
#include "tda998x.h"
#include "tda998x_ioctl.h"

/* 	jaeseong.gim 	10.08.24
 * 	we need following header files
 */
#ifdef CONFIG_MACH_LGE_BRYCE
#include "../msm/msm_fb.h"
#include "../msm/external_common.h"
#include <linux/display.h>
#include <linux/types.h>
#include <linux/bitops.h>
#include <linux/completion.h>
#endif


#ifdef ANDROID_DSS
/* DSS hack */
#include <../omap2/dss/dss.h>
#endif

/*
 *
 * DEFINITION
 * ----------
 * LEVEL 0
 *
 */

/*
 *  Global
 */

tda_instance our_instance;
static struct cdev our_cdev, *this_cdev=&our_cdev;
#ifdef ANDROID_DSS
static struct omap_video_timings video_640x480at60Hz_panel_timings = {
   .x_res          = 640,
   .y_res          = 480,
   .pixel_clock    = 25175,
   .hfp            = 16,
   .hsw            = 96,
   .hbp            = 48,
   .vfp            = 10,
   .vsw            = 2,
   .vbp            = 33,
};
static struct omap_video_timings video_640x480at72Hz_panel_timings = {
   .x_res          = 640,
   .y_res          = 480,
   .pixel_clock    = 31500,
   .hfp            = 24,
   .hsw            = 40,
   .hbp            = 128,
   .vfp            = 9,
   .vsw            = 3,
   .vbp            = 28,
};
static struct omap_video_timings video_720x480at60Hz_panel_timings = {
   .x_res          = 720,
   .y_res          = 480,
   .pixel_clock    = 27027,
   .hfp            = 16,
   .hbp            = 60,
   .hsw            = 62,
   .vfp            = 9,
   .vbp            = 30,
   .vsw            = 6,
};
static struct omap_video_timings video_1280x720at50Hz_panel_timings = {
   .x_res          = 1280,
   .y_res          = 720,
   .pixel_clock    = 74250,
#ifdef ZOOMII_PATCH
   .hfp            = 400,
   .hbp            = 260,
#else
   .hfp            = 440,
   .hbp            = 220,
#endif
   .hsw            = 40,
   .vfp            = 5,
   .vbp            = 20,
   .vsw            = 5,
};
static struct omap_video_timings video_1280x720at60Hz_panel_timings = {
   .x_res          = 1280,
   .y_res          = 720,
   .pixel_clock    = 74250,
#ifdef ZOOMII_PATCH
   .hfp            = 70,
   .hbp            = 260,
#else
   .hfp            = 110,
   .hbp            = 220,
#endif
   .hsw            = 40,
   .vfp            = 5,
   .vbp            = 20,
   .vsw            = 5,
};
static struct omap_video_timings video_1920x1080at50Hz_panel_timings = {
   .x_res          = 1920,
   .y_res          = 1080,
   .pixel_clock    = 148500, /* 2640 x 1125 x 50 /2 */
#ifdef ZOOMII_PATCH
   .hfp            = 488,
   .hbp            = 188,
#else
   .hfp            = 528,
   .hbp            = 148,
#endif
   .hsw            = 44,
   .vfp            = 4,
   .vbp            = 36,
   .vsw            = 5,
};
static struct omap_video_timings video_800x480at60Hz_panel_timings = {
   /*    .x_res = 800 /\* 1280 *\/, */
   /*    .y_res = 480 /\* 720 *\/, */
   /*    .pixel_clock = 21800 /\* 21800 23800 25700 *\/, */
   .x_res = 800,
   .y_res = 480,
   .pixel_clock = 21800,
   .hfp = 6,
   .hsw = 1,
   .hbp = 4,
   .vfp = 3,
   .vsw = 1,
   .vbp = 4,
};

struct omap_dss_device *sysdss;
#endif

/* 	jaeseong.gim 	10.08.24
 */
#ifdef CONFIG_MACH_LGE_BRYCE
static struct external_common_state_type hdmi_common;
static int hdmi_tx_init(tda_instance *this);
extern tmErrorCode_t dlHdmiTxSetTestPatternOn
(
	tmInstance_t            instance,
	tmdlHdmiTxVidFmt_t       voutFmt,
	tmdlHdmiTxVoutMode_t    voutMode,
	tmdlHdmiTxTestPattern_t pattern
);

void show_video(tda_instance *this);
/* Set pattern OFF */
extern tmErrorCode_t dlHdmiTxSetTestPatternOff
(
	tmInstance_t            instance,
	tmdlHdmiTxVidFmt_t      voutFmt,
	tmdlHdmiTxVoutMode_t    voutMode
);
int hdmi_enable(void);
int hdmi_disable(int);
static bool chip_power_on = FALSE;	/* For chip power on/off */
static bool gpio_power_on = TRUE;	/* For dtv power on/off (I2C) */

static u8 reg[256];	/* HDMI panel registers */

struct hdmi_data {
	struct msm_hdmi_platform_data *pd;
	struct work_struct work;
};
static struct hdmi_data *dd;
static struct work_struct handle_work;

#ifdef HDCP_ENABLE
static struct work_struct hdcp_handle_work;
#endif

static struct timer_list hpd_timer;
static unsigned int monitor_sense;

#include"tmbslHdmiTx_types.h"
extern tmErrorCode_t
tmbslTDA9989HotPlugGetStatus
(
    tmUnitSelect_t        txUnit,
    tmbslHdmiTxHotPlug_t *pHotPlugStatus,
    Bool                  client /* Used to determine whether the request comes from the application */
);

/* Change HDMI state */
static void change_hdmi_state(int online)
{
	if (!external_common_state)
		return;

	//printk("[jaeseong.gim] %s: %d->%d\n",__func__,external_common_state->hpd_state,online);

	mutex_lock(&external_common_state_hpd_mutex);
	external_common_state->hpd_state = online;
	mutex_unlock(&external_common_state_hpd_mutex);

	if (!external_common_state->uevent_kobj)
		return;

	if (online)
		kobject_uevent(external_common_state->uevent_kobj,
			KOBJ_ONLINE);
	else
		kobject_uevent(external_common_state->uevent_kobj,
			KOBJ_OFFLINE);
	DEV_DBG("hdmi_uevent: %d\n", online);
}

static int change_hdmi_tx_mode(int mode){


	//printk("[jaeseong.gim] %s\n",__func__);
   tda_instance *this=&our_instance;
	if (this->tda.power == tmPowerOn) {
		printk("[jaeseong.gim] %d\n",mode);
		this->tda.setio.video_in.format=mode;
		this->tda.setio.video_out.format=mode;

		tmdlHdmiTxSetInputOutput(this->tda.instance,
							  this->tda.setio.video_in, 
							  this->tda.setio.video_out,
							  this->tda.setio.audio_in, 
							  this->tda.setio.sink);
	}

	return 0;
}
int change_hdmi_power(int state)
{
   tda_instance *this=&our_instance;
	if (!external_common_state) return;

	if(state==0){
		printk("[jaeseong.gim] tx_power: OFF\n");
		if(!gpio_power_on) return 0;

		down(&this->driver.sem);
		this->tda.power=tmPowerStandby;
		tmdlHdmiTxSetPowerState(this->tda.instance,this->tda.power);

		chip_power_on = FALSE;

		printk("[jaeseong.gim] close\n");
		tmdlHdmiTxClose(this->tda.instance);
		up(&this->driver.sem);

		msleep(100);
		gpio_set_value(109, 0);
		gpio_power_on = FALSE;
		change_hdmi_state(0);
	}
	else{

		if(!gpio_power_on){
			gpio_set_value(109, 1);
			msleep(10);
			gpio_power_on=TRUE;
			hdmi_tx_init(this);
		}

		hdmi_enable();
		chip_power_on = TRUE;
		printk("[jaeseong.gim] tx_power: ON\n");

		if(this->tda.hot_plug_detect){
			hdmi_enable();
			change_hdmi_state(1);
		}
		else{
			hdmi_disable(1);
			chip_power_on = FALSE;
		}
	}
	return 0;
}
EXPORT_SYMBOL(change_hdmi_power);
	
	
static int hdmi_power_on(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd = platform_get_drvdata(pdev);

	external_common_state->dev = &pdev->dev;
	if (mfd != NULL) {
		if (!external_common_state->uevent_kobj) {
			int ret = external_common_state_create(pdev);
			if (ret)
				return ret;
		}

		printk("[jaeseong.gim] hdmi_power: ON (%dx%d %d)\n",
			mfd->var_xres, mfd->var_yres, mfd->var_pixclock);
		hdmi_common_get_video_format_from_drv_data(mfd);
		if(!chip_power_on){
			hdmi_enable();
			chip_power_on=TRUE;
		}
		change_hdmi_tx_mode(external_common_state->video_resolution+1);
	}

	return 0;
}

static int hdmi_power_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd = platform_get_drvdata(pdev);


   tda_instance *this=&our_instance;
	external_common_state->dev = &pdev->dev;
	if (mfd != NULL) {
		if (!external_common_state->uevent_kobj) {
			int ret = external_common_state_create(pdev);
			if (ret)
				return ret;
		}

		printk("[jaeseong.gim] hdmi_power: %s (%dx%d %d)\n",
				external_common_state->power_state==0?"OFF":"SUSPEND",
			mfd->var_xres, mfd->var_yres, mfd->var_pixclock);

		if(external_common_state->power_state!=0){
			hdmi_disable(1);
			chip_power_on = FALSE;
		}
	}
	return 0;
}

static struct msm_fb_panel_data hdmi_panel_data = {
	.on  = hdmi_power_on,
	.off = hdmi_power_off,
};


struct platform_device hdmi_device={
	.id=2,
	.dev={
		.platform_data=&hdmi_panel_data,
	},
};
#endif




/* #define HDCP_TEST 1 */
#ifdef HDCP_TEST
/* TEST */
int test = 0;
#endif

/*
 *  Module params
 */

/* 	jaeseong.gim 	10.08.24
 */
#ifdef CONFIG_MACH_LGE_BRYCE
static int param_verbose=1,param_major=0,param_minor=0;
#else
static int param_verbose=0,param_major=0,param_minor=0;
#endif
module_param_named(verbose,param_verbose,int,S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(verbose, "Make the driver verbose");
module_param_named(major, param_major, int, S_IRUGO);
MODULE_PARM_DESC(major, "The major number of the device mapper");

/*
 *
 * TOOLBOX
 * -------
 * LEVEL 1
 *
 * - i2c read/write
 * - chip Id check
 * - i2c client info
 * 
 */


/* 
 *  no mknod nor busybox, so patch it
 */
/* static int create_dev(char *name, dev_t dev) */
/* { */
/*    sys_unlink(name); */
/*    return sys_mknod((const char __user *) name, S_IFCHR|0666 /\* S_IFCHR | S_IRUSR | S_IWUSR *\/, new_encode_dev(dev)); */
/* } */

/* 
 *  Get main and unique I2C Client driver handle
 */
struct i2c_client *GetThisI2cClient(void)
{
   tda_instance *this=&our_instance;
   return this->driver.i2c_client;
}

/*
 * error handling
 */
char *hdmi_tx_err_string(int err)
{
   switch (err & 0x0FFF)
      {
      case TM_ERR_COMPATIBILITY: {return "SW Interface compatibility";break;}
      case TM_ERR_MAJOR_VERSION: {return "SW Major Version error";break;}
      case TM_ERR_COMP_VERSION: {return "SW component version error";break;}
      case TM_ERR_BAD_UNIT_NUMBER: {return "Invalid device unit number";break;}
      case TM_ERR_BAD_INSTANCE: {return "Bad input instance value  ";break;}
      case TM_ERR_BAD_HANDLE: {return "Bad input handle";break;}
      case TM_ERR_BAD_PARAMETER: {return "Invalid input parameter";break;}
      case TM_ERR_NO_RESOURCES: {return "Resource is not available ";break;}
      case TM_ERR_RESOURCE_OWNED: {return "Resource is already in use";break;}
      case TM_ERR_RESOURCE_NOT_OWNED: {return "Caller does not own resource";break;}
      case TM_ERR_INCONSISTENT_PARAMS: {return "Inconsistent input params";break;}
      case TM_ERR_NOT_INITIALIZED: {return "Component is not initialised";break;}
      case TM_ERR_NOT_SUPPORTED: {return "Function is not supported";break;}
      case TM_ERR_INIT_FAILED: {return "Initialization failed";break;}
      case TM_ERR_BUSY: {return "Component is busy";break;}
      case TMDL_ERR_DLHDMITX_I2C_READ: {return "Read error";break;}
      case TMDL_ERR_DLHDMITX_I2C_WRITE: {return "Write error";break;}
      case TM_ERR_FULL: {return "Queue is full";break;}
      case TM_ERR_NOT_STARTED: {return "Function is not started";break;}
      case TM_ERR_ALREADY_STARTED: {return "Function is already starte";break;}
      case TM_ERR_ASSERTION: {return "Assertion failure";break;}
      case TM_ERR_INVALID_STATE: {return "Invalid state for function";break;}
      case TM_ERR_OPERATION_NOT_PERMITTED: {return "Corresponds to posix EPERM";break;}
      case TMDL_ERR_DLHDMITX_RESOLUTION_UNKNOWN: {return "Bad format";break;}
      case TM_OK: {return "OK";break;}
      default : {printk(KERN_INFO "(err:%x) ",err);return "unknown";break;}
      }
}

static char *tda_spy_event(int event)
{
   switch (event)
      {
      case TMDL_HDMITX_HDCP_ACTIVE: {return "HDCP active";break;}
      case TMDL_HDMITX_HDCP_INACTIVE: {return "HDCP inactive";break;}
      case TMDL_HDMITX_HPD_ACTIVE: {return "HPD active";break;}
      case TMDL_HDMITX_HPD_INACTIVE: {return "HPD inactive";break;}
      case TMDL_HDMITX_RX_KEYS_RECEIVED: {return "Rx keys received";break;}
      case TMDL_HDMITX_RX_DEVICE_ACTIVE: {return "Rx device active";break;}
      case TMDL_HDMITX_RX_DEVICE_INACTIVE: {return "Rx device inactive";break;}
      case TMDL_HDMITX_EDID_RECEIVED: {return "EDID received";break;}
      case TMDL_HDMITX_VS_RPT_RECEIVED: {return "VS interrupt has been received";break;}
/*       case TMDL_HDMITX_B_STATUS: {return "TX received BStatus";break;} */ 
#if defined (TMFL_TDA19989) || defined (TMFL_TDA9984) 
      case TMDL_HDMITX_DEBUG_EVENT_1: {return "DEBUG_EVENT_1";break;}
#endif
      default : {return "Unkonwn event";break;}
      }
}
#if defined (TMFL_TDA19989) || defined (TMFL_TDA9984) 
static char *tda_spy_hsdc_fail_status(int fail)
{
   switch (fail)
      {
      case TMDL_HDMITX_HDCP_OK: {return "ok";break;}
      case  TMDL_HDMITX_HDCP_BKSV_RCV_FAIL: {return "Source does not receive Sink BKsv ";break;}
      case TMDL_HDMITX_HDCP_BKSV_CHECK_FAIL: {return "BKsv does not contain 20 zeros and 20 ones";break;}
      case TMDL_HDMITX_HDCP_BCAPS_RCV_FAIL: {return "Source does not receive Sink Bcaps";break;}
      case TMDL_HDMITX_HDCP_AKSV_SEND_FAIL: {return "Source does not send AKsv";break;}
      case TMDL_HDMITX_HDCP_R0_RCV_FAIL: {return "Source does not receive R'0";break;}
      case TMDL_HDMITX_HDCP_R0_CHECK_FAIL: {return "R0 = R'0 check fail";break;}
      case TMDL_HDMITX_HDCP_BKSV_NOT_SECURE: {return "bksv not secure";break;}
      case TMDL_HDMITX_HDCP_RI_RCV_FAIL: {return "Source does not receive R'i";break;}
      case TMDL_HDMITX_HDCP_RPT_RI_RCV_FAIL: {return "Source does not receive R'i repeater mode";break;}
      case TMDL_HDMITX_HDCP_RI_CHECK_FAIL: {return "RI = R'I check fail";break;}
      case TMDL_HDMITX_HDCP_RPT_RI_CHECK_FAIL: {return "RI = R'I check fail repeater mode";break;}
      case TMDL_HDMITX_HDCP_RPT_BCAPS_RCV_FAIL: {return "Source does not receive Sink Bcaps repeater mode";break;}
      case TMDL_HDMITX_HDCP_RPT_BCAPS_READY_TIMEOUT: {return "bcaps ready timeout";break;}
      case TMDL_HDMITX_HDCP_RPT_V_RCV_FAIL: {return "Source does not receive V";break;}
      case TMDL_HDMITX_HDCP_RPT_BSTATUS_RCV_FAIL: {return "Source does not receive BSTATUS repeater mode";break;}
      case TMDL_HDMITX_HDCP_RPT_KSVLIST_RCV_FAIL: {return "Source does not receive Ksv list in repeater mode";break;}
      case TMDL_HDMITX_HDCP_RPT_KSVLIST_NOT_SECURE: {return "ksvlist not secure";break;}
      default: {return "";break;}
      }
}

static char *tda_spy_hdcp_status(int status)
{
   switch (status)
      {
      case TMDL_HDMITX_HDCP_CHECK_NOT_STARTED: {return "Check not started";break;}
      case TMDL_HDMITX_HDCP_CHECK_IN_PROGRESS: {return "No failures, more to do";break;}
      case TMDL_HDMITX_HDCP_CHECK_PASS: {return "Final check has passed";break;}
      case TMDL_HDMITX_HDCP_CHECK_FAIL_FIRST: {return "First check failure code\nDriver not AUTHENTICATED";break;}
      case TMDL_HDMITX_HDCP_CHECK_FAIL_DEVICE_T0: {return "A T0 interrupt occurred";break;}
      case TMDL_HDMITX_HDCP_CHECK_FAIL_DEVICE_RI: {return "Device RI changed";break;}
      case TMDL_HDMITX_HDCP_CHECK_FAIL_DEVICE_FSM: {return "Device FSM not 10h";break;}
      default : {return "Unknown hdcp status";break;}
      }

}
#endif

static char *tda_spy_sink(int sink)
{
   switch (sink)
      {
      case TMDL_HDMITX_SINK_DVI: {return "DVI";break;}
      case TMDL_HDMITX_SINK_HDMI: {return "HDMI";break;}
      case TMDL_HDMITX_SINK_EDID: {return "As currently defined in EDID";break;}
      default : {return "Unkonwn sink";break;}
      }
}

#if defined (TMFL_TDA19989) || defined (TMFL_TDA9984) 
static char *tda_spy_aspect_ratio(int ar)
{
   switch (ar)
      {
      case TMDL_HDMITX_P_ASPECT_RATIO_UNDEFINED: {return "Undefined picture aspect rati";break;}
      case TMDL_HDMITX_P_ASPECT_RATIO_6_5: {return "6:5 picture aspect ratio (PAR";break;}
      case TMDL_HDMITX_P_ASPECT_RATIO_5_4: {return "5:4 PA";break;}
      case TMDL_HDMITX_P_ASPECT_RATIO_4_3: {return "4:3 PA";break;}
      case TMDL_HDMITX_P_ASPECT_RATIO_16_10: {return "16:10 PA";break;}
      case TMDL_HDMITX_P_ASPECT_RATIO_5_3: {return "5:3 PA";break;}
      case TMDL_HDMITX_P_ASPECT_RATIO_16_9: {return "16:9 PA";break;}
      case TMDL_HDMITX_P_ASPECT_RATIO_9_5: {return "9:5 PA";break;}
      default : {return "Unknown aspect ratio";break;}
      }
}

#if 0 /* no more used */
static char *tda_spy_edid_status(int status)
{
   switch (status)
      {
      case TMDL_HDMITX_EDID_READ: {return "All blocks read";break;}
      case TMDL_HDMITX_EDID_READ_INCOMPLETE: {return "All blocks read OK but buffer too small to return all of the";break;}
      case TMDL_HDMITX_EDID_ERROR_CHK_BLOCK_0: {return "Block 0 checksum erro";break;}
      case TMDL_HDMITX_EDID_ERROR_CHK: {return "Block 0 OK, checksum error in one or more other block";break;}
      case TMDL_HDMITX_EDID_NOT_READ: {return "EDID not read";break;}
      case TMDL_HDMITX_EDID_STATUS_INVALID: {return "Invalid ";break;}
      default : {return "Unknown edid status";break;}
      }
}
#endif

static char *tda_spy_vfmt(int fmt)
{
   switch (fmt)
      {
      case TMDL_ERR_DLHDMITX_COMPATIBILITY: {return "SW Interface compatibility";break;}
      case TMDL_ERR_DLHDMITX_MAJOR_VERSION: {return "SW Major Version error";break;}

      case TMDL_HDMITX_VFMT_NULL: {return "Not a valid format...";break;}
      case TMDL_HDMITX_VFMT_01_640x480p_60Hz: {return "Format 01 640  x 480p  60Hz";break;}
      case TMDL_HDMITX_VFMT_02_720x480p_60Hz: {return "Format 02 720  x 480p  60Hz";break;}
      case TMDL_HDMITX_VFMT_03_720x480p_60Hz: {return "Format 03 720  x 480p  60Hz";break;}
      case TMDL_HDMITX_VFMT_04_1280x720p_60Hz: {return "Format 04 1280 x 720p  60Hz";break;}
      case TMDL_HDMITX_VFMT_05_1920x1080i_60Hz: {return "Format 05 1920 x 1080i 60Hz";break;}
      case TMDL_HDMITX_VFMT_06_720x480i_60Hz: {return "Format 06 720  x 480i  60Hz";break;}
      case TMDL_HDMITX_VFMT_07_720x480i_60Hz: {return "Format 07 720  x 480i  60Hz";break;}
      case TMDL_HDMITX_VFMT_08_720x240p_60Hz: {return "Format 08 720  x 240p  60Hz";break;}
      case TMDL_HDMITX_VFMT_09_720x240p_60Hz: {return "Format 09 720  x 240p  60Hz";break;}
      case TMDL_HDMITX_VFMT_10_720x480i_60Hz: {return "Format 10 720  x 480i  60Hz";break;}
      case TMDL_HDMITX_VFMT_11_720x480i_60Hz: {return "Format 11 720  x 480i  60Hz";break;}
      case TMDL_HDMITX_VFMT_12_720x240p_60Hz: {return "Format 12 720  x 240p  60Hz";break;}
      case TMDL_HDMITX_VFMT_13_720x240p_60Hz: {return "Format 13 720  x 240p  60Hz";break;}
      case TMDL_HDMITX_VFMT_14_1440x480p_60Hz: {return "Format 14 1440 x 480p  60Hz";break;}
      case TMDL_HDMITX_VFMT_15_1440x480p_60Hz: {return "Format 15 1440 x 480p  60Hz";break;}
      case TMDL_HDMITX_VFMT_16_1920x1080p_60Hz: {return "Format 16 1920 x 1080p 60Hz";break;}
      case TMDL_HDMITX_VFMT_17_720x576p_50Hz: {return "Format 17 720  x 576p  50Hz";break;}
      case TMDL_HDMITX_VFMT_18_720x576p_50Hz: {return "Format 18 720  x 576p  50Hz";break;}
      case TMDL_HDMITX_VFMT_19_1280x720p_50Hz: {return "Format 19 1280 x 720p  50Hz";break;}
      case TMDL_HDMITX_VFMT_20_1920x1080i_50Hz: {return "Format 20 1920 x 1080i 50Hz";break;}
      case TMDL_HDMITX_VFMT_21_720x576i_50Hz: {return "Format 21 720  x 576i  50Hz";break;}
      case TMDL_HDMITX_VFMT_22_720x576i_50Hz: {return "Format 22 720  x 576i  50Hz";break;}
      case TMDL_HDMITX_VFMT_23_720x288p_50Hz: {return "Format 23 720  x 288p  50Hz";break;}
      case TMDL_HDMITX_VFMT_24_720x288p_50Hz: {return "Format 24 720  x 288p  50Hz";break;}
      case TMDL_HDMITX_VFMT_25_720x576i_50Hz: {return "Format 25 720  x 576i  50Hz";break;}
      case TMDL_HDMITX_VFMT_26_720x576i_50Hz: {return "Format 26 720  x 576i  50Hz";break;}
      case TMDL_HDMITX_VFMT_27_720x288p_50Hz: {return "Format 27 720  x 288p  50Hz";break;}
      case TMDL_HDMITX_VFMT_28_720x288p_50Hz: {return "Format 28 720  x 288p  50Hz";break;}
      case TMDL_HDMITX_VFMT_29_1440x576p_50Hz: {return "Format 29 1440 x 576p  50Hz";break;}
      case TMDL_HDMITX_VFMT_30_1440x576p_50Hz: {return "Format 30 1440 x 576p  50Hz";break;}
      case TMDL_HDMITX_VFMT_31_1920x1080p_50Hz: {return "Format 31 1920 x 1080p 50Hz";break;}
      case TMDL_HDMITX_VFMT_32_1920x1080p_24Hz: {return "Format 32 1920 x 1080p 24Hz";break;}
      case TMDL_HDMITX_VFMT_33_1920x1080p_25Hz: {return "Format 33 1920 x 1080p 25Hz";break;}
      case TMDL_HDMITX_VFMT_34_1920x1080p_30Hz: {return "Format 34 1920 x 1080p 30Hz";break;}
      case TMDL_HDMITX_VFMT_TV_NUM: {return "Number of TV formats & null";break;}
      case TMDL_HDMITX_VFMT_PC_MIN: {return "Lowest valid PC format";break;}
      case TMDL_HDMITX_VFMT_PC_800x600p_60Hz: {return "PC format 129";break;}
      case TMDL_HDMITX_VFMT_PC_1152x960p_60Hz: {return "PC format 130";break;}
      case TMDL_HDMITX_VFMT_PC_1024x768p_60Hz: {return "PC format 131";break;}
      case TMDL_HDMITX_VFMT_PC_1280x768p_60Hz: {return "PC format 132";break;}
      case TMDL_HDMITX_VFMT_PC_1280x1024p_60Hz: {return "PC format 133";break;}
      case TMDL_HDMITX_VFMT_PC_1360x768p_60Hz: {return "PC format 134";break;}
      case TMDL_HDMITX_VFMT_PC_1400x1050p_60Hz: {return "PC format 135";break;}
      case TMDL_HDMITX_VFMT_PC_1600x1200p_60Hz: {return "PC format 136";break;}
      case TMDL_HDMITX_VFMT_PC_1024x768p_70Hz: {return "PC format 137";break;}
      case TMDL_HDMITX_VFMT_PC_640x480p_72Hz: {return "PC format 138";break;}
      case TMDL_HDMITX_VFMT_PC_800x600p_72Hz: {return "PC format 139";break;}
      case TMDL_HDMITX_VFMT_PC_640x480p_75Hz: {return "PC format 140";break;}
      case TMDL_HDMITX_VFMT_PC_1024x768p_75Hz: {return "PC format 141";break;}
      case TMDL_HDMITX_VFMT_PC_800x600p_75Hz: {return "PC format 142";break;}
      case TMDL_HDMITX_VFMT_PC_1024x864p_75Hz: {return "PC format 143";break;}
      case TMDL_HDMITX_VFMT_PC_1280x1024p_75Hz: {return "PC format 144";break;}
      case TMDL_HDMITX_VFMT_PC_640x350p_85Hz: {return "PC format 145";break;}
      case TMDL_HDMITX_VFMT_PC_640x400p_85Hz: {return "PC format 146";break;}
      case TMDL_HDMITX_VFMT_PC_720x400p_85Hz: {return "PC format 147";break;}
      case TMDL_HDMITX_VFMT_PC_640x480p_85Hz: {return "PC format 148";break;}
      case TMDL_HDMITX_VFMT_PC_800x600p_85Hz: {return "PC format 149";break;}
      case TMDL_HDMITX_VFMT_PC_1024x768p_85Hz: {return "PC format 150";break;}
      case TMDL_HDMITX_VFMT_PC_1152x864p_85Hz: {return "PC format 151";break;}
      case TMDL_HDMITX_VFMT_PC_1280x960p_85Hz: {return "PC format 152";break;}
      case TMDL_HDMITX_VFMT_PC_1280x1024p_85Hz: {return "PC format 153";break;}
      case TMDL_HDMITX_VFMT_PC_1024x768i_87Hz: {return "PC format 154";break;}
      default : {return "Unknown video format";break;}
      }
}
#endif

static char *tda_ioctl(int io)
{
   switch (io)
      {
      case TDA_VERBOSE_ON_CMD: {return "TDA_VERBOSE_ON_CMD";break;}
      case TDA_VERBOSE_OFF_CMD: {return "TDA_VERBOSE_OFF_CMD";break;}
      case TDA_BYEBYE_CMD: {return "TDA_BYEBYE_CMD";break;}
      case TDA_GET_SW_VERSION_CMD: {return "TDA_GET_SW_VERSION_CMD";break;}
      case TDA_SET_POWER_CMD: {return "TDA_SET_POWER_CMD";break;}
      case TDA_GET_POWER_CMD: {return "TDA_GET_POWER_CMD";break;}
      case TDA_SETUP_CMD: {return "TDA_SETUP_CMD";break;}
      case TDA_GET_SETUP_CMD: {return "TDA_GET_SETUP_CMD";break;}
      case TDA_WAIT_EVENT_CMD: {return "TDA_WAIT_EVENT_CMD";break;}
      case TDA_ENABLE_EVENT_CMD: {return "TDA_ENABLE_EVENT_CMD";break;}
      case TDA_DISABLE_EVENT_CMD: {return "TDA_DISABLE_EVENT_CMD";break;}
      case TDA_GET_VIDEO_SPEC_CMD: {return "TDA_GET_VIDEO_SPEC_CMD";break;}
      case TDA_SET_INPUT_OUTPUT_CMD: {return "TDA_SET_INPUT_OUTPUT_CMD";break;}
      case TDA_SET_AUDIO_INPUT_CMD: {return "TDA_SET_AUDIO_INPUT_CMD";break;}
      case TDA_SET_VIDEO_INFOFRAME_CMD: {return "TDA_SET_VIDEO_INFOFRAME_CMD";break;}
      case TDA_SET_AUDIO_INFOFRAME_CMD: {return "TDA_SET_AUDIO_INFOFRAME_CMD";break;}
      case TDA_SET_ACP_CMD: {return "TDA_SET_ACP_CMD";break;}
      case TDA_SET_GCP_CMD: {return "TDA_SET_GCP_CMD";break;}
      case TDA_SET_ISRC1_CMD: {return "TDA_SET_ISRC1_CMD";break;}
      case TDA_SET_ISRC2_CMD: {return "TDA_SET_ISRC2_CMD";break;}
      case TDA_SET_MPS_INFOFRAME_CMD: {return "TDA_SET_MPS_INFOFRAME_CMD";break;}
      case TDA_SET_SPD_INFOFRAME_CMD: {return "TDA_SET_SPD_INFOFRAME_CMD";break;}
      case TDA_SET_VS_INFOFRAME_CMD: {return "TDA_SET_VS_INFOFRAME_CMD";break;}
      case TDA_SET_AUDIO_MUTE_CMD: {return "TDA_SET_AUDIO_MUTE_CMD";break;}
      case TDA_RESET_AUDIO_CTS_CMD: {return "TDA_RESET_AUDIO_CTS_CMD";break;}
      case TDA_GET_EDID_STATUS_CMD: {return "TDA_GET_EDID_STATUS_CMD";break;}
      case TDA_GET_EDID_AUDIO_CAPS_CMD: {return "TDA_GET_EDID_AUDIO_CAPS_CMD";break;}
      case TDA_GET_EDID_VIDEO_CAPS_CMD: {return "TDA_GET_EDID_VIDEO_CAPS_CMD";break;}
      case TDA_GET_EDID_VIDEO_PREF_CMD: {return "TDA_GET_EDID_VIDEO_PREF_CMD";break;}
      case TDA_GET_EDID_SINK_TYPE_CMD: {return "TDA_GET_EDID_SINK_TYPE_CMD";break;}
      case TDA_GET_EDID_SOURCE_ADDRESS_CMD: {return "TDA_GET_EDID_SOURCE_ADDRESS_CMD";break;}
      case TDA_SET_GAMMUT_CMD: {return "TDA_SET_GAMMUT_CMD";break;}
      case TDA_GET_EDID_DTD_CMD: {return "TDA_GET_EDID_DTD_CMD";break;}
      case TDA_GET_EDID_MD_CMD: {return "TDA_GET_EDID_MD_CMD";break;}
      case TDA_GET_EDID_TV_ASPECT_RATIO_CMD: {return "TDA_GET_EDID_TV_ASPECT_RATIO_CMD";break;}
      case TDA_GET_EDID_LATENCY_CMD: {return "TDA_GET_EDID_LATENCY_CMD";break;}
      default : {return "unknown";break;}
      }


}

#if defined (TMFL_TDA19989) || defined (TMFL_TDA9984) 
/*
 *  
 */
static int tda_spy(int verbose)
{
   tda_instance *this=&our_instance;
   int i,err=0;

   if (!verbose) {
      return err;
   }

   printk(KERN_INFO "\n<edid video caps>\n");
   this->tda.edid_video_caps.max=EXAMPLE_MAX_SVD;
   TRY(tmdlHdmiTxGetEdidVideoCaps(this->tda.instance,              \
                                  this->tda.edid_video_caps.desc, \
                                  this->tda.edid_video_caps.max,      \
                                  &this->tda.edid_video_caps.written, \
                                  &this->tda.edid_video_caps.flags));
   printk(KERN_INFO "written:%d\n",this->tda.edid_video_caps.written);
   printk(KERN_INFO "flags:0X%x\n",this->tda.edid_video_caps.flags);
   if (this->tda.edid_video_caps.written > this->tda.edid_video_caps.max) {
      printk(KERN_ERR "get %d video caps but was waiting for %d\n", \
             this->tda.edid_video_caps.written,                     \
             this->tda.edid_video_caps.max); 
      this->tda.edid_video_caps.written = this->tda.edid_video_caps.max;
   }
   for(i=0; i<this->tda.edid_video_caps.written;i++) {
      printk(KERN_INFO "videoFormat:%s\n",tda_spy_vfmt(this->tda.edid_video_caps.desc[i].videoFormat));
      printk(KERN_INFO "nativeVideoFormat:%s\n",(this->tda.edid_video_caps.desc[i].nativeVideoFormat?"yes":"no"));
   }

   printk(KERN_INFO "\n<edid video timings>\n");
   TRY(tmdlHdmiTxGetEdidVideoPreferred(this->tda.instance, \
                                       &this->tda.edid_video_timings));
   printk(KERN_INFO "Pixel Clock/10 000:%d\n",this->tda.edid_video_timings.pixelClock);
   printk(KERN_INFO "Horizontal Active Pixels:%d\n",this->tda.edid_video_timings.hActivePixels);
   printk(KERN_INFO "Horizontal Blanking Pixels:%d\n",this->tda.edid_video_timings.hBlankPixels);
   printk(KERN_INFO "Vertical Active Lines:%d\n",this->tda.edid_video_timings.vActiveLines);
   printk(KERN_INFO "Vertical Blanking Lines:%d\n",this->tda.edid_video_timings.vBlankLines);
   printk(KERN_INFO "Horizontal Sync Offset:%d\n",this->tda.edid_video_timings.hSyncOffset);
   printk(KERN_INFO "Horiz. Sync Pulse Width:%d\n",this->tda.edid_video_timings.hSyncWidth);
   printk(KERN_INFO "Vertical Sync Offset:%d\n",this->tda.edid_video_timings.vSyncOffset);
   printk(KERN_INFO "Vertical Sync Pulse Width:%d\n",this->tda.edid_video_timings.vSyncWidth);
   printk(KERN_INFO "Horizontal Image Size:%d\n",this->tda.edid_video_timings.hImageSize);
   printk(KERN_INFO "Vertical Image Size:%d\n",this->tda.edid_video_timings.vImageSize);
   printk(KERN_INFO "Horizontal Border:%d\n",this->tda.edid_video_timings.hBorderPixels);
   printk(KERN_INFO "Vertical Border:%d\n",this->tda.edid_video_timings.vBorderPixels);
   printk(KERN_INFO "Interlace/sync info:%x\n",this->tda.edid_video_timings.flags);

   printk(KERN_INFO "\n<sink type>\n");
   TRY(tmdlHdmiTxGetEdidSinkType(this->tda.instance,    \
                                 &this->tda.setio.sink));
   printk(KERN_INFO "%s\n",tda_spy_sink(this->tda.setio.sink));
   printk(KERN_INFO "\n<source address>\n");
   TRY(tmdlHdmiTxGetEdidSourceAddress(this->tda.instance,   \
                                      &this->tda.src_address));
   printk(KERN_INFO "%x\n",this->tda.src_address);
   printk(KERN_INFO "\n<detailled timing descriptors>\n");
   this->tda.edid_dtd.max=EXAMPLE_MAX_SVD;
   TRY(tmdlHdmiTxGetEdidDetailledTimingDescriptors(this->tda.instance,  \
                                                   this->tda.edid_dtd.desc, \
                                                   this->tda.edid_dtd.max, \
                                                   &this->tda.edid_dtd.written));
   printk(KERN_INFO "Interlace/sync info:%x\n",this->tda.edid_dtd.desc[i].flags);
   printk(KERN_INFO "written:%d\n",this->tda.edid_dtd.written);
   if (this->tda.edid_dtd.written > this->tda.edid_dtd.max) {
      printk(KERN_ERR "get %d video caps but was waiting for %d\n", \
             this->tda.edid_dtd.written,                     \
             this->tda.edid_dtd.max); 
      this->tda.edid_dtd.written = this->tda.edid_dtd.max;
   }
   for(i=0; i<this->tda.edid_dtd.written;i++) {
      printk(KERN_INFO "Pixel Clock/10 000:%d\n",this->tda.edid_dtd.desc[i].pixelClock);
      printk(KERN_INFO "Horizontal Active Pixels:%d\n",this->tda.edid_dtd.desc[i].hActivePixels);
      printk(KERN_INFO "Horizontal Blanking Pixels:%d\n",this->tda.edid_dtd.desc[i].hBlankPixels);
      printk(KERN_INFO "Vertical Active Lines:%d\n",this->tda.edid_dtd.desc[i].vActiveLines);
      printk(KERN_INFO "Vertical Blanking Lines:%d\n",this->tda.edid_dtd.desc[i].vBlankLines);
      printk(KERN_INFO "Horizontal Sync Offset:%d\n",this->tda.edid_dtd.desc[i].hSyncOffset);
      printk(KERN_INFO "Horiz. Sync Pulse Width:%d\n",this->tda.edid_dtd.desc[i].hSyncWidth);
      printk(KERN_INFO "Vertical Sync Offset:%d\n",this->tda.edid_dtd.desc[i].vSyncOffset);
      printk(KERN_INFO "Vertical Sync Pulse Width:%d\n",this->tda.edid_dtd.desc[i].vSyncWidth);
      printk(KERN_INFO "Horizontal Image Size:%d\n",this->tda.edid_dtd.desc[i].hImageSize);
      printk(KERN_INFO "Vertical Image Size:%d\n",this->tda.edid_dtd.desc[i].vImageSize);
      printk(KERN_INFO "Horizontal Border:%d\n",this->tda.edid_dtd.desc[i].hBorderPixels);
      printk(KERN_INFO "Vertical Border:%d\n",this->tda.edid_dtd.desc[i].vBorderPixels);
   }

   printk(KERN_INFO "\n<monitor descriptors>\n");
   this->tda.edid_md.max=EXAMPLE_MAX_SVD;
   TRY(tmdlHdmiTxGetEdidMonitorDescriptors(this->tda.instance,  \
                                           this->tda.edid_md.desc1,    \
                                           this->tda.edid_md.desc2,    \
                                           this->tda.edid_md.other,    \
                                           this->tda.edid_md.max,       \
                                           &this->tda.edid_md.written));
   printk(KERN_INFO "written:%d\n",this->tda.edid_md.written);
   if (this->tda.edid_md.written > this->tda.edid_md.max) {
      printk(KERN_ERR "get %d video caps but was waiting for %d\n", \
             this->tda.edid_md.written,                     \
             this->tda.edid_md.max); 
      this->tda.edid_md.written = this->tda.edid_md.max;
   }
   for(i=0; i<this->tda.edid_md.written;i++) {
      if (this->tda.edid_md.desc1[i].descRecord) {
         this->tda.edid_md.desc1[i].monitorName[EDID_MONITOR_DESCRIPTOR_SIZE]=0;
         printk(KERN_INFO "Monitor name:%s\n",this->tda.edid_md.desc1[i].monitorName);
      }
      if (this->tda.edid_md.desc1[i].descRecord) {
         printk(KERN_INFO "Min vertical rate in Hz:%d\n",this->tda.edid_md.desc2[i].minVerticalRate);
         printk(KERN_INFO "Max vertical rate in Hz:%d\n",this->tda.edid_md.desc2[i].maxVerticalRate);
         printk(KERN_INFO "Min horizontal rate in Hz:%d\n",this->tda.edid_md.desc2[i].minHorizontalRate);
         printk(KERN_INFO "Max horizontal rate in Hz:%d\n",this->tda.edid_md.desc2[i].maxHorizontalRate);
         printk(KERN_INFO "Max suuported pixel clock rate in MHz:%d\n",this->tda.edid_md.desc2[i].maxSupportedPixelClk);
      }
   }

   printk(KERN_INFO "\n<TV picture ratio>\n");
   TRY(tmdlHdmiTxGetEdidTVPictureRatio(this->tda.instance,  \
                                       &this->tda.edid_tv_aspect_ratio));
   printk(KERN_INFO "%s\n",tda_spy_aspect_ratio(this->tda.edid_tv_aspect_ratio));

   printk(KERN_INFO "\n<latency info>\n");
   TRY(tmdlHdmiTxGetEdidLatencyInfo(this->tda.instance, \
                                    &this->tda.edid_latency));
   if (this->tda.edid_latency.latency_available) {
      printk(KERN_INFO "Edid video:%d\n",this->tda.edid_latency.Edidvideo_latency);
      printk(KERN_INFO "Edid audio:%d\n",this->tda.edid_latency.Edidaudio_latency);
   }
   if (this->tda.edid_latency.Ilatency_available) {
      printk(KERN_INFO "Edid Ivideo:%d\n",this->tda.edid_latency.EdidIvideo_latency);
      printk(KERN_INFO "Edid Iaudio:%d\n",this->tda.edid_latency.EdidIaudio_latency);
   }
 TRY_DONE:
   return err;
}
#endif

/*
 *
 * PROCESSING
 * ----------
 * LEVEL 2
 *
 * - 
 *
 */


/*
 * On/Off HDCP
 */
void hdcp_onoff(tda_instance *this) {

   int err=0;

/*    printk("DBG %s status:%d enable:%d\n",__func__,this->tda.hdcp_status,this->tda.hdcp_enable); */
   if (this->tda.rx_device_active) {
      if (this->tda.hot_plug_detect == TMDL_HDMITX_HOTPLUG_ACTIVE) {
         if (this->tda.power == tmPowerOn) {
            if (this->tda.src_address != 0xFFFF) {
               if (this->tda.hdcp_status!=TMDL_HDMITX_HDCP_CHECK_NUM) {/* ugly is bad */
                  if (this->tda.hdcp_status == TMDL_HDMITX_HDCP_CHECK_NOT_STARTED) {
                     if (this->tda.hdcp_enable) {
                        TRY(tmdlHdmiTxSetHdcp(this->tda.instance,True));
#if defined (TMFL_TDA19989) || defined (TMFL_TDA9984) 
                        TRY(tmdlHdmiTxSetBScreen(this->tda.instance,TMDL_HDMITX_PATTERN_BLUE));
#endif
                     }
                  }
                  else if (this->tda.hdcp_status != TMDL_HDMITX_HDCP_CHECK_NOT_STARTED) {
                     if (!this->tda.hdcp_enable) {
                        TRY(tmdlHdmiTxSetHdcp(this->tda.instance,False));
                     }
                  }
               }
            }
         }
      }
   }
TRY_DONE:
   (void)0;
}


/*
 * Run video
 */
void show_video(tda_instance *this) {

   int err=0;

	printk("[jaeseong.gim] %s\n",__func__);
   if (this->tda.rx_device_active) {
      if (this->tda.hot_plug_detect == TMDL_HDMITX_HOTPLUG_ACTIVE) {
         if (this->tda.power == tmPowerOn) {
            if (this->tda.src_address != 0xFFFF) {
               TRY(tmdlHdmiTxSetInputOutput(this->tda.instance,         \
                                            this->tda.setio.video_in,   \
                                            this->tda.setio.video_out,  \
                                            this->tda.setio.audio_in,   \
                                            this->tda.setio.sink));
               hdcp_onoff(this);
            }
         }
      }
   }
 TRY_DONE:
   (void)0;
}

/*
 *  TDA interrupt polling
 */
static void interrupt_polling(struct work_struct *dummy)
{
   tda_instance *this=&our_instance;
   int err=0;

   /* Tx part */
   TRY(tmdlHdmiTxHandleInterrupt(this->tda.instance));

   /* CEC part */
   if (this->driver.cec_callback) this->driver.cec_callback(dummy);

   /* FIX : IT anti debounce */
   TRY(tmdlHdmiTxHandleInterrupt(this->tda.instance));

 TRY_DONE:

   /* setup next polling */
#ifndef IRQ
   mod_timer(&this->driver.no_irq_timer,jiffies + ( CHECK_EVERY_XX_MS * HZ / 1000 ));
#endif

   (void)0;
}

/*
 *  TDA interrupt polling
 */
static void hdcp_check(struct work_struct *dummy)
{
   int err=0;
   tda_instance *this=&our_instance;
   tmdlHdmiTxHdcpCheck_t hdcp_status;

   down(&this->driver.sem);

   if (this->tda.hdcp_status==TMDL_HDMITX_HDCP_CHECK_NUM) goto TRY_DONE; /* ugly is bad */

   TRY(tmdlHdmiTxHdcpCheck(this->tda.instance,HDCP_CHECK_EVERY_MS));
   TRY(tmdlHdmiTxGetHdcpState(this->tda.instance, &hdcp_status));
#if defined (TMFL_TDA19989) || defined (TMFL_TDA9984) 
   if (this->tda.hdcp_status != hdcp_status) {
      LOG(KERN_INFO,"HDCP status:%s\n",tda_spy_hdcp_status(hdcp_status));
      this->tda.hdcp_status = hdcp_status;
   }
#endif
#ifdef HDCP_TEST
   /* TEST */
   if (test++>500) {
      test=0;
      this->tda.hdcp_enable=1-this->tda.hdcp_enable;
      printk("TEST hdcp:%d\n",this->tda.hdcp_enable);
      hdcp_onoff(this);
   }
#endif

 TRY_DONE:

   /* setup next polling */
   mod_timer(&this->driver.hdcp_check,jiffies + ( HDCP_CHECK_EVERY_MS * HZ / 1000 ));

   up(&this->driver.sem);
}

void register_cec_interrupt(cec_callback_t fct)
{
   tda_instance *this=&our_instance;

   this->driver.cec_callback = fct;
}
EXPORT_SYMBOL(register_cec_interrupt);

void unregister_cec_interrupt(void)
{
   tda_instance *this=&our_instance;

   this->driver.cec_callback = NULL;
}
EXPORT_SYMBOL(unregister_cec_interrupt);

static DECLARE_WORK(wq_irq, interrupt_polling);
void polling_timeout(unsigned long arg)
{
   /* derefered because ATOMIC context of timer does not support I2C_transfert */
   schedule_work(&wq_irq);
}

static DECLARE_WORK(wq_hdcp, hdcp_check);
void hdcp_check_timeout(unsigned long arg)
{
   /* derefered because ATOMIC context of timer does not support I2C_transfert */
   schedule_work(&wq_hdcp);
}

#ifdef IRQ
/*
 *  TDA irq
 */
static irqreturn_t tda_irq(int irq, void *_udc)
{

   /* do it now */
   schedule_work(&wq_irq);

   return IRQ_HANDLED;
}
#endif

/*
 *  TDA callback
 */
static void eventCallbackTx(tmdlHdmiTxEvent_t event)
{
   tda_instance *this=&our_instance;
   int err=0;
   unsigned short new_addr;
#if defined (TMFL_TDA19989) || defined (TMFL_TDA9984) 
   tda_hdcp_fail hdcp_fail;
#endif

   this->tda.event=event;
   if (TMDL_HDMITX_HDCP_INACTIVE != event) {
      printk(KERN_INFO "hdmi %s\n",tda_spy_event(event));
   }

   switch (event) {
   case TMDL_HDMITX_EDID_RECEIVED:
	   /*jaeseong.gim 	10.07.25
		 edid read error
		 */
#if !defined (LG_HW_REV2) || !defined(LG_HW_REV3) || !defined(LG_HW_REV4) 
      TRY(tmdlHdmiTxGetEdidSourceAddress(this->tda.instance,        \
                                         &new_addr));
      LOG(KERN_INFO,"phy.@:%x\n",new_addr);
      /*       if (this->tda.src_address == new_addr) { */
      /*          break; */
      /*       } */
      this->tda.src_address = new_addr;
#if defined (TMFL_TDA19989) || defined (TMFL_TDA9984) 
	  //jaeseong.gim eli >1
      tda_spy(this->param.verbose);
#endif
      /* 
         Customer may add stuff to analyse EDID (see tda_spy())
         and select automatically some video/audio settings.
         By default, let go on with next case and activate
         default video/audio settings with tmdlHdmiTxSetInputOutput()
      */

      TRY(tmdlHdmiTxGetEdidSinkType(this->tda.instance,     \
                                    &this->tda.setio.sink));

      if (TMDL_HDMITX_SINK_HDMI != this->tda.setio.sink) {
         printk(KERN_INFO "/!\\ CAUTION /!\\ sink is not HDMI but %s\n",tda_spy_sink(this->tda.setio.sink));
      }
      
      msleep(100);
#else
      this->tda.src_address = 0;
#endif
/* 	jaeseong.gim 	10.08.24
 */
#ifdef CONFIG_MACH_LGE_BRYCE
		if(!external_common_state){
			this->tda.setio.video_out.format = 
			this->tda.setio.video_in.format = 
			TMDL_HDMITX_VFMT_04_1280x720p_60Hz;
		}
	   else{
			this->tda.setio.video_out.format = 
			this->tda.setio.video_in.format = 
			external_common_state->video_resolution+1;
		}
#endif
/* add chungil.chung@lge.com, 0013177: when pluged hdmi cable in, hdmi view blinking twice*/
        break;
/* end */
   case TMDL_HDMITX_RX_DEVICE_ACTIVE:
      this->tda.rx_device_active = 1;
      show_video(this);
		/* 	jaeseong.gim 	10.08.24
		 */
#ifdef CONFIG_MACH_LGE_BRYCE
		change_hdmi_state(1);
#endif
      break;
   case TMDL_HDMITX_RX_DEVICE_INACTIVE: 
      this->tda.rx_device_active = 0;
		/* 	jaeseong.gim 	10.08.24
		 */
#ifdef CONFIG_MACH_LGE_BRYCE
		change_hdmi_state(0);
#endif
      break;
   case TMDL_HDMITX_HPD_ACTIVE: 
      this->tda.hot_plug_detect = TMDL_HDMITX_HOTPLUG_ACTIVE;
      /* add chungil.chung@lge.com, 0013177: when pluged hdmi cable in, hdmi view blinking twice*/
	//show_video(this);
	/* end */	
      break;
   case TMDL_HDMITX_HPD_INACTIVE: 
      this->tda.hot_plug_detect = TMDL_HDMITX_HOTPLUG_INACTIVE;
      this->tda.src_address = 0xFFFF;
      break;
#if defined (TMFL_TDA19989) || defined (TMFL_TDA9984) 
   case TMDL_HDMITX_HDCP_INACTIVE:
      tmdlHdmiTxGetHdcpFailStatus(this->tda.instance, \
                                  &hdcp_fail, \
                                  &this->tda.hdcp_raw_status);
      if (this->tda.hdcp_fail != hdcp_fail) {
         if (this->tda.hdcp_fail) {
            LOG(KERN_INFO,"%s (%d)\n",tda_spy_hsdc_fail_status(this->tda.hdcp_fail),this->tda.hdcp_raw_status);
         }
         this->tda.hdcp_fail = hdcp_fail;
         tmdlHdmiTxSetBScreen(this->tda.instance,TMDL_HDMITX_PATTERN_BLUE);
      }
      break;
   case TMDL_HDMITX_RX_KEYS_RECEIVED:
      tmdlHdmiTxRemoveBScreen(this->tda.instance);
      break;
#endif
   default:
      break;
   }

   this->driver.poll_done=true;
   wake_up_interruptible(&this->driver.wait);
   
 TRY_DONE:
   (void)0;
}

/*
 *  hdmi Tx init
 */
static int hdmi_tx_init(tda_instance *this)
{
   int err=0;

   LOG(KERN_INFO,"called\n");

   mdelay(10);


   /*Initialize HDMI Transmiter*/
   TRY(tmdlHdmiTxOpen(&this->tda.instance));
   /* Register the HDMI TX events callbacks */
   TRY(tmdlHdmiTxRegisterCallbacks(this->tda.instance,(ptmdlHdmiTxCallback_t)eventCallbackTx));
   /* EnableEvent, all by default */
   TRY(tmdlHdmiTxEnableEvent(this->tda.instance,TMDL_HDMITX_HDCP_ACTIVE));
   TRY(tmdlHdmiTxEnableEvent(this->tda.instance,TMDL_HDMITX_HDCP_INACTIVE));
   TRY(tmdlHdmiTxEnableEvent(this->tda.instance,TMDL_HDMITX_HPD_ACTIVE));
   TRY(tmdlHdmiTxEnableEvent(this->tda.instance,TMDL_HDMITX_HPD_INACTIVE));
   TRY(tmdlHdmiTxEnableEvent(this->tda.instance,TMDL_HDMITX_RX_KEYS_RECEIVED));
   TRY(tmdlHdmiTxEnableEvent(this->tda.instance,TMDL_HDMITX_RX_DEVICE_ACTIVE));
   TRY(tmdlHdmiTxEnableEvent(this->tda.instance,TMDL_HDMITX_RX_DEVICE_INACTIVE));
   TRY(tmdlHdmiTxEnableEvent(this->tda.instance,TMDL_HDMITX_EDID_RECEIVED));

   /* Size of the application EDID buffer */
   this->tda.setup.edidBufferSize=EDID_BLOCK_COUNT * EDID_BLOCK_SIZE;
   /* Buffer to store the application EDID data */
   this->tda.setup.pEdidBuffer=this->tda.raw_edid;
   /* To Enable/disable repeater feature, nor relevant here */
   this->tda.setup.repeaterEnable=false;
   /* To enable/disable simplayHD feature: blue screen when not authenticated */
   this->tda.setup.simplayHd=false;
   

   /* Provides HDMI TX instance configuration */
   TRY(tmdlHdmiTxInstanceSetup(this->tda.instance,&this->tda.setup));
   /* Get IC version */
   TRY(tmdlHdmiTxGetCapabilities(&this->tda.capabilities));

   /* Main settings */
   this->tda.setio.video_out.mode = TMDL_HDMITX_VOUTMODE_RGB444;
   this->tda.setio.video_out.colorDepth = TMDL_HDMITX_COLORDEPTH_24;
#ifdef TMFL_TDA19989
   this->tda.setio.video_out.dviVqr = TMDL_HDMITX_VQR_DEFAULT; /* Use HDMI rules for DVI output */
#endif
/*    this->tda.setio.video_out.format = TMDL_HDMITX_VFMT_31_1920x1080p_50Hz; */
/*    this->tda.setio.video_out.format = TMDL_HDMITX_VFMT_PC_640x480p_60Hz; */
/*    this->tda.setio.video_out.format = TMDL_HDMITX_VFMT_PC_640x480p_72Hz; */
/*    this->tda.setio.video_out.format = TMDL_HDMITX_VFMT_04_1280x720p_60Hz; */
/*    this->tda.setio.video_out.format = TMDL_HDMITX_VFMT_19_1280x720p_50Hz; */
   this->tda.setio.video_out.format = TMDL_HDMITX_VFMT_04_1280x720p_60Hz;
   this->tda.setio.video_in.format = TMDL_HDMITX_VFMT_04_1280x720p_60Hz;

   this->tda.setio.video_in.mode = TMDL_HDMITX_VINMODE_RGB444;
   /*    this->tda.setio.video_in.mode = TMDL_HDMITX_VINMODE_YUV422; */


   this->tda.setio.video_in.pixelRate = TMDL_HDMITX_PIXRATE_SINGLE;
   this->tda.setio.video_in.syncSource = TMDL_HDMITX_SYNCSRC_EXT_VS; /* we use HS,VS as synchronisation source */

/* BEGIN:0011045        ehgrace.kim@lge.com     2010.11.18*/ 
/* MOD: fix the HDMI audio configuration for ATC */ 
#ifdef CONFIG_MACH_LGE_BRYCE 
  this->tda.setio.audio_in.format = TMDL_HDMITX_AFMT_I2S; 
  this->tda.setio.audio_in.rate = TMDL_HDMITX_AFS_48K;   // TMDL_HDMITX_AFS_41K;
  this->tda.setio.audio_in.i2sFormat = TMDL_HDMITX_I2SFOR_PHILIPS_L;
  this->tda.setio.audio_in.i2sQualifier = TMDL_HDMITX_I2SQ_16BITS;  // TMDL_HDMITX_I2SQ_32BITS;
  this->tda.setio.audio_in.dstRate = TMDL_HDMITX_DSTRATE_SINGLE; /* not relevant here */
  this->tda.setio.audio_in.channelAllocation = 0; /* audio channel allocation (Ref to CEA-861D p85) */
  /* audio channel allocation (Ref to CEA-861D p85) */
  this->tda.setio.audio_in.channelStatus.PcmIdentification = TMDL_HDMITX_AUDIO_DATA_PCM;
  this->tda.setio.audio_in.channelStatus.CopyrightInfo = TMDL_HDMITX_CSCOPYRIGHT_UNPROTECTED;
  this->tda.setio.audio_in.channelStatus.FormatInfo = TMDL_HDMITX_CSFI_PCM_2CHAN_NO_PRE;
  this->tda.setio.audio_in.channelStatus.categoryCode = 0;
  this->tda.setio.audio_in.channelStatus.clockAccuracy = TMDL_HDMITX_CSCLK_LEVEL_II;
  this->tda.setio.audio_in.channelStatus.maxWordLength = TMDL_HDMITX_CSMAX_LENGTH_24;
  this->tda.setio.audio_in.channelStatus.wordLength = TMDL_HDMITX_CSWORD_DEFAULT;
  this->tda.setio.audio_in.channelStatus.origSampleFreq = TMDL_HDMITX_CSOFREQ_48k;  // TMDL_HDMITX_CSOFREQ_44_1k;
#else
   this->tda.setio.audio_in.format = TMDL_HDMITX_AFMT_I2S;
   this->tda.setio.audio_in.rate = TMDL_HDMITX_AFS_48K;
   this->tda.setio.audio_in.i2sFormat = TMDL_HDMITX_I2SFOR_PHILIPS_L;
   this->tda.setio.audio_in.i2sQualifier = TMDL_HDMITX_I2SQ_32BITS;
   this->tda.setio.audio_in.dstRate = TMDL_HDMITX_DSTRATE_SINGLE; /* not relevant here */
   this->tda.setio.audio_in.channelAllocation = 0; /* audio channel allocation (Ref to CEA-861D p85) */
#endif
/* END:0011045        ehgrace.kim@lge.com     2010.11.18*/ 

   this->tda.setio.sink = TMDL_HDMITX_SINK_HDMI; /* skip edid reading */
   /*    this->tda.src_address = 0x1000; /\* debug *\/ */
   this->tda.src_address = NO_PHY_ADDR; /* it's unref */

 TRY_DONE:
   return err;
}

void reset_hdmi(int hdcp_module)
{
   tda_instance *this=&our_instance;
   int err=0;

   down(&this->driver.sem);

   /* PATCH because of SetPowerState that calls SetHdcp that has just been removed by nwolc :( */
   if (hdcp_module==2) {
      tmdlHdmiTxSetHdcp(this->tda.instance,0);
      goto TRY_DONE;
   }

   TRY(tmdlHdmiTxSetPowerState(this->tda.instance,tmPowerStandby));
   tmdlHdmiTxClose(this->tda.instance);
   /* reset */
   hdmi_tx_init(this);
   /* recover previous power state */
   TRY(tmdlHdmiTxSetPowerState(this->tda.instance,this->tda.power));
   tmdlHdmiTxGetHPDStatus(this->tda.instance,&this->tda.hot_plug_detect); /* check if activ for timer */
   show_video(this);

   /* wake up or shut down hdcp checking */
   if (hdcp_module) {
      this->driver.hdcp_check.expires = jiffies +  ( HDCP_CHECK_EVERY_MS * HZ / 1000 );
      add_timer(&this->driver.hdcp_check);
      this->tda.hdcp_status = TMDL_HDMITX_HDCP_CHECK_NOT_STARTED; /* ugly is bad ! */ 
      this->tda.hdcp_enable = 1; /* FIXME : fallback 4 HDCP on/off is ok */
#if defined (TMFL_TDA19989) || defined (TMFL_TDA9984) 
      tmdlHdmiTxSetBScreen(this->tda.instance,TMDL_HDMITX_PATTERN_BLUE);
#endif
   }
   else {
      del_timer(&this->driver.hdcp_check);
      this->tda.hdcp_status = TMDL_HDMITX_HDCP_CHECK_NUM; /* ugly is bad ! */ 
      this->tda.hdcp_enable = 0; /* FIXME : fallback 4 HDCP on/off is ok */
   }

 TRY_DONE:
   up(&this->driver.sem);
}
EXPORT_SYMBOL(reset_hdmi);

/*
 *  
 */
short edid_phy_addr(void)
{
   tda_instance *this=&our_instance;

   return this->tda.src_address;
}
EXPORT_SYMBOL(edid_phy_addr);

/*
 *  
 */
tda_power get_hdmi_status(void)
{ 
   tda_instance *this=&our_instance;
   
   return this->tda.power;
}
EXPORT_SYMBOL(get_hdmi_status);

/*
 *  
 */
tda_power get_hpd_status(void)
{ 
   tda_instance *this=&our_instance;
   
   return (this->tda.hot_plug_detect == TMDL_HDMITX_HOTPLUG_ACTIVE);
}
EXPORT_SYMBOL(get_hpd_status);

/*
 *  
 */
int edid_received(void)
{ 
   tda_instance *this=&our_instance;
   
   return (this->tda.event == TMDL_HDMITX_EDID_RECEIVED);
}
EXPORT_SYMBOL(edid_received);

/*
 *  
 */
int hdmi_enable(void)
{
   tda_instance *this=&our_instance;
   int err=0;

   LOG(KERN_INFO,"called\n");

   down(&this->driver.sem);

   this->tda.power = tmPowerOn;
   tmdlHdmiTxSetPowerState(this->tda.instance,this->tda.power);//jaeseong.gim
   if (err==TM_ERR_NO_RESOURCES) {
      LOG(KERN_INFO,"Busy...\n");
      TRY(tmdlHdmiTxHandleInterrupt(this->tda.instance));
      TRY(tmdlHdmiTxHandleInterrupt(this->tda.instance));
      TRY(tmdlHdmiTxHandleInterrupt(this->tda.instance));
   }
   tmdlHdmiTxGetHPDStatus(this->tda.instance,&this->tda.hot_plug_detect);
   //show_video(this); /* FIXME : mind usecases without HPD and RX_SEND... */
   
 TRY_DONE:
   up(&this->driver.sem);
   return err;
}
EXPORT_SYMBOL(hdmi_enable);

/*
 *  
 */
int hdmi_disable(int event_tracking)
{ 
   tda_instance *this=&our_instance;
   int err=0;
   
   LOG(KERN_INFO,"called\n");

   down(&this->driver.sem);

   this->tda.power = (event_tracking?tmPowerSuspend:tmPowerStandby);
   TRY(tmdlHdmiTxSetPowerState(this->tda.instance,this->tda.power));

 TRY_DONE:
   up(&this->driver.sem);
   return err;
}
EXPORT_SYMBOL(hdmi_disable);

/*
 *
 * ENTRY POINTS
 * ------------
 * LEVEL 3
 *
 * - 
 *
 */

#ifdef ANDROID_DSS
/*
 *  DSS driver :: probe
 */
static int hdmi_panel_probe(struct omap_dss_device *dssdev)
{
   tda_instance *this=&our_instance;

   LOG(KERN_INFO," called\n");

   sysdss=dssdev;

   /* 	OMAP_DSS_LCD_IVS		= 1<<0, */
   /* 	OMAP_DSS_LCD_IHS		= 1<<1, */
   /* 	OMAP_DSS_LCD_IPC		= 1<<2, */
   /* 	OMAP_DSS_LCD_IEO		= 1<<3, */
   /* 	OMAP_DSS_LCD_RF			= 1<<4, */
   /* 	OMAP_DSS_LCD_ONOFF		= 1<<5, */
   /* 	OMAP_DSS_LCD_TFT		= 1<<20, */

   dssdev->panel.config = OMAP_DSS_LCD_ONOFF | OMAP_DSS_LCD_IPC |  \
      OMAP_DSS_LCD_TFT | OMAP_DSS_LCD_IVS | OMAP_DSS_LCD_IHS;
   dssdev->panel.timings = video_720x480at60Hz_panel_timings;
   (void)video_1280x720at60Hz_panel_timings;
   (void)video_720x480at60Hz_panel_timings;
   (void)video_1280x720at50Hz_panel_timings;
   (void)video_800x480at60Hz_panel_timings;
   (void)video_1280x720at50Hz_panel_timings;
   (void)video_1920x1080at50Hz_panel_timings;
   (void)video_640x480at72Hz_panel_timings;
   (void)video_640x480at60Hz_panel_timings;

   return 0;
}

/*
 *  DSS driver :: enable
 */
static void hdmi_panel_remove(struct omap_dss_device *dssdev)
{
}

/*
 *  DSS driver :: enable
 */
static int hdmi_panel_enable(struct omap_dss_device *dssdev)
{
   int r = 0;

   if (dssdev->platform_enable)
      r = dssdev->platform_enable(dssdev);
   
   if (r)
      goto ERROR0;

   r = hdmi_enable();
   if (r)
      goto ERROR0;
   /* wait couple of vsyncs until enabling the LCD */
   msleep(50);

   return 0;
 ERROR0:
   return r;
}

/*
 *  DSS driver :: disable
 */
static void hdmi_panel_disable(struct omap_dss_device *dssdev)
{
   hdmi_disable(0);

   /* wait couple of vsyncs until enabling the hdmi */
   msleep(50);

   if (dssdev->platform_disable)
      dssdev->platform_disable(dssdev);
}

/*
 *  DSS driver :: suspend
 */
static int hdmi_panel_suspend(struct omap_dss_device *dssdev)
{
   hdmi_panel_disable(dssdev);
   return 0;
}

/*
 *  DSS driver :: resume
 */
static int hdmi_panel_resume(struct omap_dss_device *dssdev)
{
   return hdmi_panel_enable(dssdev);
}

/*
 *  DSS driver (frontend with omapzoom)
 *  -----------------------------------
 */
static struct omap_dss_driver hdmi_driver = {
   .probe = hdmi_panel_probe,
   .remove = hdmi_panel_remove,
   .enable = hdmi_panel_enable,
   .disable = hdmi_panel_disable,
   .suspend = hdmi_panel_suspend,
   .resume = hdmi_panel_resume,
   .driver = {
      .name = "hdmi_panel",
      .owner = THIS_MODULE,
   }
};
#endif

/*
 *  ioctl driver :: opening
 */

static int this_cdev_open(struct inode *pInode, struct file *pFile)
{
   tda_instance *this;
   int minor=iminor(pInode);

   if(minor >= MAX_MINOR) {
      printk(KERN_ERR "hdmitx:%s:only one tda can be open\n",__func__);
      return -EINVAL;
   }

   if ((pFile->private_data != NULL) && (pFile->private_data != &our_instance)) {
      printk(KERN_ERR "hdmitx:%s:pFile missmatch\n",__func__);
   }
   this = pFile->private_data = &our_instance;
   down(&this->driver.sem);

   LOG(KERN_INFO,"major:%d minor:%d user:%d\n", imajor(pInode), iminor(pInode), this->driver.user_counter);

   if ((this->driver.user_counter++) && (this->driver.minor == minor)) {
      /* init already done */
      up(&this->driver.sem);
      return 0;
   }
   this->driver.minor = minor;


   up(&this->driver.sem);
   return 0;
}

/*
 *  ioctl driver :: ioctl
 */
static int this_cdev_ioctl(struct inode *pInode, struct file *pFile, unsigned int cmd, unsigned long arg)
{
   tda_instance* this = pFile->private_data;
   int err=0;

   LOG(KERN_INFO,":%s\n",tda_ioctl(_IOC_NR(cmd)));

   BUG_ON(this->driver.minor!=iminor(pInode));
   if (_IOC_TYPE(cmd) != TDA_IOCTL_BASE) {
      printk(KERN_INFO "hdmitx:%s:unknown ioctl type: %x\n",__func__,_IOC_TYPE(cmd));
      return -ENOIOCTLCMD;
   }

   if (_IOC_DIR(cmd) & _IOC_READ) 
      err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd)) || !arg;
   else if (_IOC_DIR(cmd) & _IOC_WRITE)
      err = !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd)) || !arg;
   if (err) {
      printk(KERN_ERR "hdmitx:%s:argument access denied (check address vs value)\n",__func__);
      printk(KERN_ERR "_IOC_DIR:%d arg:%lx\n",_IOC_DIR(cmd),arg);
      return -EFAULT;
   }
   
   down(&this->driver.sem);

   /* Check DevLib consistancy here */

   switch ( _IOC_NR(cmd) )
      {
      case TDA_VERBOSE_ON_CMD:
         {
            this->param.verbose=1;
            printk(KERN_INFO "hdmitx:verbose on\n");
            break;
         }

      case TDA_VERBOSE_OFF_CMD:
         {
            printk(KERN_INFO "hdmitx:verbose off\n");
            this->param.verbose=0;
            break;
         }

      case TDA_BYEBYE_CMD:
         {
            LOG(KERN_INFO,"release event handeling request\n");
            this->tda.event=RELEASE;
            this->driver.poll_done = true;
            wake_up_interruptible(&this->driver.wait);
            break;
         }

      case TDA_GET_SW_VERSION_CMD:
         {
            TRY(tmdlHdmiTxGetSWVersion(&this->tda.version));
            BUG_ON(copy_to_user((tda_version*)arg,&this->tda.version,sizeof(tda_version)) != 0);
            break;
         }

      case TDA_SET_POWER_CMD:
         {
            BUG_ON(copy_from_user(&this->tda.power,(tda_power*)arg,sizeof(tda_power)) != 0);
            TRY(tmdlHdmiTxSetPowerState(this->tda.instance, \
                                        this->tda.power));
            break;
         }

      case TDA_GET_POWER_CMD:
         {
            TRY(tmdlHdmiTxGetPowerState(this->tda.instance, \
                                        &this->tda.power));
            BUG_ON(copy_to_user((tda_power*)arg,&this->tda.power,sizeof(tda_power)) != 0);
            break;
         }

      case TDA_SETUP_CMD:
         {
            BUG_ON(copy_from_user(&this->tda.setup,(tda_setup_info*)arg,sizeof(tda_setup_info)) != 0);
            TRY(tmdlHdmiTxInstanceSetup(this->tda.instance, \
                                        &this->tda.setup));
            break;
         }

      case TDA_GET_SETUP_CMD:
         {
            TRY(tmdlHdmiTxGetInstanceSetup(this->tda.instance, \
                                           &this->tda.setup));
            BUG_ON(copy_to_user((tda_setup*)arg,&this->tda.setup,sizeof(tda_setup)) != 0);
            break;
         }

      case TDA_WAIT_EVENT_CMD:
         {
            this->driver.poll_done = false;
            up(&this->driver.sem);
            if (wait_event_interruptible(this->driver.wait,this->driver.poll_done)) return -ERESTARTSYS;
            down(&this->driver.sem);
            BUG_ON(copy_to_user((tda_event*)arg,&this->tda.event,sizeof(tda_event)) != 0);
            break;
         }

      case TDA_ENABLE_EVENT_CMD:
         {
            tmdlHdmiTxEvent_t event;
            BUG_ON(copy_from_user(&event,(tmdlHdmiTxEvent_t*)arg,sizeof(tmdlHdmiTxEvent_t)) != 0);
            TRY(tmdlHdmiTxEnableEvent(this->tda.instance,event));
            break;
         }

      case TDA_DISABLE_EVENT_CMD:
         {
            tmdlHdmiTxEvent_t event;
            BUG_ON(copy_from_user(&event,(tmdlHdmiTxEvent_t*)arg,sizeof(tmdlHdmiTxEvent_t)) != 0);
            TRY(tmdlHdmiTxDisableEvent(this->tda.instance,event));
            break;
         }

      case TDA_GET_VIDEO_SPEC_CMD:
         {
            TRY(tmdlHdmiTxGetVideoFormatSpecs(this->tda.instance, \
                                              this->tda.video_fmt.id, \
                                              &this->tda.video_fmt.spec));
            BUG_ON(copy_to_user((tda_video_format*)arg,&this->tda.video_fmt,sizeof(tda_video_format)) != 0);
            break;
         }

      case TDA_SET_INPUT_OUTPUT_CMD:
         {
            BUG_ON(copy_from_user(&this->tda.setio,(tda_set_in_out*)arg,sizeof(tda_set_in_out)) != 0);

            TRY(tmdlHdmiTxSetInputOutput(this->tda.instance, \
                                         this->tda.setio.video_in, \
                                         this->tda.setio.video_out, \
                                         this->tda.setio.audio_in, \
                                         this->tda.setio.sink));
            break;
         }

      case TDA_SET_AUDIO_INPUT_CMD:
         {
            BUG_ON(copy_from_user(&this->tda.setio.audio_in,(tda_set_audio_in*)arg,sizeof(tda_set_audio_in)) != 0);
            TRY(tmdlHdmiTxSetAudioInput(this->tda.instance, \
                                        this->tda.setio.audio_in, \
                                        this->tda.setio.sink));
            break;
         }

      case TDA_SET_VIDEO_INFOFRAME_CMD:
         {
            BUG_ON(copy_from_user(&this->tda.video_infoframe,(tda_video_infoframe*)arg,sizeof(tda_video_infoframe)) != 0);
            TRY(tmdlHdmiTxSetVideoInfoframe(this->tda.instance, \
                                            this->tda.video_infoframe.enable, \
                                            &this->tda.video_infoframe.data));
            break;
         }

      case TDA_SET_AUDIO_INFOFRAME_CMD:
         {
            BUG_ON(copy_from_user(&this->tda.audio_infoframe,(tda_audio_infoframe*)arg,sizeof(tda_audio_infoframe)) != 0);
            TRY(tmdlHdmiTxSetAudioInfoframe(this->tda.instance, \
                                            this->tda.audio_infoframe.enable, \
                                            &this->tda.audio_infoframe.data));
            break;
         }

      case TDA_SET_ACP_CMD:
         {
            BUG_ON(copy_from_user(&this->tda.acp,(tda_acp*)arg,sizeof(tda_acp)) != 0);
            TRY(tmdlHdmiTxSetACPPacket(this->tda.instance, \
                                       this->tda.acp.enable, \
                                       &this->tda.acp.data));
            break;
         }

      case TDA_SET_GCP_CMD:
         {
            BUG_ON(copy_from_user(&this->tda.gcp,(tda_gcp*)arg,sizeof(tda_gcp)) != 0);
            TRY(tmdlHdmiTxSetGeneralControlPacket(this->tda.instance, \
                                                  this->tda.gcp.enable, \
                                                  &this->tda.gcp.data));
            break;
         }

      case TDA_SET_ISRC1_CMD:
         {
            BUG_ON(copy_from_user(&this->tda.isrc1,(tda_isrc1*)arg,sizeof(tda_isrc1)) != 0);
            TRY(tmdlHdmiTxSetISRC1Packet(this->tda.instance, \
                                         this->tda.isrc1.enable, \
                                         &this->tda.isrc1.data));
            break;
         }

      case TDA_SET_MPS_INFOFRAME_CMD:
         {
            BUG_ON(copy_from_user(&this->tda.mps_infoframe,(tda_mps_infoframe*)arg,sizeof(tda_mps_infoframe)) != 0);
            TRY(tmdlHdmiTxSetMPSInfoframe(this->tda.instance, \
                                          this->tda.mps_infoframe.enable, \
                                          &this->tda.mps_infoframe.data));
            break;
         }

      case TDA_SET_SPD_INFOFRAME_CMD:
         {
            BUG_ON(copy_from_user(&this->tda.spd_infoframe,(tda_spd_infoframe*)arg,sizeof(tda_spd_infoframe)) != 0);
            TRY(tmdlHdmiTxSetSpdInfoframe(this->tda.instance, \
                                          this->tda.spd_infoframe.enable, \
                                          &this->tda.spd_infoframe.data));
            break;
         }

      case TDA_SET_VS_INFOFRAME_CMD:
         {
            BUG_ON(copy_from_user(&this->tda.vs_infoframe,(tda_vs_infoframe*)arg,sizeof(tda_vs_infoframe)) != 0);
            TRY(tmdlHdmiTxSetVsInfoframe(this->tda.instance, \
                                         this->tda.vs_infoframe.enable, \
                                         &this->tda.vs_infoframe.data));
            break;
         }

      case TDA_SET_AUDIO_MUTE_CMD:
         {
            BUG_ON(copy_from_user(&this->tda.audio_mute,(bool*)arg,sizeof(bool)) != 0);
            TRY(tmdlHdmiTxSetAudioMute(this->tda.instance,			\
                                       this->tda.audio_mute));
            break;
         }

      case TDA_RESET_AUDIO_CTS_CMD:
         {
            TRY(tmdlHdmiTxResetAudioCts(this->tda.instance));
            break;
         }

      case TDA_GET_EDID_STATUS_CMD:
         {
            TRY(tmdlHdmiTxGetEdidStatus(this->tda.instance, \
                                        &this->tda.edid.status, \
                                        &this->tda.edid.block_count));
            BUG_ON(copy_to_user((tda_edid*)arg,&this->tda.edid,sizeof(tda_edid)) != 0);
            break;
         }

      case TDA_GET_EDID_AUDIO_CAPS_CMD:
         {
            TRY(tmdlHdmiTxGetEdidAudioCaps(this->tda.instance, \
                                           this->tda.edid_audio_caps.desc, \
                                           this->tda.edid_audio_caps.max, \
                                           &this->tda.edid_audio_caps.written, \
                                           &this->tda.edid_audio_caps.flags));
            BUG_ON(copy_to_user((tda_edid_audio_caps*)arg,&this->tda.edid_audio_caps,sizeof(tda_edid_audio_caps)) != 0);
            break;
         }

      case TDA_GET_EDID_VIDEO_CAPS_CMD:
         {
            TRY(tmdlHdmiTxGetEdidVideoCaps(this->tda.instance, \
                                           this->tda.edid_video_caps.desc, \
                                           this->tda.edid_video_caps.max, \
                                           &this->tda.edid_video_caps.written, \
                                           &this->tda.edid_video_caps.flags));
            BUG_ON(copy_to_user((tda_edid_video_caps*)arg,&this->tda.edid_video_caps,sizeof(tda_edid_video_caps)) != 0);
            break;
         }

      case TDA_GET_EDID_VIDEO_PREF_CMD:
         {
            TRY(tmdlHdmiTxGetEdidVideoPreferred(this->tda.instance, \
                                                &this->tda.edid_video_timings));
            BUG_ON(copy_to_user((tda_edid_video_timings*)arg,&this->tda.edid_video_timings,sizeof(tda_edid_video_timings)) != 0);
            break;
         }

      case TDA_GET_EDID_SINK_TYPE_CMD:
         {
            TRY(tmdlHdmiTxGetEdidSinkType(this->tda.instance, \
                                          &this->tda.setio.sink));
            BUG_ON(copy_to_user((tda_sink*)arg,&this->tda.setio.sink,sizeof(tda_sink)) != 0);
            break;
         }

      case TDA_GET_EDID_SOURCE_ADDRESS_CMD:
         {
            TRY(tmdlHdmiTxGetEdidSourceAddress(this->tda.instance, \
                                               &this->tda.src_address));
            BUG_ON(copy_to_user((unsigned short*)arg,&this->tda.src_address,sizeof(unsigned short)) != 0);
            break;
         }

      case TDA_SET_GAMMUT_CMD:
         {
            BUG_ON(copy_from_user(&this->tda.gammut,(tda_gammut*)arg,sizeof(tda_gammut)) != 0);
            TRY(tmdlHdmiTxSetGamutPacket(this->tda.instance, \
                                         this->tda.gammut.enable, \
                                         &this->tda.gammut.data));
            break;
         }
	 
      case TDA_GET_EDID_DTD_CMD:
         {
            TRY(tmdlHdmiTxGetEdidDetailledTimingDescriptors(this->tda.instance, \
                                                            this->tda.edid_dtd.desc, \
                                                            this->tda.edid_dtd.max, \
                                                            &this->tda.edid_dtd.written));
            BUG_ON(copy_to_user((tda_edid_dtd*)arg,&this->tda.edid_dtd,sizeof(tda_edid_dtd)) != 0);
            break;
         }
     
#if defined (TMFL_TDA19989) || defined (TMFL_TDA9984) 
      case TDA_GET_EDID_MD_CMD:
         {
            TRY(tmdlHdmiTxGetEdidMonitorDescriptors(this->tda.instance, \
                                                    this->tda.edid_md.desc1, \
                                                    this->tda.edid_md.desc2, \
                                                    this->tda.edid_md.other, \
                                                    this->tda.edid_md.max, \
                                                    &this->tda.edid_md.written));
            BUG_ON(copy_to_user((tda_edid_md*)arg,&this->tda.edid_md,sizeof(tda_edid_md)) != 0);
            break;
         }

      case TDA_GET_EDID_TV_ASPECT_RATIO_CMD:
         {
            TRY(tmdlHdmiTxGetEdidTVPictureRatio(this->tda.instance, \
                                                &this->tda.edid_tv_aspect_ratio));
            BUG_ON(copy_to_user((tda_edid_tv_aspect_ratio*)arg,&this->tda.edid_tv_aspect_ratio,sizeof(tda_edid_tv_aspect_ratio)) != 0);
            break;
         }

      case TDA_GET_EDID_LATENCY_CMD:
         {
            TRY(tmdlHdmiTxGetEdidLatencyInfo(this->tda.instance, \
                                             &this->tda.edid_latency));
            BUG_ON(copy_to_user((tda_edid_latency*)arg,&this->tda.edid_latency,sizeof(tda_edid_latency)) != 0);
            break;
         }

      case TDA_SET_HDCP_CMD:
         {
            BUG_ON(copy_from_user(&this->tda.hdcp_enable,(bool*)arg,sizeof(bool)) != 0);
            break;
         }

      case TDA_GET_HDCP_STATUS_CMD:
         {
            BUG_ON(copy_to_user((tda_edid_latency*)arg,&this->tda.hdcp_status,sizeof(tda_hdcp_status)) != 0);
            break;
         }
#endif

      default:
         {
            /* unrecognized ioctl */	
            printk(KERN_INFO "hdmitx:%s:unknown ioctl number: %x\n",__func__,cmd);
            up(&this->driver.sem);
            return -ENOIOCTLCMD;
         }
      }

 TRY_DONE:
   up(&this->driver.sem);
   return err;
}

/*
 *  ioctl driver :: releasing
 */
static int this_cdev_release(struct inode *pInode, struct file *pFile)
{
   tda_instance* this = pFile->private_data;
   int minor = iminor(pInode);

   LOG(KERN_INFO,"called\n");

   if(minor >= MAX_MINOR) {
      LOG(KERN_ERR,"minor too big!\n");
      return -EINVAL;
   }

   BUG_ON(this->driver.minor!=iminor(pInode));
   down(&this->driver.sem);

   this->driver.user_counter--;
   if(this->driver.user_counter == 0) {
      pFile->private_data = NULL;
   }
   else {
      LOG(KERN_INFO,"Still %d users pending\n",this->driver.user_counter);
   }

   up(&this->driver.sem);
   return 0;
}

/*
 *  I2C client :: creation
 */
static int this_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
   tda_instance *this=&our_instance;
   int err=0;

   LOG(KERN_INFO,"called\n");



/* 	jaeseong.gim 	10.08.24
 */
#ifdef CONFIG_MACH_LGE_BRYCE
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)){
		printk("[jaeseong.gim]i2c_check_error\n");
		return -ENODEV;
	}
	gpio_set_value(109, 1);
	gpio_power_on=TRUE;
	msleep(10);
#endif

   /*
     I2C setup
   */
   if (this->driver.i2c_client) {
      dev_err(&this->driver.i2c_client->dev, "<%s> HDMI Device already created \n",
              __func__);
      return -ENODEV;
   }

   this->driver.i2c_client = kmalloc(sizeof(struct i2c_client), GFP_KERNEL);
   if (!this->driver.i2c_client) {
      return -ENOMEM;
   }
   memset(this->driver.i2c_client, 0, sizeof(struct i2c_client));

   strncpy(this->driver.i2c_client->name, TX_NAME, I2C_NAME_SIZE);
   this->driver.i2c_client->addr = TDA998X_I2C_SLAVEADDRESS;
   this->driver.i2c_client->adapter = client->adapter;

   i2c_set_clientdata(client, this->driver.i2c_client);
   /* I2C ok, then let's startup TDA */
   err = hdmi_tx_init(this);
   if (err) goto i2c_out;
   this->tda.hdcp_enable = 0;
   /* Standby the HDMI TX instance */
   this->tda.power = tmPowerStandby;
   tmdlHdmiTxSetPowerState(this->tda.instance,this->tda.power);
   /* update HPD */
   tmdlHdmiTxGetHPDStatus(this->tda.instance,&this->tda.hot_plug_detect);

#ifdef ANDROID_DSS
   /* probe DSS */
   err = omap_dss_register_driver(&hdmi_driver);
#endif
   if (err) goto i2c_tx_out;

   /* prepare event */
   this->driver.poll_done = true; /* currently idle */
   init_waitqueue_head(&this->driver.wait);

#ifdef IRQ
   /* FRO calibration */
   err=gpio_request(TDA_IRQ_CALIB,"tda998x calibration");
   if (err < 0) {
	   //[jaeseong.gim] 107->104
      printk(KERN_ERR "hdmitx:%s:cannot use GPIO 104\n",__func__);
      goto i2c_out;
   }
   /* turn GPIO into IRQ */
   gpio_direction_input(TDA_IRQ_CALIB);
   msleep(1);
   if (request_irq(gpio_to_irq(TDA_IRQ_CALIB), \
                   tda_irq, IRQF_TRIGGER_FALLING|IRQF_DISABLED, "TDA IRQ", NULL)) {
      printk(KERN_ERR "hdmitx:%s:Cannot request irq, err:%d\n",__func__,err);
      gpio_free(TDA_IRQ_CALIB);
      goto i2c_out;
   }
#else
   init_timer(&this->driver.no_irq_timer);
   this->driver.no_irq_timer.function=polling_timeout;
   this->driver.no_irq_timer.data=0;
   this->driver.no_irq_timer.expires = jiffies + HZ; /* start polling in one sec */
   add_timer(&this->driver.no_irq_timer);
#endif

   /* setup hdcp check timer */
   init_timer(&this->driver.hdcp_check);
   this->driver.hdcp_check.function=hdcp_check_timeout;
   this->driver.hdcp_check.data=0;

   tmdlHdmiTxGetSWVersion(&this->tda.version);
   printk(KERN_INFO "HDMI TX SW Version:%lu.%lu compatibility:%lu\n",   \
          this->tda.version.majorVersionNr,\
          this->tda.version.minorVersionNr,\
          this->tda.version.compatibilityNr);
   return 0;

 i2c_tx_out:
   LOG(KERN_INFO,"tmdlHdmiTx closed\n");
   /* close DevLib */
   err=tmdlHdmiTxClose(this->tda.instance);

 i2c_out:
   LOG(KERN_INFO,"this->driver.i2c_client removed\n");
   kfree(this->driver.i2c_client);
   this->driver.i2c_client = NULL;

   return err;
}

/*
 *  I2C client :: destroy
 */
static int this_i2c_remove(struct i2c_client *client)
{
   tda_instance *this=&our_instance;
   int err=0;

   LOG(KERN_INFO,"called\n");

#ifdef ANDROID_DSS
   /* unplug DSS */
   omap_dss_unregister_driver(&hdmi_driver);
#endif

   if (!client->adapter) {
      dev_err(&this->driver.i2c_client->dev, "<%s> No HDMI Device \n",
              __func__);
      return -ENODEV;
   }
   kfree(this->driver.i2c_client);
   this->driver.i2c_client = NULL;

   return err;
}

/*
 *  I2C client driver (backend)
 *  -----------------
 */
static const struct i2c_device_id this_i2c_id[] = {
   { TX_NAME, 0 },
   { },
};

MODULE_DEVICE_TABLE(i2c, this_i2c_id);

static struct i2c_driver this_i2c_driver = {
   .driver = {
      .owner = THIS_MODULE,
      .name = TX_NAME,
   },
   .probe = this_i2c_probe,
   .remove = this_i2c_remove,
   .id_table = this_i2c_id,
};

/*
 *  ioctl driver (userland frontend)
 *  ------------
 */
static struct file_operations this_cdev_fops = {
 owner:    THIS_MODULE,
 open:     this_cdev_open,
 release:  this_cdev_release,
 ioctl:    this_cdev_ioctl,
};

/*
 *  Module :: start up
 */
static int __init tx_init(void)
{
   tda_instance *this=&our_instance;
   dev_t dev=0;
   int err=0;

/* 	jaeseong.gim 	10.08.24
 */
#ifdef CONFIG_MACH_LGE_BRYCE
	external_common_state = &hdmi_common;
	external_common_state->video_resolution = HDMI_VFRMT_1280x720p60_16_9;
	HDMI_SETUP_LUT(640x480p60_4_3);
	HDMI_SETUP_LUT(720x480p60_16_9);
	HDMI_SETUP_LUT(1280x720p60_16_9);

	hdmi_common_init_panel_info(&hdmi_panel_data.panel_info);

   msm_fb_add_device(&hdmi_device);

	//temp
	//hdmi_power_on(&hdmi_device);

	if (machine_is_msm7x30_surf()) {
		short *hdtv_mux = (short *)ioremap(0x8e000170 , 0x100);
		*hdtv_mux++  = 0x020b;
		*hdtv_mux  = 0x8000;
		iounmap(hdtv_mux);
	}
#endif
   /* 
      general device context
   */
   memset(this,0,sizeof(tda_instance)); 
   this->param.verbose = param_verbose;
   this->param.major = param_major;
   this->param.minor = param_minor;

   /* Hello word */
   printk(KERN_INFO "%s(%s) %d.%d.%d compiled: %s %s %s\n", HDMITX_NAME, TDA_NAME, 
          TDA_VERSION_MAJOR,
          TDA_VERSION_MINOR,
          TDA_VERSION_PATCHLEVEL, 
          __DATE__, __TIME__, TDA_VERSION_EXTRA);
   if (this->param.verbose) LOG(KERN_INFO,".verbose mode\n");

   /*
     plug I2C (backend : Hw interfacing)
   */
   err = i2c_add_driver(&this_i2c_driver);
   if (err < 0) {
      printk(KERN_ERR "Driver registration failed\n");
      return -ENODEV;
   }

   if (this->driver.i2c_client == NULL) {
      printk(KERN_ERR "this->driver.i2c_client not allocated\n");
      /* unregister i2c */
      err = -ENODEV;
      goto init_out;
   }

   /*
     cdev init (userland frontend)
   */

   /* arbitray range of device numbers */
   if (this->param.major) {
      /* user force major number @ insmod */
      dev = MKDEV(this->param.major, this->param.minor);
      err = register_chrdev_region(dev,MAX_MINOR,HDMITX_NAME);
      if (err) {
         printk(KERN_ERR "unable to register %s, dev=%d %s\n",HDMITX_NAME,dev,ERR_TO_STR(err));
         goto init_out;
      }
   } else {
      /* fully dynamic major number */
      err = alloc_chrdev_region(&dev, this->param.minor, MAX_MINOR,HDMITX_NAME);
      if (err) {
         printk(KERN_ERR "unable to alloc chrdev region for %s, dev=%d %s\n",HDMITX_NAME,dev,ERR_TO_STR(err));
         goto init_out;
      }
      this->param.major = MAJOR(dev);
      this->param.minor = MINOR(dev);
      /*       create_dev("/dev/hdmitx",dev); */
      LOG(KERN_INFO,"/dev/hdmitx created major:%d minor:%d\n",this->param.major, this->param.minor);
   }

   cdev_init(this_cdev, &this_cdev_fops);
   this_cdev->owner = THIS_MODULE;

   this->driver.class = class_create(THIS_MODULE, HDMITX_NAME);
   if (IS_ERR(this->driver.class)) {
      printk(KERN_INFO "Error creating mmap device class.\n");
      err =-EIO;
      goto init_out;
   }
   this->driver.dev=device_create(this->driver.class, NULL /* parent */, dev, NULL, HDMITX_NAME);

   this->driver.devno = dev;
   err = cdev_add(this_cdev, this->driver.devno, MAX_MINOR);
   if (err){
      printk(KERN_INFO "unable to add device for %s, ipp_driver.devno=%d %s\n",HDMITX_NAME,this->driver.devno,ERR_TO_STR(err));
      device_destroy(this->driver.class,this->driver.devno);
      class_destroy(this->driver.class);
      unregister_chrdev_region(this->driver.devno, MAX_MINOR);
      goto init_out;
   }   

   /* 
      general device context
   */
   init_MUTEX(&this->driver.sem);
   hdmi_enable();
   return 0;

 init_out:
   i2c_del_driver(&this_i2c_driver);
   return err;
}

/*
 *  Module :: shut down
 */
static void __exit tx_exit(void)
{
   tda_instance *this=&our_instance;

   LOG(KERN_INFO,"called\n");

#ifdef IRQ
   free_irq(gpio_to_irq(TDA_IRQ_CALIB), NULL);
   gpio_free(TDA_IRQ_CALIB);
#else
   del_timer(&this->driver.no_irq_timer);
#endif

   del_timer(&this->driver.hdcp_check);
   msleep(100);

   /* close DevLib */
   tmdlHdmiTxClose(this->tda.instance);

   /* unregister cdevice */
   cdev_del(this_cdev); 
   unregister_chrdev_region(this->driver.devno, MAX_MINOR);
   
   /* unregister device */
   device_destroy(this->driver.class,this->driver.devno);
   class_destroy(this->driver.class);

   /* unregister i2c */
   i2c_del_driver(&this_i2c_driver);
}


/*
 *  Module
 *  ------
 */
/* late_initcall(tx_init); */
//[jaeseong.gim] module or built-in option
#ifdef MODULE
module_init(tx_init);
#else
late_initcall(tx_init);
#endif
module_exit(tx_exit);

/*
 *  Disclamer
 *  ---------
 */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Andre Lepine <andre.lepine@nxp.com>");
MODULE_DESCRIPTION(HDMITX_NAME " driver");
