/*
 *  arch/arm/mach-msm/lge_headset.c
 *
 *  LGE 3.5 PI Headset detection driver.
 *  HSD is HeadSet Detection with one GPIO
 *
 * Copyright (C) 2008 Google, Inc.
 * Author: Mike Lockwood <lockwood@android.com>
 *
 * Copyright (C) 2009 ~ 2010 LGE, Inc.
 * Author: Lee SungYoung < lsy@lge.com>
 *
 * Copyright (C) 2010 LGE, Inc.
 * Author: Kim Eun Hye < ehgrace.kim@lge.com>
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

/* The Android Donut's Headset Detection Interface is following;
 * source file is android/frameworks/base/services/java/com/android/server/HeadsetObserver.java
 * HEADSET_UEVENT_MATCH = "DEVPATH=/sys/devices/virtual/switch/h2w"
 * HEADSET_STATE_PATH = /sys/class/switch/h2w/state
 * HEADSET_NAME_PATH = /sys/class/switch/h2w/name
 * INPUT = SW_HEADPHONE_INSERT ==> KEY_MEDIA
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/switch.h>
#include <linux/workqueue.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <linux/mutex.h>
#include <linux/hrtimer.h>
#include <linux/input.h>
#include <linux/debugfs.h>
#include <mach/board.h>
/* ehgrace.kim	2010-08-24
   add hook detect */
#include <mach/mpp.h>   
#include <linux/mfd/pmic8058.h>
#include <mach/vreg.h>

#include <linux/slab.h>

#define PM8058_GPIO_PM_TO_SYS(pm_gpio)     (pm_gpio + NR_GPIO_IRQS)  
#define PM_MPP_06 5 /* PMIC MPP 06 */

/* BEGIN:0015283 ehgrace.kim@lge.com     2011.01.29*/
/* MOD: [Audio]reduce the earjack detection noise and add the wakelock */
#include <linux/wakelock.h>

static struct wake_lock hsd_wake_lock;
static struct wake_lock btn_wake_lock;
/* END:0015283 ehgrace.kim@lge.com     2011.01.29*/
#undef  LGE_HSD_DEBUG_PRINT
#define LGE_HSD_DEBUG_PRINT
#undef  LGE_HSD_ERROR_PRINT
#define LGE_HSD_ERROR_PRINT

#if defined(LGE_HSD_DEBUG_PRINT)
#define HSD_DBG(fmt, args...) printk(KERN_INFO "HSD[%-18s:%5d]" fmt, __func__,__LINE__, ## args) 
#else
#define HSD_DBG(fmt, args...) do {} while(0)
#endif

#if defined(LGE_HSD_ERROR_PRINT)
#define HSD_ERR(fmt, args...) printk(KERN_ERR "HSD[%-18s:%5d]" fmt, __func__,__LINE__, ## args) 
#else
#define HSD_ERR(fmt, args...) do {} while(0)
#endif

/* BEGIN:0016647 ehgrace.kim@lge.com     2011.02.22*/
/* MOD: [Audio]supplement 3/4 pol detection */
/* BEGIN:0012406 ehgrace.kim@lge.com     2010.12.14*/
/* MOD: [Audio]fix the earjack detection probelm */
#define HSD_DETECT_TIMER 0	
#define MPP_IRQ 0
#define VREG_CNT 1
/* END:0012406 ehgrace.kim@lge.com     2010.12.14*/
/* END:0016647 ehgrace.kim@lge.com     2011.02.22*/
static struct workqueue_struct *hs_detect_work_queue;
static void detect_work(struct work_struct *work);
static DECLARE_WORK(hs_detect_work, detect_work);
//static DECLARE_DELAYED_WORK(hs_detect_work, detect_work); //dwork
 
static struct vreg *gp2_vreg;

struct hsd_info {
   struct switch_dev sdev;
   struct input_dev *input;
   struct mutex mutex_lock;
   const char *name_on;
   const char *name_off;
   const char *state_on;
   const char *state_off;
   unsigned gpio;
/* ehgrace.kim	2010-08-24
   add hook detect */
   atomic_t btn_state;
   atomic_t ignore_btn;
/* BEGIN:0012616 ehgrace.kim@lge.com     2010.12.17*/
/* MOD: [Audio]delete the dummy adc intr */
   atomic_t mpp_irq_state;
/* END:0012616 ehgrace.kim@lge.com     2010.12.17*/
   unsigned int irq;
   unsigned int mpp_irq; 
   unsigned int mpp; 
/* BEGIN:0012406 ehgrace.kim@lge.com     2010.12.14*/
/* MOD: [Audio]fix the earjack detection probelm */
#if HSD_DETECT_TIMER
	struct hrtimer hsd_timer;
	ktime_t hsd_time;
#endif
/* END:0012406 ehgrace.kim@lge.com     2010.12.14*/
   struct work_struct work;
	struct hrtimer btn_timer;
	ktime_t btn_debounce_time;
#if VREG_CNT
   atomic_t vreg_cnt;
#endif
};

