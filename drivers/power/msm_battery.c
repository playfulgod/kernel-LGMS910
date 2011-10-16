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

/*
 * this needs to be before <linux/kernel.h> is loaded,
 * and <linux/sched.h> loads <linux/kernel.h>
 */
//#define DEBUG  1

#include <linux/slab.h>
#include <linux/earlysuspend.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/sched.h>
#include <linux/signal.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <linux/workqueue.h>

#include <asm/atomic.h>

#include <mach/msm_rpcrouter.h>
#include <mach/msm_battery.h>
// BEGIN: 0010099 hyunjong.do@lge.com 2010-10-21
// ADD: 0010099: [Power] Charging current control by thermister on Kernel
#include <mach/msm_battery_thunderc.h>
#define CONFIG_MACH_MSM7X27_THUNDERC
// END:  0010099 hyunjong.do@lge.com 2010-10-21

/* BEGIN: 0009612 tei.kim@lge.com 2010-10-01 */
/* ADD 0009612: [kernel] add pif mode in msm_battery.c */
#ifdef CONFIG_MACH_LGE_BRYCE
#include <linux/delay.h>
#include "../../arch/arm/mach-msm/proc_comm.h"

static int pif_value = 0;
unsigned cmd_pif = 8;
unsigned cmd_lpm = 7;
#endif
/* END: 0009612 tei.kim@lge.com 2010-10-01 */

/* BEGIN: 0015566 jihoon.lee@lge.com 20110207 */
/* ADD 0015566: [Kernel] charging mode check command */
#ifdef CONFIG_LGE_CHARGING_MODE_INFO
#include <linux/types.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#endif
/* END: 0015566 jihoon.lee@lge.com 20110207 */

// BEGIN: 0016859: hyunjong.do@lge.com 2011-02-23
// ADD: 0016859: [ORD] [Wireless Charging] preventing suspend while charging with wireless PAD
#include <linux/wakelock.h>
// END: 0016859: hyunjong.do@lge.com 2011-02-23

#ifdef CONFIG_LGE_CHARGING
#include <mach/lg_comdef.h> /* boolean */
#endif

#define BATTERY_RPC_PROG	0x30000089
#define BATTERY_RPC_VER_1_1	0x00010001
#define BATTERY_RPC_VER_2_1	0x00020001
#define BATTERY_RPC_VER_4_1     0x00040001
#define BATTERY_RPC_VER_5_1     0x00050001

#define BATTERY_RPC_CB_PROG	(BATTERY_RPC_PROG | 0x01000000)

#define CHG_RPC_PROG		0x3000001a
#define CHG_RPC_VER_1_1		0x00010001
#define CHG_RPC_VER_1_3		0x00010003
#define CHG_RPC_VER_2_2		0x00020002
#define CHG_RPC_VER_3_1         0x00030001
#define CHG_RPC_VER_4_1         0x00040001

#define BATTERY_REGISTER_PROC				2
#define BATTERY_MODIFY_CLIENT_PROC			4
#define BATTERY_DEREGISTER_CLIENT_PROC			5
#define BATTERY_READ_MV_PROC				12
#define BATTERY_ENABLE_DISABLE_FILTER_PROC		14

#define VBATT_FILTER			2

#define BATTERY_CB_TYPE_PROC		1
#define BATTERY_CB_ID_ALL_ACTIV		1
#define BATTERY_CB_ID_LOW_VOL		2

#define BATTERY_LOW		3200
#define BATTERY_HIGH		4300

#define ONCRPC_CHG_GET_GENERAL_STATUS_PROC	12
// BEGIN: 0010099 hyunjong.do@lge.com 2010-10-21
// ADD: 0010099: [Power] Charging current control by thermister on Kernel
#define ONCRPC_LG_CHG_GET_GENERAL_STATUS_PROC 20
// END:  0010099 hyunjong.do@lge.com 2010-10-21
#define ONCRPC_CHARGER_API_VERSIONS_PROC	0xffffffff

// BEGIN: 0010099 hyunjong.do@lge.com 2010-10-21
// MOD: 0010099: [Power] Charging current control by thermister on Kernel
#define BATT_RPC_TIMEOUT    10000	/* 10 sec */
// END:  0010099 hyunjong.do@lge.com 2010-10-21

#define INVALID_BATT_HANDLE    -1

#define RPC_TYPE_REQ     0
#define RPC_TYPE_REPLY   1
#define RPC_REQ_REPLY_COMMON_HEADER_SIZE   (3 * sizeof(uint32_t))


#if DEBUG
#define DBG_LIMIT(x...) do {if (printk_ratelimit()) pr_debug(x); } while (0)
#else
#define DBG_LIMIT(x...) do {} while (0)
#endif
// BEGIN: 0010099 hyunjong.do@lge.com 2010-10-21
// ADD: 0010099: [Power] Charging current control by thermister on Kernel
/* LGE_CHANGE_S [woonghee.park@lge.com] 2010-03-18, ALARM */
extern int msm_chg_LG_cable_type(void);

#define LG_FACTORY_CABLE_TYPE           3
#define LG_FACTORY_CABLE_130K_TYPE      10

struct wake_lock battery_wake_lock;
/* LGE_CHANGE_E [woonghee.park@lge.com] 2010-03-18, ALARM */
// END:  0010099 hyunjong.do@lge.com 2010-10-21

enum {
	BATTERY_REGISTRATION_SUCCESSFUL = 0,
	BATTERY_DEREGISTRATION_SUCCESSFUL = BATTERY_REGISTRATION_SUCCESSFUL,
	BATTERY_MODIFICATION_SUCCESSFUL = BATTERY_REGISTRATION_SUCCESSFUL,
	BATTERY_INTERROGATION_SUCCESSFUL = BATTERY_REGISTRATION_SUCCESSFUL,
	BATTERY_CLIENT_TABLE_FULL = 1,
	BATTERY_REG_PARAMS_WRONG = 2,
	BATTERY_DEREGISTRATION_FAILED = 4,
	BATTERY_MODIFICATION_FAILED = 8,
	BATTERY_INTERROGATION_FAILED = 16,
	/* Client's filter could not be set because perhaps it does not exist */
	BATTERY_SET_FILTER_FAILED         = 32,
	/* Client's could not be found for enabling or disabling the individual
	 * client */
	BATTERY_ENABLE_DISABLE_INDIVIDUAL_CLIENT_FAILED  = 64,
	BATTERY_LAST_ERROR = 128,
};

enum {
	BATTERY_VOLTAGE_UP = 0,
	BATTERY_VOLTAGE_DOWN,
	BATTERY_VOLTAGE_ABOVE_THIS_LEVEL,
	BATTERY_VOLTAGE_BELOW_THIS_LEVEL,
	BATTERY_VOLTAGE_LEVEL,
	BATTERY_ALL_ACTIVITY,
	VBATT_CHG_EVENTS,
	BATTERY_VOLTAGE_UNKNOWN,
};
// BEGIN: 0010099 hyunjong.do@lge.com 2010-10-21
// ADD: 0010099: [Power] Charging current control by thermister on Kernel
enum {
	CHG_UI_EVENT_IDLE,	/* Starting point, no charger.  */
	CHG_UI_EVENT_NO_POWER,	/* No/Weak Battery + Weak Charger. */
	CHG_UI_EVENT_VERY_LOW_POWER,	/* No/Weak Battery + Strong Charger. */
	CHG_UI_EVENT_LOW_POWER,	/* Low Battery + Strog Charger.  */
	CHG_UI_EVENT_NORMAL_POWER, /* Enough Power for most applications. */
	CHG_UI_EVENT_DONE,	/* Done charging, batt full.  */
	CHG_UI_EVENT_INVALID,
	CHG_UI_EVENT_MAX32 = 0x7fffffff
};
// END:  0010099 hyunjong.do@lge.com 2010-10-21

/*
 * This enum contains defintions of the charger hardware status
 */
enum chg_charger_status_type {
	/* The charger is good      */
	CHARGER_STATUS_GOOD,
	/* The charger is bad       */
	CHARGER_STATUS_BAD,
	/* The charger is weak      */
	CHARGER_STATUS_WEAK,
	/* Invalid charger status.  */
	CHARGER_STATUS_INVALID
};

/*
 *This enum contains defintions of the charger hardware type
 */
enum chg_charger_hardware_type {
	/* The charger is removed                 */
	CHARGER_TYPE_NONE,
	/* The charger is a regular wall charger   */
	CHARGER_TYPE_WALL,
	/* The charger is a PC USB                 */
	CHARGER_TYPE_USB_PC,
	/* The charger is a wall USB charger       */
	CHARGER_TYPE_USB_WALL,
	/* The charger is a USB carkit             */
	CHARGER_TYPE_USB_CARKIT,
	/* Invalid charger hardware status.        */
	CHARGER_TYPE_INVALID
};

/*
 *  This enum contains defintions of the battery status
 */
enum chg_battery_status_type {
	/* The battery is good        */
	BATTERY_STATUS_GOOD,
	/* The battery is cold/hot    */
	BATTERY_STATUS_BAD_TEMP,
	/* The battery is bad         */
	BATTERY_STATUS_BAD,
	/* The battery is removed     */
	BATTERY_STATUS_REMOVED,		/* on v2.2 only */
	BATTERY_STATUS_INVALID_v1 = BATTERY_STATUS_REMOVED,
	/* Invalid battery status.    */
	BATTERY_STATUS_INVALID
};

/*
 *This enum contains defintions of the battery voltage level
 */
enum chg_battery_level_type {
	/* The battery voltage is dead/very low (less than 3.2V) */
	BATTERY_LEVEL_DEAD,
	/* The battery voltage is weak/low (between 3.2V and 3.4V) */
	BATTERY_LEVEL_WEAK,
	/* The battery voltage is good/normal(between 3.4V and 4.2V) */
	BATTERY_LEVEL_GOOD,
	/* The battery voltage is up to full (close to 4.2V) */
	BATTERY_LEVEL_FULL,
	/* Invalid battery voltage level. */
	BATTERY_LEVEL_INVALID
};

#ifndef CONFIG_BATTERY_MSM_FAKE
struct rpc_reply_batt_chg_v1 {
	struct rpc_reply_hdr hdr;
	u32 	more_data;

