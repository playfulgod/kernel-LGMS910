/* include/asm/mach-msm/htc_pwrsink.h
 *
 * Copyright (C) 2008 HTC Corporation.
 * Copyright (C) 2007 Google, Inc.
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
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/hrtimer.h>
#include <../../../drivers/staging/android/timed_output.h>
#include <linux/sched.h>

#include <mach/msm_rpcrouter.h>

#define PM_LIBPROG      0x30000061
#if (CONFIG_MSM_AMSS_VERSION == 6220) || (CONFIG_MSM_AMSS_VERSION == 6225)
// BEGIN : munho.lee@lge.com 2010-11-17
// MOD : 0011006: [Vibrator] To apply Qualcomm's 2030 patch.
#if 1
	#define PM_LIBVERS		0x30004 /* 0x30003 */ /*munho.lee@lge.com 2010-12-01*/
#else
/* sungwoo.cho 10.09.17 - START
   changed PM_LIBVERS 0x30001 -> 0x30002, because of 2020 version
*/
	#if 1
		#define PM_LIBVERS      0x30002
	#else
		#define PM_LIBVERS      0xfb837d0b
	#endif
/* sungwoo.cho 10.08.12 - END */
#endif	
// END : munho.lee@lge.com 2010-11-17
#else
	#if 1
		#define PM_LIBVERS      0x30001
	#else
		#define PM_LIBVERS      0x10001
	#endif
#endif

#define HTC_PROCEDURE_SET_VIB_ON_OFF	21
#define PM_VIB_MOT_SET_VOLT_PROC	22 //sungwoo.cho
#define PMIC_VIBRATOR_LEVEL	(3000)

/* sungwoo.cho 10.10.04 - START
 * 0009686: [kernel][vibrator] pmic vibrator driver is change from 2 work_queue to 1 work_queue.
 * There were some issues, because of using 2 work_queres.
 */
#if 1
static struct work_struct vibrator_work;
static struct hrtimer vibe_timer;
static spinlock_t vibe_lock;
static int vibe_state;

static void set_pmic_vibrator(int on)
{
	static struct msm_rpc_endpoint *vib_endpoint;
	struct set_vib_on_off_req {
		struct rpc_request_hdr hdr;
		uint32_t data;
	} req;

	printk("[pmic_vibrator] %s is called on/off : %d \n", __func__, on);
	
	if (!vib_endpoint) {
		vib_endpoint = msm_rpc_connect(PM_LIBPROG, PM_LIBVERS, 0);
		if (IS_ERR(vib_endpoint)) {
			printk(KERN_ERR "init vib rpc failed!\n");
			vib_endpoint = 0;
			return;
		}
	}

	if (on)
		req.data = cpu_to_be32(PMIC_VIBRATOR_LEVEL);
	else
		req.data = cpu_to_be32(0);

	msm_rpc_call(vib_endpoint, PM_VIB_MOT_SET_VOLT_PROC, &req,
		sizeof(req), 5 * HZ);
}

static void update_vibrator(struct work_struct *work)
{
	set_pmic_vibrator(vibe_state);
}

static void vibrator_enable(struct timed_output_dev *dev, int value)
{
	unsigned long	flags;

	printk("[pmic_vibrator] %s is called value:%d \n", __func__, value);
	
	spin_lock_irqsave(&vibe_lock, flags);
	hrtimer_cancel(&vibe_timer);

	if (value == 0)
		vibe_state = 0;
	else {
		value = (value > 15000 ? 15000 : value);
		vibe_state = 1;
	
		printk("[pmic_vibrator] %s is called value:%d \n", __func__, value);
		
		hrtimer_start(&vibe_timer,
			ktime_set(value / 1000, (value % 1000) * 1000000),
			HRTIMER_MODE_REL);
	}
	spin_unlock_irqrestore(&vibe_lock, flags);

	schedule_work(&vibrator_work);
}

static int vibrator_get_time(struct timed_output_dev *dev)
{
	if (hrtimer_active(&vibe_timer)) {
		ktime_t r = hrtimer_get_remaining(&vibe_timer);
		return r.tv.sec * 1000 + r.tv.nsec / 1000000;
	} else
		return 0;
}

static enum hrtimer_restart vibrator_timer_func(struct hrtimer *timer)
{
	vibe_state = 0;
	schedule_work(&vibrator_work);
	return HRTIMER_NORESTART;
}

static struct timed_output_dev pmic_vibrator = {
	.name = "pmic_vibrator",
	.get_time = vibrator_get_time,
	.enable = vibrator_enable,
};