static struct hsd_info *hi;

enum {
   NO_DEVICE   = 0,
   LGE_HEADSET = 1,
/* ehgrace.kim	2010-09-30
   add headset_no_mic */
//MS910_LGE_CHANGE_AUDIO [jy0127.jang@lge.com] 2011-03-16 
// audio path for 3 pole headset 
   LGE_HEADSET_NO_MIC = 1<<6,
};

/* BEGIN : 0009847 ehgrace.kim	2010-10-11 */
/* fix to detect 3/4 polarity at boot time */
enum {
   FALSE = 0,
   TRUE = 1,
};
/* END : 0009847 ehgrace.kim	2010-10-11 */
#define LGE_HEADSET_DETECT_GPIO  26

/* BEGIN:0016647 ehgrace.kim@lge.com     2011.02.22*/
/* MOD: [Audio]supplement 3/4 pol detection */
#if VREG_CNT
static void hsd_vreg_on(void)
{
	if (atomic_read(&hi->vreg_cnt) == 0 ){
		vreg_set_level(gp2_vreg, 2600); //EARMIC
    		vreg_enable(gp2_vreg);
		atomic_set(&hi->vreg_cnt, 1);
		HSD_DBG("hsd_vreg_on \n");
	}
}

static void hsd_vreg_off(void)
{
	if (atomic_read(&hi->vreg_cnt) == 1){
		if (gp2_vreg != NULL){
			vreg_disable(gp2_vreg);
			atomic_set(&hi->vreg_cnt, 0);			
			HSD_DBG("hsd_vreg_off \n");
		}
	}
}
#endif
/* END:0016647 ehgrace.kim@lge.com     2011.02.22*/
static ssize_t lge_hsd_print_name(struct switch_dev *sdev, char *buf)
{
   switch (switch_get_state(&hi->sdev)) {
   case NO_DEVICE:
      return sprintf(buf, "No Device\n");
   case LGE_HEADSET:
      return sprintf(buf, "Headset\n");
/* ehgrace.kim	2010-09-30
   add headset_no_mic */
   case LGE_HEADSET_NO_MIC:
      return sprintf(buf, "Headset_no_mic\n");
   }
   return -EINVAL;
}

static ssize_t lge_hsd_print_state(struct switch_dev *sdev, char *buf)
{
	const char *state;

	if (switch_get_state(&hi->sdev))
		state = hi->state_on;
	else
		state = hi->state_off;

	if (state)
		return sprintf(buf, "%s\n", state);
	return -1;
}

/* ehgrace.kim	2010-08-24
   add hook detect */
static void button_pressed(void)
{
	HSD_DBG("button_pressed \n");
	atomic_set(&hi->btn_state, 1);
	input_report_key(hi->input, KEY_MEDIA, 1);
	input_sync(hi->input);
}

static void button_released(void)
{
	HSD_DBG("button_released \n");
	atomic_set(&hi->btn_state, 0);
	input_report_key(hi->input, KEY_MEDIA, 0);
	input_sync(hi->input);
}

static void remove_headset(void)
{
/* ehgrace.kim	2010-08-24
   add hook detect */
	unsigned long irq_flags;
#if VREG_CNT
	hsd_vreg_off();
#else
	if (gp2_vreg != NULL){
/* BEGIN : 0009847 ehgrace.kim	2010-10-11 */
/* fix to detect 3/4 polarity at boot time */
		vreg_disable(gp2_vreg);
/* END : 0009847 ehgrace.kim	2010-10-11 */
	}
#endif
	atomic_set(&hi->ignore_btn, 1); 
	mutex_lock(&hi->mutex_lock);
	switch_set_state(&hi->sdev, NO_DEVICE);
   	mutex_unlock(&hi->mutex_lock);
/* BEGIN:0012616 ehgrace.kim@lge.com     2010.12.17*/
/* MOD: [Audio]delete the dummy adc intr */
	if (atomic_read(&hi->mpp_irq_state) == TRUE) {
		local_irq_save(irq_flags);
#if MPP_IRQ
		disable_irq(hi->mpp_irq);
#else
		disable_irq(hi->mpp);
#endif
		local_irq_restore(irq_flags);
	   	atomic_set(&hi->mpp_irq_state, FALSE);
	}
/* END:0012616 ehgrace.kim@lge.com     2010.12.17*/

	if (atomic_read(&hi->btn_state))
		button_released();

//	input_report_switch(hi->input, SW_HEADPHONE_INSERT, 0);
//	input_sync(hi->input);

}