	u32	charger_status;
	u32	charger_type;
	u32	battery_status;
	u32	battery_level;
	u32     battery_voltage;
	u32	battery_temp;
/* CONFIG_MACH_LGE_BRYCE hyunjong.do@lge.com 10.08.10
   */
#if CONFIG_MACH_LGE_BRYCE
	u32 battery_soc; // bart	
// BEGIN: 0010099 hyunjong.do@lge.com 2010-10-21
// ADD: 0010099: [Power] Charging current control by thermister on Kernel
#ifdef CONFIG_LGE_BATTERY_ID
	u32 battery_id;
#else
/* LGE_CHANGE_S [jaeho.cho@lge.com] 2010-11-01, check battery thermistor is valid or not */
    u32 batt_thm_valid;
/* LGE_CHANGE_S [jaeho.cho@lge.com] 2010-11-01, check battery thermistor is valid or not */
#endif
	u32 battery_therm;
// END:  0010099 hyunjong.do@lge.com 2010-10-21
#endif
/* LGE_CHANGE_S [jaeho.cho@lge.com] 2010-10-19, workaround to fix unexpected charging without ext_pwr */
#ifdef CONFIG_LGE_CHARGING
    u32  ext_pwr;
#endif
/* LGE_CHANGE_S [jaeho.cho@lge.com] 2010-10-19, workaround to fix unexpected charging without ext_pwr */	

};

struct rpc_reply_batt_chg_v2 {
	struct rpc_reply_batt_chg_v1	v1;

	u32	is_charger_valid;
	u32	is_charging;
	u32	is_battery_valid;
	u32	ui_event;
};

union rpc_reply_batt_chg {
	struct rpc_reply_batt_chg_v1	v1;
	struct rpc_reply_batt_chg_v2	v2;
};

static union rpc_reply_batt_chg rep_batt_chg;
#endif

struct msm_battery_info {
	u32 voltage_max_design;
	u32 voltage_min_design;
	u32 chg_api_version;
	u32 batt_technology;
	u32 batt_api_version;

	u32 avail_chg_sources;
	u32 current_chg_source;

	u32 batt_status;
	u32 batt_health;
	u32 charger_valid;
	u32 batt_valid;
	u32 batt_capacity; /* in percentage */

	u32 charger_status;
	u32 charger_type;
	u32 battery_status;
	u32 battery_level;
	u32 battery_voltage; /* in millie volts */
	u32 battery_temp;  /* in celsius */
// BEGIN: 0010099 hyunjong.do@lge.com 2010-10-21
// ADD: 0010099: [Power] Charging current control by thermister on Kernel

  u32 valid_battery_id;
#ifdef CONFIG_LGE_BATTERY_ID
#else
/* LGE_CHANGE_S [jaeho.cho@lge.com] 2010-11-01, check battery thermistor is valid or not */
  u32 batt_thm_valid;
/* LGE_CHANGE_S [jaeho.cho@lge.com] 2010-11-01, check battery thermistor is valid or not */
#endif
  u32 battery_therm;
// END:  0010099 hyunjong.do@lge.com 2010-10-21
/* CONFIG_MACH_LGE_BRYCE hyunjong.do@lge.com 10.08.10
   */
/* LGE_CHANGE_S [jaeho.cho@lge.com] 2010-10-19, workaround to fix unexpected charging without ext_pwr */
#ifdef CONFIG_LGE_CHARGING
  u32 ext_pwr;
#endif
#if CONFIG_MACH_LGE_BRYCE
	u32 battery_soc; // bart	
#endif

	u32(*calculate_capacity) (u32 voltage);

	s32 batt_handle;

	struct power_supply *msm_psy_ac;
	struct power_supply *msm_psy_usb;
	struct power_supply *msm_psy_batt;
	struct power_supply *current_ps;

	struct msm_rpc_client *batt_client;
	struct msm_rpc_endpoint *chg_ep;

	wait_queue_head_t wait_q;

	u32 vbatt_modify_reply_avail;

	struct early_suspend early_suspend;
};

static struct msm_battery_info msm_batt_info = {
	.batt_handle = INVALID_BATT_HANDLE,
	.charger_status = CHARGER_STATUS_BAD,
	.charger_type = CHARGER_TYPE_INVALID,
	.battery_status = BATTERY_STATUS_GOOD,
	.battery_level = BATTERY_LEVEL_FULL,
	.battery_voltage = BATTERY_HIGH,
	.batt_capacity = 100,
	.batt_status = POWER_SUPPLY_STATUS_DISCHARGING,
	.batt_health = POWER_SUPPLY_HEALTH_GOOD,
	.batt_valid  = 1,
	.battery_temp = 23,

// BEGIN: 0010099 hyunjong.do@lge.com 2010-10-21
// ADD: 0010099: [Power] Charging current control by thermister on Kernel
 	.valid_battery_id=1,
	.battery_therm=99,
// END:  0010099 hyunjong.do@lge.com 2010-10-21
	
/* CONFIG_MACH_LGE_BRYCE hyunjong.do@lge.com 10.08.10
   */
#if CONFIG_MACH_LGE_BRYCE
	.battery_soc = 100, // bart
#endif
	
	.vbatt_modify_reply_avail = 0,
};

// BEGIN: 0010099 hyunjong.do@lge.com 2010-10-21
// ADD: 0010099: [Power] Charging current control by thermister on Kernel
static struct pseudo_batt_info_type pseudo_batt_info = {
  .mode = 0,
  .thm_only_mode =0,
};

static int block_charging_state = 1;   //  1 : charging , 0: block charging
// END:  0010099 hyunjong.do@lge.com 2010-10-21

static enum power_supply_property msm_power_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
};

static char *msm_power_supplied_to[] = {
	"battery",
};

static int msm_power_get_property(struct power_supply *psy,
				  enum power_supply_property psp,
				  union power_supply_propval *val)
{
	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		if (psy->type == POWER_SUPPLY_TYPE_MAINS) {
			val->intval = msm_batt_info.current_chg_source & AC_CHG
			    ? 1 : 0;
		}
		if (psy->type == POWER_SUPPLY_TYPE_USB) {
			val->intval = msm_batt_info.current_chg_source & USB_CHG
			    ? 1 : 0;
		}
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static struct power_supply msm_psy_ac = {
	.name = "ac",
	.type = POWER_SUPPLY_TYPE_MAINS,
	.supplied_to = msm_power_supplied_to,
	.num_supplicants = ARRAY_SIZE(msm_power_supplied_to),
	.properties = msm_power_props,
	.num_properties = ARRAY_SIZE(msm_power_props),
	.get_property = msm_power_get_property,
};

static struct power_supply msm_psy_usb = {
	.name = "usb",
	.type = POWER_SUPPLY_TYPE_USB,
	.supplied_to = msm_power_supplied_to,
	.num_supplicants = ARRAY_SIZE(msm_power_supplied_to),
	.properties = msm_power_props,
	.num_properties = ARRAY_SIZE(msm_power_props),
	.get_property = msm_power_get_property,
};

static enum power_supply_property msm_batt_power_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_TECHNOLOGY,
	POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN,
	POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,

/* CONFIG_MACH_LGE_BRYCE hyunjong.do@lge.com 10.08.10
   */
#if CONFIG_MACH_LGE_BRYCE
	POWER_SUPPLY_PROP_CHARGE_COUNTER, // bart
#endif
	
	POWER_SUPPLY_PROP_CAPACITY,
// BEGIN: 0010099 hyunjong.do@lge.com 2010-10-21
// ADD: 0010099: [Power] Charging current control by thermister on Kernel
	POWER_SUPPLY_PROP_TEMP,
/* LGE_CHANGES_E [woonghee.park@lge.com]*/
/* LGE_CHANGES_S [woonghee.park@lge.com] 2010-02-09, [VS740], LG_FW_BATT_ID_CHECK, LG_FW_BATT_THM*/
	POWER_SUPPLY_PROP_BATTERY_ID_CHECK,
#ifdef CONFIG_LGE_BATTERY_ID
#else
/* LGE_CHANGE_S [jaeho.cho@lge.com] 2010-11-01, check battery thermistor is valid or not */
    POWER_SUPPLY_PROP_BATT_THM_VALID,
/* LGE_CHANGE_S [jaeho.cho@lge.com] 2010-11-01, check battery thermistor is valid or not */
#endif
	POWER_SUPPLY_PROP_BATTERY_TEMP_ADC,
	POWER_SUPPLY_PROP_PSEUDO_BATT,
	POWER_SUPPLY_PROP_BLOCK_CHARGING,
/* LGE_CHANGES_E [woonghee.park@lge.com]*/
// END:  0010099 hyunjong.do@lge.com 2010-10-21
/* LGE_CHANGE_S [jaeho.cho@lge.com] 2010-10-19, workaround to fix unexpected charging without ext_pwr */
#ifdef CONFIG_LGE_CHARGING
    POWER_SUPPLY_PROP_EXT_PWR_CHECK,
#endif
/* LGE_CHANGE_S [jaeho.cho@lge.com] 2010-10-19, workaround to fix unexpected charging without ext_pwr */
};

// BEGIN: 0010099 hyunjong.do@lge.com 2010-10-21
// ADD: 0010099: [Power] Charging current control by thermister on Kernel
static void msm_batt_update_psy_status(void);

static void msm_batt_external_power_changed(struct power_supply *psy)
{
	power_supply_changed(psy);
}
// END:  0010099 hyunjong.do@lge.com 2010-10-21