void __init msm_init_pmic_vibrator(void)
{
	INIT_WORK(&vibrator_work, update_vibrator);

	spin_lock_init(&vibe_lock);
	vibe_state = 0;
	hrtimer_init(&vibe_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	vibe_timer.function = vibrator_timer_func;

	timed_output_dev_register(&pmic_vibrator);
}

/* sungwoo.cho 10.08.12 - START
   add msm_exit_pmic_vibrator function.
*/
static int  __exit msm_exit_pmic_vibrator(struct platform_device *dev)
{
	timed_output_dev_unregister(&pmic_vibrator);
	return 0;
}
/* sungwoo.cho 10.08.12 - END */

#else
static struct work_struct work_vibrator_on;
static struct work_struct work_vibrator_off;
static struct hrtimer vibe_timer;

static void set_pmic_vibrator(int on)
{
	static struct msm_rpc_endpoint *vib_endpoint;
	struct set_vib_on_off_req {
		struct rpc_request_hdr hdr;
		uint32_t data;
	} req;

	if (!vib_endpoint) {
		vib_endpoint = msm_rpc_connect(PM_LIBPROG, PM_LIBVERS, 0);
		if (IS_ERR(vib_endpoint)) {
			printk(KERN_ERR "init vib rpc failed!\n");
			vib_endpoint = 0;
			return;
		}
	}

	if (on)
		req.data = cpu_to_be32(PMIC_VIBRATOR_LEVEL);
	else
		req.data = cpu_to_be32(0);

/* sungwoo.cho 10.08.12 - START
   change the argument of msm_rpc_call function
*/
#if 1
	msm_rpc_call(vib_endpoint, PM_VIB_MOT_SET_VOLT_PROC, &req,
		sizeof(req), 5 * HZ);
#else
	msm_rpc_call(vib_endpoint, HTC_PROCEDURE_SET_VIB_ON_OFF, &req,
		sizeof(req), 5 * HZ);
#endif
/* sungwoo.cho 10.08.12 - END */
}

static void pmic_vibrator_on(struct work_struct *work)
{
	set_pmic_vibrator(1);
}

static void pmic_vibrator_off(struct work_struct *work)
{
	set_pmic_vibrator(0);
}

static void timed_vibrator_on(struct timed_output_dev *sdev)
{
	schedule_work(&work_vibrator_on);
}

static void timed_vibrator_off(struct timed_output_dev *sdev)
{
	schedule_work(&work_vibrator_off);
}

static void vibrator_enable(struct timed_output_dev *dev, int value)
{
	hrtimer_cancel(&vibe_timer);

	if (value == 0)
		timed_vibrator_off(dev);
	else {
		value = (value > 15000 ? 15000 : value);

		timed_vibrator_on(dev);

		hrtimer_start(&vibe_timer,
			      ktime_set(value / 1000, (value % 1000) * 1000000),
			      HRTIMER_MODE_REL);
	}
}

static int vibrator_get_time(struct timed_output_dev *dev)
{
	if (hrtimer_active(&vibe_timer)) {
		ktime_t r = hrtimer_get_remaining(&vibe_timer);
		return r.tv.sec * 1000 + r.tv.nsec / 1000000;
	} else
		return 0;
}

static enum hrtimer_restart vibrator_timer_func(struct hrtimer *timer)
{
	timed_vibrator_off(NULL);
	return HRTIMER_NORESTART;
}

static struct timed_output_dev pmic_vibrator = {
	.name = "pmic_vibrator",
	.get_time = vibrator_get_time,
	.enable = vibrator_enable,
};

/* sungwoo.cho 10.08.12 - START
   add msm_exit_pmic_vibrator function.
*/
static int  __exit msm_exit_pmic_vibrator(struct platform_device *dev)
{
	timed_output_dev_unregister(&pmic_vibrator);
	
	return 0;
}
/* sungwoo.cho 10.08.12 - END */
static void __init msm_init_pmic_vibrator(void)
{
	INIT_WORK(&work_vibrator_on, pmic_vibrator_on);
	INIT_WORK(&work_vibrator_off, pmic_vibrator_off);

	hrtimer_init(&vibe_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	vibe_timer.function = vibrator_timer_func;

	timed_output_dev_register(&pmic_vibrator);
}
#endif
/* sungwoo.cho 10.10.04 - end */

/* sungwoo.cho 10.08.12 - START
   Add module_init / module_exit.
*/
module_init(msm_init_pmic_vibrator);
module_exit(msm_exit_pmic_vibrator);
/* sungwoo.cho 10.08.12 - END */

MODULE_DESCRIPTION("timed output pmic vibrator device");
MODULE_LICENSE("GPL");