/* ehgrace.kim	2010-08-24
   add hook detect */
/* ehgrace.kim	2010-09-30
   add headset_no_mic */
static void insert_headset(bool type)
{
	unsigned long irq_flags;
//   	static struct vreg *gp2_vreg;
   	int rc;
/* BEGIN : 0009847 ehgrace.kim	2010-10-11 */
/* fix to detect 3/4 polarity at boot time */
#if 0
	if (gp2_vreg == NULL){
    	gp2_vreg = vreg_get(NULL, "gp2");
	}
    rc = vreg_set_level(gp2_vreg, 2600); //EARMIC
    vreg_enable(gp2_vreg);
    HSD_DBG("set the vreg gp2");
#endif	
/* END : 0009847 ehgrace.kim	2010-10-11 */
//	input_report_switch(hi->input, SW_HEADPHONE_INSERT, 1);
//	input_sync(hi->input);
/* BEGIN:0012616 ehgrace.kim@lge.com     2010.12.17*/
/* MOD: [Audio]delete the dummy adc intr */
	if (atomic_read(&hi->mpp_irq_state) == FALSE) {
   	local_irq_save(irq_flags);
#if MPP_IRQ
   	enable_irq(hi->mpp_irq);
#else
   	enable_irq(hi->mpp);
#endif
   	local_irq_restore(irq_flags);
   		atomic_set(&hi->mpp_irq_state, TRUE);
   	HSD_DBG("mpp_irq_state : %d\n", atomic_read(&hi->mpp_irq_state));  //for test
	}
/* END:0012616 ehgrace.kim@lge.com     2010.12.17*/
/* BEGIN : 0009847 ehgrace.kim	2010-10-11 */
/* fix to detect 3/4 polarity at boot time */
//	atomic_set(&hi->ignore_btn, 0); 
/* END : 0009847 ehgrace.kim	2010-10-11 */
   	HSD_DBG("insert_headset : %d\n", type);
	if (type == 1) {
		mutex_lock(&hi->mutex_lock);
		switch_set_state(&hi->sdev, LGE_HEADSET_NO_MIC);
		mutex_unlock(&hi->mutex_lock);
/* BEGIN : 0016838 ehgrace.kim	2011-02-23 */
/* add the vreg_disable for 3 pol earjack */
#if VREG_CNT
		hsd_vreg_off();
#endif
/* END: 0016838 ehgrace.kim	2011-02-23 */
	} 
	else if (type == 0) {
		mutex_lock(&hi->mutex_lock);
		switch_set_state(&hi->sdev, LGE_HEADSET);
		mutex_unlock(&hi->mutex_lock);
	} 
	else {
   		HSD_DBG("invalid headset type\n");
	}
	
}