static int msm_batt_power_get_property(struct power_supply *psy,
				       enum power_supply_property psp,
				       union power_supply_propval *val)
{
	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
// BEGIN: 0010099 hyunjong.do@lge.com 2010-10-21
// ADD: 0010099: [Power] Charging current control by thermister on Kernel
    if(pseudo_batt_info.mode == 1)
      val->intval = pseudo_batt_info.charging;
    else
// END:  0010099 hyunjong.do@lge.com 2010-10-21
		val->intval = msm_batt_info.batt_status;
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = msm_batt_info.batt_health;
		break;
	case POWER_SUPPLY_PROP_PRESENT:
// BEGIN: 0010099 hyunjong.do@lge.com 2010-10-21
// ADD: 0010099: [Power] Charging current control by thermister on Kernel
		/* LGE_CHANGES_S [woonghee.park@lge.com] 2010-02-09, [VS740], LG_FW_BATT_ID_CHECK, LG_FW_BATT_THM*/
    if(pseudo_batt_info.mode == 1)
    {
#ifdef CONFIG_LGE_BATTERY_ID
      if(pseudo_batt_info.id == 1 || pseudo_batt_info.therm != 0)
#else
      if(pseudo_batt_info.thm_valid == 1 || pseudo_batt_info.therm != 0)
#endif
        val->intval = 1;
      else
        val->intval = 0;
    }
    else
    {
      if(msm_batt_info.batt_valid == 1 || msm_batt_info.battery_therm != 0)    
        val->intval = 1;
      else
        val->intval = 0;
    }		
// END:  0010099 hyunjong.do@lge.com 2010-10-21

// BEGIN: 0010099 hyunjong.do@lge.com 2010-10-21
// DEL: 0010099: [Power] Charging current control by thermister on Kernel
    /* LGE_COMMENT_OUT
         val->intval = msm_batt_info.batt_valid; */
		/* LGE_CHANGES_E [woonghee.park@lge.com]*/
// END:  0010099 hyunjong.do@lge.com 2010-10-21
		break;
	case POWER_SUPPLY_PROP_TECHNOLOGY:
		val->intval = msm_batt_info.batt_technology;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN:
		val->intval = msm_batt_info.voltage_max_design;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN:
		val->intval = msm_batt_info.voltage_min_design;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
// BEGIN: 0010099 hyunjong.do@lge.com 2010-10-21
// ADD: 0010099: [Power] Charging current control by thermister on Kernel
    if(pseudo_batt_info.mode == 1)
      val->intval = pseudo_batt_info.volt;
    else
		val->intval = msm_batt_info.battery_voltage;
		break;
// END:  0010099 hyunjong.do@lge.com 2010-10-21

// BEGIN: 0010099 hyunjong.do@lge.com 2010-10-21
// ADD: 0010099: [Power] Charging current control by thermister on Kernel
	case POWER_SUPPLY_PROP_CAPACITY:
    if(pseudo_batt_info.mode == 1)
      val->intval = pseudo_batt_info.capacity;
    else
		  val->intval = msm_batt_info.batt_capacity;
		break;
	/* LGE_CHANGES_S [woonghee.park@lge.com] 2010-02-09, [VS740], LG_FW_BATT_INFO_TEMP*/
  case POWER_SUPPLY_PROP_TEMP:
    if((pseudo_batt_info.mode == 1)||(pseudo_batt_info.thm_only_mode == 1))//yunjeong.kang 201.06.03
      val->intval = pseudo_batt_info.temp;
    else
      val->intval = msm_batt_info.battery_temp;
    break;
	/* LGE_CHANGES_E [woonghee.park@lge.com]*/
	/* LGE_CHANGES_S [woonghee.park@lge.com] 2010-02-09, [VS740], LG_FW_BATT_ID_CHECK, LG_FW_BATT_THM*/
#ifdef CONFIG_LGE_BATTERY_ID
	case POWER_SUPPLY_PROP_BATTERY_ID_CHECK:
    if(pseudo_batt_info.mode == 1)
      val->intval = pseudo_batt_info.id;
    else
      val->intval = msm_batt_info.valid_battery_id;
    break;
#else
	case POWER_SUPPLY_PROP_BATTERY_ID_CHECK:
	if(pseudo_batt_info.mode == 1)
	  val->intval = pseudo_batt_info.thm_valid;
	else
	  val->intval = msm_batt_info.valid_battery_id;
	break;

/* LGE_CHANGE_S [jaeho.cho@lge.com] 2010-11-01, check battery thermistor is valid or not */
    case POWER_SUPPLY_PROP_BATT_THM_VALID:
    if(pseudo_batt_info.mode == 1)
      val->intval = pseudo_batt_info.thm_valid;
    else
      val->intval = msm_batt_info.batt_thm_valid;
    break;
/* LGE_CHANGE_S [jaeho.cho@lge.com] 2010-11-01, check battery thermistor is valid or not */
#endif
	case POWER_SUPPLY_PROP_BATTERY_TEMP_ADC:
    if((pseudo_batt_info.mode == 1) ||(pseudo_batt_info.thm_only_mode == 1)) //yunjeong.kang 201.06.03
      val->intval = pseudo_batt_info.therm;
    else
      val->intval = msm_batt_info.battery_therm;
    break;
	/* LGE_CHANGES_E [woonghee.park@lge.com]*/

  case POWER_SUPPLY_PROP_PSEUDO_BATT:
  	if (pseudo_batt_info.mode)
		val->intval = 2;
	else if (pseudo_batt_info.thm_only_mode)
   		val->intval =1;
	else
		val->intval =0;
    break;

  case POWER_SUPPLY_PROP_BLOCK_CHARGING:
    val->intval = block_charging_state;
    break;
// END:  0010099 hyunjong.do@lge.com 2010-10-21

/* CONFIG_MACH_LGE_BRYCE hyunjong.do@lge.com 10.08.10
   */
#if CONFIG_MACH_LGE_BRYCE
	case POWER_SUPPLY_PROP_CHARGE_COUNTER:  // bart, need to fix
		val->intval = msm_batt_info.battery_soc;
		break;
#endif
	/* LGE_CHANGE_S [jaeho.cho@lge.com] 2010-10-19, workaround to fix unexpected charging without ext_pwr */
#ifdef CONFIG_LGE_CHARGING
    case POWER_SUPPLY_PROP_EXT_PWR_CHECK:
  	    val->intval = msm_batt_info.ext_pwr;
  	    break;
#endif
	/* LGE_CHANGE_S [jaeho.cho@lge.com] 2010-10-19, workaround to fix unexpected charging without ext_pwr */

	default:
		return -EINVAL;
	}
	return 0;
}

static struct power_supply msm_psy_batt = {
	.name = "battery",
	.type = POWER_SUPPLY_TYPE_BATTERY,
	.properties = msm_batt_power_props,
	.num_properties = ARRAY_SIZE(msm_batt_power_props),
	.get_property = msm_batt_power_get_property,
// BEGIN: 0010098 hyunjong.do@lge.com 2010-10-21
// ADD: 0010098: [Power] Charging current control by thermister on modem
	.external_power_changed = msm_batt_external_power_changed,

// END:  0010098 hyunjong.do@lge.com 2010-10-21
};

// BEGIN: 0010098 hyunjong.do@lge.com 2010-10-21
// ADD: 0010098: [Power] Charging current control by thermister on modem
enum charger_type {  // it  comes from msm_hsusb.c
	CHG_HOST_PC,
	CHG_WALL = 2,
	CHG_UNDEFINED,
};

int charger_hw_type;

/* LGE_CHANGES_S [woonghee.park@lge.com] 2010-02-09, [VS740], LG_FW_BATT_ID_CHECK, LG_FW_BATT_THM*/
extern void battery_info_get(struct batt_info* rsp);
extern void pseudo_batt_info_set(struct pseudo_batt_info_type*);

int pseudo_batt_set(struct pseudo_batt_info_type* info)
{
  pseudo_batt_info.mode = info->mode;
  pseudo_batt_info.thm_only_mode = info->thm_only_mode;//yunjeong.kang 201.06.03
#ifdef CONFIG_LGE_BATTERY_ID
  pseudo_batt_info.id = info->id;
#else
  pseudo_batt_info.thm_valid = info->thm_valid;
#endif
  pseudo_batt_info.therm = info->therm;
  pseudo_batt_info.temp = info->temp;
  pseudo_batt_info.volt = info->volt;
  pseudo_batt_info.capacity = info->capacity;
  pseudo_batt_info.charging = info->charging;

	power_supply_changed(&msm_psy_batt);
  pseudo_batt_info_set(&pseudo_batt_info);
  return 0;
}
EXPORT_SYMBOL(pseudo_batt_set);
extern void block_charging_set(int);
void batt_block_charging_set(int block)
{
	block_charging_state = block;
	block_charging_set(block);
}
EXPORT_SYMBOL(batt_block_charging_set);

struct batt_info batt_info_buf;
/* LGE_CHANGES_E [woonghee.park@lge.com]*/
// END:  0010098 hyunjong.do@lge.com 2010-10-21

#ifndef CONFIG_BATTERY_MSM_FAKE
struct msm_batt_get_volt_ret_data {
	u32 battery_voltage;
};

static int msm_batt_get_volt_ret_func(struct msm_rpc_client *batt_client,
				       void *buf, void *data)
{
	struct msm_batt_get_volt_ret_data *data_ptr, *buf_ptr;

	data_ptr = (struct msm_batt_get_volt_ret_data *)data;
	buf_ptr = (struct msm_batt_get_volt_ret_data *)buf;

	data_ptr->battery_voltage = be32_to_cpu(buf_ptr->battery_voltage);

	return 0;
}

static u32 msm_batt_get_vbatt_voltage(void)
{
	int rc;

	struct msm_batt_get_volt_ret_data rep;

	rc = msm_rpc_client_req(msm_batt_info.batt_client,
			BATTERY_READ_MV_PROC,
			NULL, NULL,
			msm_batt_get_volt_ret_func, &rep,
			msecs_to_jiffies(BATT_RPC_TIMEOUT));

	if (rc < 0) {
		pr_err("%s: FAIL: vbatt get volt. rc=%d\n", __func__, rc);
		return 0;
	}

	return rep.battery_voltage;
}

#define	be32_to_cpu_self(v)	(v = be32_to_cpu(v))

static int msm_batt_get_batt_chg_status(void)
{
	int rc;

	struct rpc_req_batt_chg {
		struct rpc_request_hdr hdr;
		u32 more_data;
	} req_batt_chg;
	struct rpc_reply_batt_chg_v1 *v1p;

	req_batt_chg.more_data = cpu_to_be32(1);

	memset(&rep_batt_chg, 0, sizeof(rep_batt_chg));

	v1p = &rep_batt_chg.v1;
	rc = msm_rpc_call_reply(msm_batt_info.chg_ep,
				ONCRPC_CHG_GET_GENERAL_STATUS_PROC,
				&req_batt_chg, sizeof(req_batt_chg),
				&rep_batt_chg, sizeof(rep_batt_chg),
				msecs_to_jiffies(BATT_RPC_TIMEOUT));
	if (rc < 0) {
		pr_err("%s: ERROR. msm_rpc_call_reply failed! proc=%d rc=%d\n",
		       __func__, ONCRPC_CHG_GET_GENERAL_STATUS_PROC, rc);
		return rc;
	} else if (be32_to_cpu(v1p->more_data)) {
		be32_to_cpu_self(v1p->charger_status);
		be32_to_cpu_self(v1p->charger_type);
		be32_to_cpu_self(v1p->battery_status);
		be32_to_cpu_self(v1p->battery_level);
		be32_to_cpu_self(v1p->battery_voltage);
		be32_to_cpu_self(v1p->battery_temp);
/* CONFIG_MACH_LGE_BRYCE hyunjong.do@lge.com 10.08.10
   */
		be32_to_cpu_self(v1p->battery_soc); // bart		
#ifdef CONFIG_LGE_BATTERY_ID
		be32_to_cpu_self(v1p->battery_id); // bart	
#else
/* LGE_CHANGE_S [jaeho.cho@lge.com] 2010-11-01, check battery thermistor is valid or not */
        be32_to_cpu_self(v1p->batt_thm_valid);
/* LGE_CHANGE_S [jaeho.cho@lge.com] 2010-11-01, check battery thermistor is valid or not */
#endif
		be32_to_cpu_self(v1p->battery_therm); // bart		
/* LGE_CHANGE_S [jaeho.cho@lge.com] 2010-10-19, workaround to fix unexpected charging without ext_pwr */
#ifdef CONFIG_LGE_CHARGING
        be32_to_cpu_self(v1p->ext_pwr); // bart		
#endif
/* LGE_CHANGE_S [jaeho.cho@lge.com] 2010-10-19, workaround to fix unexpected charging without ext_pwr */
	} else {
		pr_err("%s: No battery/charger data in RPC reply\n", __func__);
		return -EIO;
	}

	return 0;
}