/* BEGIN : 0011638 ehgrace.kim	2010-11-30 */
/* MOD : fix the vreg control of earjack detect for power consumption */
static void detect_work(struct work_struct *work)
{
   int state,retry_count;
   unsigned long irq_flags;
   bool earjack_type;

/* BEGIN:0016647 ehgrace.kim@lge.com     2011.02.22*/
/* MOD: [Audio]supplement 3/4 pol detection */
#if 1
	HSD_DBG("wake_lock_timer\n");
   	wake_lock_timeout(&hsd_wake_lock, HZ);
#endif
/* END:0016647 ehgrace.kim@lge.com     2011.02.22*/
   local_irq_save(irq_flags);
   disable_irq(hi->irq);
   local_irq_restore(irq_flags);

   state = gpio_get_value(hi->gpio);

   HSD_DBG("hs:%d\n", state);
   
   local_irq_save(irq_flags);
   enable_irq(hi->irq);
   local_irq_restore(irq_flags);
/* BEGIN:0012616 ehgrace.kim@lge.com     2010.12.17*/
/* MOD: [Audio]delete the dummy adc intr */
   if (state != 1) {
      if (switch_get_state(&hi->sdev) != NO_DEVICE){
         HSD_DBG("==== LGE headset removing\n");
         remove_headset();
      } else {
	goto err_invalid_state;
      }
   } else { //state == 1

/* BEGIN : 0009847 ehgrace.kim	2010-10-11 */
/* fix to detect 3/4 polarity at boot time */
/* BEGIN:0012406 ehgrace.kim@lge.com     2010.12.14*/
/* MOD: [Audio]fix the earjack detection probelm */
/* BEGIN:0012530 ehgrace.kim@lge.com     2010.12.16*/
/* MOD: [Audio]extend the adc time to fix the earjack detection probelm */
/* BEGIN:0016647 ehgrace.kim@lge.com     2011.02.22*/
/* MOD: [Audio]supplement 3/4 pol detection */
#if 1 //HSD_DETECT_TIMER

	msleep(500);
#if VREG_CNT
	hsd_vreg_on();
#else
	vreg_set_level(gp2_vreg, 2600); //EARMIC
    	vreg_enable(gp2_vreg);
#endif
    	HSD_DBG("set the vreg gp2");
/*BEGIN:0015283	ehgrace.kim@lge.com 	2011.02.09 */
/*MOD:[Audio]add the checking routine */
	msleep(200);
/*END:0015283	ehgrace.kim@lge.com 	2011.02.09 */
#endif
/* END:0012530 ehgrace.kim@lge.com     2010.12.16*/
/* END:0012406 ehgrace.kim@lge.com     2010.12.14*/
/* END : 0009847 ehgrace.kim	2010-10-11 */
	/* distinguish 3 polarity earjack */
/*BEGIN: 0015283	ehgrace.kim@lge.com 	2011.02.09 */
/*MOD:[Audio]add the checking routine */
	/*Assuemed Mpp interrupt is not enabled yet & reading with out irq locks*/
//	retry_count = 20;
	retry_count = 50;
	do{
		earjack_type = gpio_get_value(hi->mpp);
//		HSD_DBG("%d-%d", earjack_type, retry_count);
	}while(retry_count-- > 0);
/*END:	0015283 ehgrace.kim@lge.com 	2011.02.09 */
/* END:0016647 ehgrace.kim@lge.com     2011.02.22*/
	if (earjack_type == 0){
        	atomic_set(&hi->ignore_btn, 1);
		HSD_DBG("3 polarity earjack");
	}
	else {
        	atomic_set(&hi->ignore_btn, 0);
		HSD_DBG("4 polarity earjack");
	}
/*
   if (state != 1) {
      if (switch_get_state(&hi->sdev) != NO_DEVICE){
         HSD_DBG("==== LGE headset removing\n");
         remove_headset();
      } else {
	goto err_invalid_state;
      }
   } else { //state == 1
*/
      if (switch_get_state(&hi->sdev) == NO_DEVICE) {
         HSD_DBG("==== LGE headset inserting\n");
/* BEGIN:0014800 ehgrace.kim@lge.com     2011.01.25*/
/* MOD: [Audio]fix the dummy interrupt for earjack */
	 if (gpio_get_value(hi->gpio)==1) {//for confirmation about detection
         	insert_headset(atomic_read(&hi->ignore_btn));
	 } else {
		HSD_DBG("delete insert irq\n");
/* BEGIN:0014829 ehgrace.kim@lge.com     2011.01.26*/
/* MOD: [Audio]add the vreg disable for earjack detection */
		goto err_invalid_state;
/* END:0014829 ehgrace.kim@lge.com     2011.01.26*/
	 }
/* END:0014800 ehgrace.kim@lge.com     2011.01.25*/
      } else {
/*BEGIN:0015283	ehgrace.kim@lge.com 	2011.02.09 */
/*MOD:[Audio]	add the checking routine */
		HSD_DBG("ignore insert irq\n");
	goto err_duplicated_state;
/*END:0015283	ehgrace.kim@lge.com 	2011.02.09 */
      }
   }
   return;
/* END:0012616 ehgrace.kim@lge.com     2010.12.17*/

err_invalid_state:
#if VREG_CNT
	hsd_vreg_off();
#else
	if (gp2_vreg != NULL){
         	HSD_DBG("==== LGE headset invalid state\n");
		vreg_disable(gp2_vreg);
	}
#endif
/*BEGIN: 0015283 ehgrace.kim@lge.com 	2011.02.09 */
/*MOD:[Audio]add the checking routine */
err_duplicated_state:
/*END: 0015283	ehgrace.kim@lge.com 	2011.02.09 */
/* BEGIN:0016647 ehgrace.kim@lge.com     2011.02.22*/
/* MOD: [Audio]supplement 3/4 pol detection */
	if (atomic_read(&hi->mpp_irq_state) == FALSE) {
		local_irq_save(irq_flags);
#if MPP_IRQ
		enable_irq(hi->mpp_irq);
#else
		enable_irq(hi->mpp);
#endif
		local_irq_restore(irq_flags);
   		atomic_set(&hi->mpp_irq_state, TRUE);
   		HSD_DBG("mpp_irq_state : %d\n", atomic_read(&hi->mpp_irq_state));  //for test
	}
/* END:0016647 ehgrace.kim@lge.com     2011.02.22*/
	return;
}
/*END  : 0011638 ehgrace.kim	2010-11-30 */

/* BEGIN:0012406 ehgrace.kim@lge.com     2010.12.14*/
/* MOD: [Audio]fix the earjack detection probelm */
#if HSD_DETECT_TIMER
static enum hrtimer_restart hsd_event_timer_func(struct hrtimer *data)
{
/* BEGIN:0015283 ehgrace.kim@lge.com     2011.01.29*/
/* MOD: [Audio]reduce the earjack detection noise and add the wakelock */
   int timer_init_retVal = 0;
/* BEGIN:0016647 ehgrace.kim@lge.com     2011.02.22*/
/* MOD: [Audio]supplement 3/4 pol detection */
#if 0//1
	HSD_DBG("wake_lock_timer\n");
   	wake_lock_timeout(&hsd_wake_lock, HZ);
#endif
/* END:0016647 ehgrace.kim@lge.com     2011.02.22*/
   	queue_work(hs_detect_work_queue, &hs_detect_work);
	return HRTIMER_NORESTART;
/* END:0015283 ehgrace.kim@lge.com     2011.01.29*/
}
#endif
/* END:0012406 ehgrace.kim@lge.com     2010.12.14*/

/* ehgrace.kim	2010-08-24
   add hook detect */
static enum hrtimer_restart button_event_timer_func(struct hrtimer *data)
{
	int key, press, keyname, h2w_key = 1;
/* BEGIN:0015283 ehgrace.kim@lge.com     2011.01.29*/
/* MOD: [Audio]reduce the earjack detection noise and add the wakelock */
#if 0
	HSD_DBG("wake_lock_timer\n");
   	wake_lock_timeout(&btn_wake_lock, HZ);
#endif
/* END:0015283 ehgrace.kim@lge.com     2011.01.29*/

	if (switch_get_state(&hi->sdev) == LGE_HEADSET) {
/* BEGIN:0012616 ehgrace.kim@lge.com     2010.12.17*/
/* MOD: [Audio]delete the dummy adc intr */
		if (atomic_read(&hi->mpp_irq_state)==TRUE) {
   			HSD_DBG("mpp_irq_state : %d\n", atomic_read(&hi->mpp_irq_state));  //for test
			key = gpio_get_value(hi->mpp);
			HSD_DBG("btn %d, ignore_btn %d\n", key, atomic_read(&hi->ignore_btn));
			if (key) {
				if (atomic_read(&hi->ignore_btn))
					atomic_set(&hi->ignore_btn, 0);
				else if (atomic_read(&hi->btn_state))
					button_released();
				else
					HSD_DBG("btn release ignore");
			} else {
				if (!atomic_read(&hi->ignore_btn) && !atomic_read(&hi->btn_state))
					button_pressed();
				else
					HSD_DBG("btn press ignore");
			}
		} else
			HSD_DBG("ignore btn intr");
	}
	else
		HSD_DBG("It is not 4pol headset");
/* END:0012616 ehgrace.kim@lge.com     2010.12.17*/
		
	return HRTIMER_NORESTART;
}


static irqreturn_t gpio_irq_handler(int irq, void *dev_id)
{
   int value1, value2;
   int retry_limit = 10;

#if 0//1
	HSD_DBG("wake_lock_timer\n");
   	wake_lock_timeout(&hsd_wake_lock, HZ);
#endif
   value1 = gpio_get_value(hi->gpio);
   do {
      value2 = gpio_get_value(hi->gpio);
   } while(retry_limit-- < 0);
   if (value1 ^ value2)
      return IRQ_HANDLED;

   HSD_DBG("value2 = %d\n", value2);

   if ((switch_get_state(&hi->sdev) ^ value2)) {
/* BEGIN:0012616 ehgrace.kim@lge.com     2010.12.17*/
/* MOD: [Audio]delete the dummy adc intr */
#if MPP_IRQ   //modem crash
#else
   	unsigned long irq_flags;
	if (atomic_read(&hi->mpp_irq_state) == TRUE) {
		local_irq_save(irq_flags);
		disable_irq(hi->mpp);
		local_irq_restore(irq_flags);
   		atomic_set(&hi->mpp_irq_state, FALSE);
   		HSD_DBG("mpp_irq_state : %d\n", atomic_read(&hi->mpp_irq_state));  //for test
	}
#endif		
/* END:0012616 ehgrace.kim@lge.com     2010.12.17*/
/* BEGIN:0012406 ehgrace.kim@lge.com     2010.12.14*/
/* MOD: [Audio]fix the earjack detection probelm */
#if HSD_DETECT_TIMER
	hrtimer_start(&hi->hsd_timer, hi->hsd_time, HRTIMER_MODE_REL);
#else
   	queue_work(hs_detect_work_queue, &hs_detect_work);
#endif
/* END:0012406 ehgrace.kim@lge.com     2010.12.14*/
   }

	return IRQ_HANDLED;
}

   /* ehgrace.kim  2010-08-24
	  add hook detect */