static void msm_batt_update_psy_status(void)
{
	static u32 unnecessary_event_count;
	u32	charger_status;
	u32	charger_type;
	u32	battery_status;
	u32	battery_level;
	u32     battery_voltage;
	u32	battery_temp;
// BEGIN: 0010099 hyunjong.do@lge.com 2010-10-21
// ADD: 0010099: [Power] Charging current control by thermister on Kernel
#ifdef CONFIG_LGE_BATTERY_ID
	u32 battery_id;
#else
    u32 batt_thm_valid;
#endif
	u32 battery_therm;
// END:  0010099 hyunjong.do@lge.com 2010-10-21
#ifdef CONFIG_LGE_CHARGING
    u32 ext_pwr;
#endif
	struct	power_supply	*supp;
/* CONFIG_MACH_LGE_BRYCE hyunjong.do@lge.com 10.08.10
   */
#if CONFIG_MACH_LGE_BRYCE
    u32 battery_soc; // bart
#endif

#if CONFIG_MACH_LGE_BRYCE
	if (msm_batt_get_batt_chg_status())
	{
		DBG_LIMIT("BATT: msm_batt_get_batt_chg_status() read fail\n");
	
		return;
	}
	else 
		DBG_LIMIT("BATT: msm_batt_get_batt_chg_status() read success \n");
#else
	if (msm_batt_get_batt_chg_status())
		return;
#endif

	charger_status = rep_batt_chg.v1.charger_status;
	charger_type = rep_batt_chg.v1.charger_type;
	battery_status = rep_batt_chg.v1.battery_status;
	battery_level = rep_batt_chg.v1.battery_level;
	battery_voltage = rep_batt_chg.v1.battery_voltage;
	battery_temp = rep_batt_chg.v1.battery_temp;
// BEGIN: 0010099 hyunjong.do@lge.com 2010-10-21
// ADD: 0010099: [Power] Charging current control by thermister on Kernel
	battery_temp = battery_temp * 10 ;
#ifdef CONFIG_LGE_BATTERY_ID
	battery_id=rep_batt_chg.v1.battery_id;
#else
    batt_thm_valid=rep_batt_chg.v1.batt_thm_valid;
#endif
	battery_therm=rep_batt_chg.v1.battery_therm;
	ext_pwr = rep_batt_chg.v1.ext_pwr;
	battery_soc = rep_batt_chg.v1.battery_soc; // bart 
//	if (battery_soc < 1) 
//		battery_soc = 1 ; // don't need this, comment out // 2010.09.24
//	DBG_LIMIT("BATT: charger_status from modem through RPC.\n" ); 
	DBG_LIMIT("BATT: charger_status  = [%d] \n",charger_status ); 
	DBG_LIMIT("BATT: charger_type   = [%d]\n", charger_type ); 
	DBG_LIMIT("BATT: battery_status  = [%d]\n", battery_status); 
	DBG_LIMIT("BATT: battery_level  = [%d]\n",battery_level ); 
	DBG_LIMIT("BATT: battery_voltage  = [%d]\n", battery_voltage); 
	DBG_LIMIT("BATT: battery_temp   = [%d]\n", battery_temp); 
	DBG_LIMIT("BATT: battery_soc    = [%d]%%\n", battery_soc);  // bart
//	DBG_LIMIT("BATT: msm_batt_info.battery_soc  = [%d]%%\n", msm_batt_info.battery_soc);  // bart
// BEGIN: 0010099 hyunjong.do@lge.com 2010-10-21
// ADD: 0010099: [Power] Charging current control by thermister on Kernel
#ifdef CONFIG_LGE_BATTERY_ID
	DBG_LIMIT("BATT: battery_id   = [%d]\n", battery_id); 
#else
    DBG_LIMIT("BATT: batt_thm_valid   = [%d]\n", batt_thm_valid);
#endif
	DBG_LIMIT("BATT: battery_therm   = [%d]\n", battery_therm); 
// END:  0010099 hyunjong.do@lge.com 2010-10-21

	/* Make correction for battery status */
	if (battery_status == BATTERY_STATUS_INVALID_v1) {
		if (msm_batt_info.chg_api_version < CHG_RPC_VER_3_1)
			battery_status = BATTERY_STATUS_INVALID;
	}

	if (charger_status == msm_batt_info.charger_status &&
	    charger_type == msm_batt_info.charger_type &&
	    battery_status == msm_batt_info.battery_status &&
	    battery_level == msm_batt_info.battery_level &&
/* CONFIG_MACH_LGE_BRYCE hyunjong.do@lge.com 10.08.10
   */
#if CONFIG_MACH_LGE_BRYCE
	    battery_soc == msm_batt_info.battery_soc && // bart
#endif
	    battery_voltage == msm_batt_info.battery_voltage &&
	    battery_temp == msm_batt_info.battery_temp) {
		/* Got unnecessary event from Modem PMIC VBATT driver.
		 * Nothing changed in Battery or charger status.
		 */
		unnecessary_event_count++;
		if ((unnecessary_event_count % 20) == 1)
			DBG_LIMIT("BATT: same event count = %u\n",
				 unnecessary_event_count);
		return;
	}

	unnecessary_event_count = 0;

	DBG_LIMIT("BATT: rcvd: %d, %d, %d, %d; %d, %d\n",
		 charger_status, charger_type, battery_status,
		 battery_level, battery_voltage, battery_temp);

	if (battery_status == BATTERY_STATUS_INVALID &&
	    battery_level != BATTERY_LEVEL_INVALID) {
		DBG_LIMIT("BATT: change status(%d) to (%d) for level=%d\n",
			 battery_status, BATTERY_STATUS_GOOD, battery_level);
		battery_status = BATTERY_STATUS_GOOD;
	}

	if ((msm_batt_info.charger_type != charger_type)) {
		
// BEGIN: 0014492: hyunjong.do@lge.com 2011-01-20
// MOD: 0014492: distinguish AC/USB when charger inserted 
		if (/*charger_type == CHARGER_TYPE_USB_WALL ||*/
//		if (charger_type == CHARGER_TYPE_USB_WALL ||
// END: 0014492: hyunjong.do@lge.com 2011-01-20
		    charger_type == CHARGER_TYPE_USB_PC ||
		    charger_type == CHARGER_TYPE_USB_CARKIT) {
/* CONFIG_MACH_LGE_BRYCE hyunjong.do@lge.com 10.09.21
  get specific charger type */
#if CONFIG_MACH_LGE_BRYCE
			DBG_LIMIT("BATT: USB charger plugged in as type of [%d]\n",charger_type);
#else   
			DBG_LIMIT("BATT: USB charger plugged in\n");
#endif
			msm_batt_info.current_chg_source = USB_CHG;
			supp = &msm_psy_usb;
// BEGIN: 0014492: hyunjong.do@lge.com 2011-01-20
// MOD: 0014492: distinguish AC/USB when charger inserted 
		} else if (charger_type == CHARGER_TYPE_WALL||charger_type == CHARGER_TYPE_USB_WALL) {
//		} else if (charger_type == CHARGER_TYPE_WALL) {
// END: 0014492: hyunjong.do@lge.com 2011-01-20
			DBG_LIMIT("BATT: AC Wall changer plugged in\n");
			msm_batt_info.current_chg_source = AC_CHG;
			supp = &msm_psy_ac;
		} else {
			if (msm_batt_info.current_chg_source & AC_CHG)
				DBG_LIMIT("BATT: AC Wall charger removed\n");
			else if (msm_batt_info.current_chg_source & USB_CHG)
				DBG_LIMIT("BATT: USB charger removed\n");
			else
				DBG_LIMIT("BATT: No charger present\n");
			msm_batt_info.current_chg_source = 0;
			supp = &msm_psy_batt;

			/* Correct charger status */
			if (charger_status != CHARGER_STATUS_INVALID) {
				DBG_LIMIT("BATT: No charging!\n");
				charger_status = CHARGER_STATUS_INVALID;
				msm_batt_info.batt_status =
					POWER_SUPPLY_STATUS_NOT_CHARGING;
			}
		}
	} else
		supp = NULL;

	if (msm_batt_info.charger_status != charger_status) {
		if (charger_status == CHARGER_STATUS_GOOD ||
		    charger_status == CHARGER_STATUS_WEAK) {
			if (msm_batt_info.current_chg_source) {
				DBG_LIMIT("BATT: Charging.\n");

// BEGIN: 0018132: hyunjong.do@lge.com 2011-03-17
// ADD: 0018132: returns battery full status
				if ( battery_soc >= 100)
					msm_batt_info.batt_status =
						POWER_SUPPLY_STATUS_FULL;
				else
// END: 0018132: hyunjong.do@lge.com 2011-03-17
					msm_batt_info.batt_status =
						POWER_SUPPLY_STATUS_CHARGING;

				/* Correct when supp==NULL */
				if (msm_batt_info.current_chg_source & AC_CHG)
					supp = &msm_psy_ac;
				else
					supp = &msm_psy_usb;
			}
		} else {
			DBG_LIMIT("BATT: No charging.\n");
			msm_batt_info.batt_status =
				POWER_SUPPLY_STATUS_NOT_CHARGING;
			supp = &msm_psy_batt;
		}
	} else {
		/* Correct charger status */
/* CONFIG_MACH_LGE_BRYCE hyunjong.do@lge.com 10.09.21
  in case when charge type is NONE  */
#if CONFIG_MACH_LGE_BRYCE
		if ( (charger_type != CHARGER_TYPE_INVALID && charger_type != CHARGER_TYPE_NONE) &&
#else	
		if (charger_type != CHARGER_TYPE_INVALID &&
#endif
		    charger_status == CHARGER_STATUS_GOOD) {
			DBG_LIMIT("BATT: In charging\n");
// BEGIN: 0018132: hyunjong.do@lge.com 2011-03-17
// ADD: 0018132: returns battery full status
			if ( battery_soc >= 100)
				msm_batt_info.batt_status =
					POWER_SUPPLY_STATUS_FULL;
			else
// END: 0018132: hyunjong.do@lge.com 2011-03-17
				msm_batt_info.batt_status =
					POWER_SUPPLY_STATUS_CHARGING;
		}
	}

	/* Correct battery voltage and status */
	if (!battery_voltage) {
		if (charger_status == CHARGER_STATUS_INVALID) {
			DBG_LIMIT("BATT: Read VBATT\n");
			battery_voltage = msm_batt_get_vbatt_voltage();
		} else
		{
			/* Use previous */
			battery_voltage = msm_batt_info.battery_voltage;
/* CONFIG_MACH_LGE_BRYCE hyunjong.do@lge.com 10.08.10
   */
#if CONFIG_MACH_LGE_BRYCE
			battery_soc= msm_batt_info.battery_soc;
#endif
		}
		
/* CONFIG_MACH_LGE_BRYCE hyunjong.do@lge.com 10.08.10
  */
#if CONFIG_MACH_LGE_BRYCE
		
		DBG_LIMIT("BATT: Read VBATT=%dmV and SOC=%d%%\n",battery_voltage,battery_soc);
#endif
	}
	if (battery_status == BATTERY_STATUS_INVALID) {
// BEGIN: 0010099 hyunjong.do@lge.com 2010-10-21
// ADD: 0010099: [Power] Charging current control by thermister on Kernel
		if (battery_level != BATTERY_LEVEL_INVALID) 
// END:  0010099 hyunjong.do@lge.com 2010-10-21
		if (battery_voltage >= msm_batt_info.voltage_min_design &&
		    battery_voltage <= msm_batt_info.voltage_max_design) {
			DBG_LIMIT("BATT: Battery valid\n");

/* CONFIG_MACH_LGE_BRYCE hyunjong.do@lge.com 10.08.10
   */
#if CONFIG_MACH_LGE_BRYCE
			if (battery_soc)
			{
				msm_batt_info.batt_valid = 1;
				battery_status = BATTERY_STATUS_GOOD;
			}
#endif
		}
	}
	if (msm_batt_info.battery_status != battery_status) {
		if (battery_status != BATTERY_STATUS_INVALID) {
			msm_batt_info.batt_valid = 1;

			if (battery_status == BATTERY_STATUS_BAD) {
				DBG_LIMIT("BATT: Battery bad.\n");
				msm_batt_info.batt_health =
					POWER_SUPPLY_HEALTH_DEAD;
			} else if (battery_status == BATTERY_STATUS_BAD_TEMP) {
				DBG_LIMIT("BATT: Battery overheat.\n");
				msm_batt_info.batt_health =
					POWER_SUPPLY_HEALTH_OVERHEAT;
			} else {
				DBG_LIMIT("BATT: Battery good.\n");
				msm_batt_info.batt_health =
					POWER_SUPPLY_HEALTH_GOOD;
			}
		} else {
			msm_batt_info.batt_valid = 0;
			DBG_LIMIT("BATT: Battery invalid.\n");
			msm_batt_info.batt_health = POWER_SUPPLY_HEALTH_UNKNOWN;
		}

// BEGIN: 0018132: hyunjong.do@lge.com 2011-03-17
// MOD: 0018132: returns battery full status
		if (msm_batt_info.batt_status != POWER_SUPPLY_STATUS_CHARGING && msm_batt_info.batt_status !=POWER_SUPPLY_STATUS_FULL) {
// END: 0018132: hyunjong.do@lge.com 2011-03-17
			if (battery_status == BATTERY_STATUS_INVALID) {
				DBG_LIMIT("BATT: Battery -> unknown\n");
				msm_batt_info.batt_status =
					POWER_SUPPLY_STATUS_UNKNOWN;
			} else {
				DBG_LIMIT("BATT: Battery -> discharging\n");
				msm_batt_info.batt_status =
					POWER_SUPPLY_STATUS_DISCHARGING;
			}
		}

		if (!supp) {
			if (msm_batt_info.current_chg_source) {
				if (msm_batt_info.current_chg_source & AC_CHG)
					supp = &msm_psy_ac;
				else
					supp = &msm_psy_usb;
			} else
				supp = &msm_psy_batt;
		}
	}
	
	msm_batt_info.charger_status 	= charger_status;
	msm_batt_info.charger_type 	= charger_type;
	msm_batt_info.battery_status 	= battery_status;
	msm_batt_info.battery_level 	= battery_level;
	msm_batt_info.battery_temp 	= battery_temp;

// BEGIN: 0010099 hyunjong.do@lge.com 2010-10-21
// ADD: 0010099: [Power] Charging current control by thermister on Kernel
#ifdef CONFIG_LGE_BATTERY_ID
	msm_batt_info.valid_battery_id=battery_id;
#else
/* LGE_CHANGE_S [jaeho.cho@lge.com] 2010-11-01, check battery thermistor is valid or not */
    msm_batt_info.batt_thm_valid = rep_batt_chg.v1.batt_thm_valid;
/* LGE_CHANGE_S [jaeho.cho@lge.com] 2010-11-01, check battery thermistor is valid or not */
#endif
	msm_batt_info.battery_therm=battery_therm;
// END:  0010099 hyunjong.do@lge.com 2010-10-21
/* LGE_CHANGE_S [jaeho.cho@lge.com] 2010-10-19, workaround to fix unexpected charging without ext_pwr */
#ifdef CONFIG_LGE_CHARGING
	msm_batt_info.ext_pwr = rep_batt_chg.v1.ext_pwr;
#endif
/* LGE_CHANGE_S [jaeho.cho@lge.com] 2010-10-19, workaround to fix unexpected charging without ext_pwr */

	if (msm_batt_info.battery_voltage != battery_voltage
/* CONFIG_MACH_LGE_BRYCE hyunjong.do@lge.com 10.08.10
   */
#if CONFIG_MACH_LGE_BRYCE
	|| msm_batt_info.battery_soc != battery_soc
#endif
		 ) 
	{
		msm_batt_info.battery_voltage  	= battery_voltage;
		msm_batt_info.batt_capacity =
			msm_batt_info.calculate_capacity(battery_voltage);
/* CONFIG_MACH_LGE_BRYCE hyunjong.do@lge.com 10.08.10
   */
#if CONFIG_MACH_LGE_BRYCE
		msm_batt_info.batt_capacity = battery_soc ;  // bart
		msm_batt_info.battery_soc = battery_soc ;  // bart
#endif		
		DBG_LIMIT("BATT: voltage = %u mV [capacity = %d%%]\n",
			 battery_voltage, msm_batt_info.batt_capacity);

		if (!supp)
			supp = msm_batt_info.current_ps;
	}

	if (supp) {
		msm_batt_info.current_ps = supp;
		DBG_LIMIT("BATT: Supply = %s\n", supp->name);
		power_supply_changed(supp);
	}
}
#ifdef CONFIG_LGE_CHARGING
extern void set_operation_mode(boolean info);
static ssize_t msm_batt_modem_lpm_store(struct device *dev, struct device_attribute *attr,
		                 const char *buf, size_t count)
{
	int ret = -EINVAL;
	int online;

	if (sscanf(buf, "%d", &online) != 1) {
		dev_err(dev, "%s: usage: echo [0/1] > modem_lpm", __func__);
		return ret;
	}

	set_operation_mode(online);

	ret = count;
	return ret;
}

static DEVICE_ATTR(modem_lpm, 0220, NULL, msm_batt_modem_lpm_store);

extern unsigned lge_get_power_on_status(void);
static ssize_t msm_batt_power_on_status_show(struct device *dev, 
		struct device_attribute *attr, char *buf)
{
	unsigned power_on_status = 0;

	power_on_status = lge_get_power_on_status();
	dev_info(dev, "%s : Power On Status (%x)\n", __func__, power_on_status);

	return sprintf(buf, "0x%x\n", power_on_status);
}
static DEVICE_ATTR(power_on_status, 0444, msm_batt_power_on_status_show, NULL);
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
struct batt_modify_client_req {

	u32 client_handle;

	/* The voltage at which callback (CB) should be called. */
	u32 desired_batt_voltage;

	/* The direction when the CB should be called. */
	u32 voltage_direction;

	/* The registered callback to be called when voltage and
	 * direction specs are met. */
	u32 batt_cb_id;

	/* The call back data */
	u32 cb_data;
};

struct batt_modify_client_rep {
	u32 result;
};

static int msm_batt_modify_client_arg_func(struct msm_rpc_client *batt_client,
				       void *buf, void *data)
{
	struct batt_modify_client_req *batt_modify_client_req =
		(struct batt_modify_client_req *)data;
	u32 *req = (u32 *)buf;
	int size = 0;

	*req = cpu_to_be32(batt_modify_client_req->client_handle);
	size += sizeof(u32);
	req++;

	*req = cpu_to_be32(batt_modify_client_req->desired_batt_voltage);
	size += sizeof(u32);
	req++;

	*req = cpu_to_be32(batt_modify_client_req->voltage_direction);
	size += sizeof(u32);
	req++;

	*req = cpu_to_be32(batt_modify_client_req->batt_cb_id);
	size += sizeof(u32);
	req++;

	*req = cpu_to_be32(batt_modify_client_req->cb_data);
	size += sizeof(u32);

	return size;
}

static int msm_batt_modify_client_ret_func(struct msm_rpc_client *batt_client,
				       void *buf, void *data)
{
	struct  batt_modify_client_rep *data_ptr, *buf_ptr;

	data_ptr = (struct batt_modify_client_rep *)data;
	buf_ptr = (struct batt_modify_client_rep *)buf;

	data_ptr->result = be32_to_cpu(buf_ptr->result);

	return 0;
}

static int msm_batt_modify_client(u32 client_handle, u32 desired_batt_voltage,
	     u32 voltage_direction, u32 batt_cb_id, u32 cb_data)
{
	int rc;

	struct batt_modify_client_req  req;
	struct batt_modify_client_rep rep;

	req.client_handle = client_handle;
	req.desired_batt_voltage = desired_batt_voltage;
	req.voltage_direction = voltage_direction;
	req.batt_cb_id = batt_cb_id;
	req.cb_data = cb_data;

	rc = msm_rpc_client_req(msm_batt_info.batt_client,
			BATTERY_MODIFY_CLIENT_PROC,
			msm_batt_modify_client_arg_func, &req,
			msm_batt_modify_client_ret_func, &rep,
			msecs_to_jiffies(BATT_RPC_TIMEOUT));

	if (rc < 0) {
		pr_err("%s: ERROR. failed to modify  Vbatt client\n",
		       __func__);
		return rc;
	}

	if (rep.result != BATTERY_MODIFICATION_SUCCESSFUL) {
		pr_err("%s: ERROR. modify client failed. result = %u\n",
		       __func__, rep.result);
		return -EIO;
	}

	return 0;
}

void msm_batt_early_suspend(struct early_suspend *h)
{
	int rc;

	pr_debug("%s: enter\n", __func__);

	if (msm_batt_info.batt_handle != INVALID_BATT_HANDLE) {
		rc = msm_batt_modify_client(msm_batt_info.batt_handle,
				BATTERY_LOW, BATTERY_VOLTAGE_BELOW_THIS_LEVEL,
				BATTERY_CB_ID_LOW_VOL, BATTERY_LOW);

		if (rc < 0) {
			pr_err("%s: msm_batt_modify_client. rc=%d\n",
			       __func__, rc);
			return;
		}
	} else {
		pr_err("%s: ERROR. invalid batt_handle\n", __func__);
		return;
	}

	pr_debug("%s: exit\n", __func__);
}

void msm_batt_late_resume(struct early_suspend *h)
{
	int rc;

	pr_debug("%s: enter\n", __func__);

	if (msm_batt_info.batt_handle != INVALID_BATT_HANDLE) {
		rc = msm_batt_modify_client(msm_batt_info.batt_handle,
				BATTERY_LOW, BATTERY_ALL_ACTIVITY,
			       BATTERY_CB_ID_ALL_ACTIV, BATTERY_ALL_ACTIVITY);
		if (rc < 0) {
			pr_err("%s: msm_batt_modify_client FAIL rc=%d\n",
			       __func__, rc);
			return;
		}
	} else {
		pr_err("%s: ERROR. invalid batt_handle\n", __func__);
		return;
	}

/* CONFIG_MACH_LGE_BRYCE hyunjong.do@lge.com 10.08.10
   */
#if CONFIG_MACH_LGE_BRYCE
		DBG_LIMIT("BATT: msm_batt_info.chg_api_version = [0x%x] \n",msm_batt_info.chg_api_version);
			msm_batt_update_psy_status();
#endif
	
	pr_debug("%s: exit\n", __func__);
}
#endif

struct msm_batt_vbatt_filter_req {
	u32 batt_handle;
	u32 enable_filter;
	u32 vbatt_filter;
};

struct msm_batt_vbatt_filter_rep {
	u32 result;
};

static int msm_batt_filter_arg_func(struct msm_rpc_client *batt_client,

		void *buf, void *data)
{
	struct msm_batt_vbatt_filter_req *vbatt_filter_req =
		(struct msm_batt_vbatt_filter_req *)data;
	u32 *req = (u32 *)buf;
	int size = 0;

	*req = cpu_to_be32(vbatt_filter_req->batt_handle);
	size += sizeof(u32);
	req++;

	*req = cpu_to_be32(vbatt_filter_req->enable_filter);
	size += sizeof(u32);
	req++;

	*req = cpu_to_be32(vbatt_filter_req->vbatt_filter);
	size += sizeof(u32);
	return size;
}

static int msm_batt_filter_ret_func(struct msm_rpc_client *batt_client,
				       void *buf, void *data)
{

	struct msm_batt_vbatt_filter_rep *data_ptr, *buf_ptr;

	data_ptr = (struct msm_batt_vbatt_filter_rep *)data;
	buf_ptr = (struct msm_batt_vbatt_filter_rep *)buf;

	data_ptr->result = be32_to_cpu(buf_ptr->result);
	return 0;
}

static int msm_batt_enable_filter(u32 vbatt_filter)
{
	int rc;
	struct  msm_batt_vbatt_filter_req  vbatt_filter_req;
	struct  msm_batt_vbatt_filter_rep  vbatt_filter_rep;

	vbatt_filter_req.batt_handle = msm_batt_info.batt_handle;
	vbatt_filter_req.enable_filter = 1;
	vbatt_filter_req.vbatt_filter = vbatt_filter;

	rc = msm_rpc_client_req(msm_batt_info.batt_client,
			BATTERY_ENABLE_DISABLE_FILTER_PROC,
			msm_batt_filter_arg_func, &vbatt_filter_req,
			msm_batt_filter_ret_func, &vbatt_filter_rep,
			msecs_to_jiffies(BATT_RPC_TIMEOUT));

	if (rc < 0) {
		pr_err("%s: FAIL: enable vbatt filter. rc=%d\n",
		       __func__, rc);
		return rc;
	}

	if (vbatt_filter_rep.result != BATTERY_DEREGISTRATION_SUCCESSFUL) {
		pr_err("%s: FAIL: enable vbatt filter: result=%d\n",
		       __func__, vbatt_filter_rep.result);
		return -EIO;
	}

	pr_debug("%s: enable vbatt filter: OK\n", __func__);
	return rc;
}

struct batt_client_registration_req {
	/* The voltage at which callback (CB) should be called. */
	u32 desired_batt_voltage;

	/* The direction when the CB should be called. */
	u32 voltage_direction;

	/* The registered callback to be called when voltage and
	 * direction specs are met. */
	u32 batt_cb_id;

	/* The call back data */
	u32 cb_data;
	u32 more_data;
	u32 batt_error;
};

struct batt_client_registration_req_4_1 {
	/* The voltage at which callback (CB) should be called. */
	u32 desired_batt_voltage;

	/* The direction when the CB should be called. */
	u32 voltage_direction;

	/* The registered callback to be called when voltage and
	 * direction specs are met. */
	u32 batt_cb_id;

	/* The call back data */
	u32 cb_data;
	u32 batt_error;
};

struct batt_client_registration_rep {
	u32 batt_handle;
};

struct batt_client_registration_rep_4_1 {
	u32 batt_handle;
	u32 more_data;
	u32 err;
};

static int msm_batt_register_arg_func(struct msm_rpc_client *batt_client,
				       void *buf, void *data)
{
	struct batt_client_registration_req *batt_reg_req =
		(struct batt_client_registration_req *)data;

	u32 *req = (u32 *)buf;
	int size = 0;


	if (msm_batt_info.batt_api_version == BATTERY_RPC_VER_4_1) {
		*req = cpu_to_be32(batt_reg_req->desired_batt_voltage);
		size += sizeof(u32);
		req++;

		*req = cpu_to_be32(batt_reg_req->voltage_direction);
		size += sizeof(u32);
		req++;

		*req = cpu_to_be32(batt_reg_req->batt_cb_id);
		size += sizeof(u32);
		req++;

		*req = cpu_to_be32(batt_reg_req->cb_data);
		size += sizeof(u32);
		req++;

		*req = cpu_to_be32(batt_reg_req->batt_error);
		size += sizeof(u32);

		return size;
	} else {
		*req = cpu_to_be32(batt_reg_req->desired_batt_voltage);
		size += sizeof(u32);
		req++;

		*req = cpu_to_be32(batt_reg_req->voltage_direction);
		size += sizeof(u32);
		req++;

		*req = cpu_to_be32(batt_reg_req->batt_cb_id);
		size += sizeof(u32);
		req++;

		*req = cpu_to_be32(batt_reg_req->cb_data);
		size += sizeof(u32);
		req++;

		*req = cpu_to_be32(batt_reg_req->more_data);
		size += sizeof(u32);
		req++;

		*req = cpu_to_be32(batt_reg_req->batt_error);
		size += sizeof(u32);

		return size;
	}

}

static int msm_batt_register_ret_func(struct msm_rpc_client *batt_client,
				       void *buf, void *data)
{
	struct batt_client_registration_rep *data_ptr, *buf_ptr;
	struct batt_client_registration_rep_4_1 *data_ptr_4_1, *buf_ptr_4_1;

	if (msm_batt_info.batt_api_version == BATTERY_RPC_VER_4_1) {
		data_ptr_4_1 = (struct batt_client_registration_rep_4_1 *)data;
		buf_ptr_4_1 = (struct batt_client_registration_rep_4_1 *)buf;

		data_ptr_4_1->batt_handle
			= be32_to_cpu(buf_ptr_4_1->batt_handle);
		data_ptr_4_1->more_data
			= be32_to_cpu(buf_ptr_4_1->more_data);
		data_ptr_4_1->err = be32_to_cpu(buf_ptr_4_1->err);
		return 0;
	} else {
		data_ptr = (struct batt_client_registration_rep *)data;
		buf_ptr = (struct batt_client_registration_rep *)buf;

		data_ptr->batt_handle = be32_to_cpu(buf_ptr->batt_handle);
		return 0;
	}
}

static int msm_batt_register(u32 desired_batt_voltage,
			     u32 voltage_direction, u32 batt_cb_id, u32 cb_data)
{
	struct batt_client_registration_req batt_reg_req;
	struct batt_client_registration_req_4_1 batt_reg_req_4_1;
	struct batt_client_registration_rep batt_reg_rep;
	struct batt_client_registration_rep_4_1 batt_reg_rep_4_1;
	void *request;
	void *reply;
	int rc;

	if (msm_batt_info.batt_api_version == BATTERY_RPC_VER_4_1) {
		batt_reg_req_4_1.desired_batt_voltage = desired_batt_voltage;
		batt_reg_req_4_1.voltage_direction = voltage_direction;
		batt_reg_req_4_1.batt_cb_id = batt_cb_id;
		batt_reg_req_4_1.cb_data = cb_data;
		batt_reg_req_4_1.batt_error = 1;
		request = &batt_reg_req_4_1;
	} else {
		batt_reg_req.desired_batt_voltage = desired_batt_voltage;
		batt_reg_req.voltage_direction = voltage_direction;
		batt_reg_req.batt_cb_id = batt_cb_id;
		batt_reg_req.cb_data = cb_data;
		batt_reg_req.more_data = 1;
		batt_reg_req.batt_error = 0;
		request = &batt_reg_req;
	}

	if (msm_batt_info.batt_api_version == BATTERY_RPC_VER_4_1)
		reply = &batt_reg_rep_4_1;
	else
		reply = &batt_reg_rep;

	rc = msm_rpc_client_req(msm_batt_info.batt_client,
			BATTERY_REGISTER_PROC,
			msm_batt_register_arg_func, request,
			msm_batt_register_ret_func, reply,
			msecs_to_jiffies(BATT_RPC_TIMEOUT));

	if (rc < 0) {
		pr_err("%s: FAIL: vbatt register. rc=%d\n", __func__, rc);
		return rc;
	}

	if (msm_batt_info.batt_api_version == BATTERY_RPC_VER_4_1) {
		if (batt_reg_rep_4_1.more_data != 0
			&& batt_reg_rep_4_1.err
				!= BATTERY_REGISTRATION_SUCCESSFUL) {
			pr_err("%s: vBatt Registration Failed proc_num=%d\n"
					, __func__, BATTERY_REGISTER_PROC);
			return -EIO;
		}
		msm_batt_info.batt_handle = batt_reg_rep_4_1.batt_handle;
	} else
		msm_batt_info.batt_handle = batt_reg_rep.batt_handle;

	return 0;
}

struct batt_client_deregister_req {
	u32 batt_handle;
};

struct batt_client_deregister_rep {
	u32 batt_error;
};

static int msm_batt_deregister_arg_func(struct msm_rpc_client *batt_client,
				       void *buf, void *data)
{
	struct batt_client_deregister_req *deregister_req =
		(struct  batt_client_deregister_req *)data;
	u32 *req = (u32 *)buf;
	int size = 0;

	*req = cpu_to_be32(deregister_req->batt_handle);
	size += sizeof(u32);

	return size;
}

static int msm_batt_deregister_ret_func(struct msm_rpc_client *batt_client,
				       void *buf, void *data)
{
	struct batt_client_deregister_rep *data_ptr, *buf_ptr;

	data_ptr = (struct batt_client_deregister_rep *)data;
	buf_ptr = (struct batt_client_deregister_rep *)buf;

	data_ptr->batt_error = be32_to_cpu(buf_ptr->batt_error);

	return 0;
}

static int msm_batt_deregister(u32 batt_handle)
{
	int rc;
	struct batt_client_deregister_req req;
	struct batt_client_deregister_rep rep;

	req.batt_handle = batt_handle;

	rc = msm_rpc_client_req(msm_batt_info.batt_client,
			BATTERY_DEREGISTER_CLIENT_PROC,
			msm_batt_deregister_arg_func, &req,
			msm_batt_deregister_ret_func, &rep,
			msecs_to_jiffies(BATT_RPC_TIMEOUT));

	if (rc < 0) {
		pr_err("%s: FAIL: vbatt deregister. rc=%d\n", __func__, rc);
		return rc;
	}

	if (rep.batt_error != BATTERY_DEREGISTRATION_SUCCESSFUL) {
		pr_err("%s: vbatt deregistration FAIL. error=%d, handle=%d\n",
		       __func__, rep.batt_error, batt_handle);
		return -EIO;
	}

	return 0;
}
#endif  /* CONFIG_BATTERY_MSM_FAKE */

static int msm_batt_cleanup(void)
{
	int rc = 0;

#ifndef CONFIG_BATTERY_MSM_FAKE
	if (msm_batt_info.batt_handle != INVALID_BATT_HANDLE) {

		rc = msm_batt_deregister(msm_batt_info.batt_handle);
		if (rc < 0)
			pr_err("%s: FAIL: msm_batt_deregister. rc=%d\n",
			       __func__, rc);
	}

	msm_batt_info.batt_handle = INVALID_BATT_HANDLE;

	if (msm_batt_info.batt_client)
		msm_rpc_unregister_client(msm_batt_info.batt_client);
#endif  /* CONFIG_BATTERY_MSM_FAKE */

	if (msm_batt_info.msm_psy_ac)
		power_supply_unregister(msm_batt_info.msm_psy_ac);

	if (msm_batt_info.msm_psy_usb)
		power_supply_unregister(msm_batt_info.msm_psy_usb);
	if (msm_batt_info.msm_psy_batt)
		power_supply_unregister(msm_batt_info.msm_psy_batt);

#ifndef CONFIG_BATTERY_MSM_FAKE
	if (msm_batt_info.chg_ep) {
		rc = msm_rpc_close(msm_batt_info.chg_ep);
		if (rc < 0) {
			pr_err("%s: FAIL. msm_rpc_close(chg_ep). rc=%d\n",
			       __func__, rc);
		}
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
	if (msm_batt_info.early_suspend.suspend == msm_batt_early_suspend)
		unregister_early_suspend(&msm_batt_info.early_suspend);
#endif
#endif
	return rc;
}

static u32 msm_batt_capacity(u32 current_voltage)
{
	u32 low_voltage = msm_batt_info.voltage_min_design;
	u32 high_voltage = msm_batt_info.voltage_max_design;
/* CONFIG_MACH_LGE_BRYCE hyunjong.do@lge.com 10.08.10
   */
#if CONFIG_MACH_LGE_BRYCE
	pr_info("%s: low_voltage = %d  high_voltage = %d\n",__func__,low_voltage,high_voltage); // bart
	pr_info("%s: current_voltage = %d\n",__func__,current_voltage); // bart
	pr_info("%s: Capacity calculated = %d battery_soc=%d\n",__func__,(current_voltage - low_voltage) * 100/ (high_voltage - low_voltage),msm_batt_info.battery_soc); // bart
#endif

	if (current_voltage <= low_voltage)
		return 0;
	else if (current_voltage >= high_voltage)
		return 100;
	else
		return (current_voltage - low_voltage) * 100
			/ (high_voltage - low_voltage);
}

#ifndef CONFIG_BATTERY_MSM_FAKE
int msm_batt_get_charger_api_version(void)
{
	int rc ;
	struct rpc_reply_hdr *reply;

	struct rpc_req_chg_api_ver {
		struct rpc_request_hdr hdr;
		u32 more_data;
	} req_chg_api_ver;

	struct rpc_rep_chg_api_ver {
		struct rpc_reply_hdr hdr;
		u32 num_of_chg_api_versions;
		u32 *chg_api_versions;
	};

	u32 num_of_versions;

	struct rpc_rep_chg_api_ver *rep_chg_api_ver;


	req_chg_api_ver.more_data = cpu_to_be32(1);

	msm_rpc_setup_req(&req_chg_api_ver.hdr, CHG_RPC_PROG, CHG_RPC_VER_1_1,
			  ONCRPC_CHARGER_API_VERSIONS_PROC);

	rc = msm_rpc_write(msm_batt_info.chg_ep, &req_chg_api_ver,
			sizeof(req_chg_api_ver));
	if (rc < 0) {
		pr_err("%s: FAIL: msm_rpc_write. proc=0x%08x, rc=%d\n",
		       __func__, ONCRPC_CHARGER_API_VERSIONS_PROC, rc);
		return rc;
	}

	for (;;) {
		rc = msm_rpc_read(msm_batt_info.chg_ep, (void *) &reply, -1,
				BATT_RPC_TIMEOUT);
		if (rc < 0)
			return rc;
		if (rc < RPC_REQ_REPLY_COMMON_HEADER_SIZE) {
			pr_err("%s: LENGTH ERR: msm_rpc_read. rc=%d (<%d)\n",
			       __func__, rc, RPC_REQ_REPLY_COMMON_HEADER_SIZE);

			rc = -EIO;
			break;
		}
		/* we should not get RPC REQ or call packets -- ignore them */
		if (reply->type == RPC_TYPE_REQ) {
			pr_err("%s: TYPE ERR: type=%d (!=%d)\n",
			       __func__, reply->type, RPC_TYPE_REQ);
			kfree(reply);
			continue;
		}

		/* If an earlier call timed out, we could get the (no
		 * longer wanted) reply for it.	 Ignore replies that
		 * we don't expect
		 */
		if (reply->xid != req_chg_api_ver.hdr.xid) {
			pr_err("%s: XID ERR: xid=%d (!=%d)\n", __func__,
			       reply->xid, req_chg_api_ver.hdr.xid);
			kfree(reply);
			continue;
		}
		if (reply->reply_stat != RPCMSG_REPLYSTAT_ACCEPTED) {
			rc = -EPERM;
			break;
		}
		if (reply->data.acc_hdr.accept_stat !=
				RPC_ACCEPTSTAT_SUCCESS) {
			rc = -EINVAL;
			break;
		}

		rep_chg_api_ver = (struct rpc_rep_chg_api_ver *)reply;

		num_of_versions =
			be32_to_cpu(rep_chg_api_ver->num_of_chg_api_versions);

		rep_chg_api_ver->chg_api_versions =  (u32 *)
			((u8 *) reply + sizeof(struct rpc_reply_hdr) +
			sizeof(rep_chg_api_ver->num_of_chg_api_versions));

		rc = be32_to_cpu(
			rep_chg_api_ver->chg_api_versions[num_of_versions - 1]);

		pr_debug("%s: num_of_chg_api_versions = %u. "
			"The chg api version = 0x%08x\n", __func__,
			num_of_versions, rc);
		break;
	}
	kfree(reply);
	return rc;
}

/* BEGIN: 0015566 jihoon.lee@lge.com 20110207 */
/* ADD 0015566: [Kernel] charging mode check command */
#ifdef CONFIG_LGE_CHARGING_MODE_INFO
extern void remote_set_chg_logo_mode(int info);
#ifdef CONFIG_LGE_DIAGTEST
extern void set_first_booting_chg_mode_status(int status);
#endif
static ssize_t chg_logo_read(struct file *fp, char __user *buf,
				size_t count, loff_t *pos)
{
    return 1;
}

static ssize_t chg_logo_write(struct file *fp, const char __user *buf,
				 size_t count, loff_t *pos)
{
	int ret = -EINVAL;
	int update;

	if (sscanf(buf, "%d", &update) != 1) {
		return ret;
	}

// set testmode chg mode command to notifiy the target is in the charging mode
#ifdef CONFIG_LGE_DIAGTEST
	set_first_booting_chg_mode_status(1);
#endif

	remote_set_chg_logo_mode(update);

	ret = count;
	return ret;
}

static const struct file_operations chg_logo_fops = {
	.owner = THIS_MODULE,
	.read = chg_logo_read,
	.write = chg_logo_write,
};

static struct miscdevice chg_logo_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "chg_logo",
	.fops = &chg_logo_fops,
};
#endif
/* END: 0015566 jihoon.lee@lge.com 20110207 */

static int msm_batt_cb_func(struct msm_rpc_client *client,
			    void *buffer, int in_size)
{
	int rc = 0;
	struct rpc_request_hdr *req;
	u32 procedure;
	u32 accept_status;

	req = (struct rpc_request_hdr *)buffer;
	procedure = be32_to_cpu(req->procedure);

	switch (procedure) {
	case BATTERY_CB_TYPE_PROC:
		accept_status = RPC_ACCEPTSTAT_SUCCESS;
		break;

	default:
		accept_status = RPC_ACCEPTSTAT_PROC_UNAVAIL;
		pr_err("%s: ERROR. procedure (%d) not supported\n",
		       __func__, procedure);
		break;
	}

	msm_rpc_start_accepted_reply(msm_batt_info.batt_client,
			be32_to_cpu(req->xid), accept_status);

	rc = msm_rpc_send_accepted_reply(msm_batt_info.batt_client, 0);
	if (rc)
		pr_err("%s: FAIL: sending reply. rc=%d\n", __func__, rc);

	if (accept_status == RPC_ACCEPTSTAT_SUCCESS)
		msm_batt_update_psy_status();

	return rc;
}
#endif  /* CONFIG_BATTERY_MSM_FAKE */

#ifdef CONFIG_MACH_LGE_BRYCE
static ssize_t pif_show(struct device* dev, struct device_attribute* attr, char* buf)
{
    return sprintf(buf, "%d\n", pif_value);
}

static DEVICE_ATTR(pif, S_IRUGO, pif_show, NULL);

static struct attribute* dev_attrs[] = {
	&dev_attr_pif.attr,
#ifdef CONFIG_LGE_CHARGING
	&dev_attr_modem_lpm.attr,
	&dev_attr_power_on_status.attr,
#endif
	NULL,
};

static struct attribute_group dev_attr_grp = {
	.attrs = dev_attrs,
};
#endif

static int __devinit msm_batt_probe(struct platform_device *pdev)
{
	int rc;
	struct msm_psy_batt_pdata *pdata = pdev->dev.platform_data;

	if (pdev->id != -1) {
		dev_err(&pdev->dev,
			"%s: MSM chipsets Can only support one"
			" battery ", __func__);
		return -EINVAL;
	}

#ifndef CONFIG_BATTERY_MSM_FAKE
	if (pdata->avail_chg_sources & AC_CHG) {
#else
	{
#endif
		rc = power_supply_register(&pdev->dev, &msm_psy_ac);
		if (rc < 0) {
			dev_err(&pdev->dev,
				"%s: power_supply_register failed"
				" rc = %d\n", __func__, rc);
			msm_batt_cleanup();
			return rc;
		}
		msm_batt_info.msm_psy_ac = &msm_psy_ac;
		msm_batt_info.avail_chg_sources |= AC_CHG;
	}

	if (pdata->avail_chg_sources & USB_CHG) {
		rc = power_supply_register(&pdev->dev, &msm_psy_usb);
		if (rc < 0) {
			dev_err(&pdev->dev,
				"%s: power_supply_register failed"
				" rc = %d\n", __func__, rc);
			msm_batt_cleanup();
			return rc;
		}
		msm_batt_info.msm_psy_usb = &msm_psy_usb;
		msm_batt_info.avail_chg_sources |= USB_CHG;
	}

	if (!msm_batt_info.msm_psy_ac && !msm_batt_info.msm_psy_usb) {

		dev_err(&pdev->dev,
			"%s: No external Power supply(AC or USB)"
			"is avilable\n", __func__);
		msm_batt_cleanup();
		return -ENODEV;
	}

	msm_batt_info.voltage_max_design = pdata->voltage_max_design;
	msm_batt_info.voltage_min_design = pdata->voltage_min_design;
	msm_batt_info.batt_technology = pdata->batt_technology;
	msm_batt_info.calculate_capacity = pdata->calculate_capacity;

	if (!msm_batt_info.voltage_min_design)
		msm_batt_info.voltage_min_design = BATTERY_LOW;
	if (!msm_batt_info.voltage_max_design)
		msm_batt_info.voltage_max_design = BATTERY_HIGH;

	if (msm_batt_info.batt_technology == POWER_SUPPLY_TECHNOLOGY_UNKNOWN)
		msm_batt_info.batt_technology = POWER_SUPPLY_TECHNOLOGY_LION;

	if (!msm_batt_info.calculate_capacity)
		msm_batt_info.calculate_capacity = msm_batt_capacity;

	rc = power_supply_register(&pdev->dev, &msm_psy_batt);
	if (rc < 0) {
		dev_err(&pdev->dev, "%s: power_supply_register failed"
			" rc=%d\n", __func__, rc);
		msm_batt_cleanup();
		return rc;
	}
	msm_batt_info.msm_psy_batt = &msm_psy_batt;

#ifndef CONFIG_BATTERY_MSM_FAKE
	rc = msm_batt_register(BATTERY_LOW, BATTERY_ALL_ACTIVITY,
			       BATTERY_CB_ID_ALL_ACTIV, BATTERY_ALL_ACTIVITY);
	if (rc < 0) {
		dev_err(&pdev->dev,
			"%s: msm_batt_register failed rc = %d\n", __func__, rc);
		msm_batt_cleanup();
		return rc;
	}

	rc =  msm_batt_enable_filter(VBATT_FILTER);

	if (rc < 0) {
		dev_err(&pdev->dev,
			"%s: msm_batt_enable_filter failed rc = %d\n",
			__func__, rc);
		msm_batt_cleanup();
		return rc;
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
	msm_batt_info.early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN;
	msm_batt_info.early_suspend.suspend = msm_batt_early_suspend;
	msm_batt_info.early_suspend.resume = msm_batt_late_resume;
	register_early_suspend(&msm_batt_info.early_suspend);
#endif
	msm_batt_update_psy_status();

#else
	power_supply_changed(&msm_psy_ac);
#endif  /* CONFIG_BATTERY_MSM_FAKE */
#ifdef CONFIG_MACH_LGE_BRYCE
	rc = sysfs_create_group(&pdev->dev.kobj, &dev_attr_grp);
	if (rc < 0) {
		printk(KERN_ERR "%s : device create file error!\n", __func__);
	}
	else {
		extern int get_msm_cable_type();
		pif_value = get_msm_cable_type();
		printk(KERN_INFO "kimth: pif cable number %d\n", pif_value);
	}
#endif

	return 0;
}

static int __devexit msm_batt_remove(struct platform_device *pdev)
{
	int rc;
	rc = msm_batt_cleanup();

	if (rc < 0) {
		dev_err(&pdev->dev,
			"%s: msm_batt_cleanup  failed rc=%d\n", __func__, rc);
		return rc;
	}
	return 0;
}

static struct platform_driver msm_batt_driver = {
	.probe = msm_batt_probe,
	.remove = __devexit_p(msm_batt_remove),
	.driver = {
		   .name = "msm-battery",
		   .owner = THIS_MODULE,
		   },
};

static int __devinit msm_batt_init_rpc(void)
{
	int rc;

#ifdef CONFIG_BATTERY_MSM_FAKE
	pr_info("Faking MSM battery\n");
#else

	msm_batt_info.chg_ep =
		msm_rpc_connect_compatible(CHG_RPC_PROG, CHG_RPC_VER_4_1, 0);
	msm_batt_info.chg_api_version =  CHG_RPC_VER_4_1;
	if (msm_batt_info.chg_ep == NULL) {
		pr_err("%s: rpc connect CHG_RPC_PROG = NULL\n", __func__);
		return -ENODEV;
	}

	if (IS_ERR(msm_batt_info.chg_ep)) {
		msm_batt_info.chg_ep = msm_rpc_connect_compatible(
				CHG_RPC_PROG, CHG_RPC_VER_3_1, 0);
		msm_batt_info.chg_api_version =  CHG_RPC_VER_3_1;
	}
	if (IS_ERR(msm_batt_info.chg_ep)) {
		msm_batt_info.chg_ep = msm_rpc_connect_compatible(
				CHG_RPC_PROG, CHG_RPC_VER_1_1, 0);
		msm_batt_info.chg_api_version =  CHG_RPC_VER_1_1;
	}
	if (IS_ERR(msm_batt_info.chg_ep)) {
		msm_batt_info.chg_ep = msm_rpc_connect_compatible(
				CHG_RPC_PROG, CHG_RPC_VER_1_3, 0);
		msm_batt_info.chg_api_version =  CHG_RPC_VER_1_3;
	}
	if (IS_ERR(msm_batt_info.chg_ep)) {
		msm_batt_info.chg_ep = msm_rpc_connect_compatible(
				CHG_RPC_PROG, CHG_RPC_VER_2_2, 0);
		msm_batt_info.chg_api_version =  CHG_RPC_VER_2_2;
	}
	if (IS_ERR(msm_batt_info.chg_ep)) {
		rc = PTR_ERR(msm_batt_info.chg_ep);
		pr_err("%s: FAIL: rpc connect for CHG_RPC_PROG. rc=%d\n",
		       __func__, rc);
		msm_batt_info.chg_ep = NULL;
		return rc;
	}

	/* Get the real 1.x version */
	if (msm_batt_info.chg_api_version == CHG_RPC_VER_1_1)
		msm_batt_info.chg_api_version =
			msm_batt_get_charger_api_version();

	/* Fall back to 1.1 for default */
	if (msm_batt_info.chg_api_version < 0)
		msm_batt_info.chg_api_version = CHG_RPC_VER_1_1;
	msm_batt_info.batt_api_version =  BATTERY_RPC_VER_4_1;

	msm_batt_info.batt_client =
		msm_rpc_register_client("battery", BATTERY_RPC_PROG,
					BATTERY_RPC_VER_4_1,
					1, msm_batt_cb_func);

	if (msm_batt_info.batt_client == NULL) {
		pr_err("%s: FAIL: rpc_register_client. batt_client=NULL\n",
		       __func__);
		return -ENODEV;
	}
	if (IS_ERR(msm_batt_info.batt_client)) {
		msm_batt_info.batt_client =
			msm_rpc_register_client("battery", BATTERY_RPC_PROG,
						BATTERY_RPC_VER_1_1,
						1, msm_batt_cb_func);
		msm_batt_info.batt_api_version =  BATTERY_RPC_VER_1_1;
	}
	if (IS_ERR(msm_batt_info.batt_client)) {
		msm_batt_info.batt_client =
			msm_rpc_register_client("battery", BATTERY_RPC_PROG,
						BATTERY_RPC_VER_2_1,
						1, msm_batt_cb_func);
		msm_batt_info.batt_api_version =  BATTERY_RPC_VER_2_1;
	}
	if (IS_ERR(msm_batt_info.batt_client)) {
		msm_batt_info.batt_client =
			msm_rpc_register_client("battery", BATTERY_RPC_PROG,
						BATTERY_RPC_VER_5_1,
						1, msm_batt_cb_func);
		msm_batt_info.batt_api_version =  BATTERY_RPC_VER_5_1;
	}
	if (IS_ERR(msm_batt_info.batt_client)) {
		rc = PTR_ERR(msm_batt_info.batt_client);
		pr_err("%s: ERROR: rpc_register_client: rc = %d\n ",
		       __func__, rc);
		msm_batt_info.batt_client = NULL;
		return rc;
	}
#endif  /* CONFIG_BATTERY_MSM_FAKE */

	rc = platform_driver_register(&msm_batt_driver);

	if (rc < 0)
		pr_err("%s: FAIL: platform_driver_register. rc = %d\n",
		       __func__, rc);

	return rc;
}

static int __init msm_batt_init(void)
{
	int rc;

	pr_debug("%s: enter\n", __func__);

	rc = msm_batt_init_rpc();

	if (rc < 0) {
		pr_err("%s: FAIL: msm_batt_init_rpc.  rc=%d\n", __func__, rc);
		msm_batt_cleanup();
		return rc;
	}

/* LGE_CHANGES_S [jaeho.cho@lge.com] 2010-10-02, charger logo notification to modem */
#ifdef CONFIG_LGE_CHARGING_MODE_INFO
	rc = misc_register(&chg_logo_device);
	if (rc)
	{
		printk(KERN_ERR "chg logo device failed to initialize\n");
		misc_deregister(&chg_logo_device);
	}
#endif
/* LGE_CHANGES_S [jaeho.cho@lge.com] 2010-10-02, charger logo notification to modem */

	pr_info("%s: Charger/Battery = 0x%08x/0x%08x (RPC version)\n",
		__func__, msm_batt_info.chg_api_version,
		msm_batt_info.batt_api_version);

	return 0;
}

static void __exit msm_batt_exit(void)
{
	platform_driver_unregister(&msm_batt_driver);
/* LGE_CHANGES_S [jaeho.cho@lge.com] 2010-10-02, charger logo notification to modem */
#ifdef CONFIG_LGE_CHARGING_MODE_INFO
	misc_deregister(&chg_logo_device);
#endif
/* LGE_CHANGES_S [jaeho.cho@lge.com] 2010-10-02, charger logo notification to modem */
}

module_init(msm_batt_init);
module_exit(msm_batt_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Kiran Kandi, Qualcomm Innovation Center, Inc.");
MODULE_DESCRIPTION("Battery driver for Qualcomm MSM chipsets.");
MODULE_VERSION("1.0");
MODULE_ALIAS("platform:msm_battery");