static irqreturn_t button_irq_handler(int irq, void *dev_id)
{
	int value1, value2;
	int retry_limit = 10;
   	value1 = gpio_get_value(hi->mpp);
   	HSD_DBG("start %d\n", value1);
/* BEGIN:0016647 ehgrace.kim@lge.com     2011.02.22*/
/* MOD: [Audio]supplement 3/4 pol detection */
	do {
      		value2 = gpio_get_value(hi->mpp);
//		HSD_DBG("%d \n", value2);
//   	} while(retry_limit-- < 0);
   	} while(retry_limit-- > 0);
/* END:0016647 ehgrace.kim@lge.com     2011.02.22*/
   		if (value1 ^ value2)
      		return IRQ_HANDLED;

/* BEGIN:0012616 ehgrace.kim@lge.com     2010.12.17*/
/* MOD: [Audio]delete the dummy adc intr */
	//add for test by ehgrace.kim@lge.com
	if (atomic_read(&hi->mpp_irq_state) == TRUE) {
   	HSD_DBG("mpp_irq_state : %d\n", atomic_read(&hi->mpp_irq_state));  //for test
   	HSD_DBG("value2 = %d\n", value2);

	hrtimer_start(&hi->btn_timer, hi->btn_debounce_time, HRTIMER_MODE_REL);
	} else {
		HSD_DBG("ignore the btn irq");
	}
/* END:0012616 ehgrace.kim@lge.com     2010.12.17*/
	return IRQ_HANDLED;
}

#if defined(CONFIG_DEBUG_FS)
static int hsd_debug_set(void *data, u64 val)
{
   mutex_lock(&hi->mutex_lock);
   switch_set_state(&hi->sdev, (int)val);
   mutex_unlock(&hi->mutex_lock);
   return 0;
}

static int hsd_debug_get(void *data, u64 *val)
{
   return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(hsd_debug_fops, hsd_debug_get, hsd_debug_set, "%llu\n");
static int __init hsd_debug_init(void)
{
   struct dentry *dent;

   dent = debugfs_create_dir("hsd", 0);
   if (IS_ERR(dent)) {
      HSD_ERR("Fail to create debug fs directory\n");
      return PTR_ERR(dent);
   }

   debugfs_create_file("state", 0644, dent, NULL, &hsd_debug_fops);

   return 0;
}

device_initcall(hsd_debug_init);
#endif

static int lge_hsd_probe(struct platform_device *pdev)
{
   int ret;
   struct gpio_switch_platform_data *pdata = pdev->dev.platform_data;

	HSD_DBG("%s\n", pdata->name);

	if (!pdata) {
      HSD_ERR("The platform data is null\n");
		return -EBUSY;
   }

   hi = kzalloc(sizeof(struct hsd_info), GFP_KERNEL);
   if (!hi) {
      HSD_ERR("Failed to allloate headset per device info\n");
      return -ENOMEM;
   }

   /* ehgrace.kim  2010-08-24
	  add hook detect */
   atomic_set(&hi->btn_state, 0);
   atomic_set(&hi->ignore_btn, 0);
#if VREG_CNT
   atomic_set(&hi->vreg_cnt, 0);
#endif
   hi->gpio = pdata->gpio;

   mutex_init(&hi->mutex_lock);
/* BEGIN:0012406 ehgrace.kim@lge.com     2010.12.14*/
/* MOD: [Audio]fix the earjack detection probelm */
#if HSD_DETECT_TIMER
	hi->hsd_time = ktime_set(0, 600000000); /* 600 ms */
#endif
/* END:0012406 ehgrace.kim@lge.com     2010.12.14*/
	hi->btn_debounce_time = ktime_set(0, 100000000); /* 100 ms */
#if 0
   hi->name_on = pdata->name_on;
   hi->name_off = pdata->name_off;
   hi->state_on = pdata->state_on;
   hi->state_off = pdata->state_off;
#endif

   hi->sdev.name = pdata->name;
   hi->sdev.print_state = lge_hsd_print_state;
   hi->sdev.print_name = lge_hsd_print_name;

   ret = switch_dev_register(&hi->sdev);
	if (ret < 0) {
      HSD_ERR("Failed to register switch device\n");
		goto err_switch_dev_register;
   }

/* BEGIN:0012406 ehgrace.kim@lge.com     2010.12.14*/
/* MOD: [Audio]fix the earjack detection probelm */
#if HSD_DETECT_TIMER
   hrtimer_init(&hi->hsd_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
   hi->hsd_timer.function = hsd_event_timer_func;
#endif
/* END:0012406 ehgrace.kim@lge.com     2010.12.14*/
   hs_detect_work_queue = create_workqueue("hs_detect");
   if (hs_detect_work_queue == NULL) {
      HSD_ERR("Failed to create workqueue\n");
      goto err_create_work_queue;
   }

   ret = gpio_request(hi->gpio, pdev->name);
   if (ret < 0) {
      HSD_ERR("Failed to request gpio%d\n", hi->gpio);
      goto err_request_detect_gpio;
   }

   /* ehgrace.kim  2010-08-24
	  add hook detect */
   hi->mpp = PM8058_GPIO_PM_TO_SYS(PM8058_GPIOS) + PM_MPP_06;
   ret = gpio_request(hi->mpp, "mpp_gpio");
   if (ret) {
	HSD_ERR("Failed to set request mpp\n");
		goto err_request_button_mpp;
   }

   ret = gpio_direction_input(hi->gpio); 
   if (ret < 0) {
      HSD_ERR("Failed to set gpio%d as input\n", hi->gpio);
      goto err_set_detect_gpio;
   }

   if (hi->gpio == LGE_HEADSET_DETECT_GPIO) {
      ret = gpio_tlmm_config(GPIO_CFG(hi->gpio, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, 
               GPIO_CFG_2MA), GPIO_CFG_ENABLE);
      if (ret < 0) {
         HSD_ERR("Failed to configure gpio%d tlmm\n", hi->gpio);
		goto err_set_button_mpp;
      }
   }

   /* ehgrace.kim  2010-08-24
	  add hook detect */
   ret = pm8058_mpp_config_digital_in(PM_MPP_06, PM8058_MPP_DIG_LEVEL_S3, PM_MPP_DIN_TO_INT);
   if (ret) {
	HSD_ERR("Failed to config mpp\n");
		goto err_set_button_mpp;
   }

   hi->irq = gpio_to_irq(pdata->gpio);
   if (hi->irq < 0) {
      HSD_ERR("Failed to get interrupt number\n");
      ret = hi->irq;
		goto err_get_h2w_detect_irq_num_failed;
   }

   ret = request_irq(hi->irq, gpio_irq_handler, 
           IRQF_TRIGGER_RISING|IRQF_TRIGGER_FALLING, pdev->name, NULL);

   if (ret < 0) {
      HSD_ERR("Failed to request interrupt handler\n");
		goto err_request_detect_irq;
   }

   /* ehgrace.kim  2010-08-24
	  add hook detect */
   gp2_vreg = vreg_get(NULL, "gp2");
/* BEGIN : 0009847 ehgrace.kim	2010-10-11 */
/* fix to detect 3/4 polarity at boot time */
#if 0
   ret = vreg_set_level(gp2_vreg, 2600); //EARMIC
   vreg_enable(gp2_vreg);
   HSD_DBG("set the vreg gp2");
#endif
/* END : 0009847 ehgrace.kim	2010-10-11 */

	hrtimer_init(&hi->btn_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	hi->btn_timer.function = button_event_timer_func;
	
/* BEGIN:0016647 ehgrace.kim@lge.com     2011.02.22*/
/* MOD: [Audio]supplement 3/4 pol detection */
	hi->mpp_irq = gpio_to_irq(hi->mpp);
	ret = request_threaded_irq(gpio_to_irq(hi->mpp),
		NULL, button_irq_handler,
		IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
		"mpp_irq", NULL);
	if (ret) {
		HSD_ERR("failed to request button irq");
		goto err_request_h2w_headset_button_irq;
	}
/* END:0016647 ehgrace.kim@lge.com     2011.02.22*/

   ret = set_irq_wake(hi->irq, 1);
   if (ret < 0) {
      HSD_ERR("Failed to set interrupt wake\n");
      goto err_request_input_dev;
   }

/* BEGIN: 0009747	ehgrace.kim	2010-10-07 */
/* ADD 0009747  add hook detection in sleep mode */
   ret = set_irq_wake(gpio_to_irq(hi->mpp), 1);
   if (ret < 0) {
      HSD_ERR("Failed to set interrupt wake\n");
      goto err_request_input_dev;
   }
/* END: 0009747	ehgrace.kim	2010-10-07 */

   	hi->input = input_allocate_device();
	if (!hi->input) {
      		HSD_ERR("Failed to allocate input device\n");
		ret = -ENOMEM;
		goto err_request_input_dev;
	}

   /* ehgrace.kim  2010-08-24
	  add hook detect */
	if (pdev->dev.platform_data)
			hi->input->name = "7k_handset";
//			hi->input->name = "7k_headset";
	else
			hi->input->name = "hsd_headset";

	hi->input->id.vendor	= 0x0001;
	hi->input->id.product	= 1;
	hi->input->id.version	= 1;

//	input_set_capability(hi->input, EV_SW, SW_HEADPHONE_INSERT);
	set_bit(EV_SYN, hi->input->evbit);
	set_bit(EV_KEY, hi->input->evbit);
	set_bit(KEY_MEDIA, hi->input->keybit);
	ret = input_register_device(hi->input);
	if (ret) {
         HSD_ERR("Failed to register input device\n");
			goto err_register_input_dev;
	}

	/* Perform initial detection */
//	gpio_switch_work(&switch_data->work);
/* BEGIN : 0009847 ehgrace.kim	2010-10-11 */
/* fix to detect 3/4 polarity at boot time */
   	queue_work(hs_detect_work_queue, &hs_detect_work);
/* BEGIN:0012616 ehgrace.kim@lge.com     2010.12.17*/
/* MOD: [Audio]delete the dummy adc intr */
   	atomic_set(&hi->mpp_irq_state, FALSE);
/* END:0012616 ehgrace.kim@lge.com     2010.12.17*/
/* END : 0009847 ehgrace.kim	2010-10-11 */
/* BEGIN:0015283 ehgrace.kim@lge.com     2011.01.29*/
/* MOD: [Audio]reduce the earjack detection noise and add the wakelock */
	wake_lock_init(&hsd_wake_lock, WAKE_LOCK_SUSPEND, "lge_hsd");
	wake_lock_init(&btn_wake_lock, WAKE_LOCK_SUSPEND, "lge_btn");
/* END:0015283 ehgrace.kim@lge.com     2011.01.29*/
	return 0;

err_register_input_dev:
   input_free_device(hi->input);
err_request_input_dev:
err_request_h2w_headset_button_irq:
   free_irq(hi->irq, 0);
err_request_detect_irq:
err_get_button_irq_num_failed:
err_get_h2w_detect_irq_num_failed:
err_set_button_mpp:
err_set_detect_gpio:
	gpio_free(hi->mpp);
err_request_button_mpp:
	gpio_free(hi->gpio);
err_request_detect_gpio:
   destroy_workqueue(hs_detect_work_queue);
err_create_work_queue:
   switch_dev_unregister(&hi->sdev);
err_switch_dev_register:
   HSD_ERR("Failed to register driver\n");

	return ret;
}

static int lge_hsd_remove(struct platform_device *pdev)
{
//   H2WD("");
   /* ehgrace.kim  2010-08-24
	  add hook detect */
#if VREG_CNT
   hsd_vreg_off();
#else
   if (gp2_vreg != NULL){
	vreg_disable(gp2_vreg);
   }
#endif
   if (switch_get_state(&hi->sdev))
      remove_headset();
   
   input_unregister_device(hi->input);
   gpio_free(hi->gpio);
	gpio_free(hi->mpp);
   free_irq(hi->irq, 0);
   destroy_workqueue(hs_detect_work_queue);
   switch_dev_unregister(&hi->sdev);

	return 0;
}

static struct gpio_switch_platform_data lge_hs_pdata = {
   .name = "h2w",
   .gpio = LGE_HEADSET_DETECT_GPIO,
};

static struct platform_device lge_hsd_device = {
   .name = "lge-h2w",
   .id   = -1,
   .dev = {
      .platform_data = &lge_hs_pdata,
   },
};

static struct platform_driver lge_hsd_driver = {
	.probe		= lge_hsd_probe,
	.remove		= lge_hsd_remove,
	.driver		= {
		.name	= "lge-h2w",
		.owner	= THIS_MODULE,
	},
};

static int __init lge_hsd_init(void)
{
   int ret;

   HSD_DBG("");
   ret = platform_driver_register(&lge_hsd_driver);
   if (ret) {
      HSD_ERR("Fail to register platform driver\n");
      return ret;
   }

	return platform_device_register(&lge_hsd_device);
}

static void __exit lge_hsd_exit(void)
{
   platform_device_unregister(&lge_hsd_device);
	platform_driver_unregister(&lge_hsd_driver);
}

module_init(lge_hsd_init);
module_exit(lge_hsd_exit);

MODULE_AUTHOR("Kim EunHye <ehgrace.kim@lge.com>");
MODULE_DESCRIPTION("LGE Headset detection driver");
MODULE_LICENSE("GPL");
