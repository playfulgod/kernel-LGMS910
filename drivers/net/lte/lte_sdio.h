/*
 * LGE L2000 LTE driver for SDIO
 * 
 * Jae-gyu Lee <jaegyu.lee@lge.com>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 as published by the Free Software Foundation.
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
*/

#ifndef _LTE_SDIO_API_H_
#define _LTE_SDIO_API_H_

#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/time.h>
#include <linux/completion.h>
#include <linux/time.h>

#include <linux/mmc/core.h>
#include <linux/mmc/host.h>
#include <linux/mmc/card.h>
#include <linux/mmc/sdio.h>
#include <linux/mmc/sdio_func.h>
#include <linux/mmc/sdio_ids.h>

#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/serial.h>
#include <linux/sched.h>

#include <linux/mutex.h>
#include <linux/list.h>

/* BEGIN: 0013584: seongmook.yim@lge.com 2011-01-05  */
/* [LTE] Wakeup Scheme  for Power Saving Mode   */
#include <linux/wakelock.h>
/* END: 0013584: seongmook.yim@lge.com 2011-01-05 */   

/* BEGIN: 0013584: jihyun.park@lge.com 2011-03-11  */
/* [LTE] Wakeup Scheme  for Power Saving Mode   */
#include <linux/timer.h> 
/* END: 0013584: jihyun.park@lge.com 2011-03-11 */   

#include "lte_him.h"

/*for LTE_ROUTING*/
#define LTE_ROUTING
#define LTE_SDIO_BLK_SIZE 512

#define LTE_SDIO_BASE_ADDRESS 0x00
#define DATA_PORT			LTE_SDIO_BASE_ADDRESS+0x00
#define INTERRUPT_ID		LTE_SDIO_BASE_ADDRESS+0x04
#define CIS_FUN0_ADDR		LTE_SDIO_BASE_ADDRESS+0x08
#define CIS_FUN1_ADDR		LTE_SDIO_BASE_ADDRESS+0x0C
#define CSA_ADDR			LTE_SDIO_BASE_ADDRESS+0x10
#define READ_ADDR			LTE_SDIO_BASE_ADDRESS+0x14
#define WRITE_ADDR 			LTE_SDIO_BASE_ADDRESS+0x18
#define AHB_TRANSFER_COUNT	LTE_SDIO_BASE_ADDRESS+0x1C
#define SDIO_TRANSFER_COUNT	LTE_SDIO_BASE_ADDRESS+0x20
#define CIA_REGISTRER		LTE_SDIO_BASE_ADDRESS+0x24
#define PROG_REGISTRER		LTE_SDIO_BASE_ADDRESS+0x28
#define INTERRUPT_STATUS	LTE_SDIO_BASE_ADDRESS+0x2C
#define INTERRUPT_ENABLE	LTE_SDIO_BASE_ADDRESS+0x30
#define OCR_REGISTER		LTE_SDIO_BASE_ADDRESS+0x34
#define CLOCK_WAKEUP		LTE_SDIO_BASE_ADDRESS+0x38
#define AHB_BUS_SIZE		LTE_SDIO_BASE_ADDRESS+0x3C
#define GPR_0				LTE_SDIO_BASE_ADDRESS+0x44
#define GPR_1				LTE_SDIO_BASE_ADDRESS+0x48
#define GPR_2				LTE_SDIO_BASE_ADDRESS+0x4C
#define GPR_3				LTE_SDIO_BASE_ADDRESS+0x50

#define AHB_TRANSFER_COUNT_BIT_MASK	0x001FFFFF

/* BEGIN: 0013584: seongmook.yim@lge.com 2011-01-05  */
/* [LTE] Wakeup Scheme  for Power Saving Mode   */
#define LTE_WAKE_UP

/* BEGIN: 0018385 seungyeol.seo@lge.com 2011-03-22 */
/* MOD 0018385 : [LTE] Rearrangement of 'Wake Lock' section */
//#define SSY_TEST_SUSPENDED_WORKQUEUE
/* END: 0018385 seungyeol.seo@lge.com 2011-03-22 */

#ifdef LTE_WAKE_UP

#define TRUE 1
#define FALSE 0

#define GPIO_L2K_LTE_WAKEUP 153
#define GPIO_L2K_LTE_STATUS 32
#define GPIO_L2K_HOST_WAKEUP 18
#define GPIO_L2K_HOST_STATUS 123
#endif 
/* END: 0013584: seongmook.yim@lge.com 2011-01-05 */   

//#define SDIO_LOOPBACK_TEST

//#define SDIO_TX_TEST

//#define SDIO_HIM_TEST

//#define SDIO_CAL_THROUGHPUT

//#define SDIO_TX_TIMER

#define SDIO_TX_POLL_TIME_MS 1

#ifdef LGE_NO_HARDWARE
#ifndef LGE_NO_HARDWARE_TX_DELAY
#define LGE_NO_HARDWARE_TX_DELAY 10
#endif
#endif


#define LTE_SYSFS

typedef struct _LTE_SDIO_INFO {

/* BEGIN: 0013584: seongmook.yim@lge.com 2011-01-05  */
/* [LTE] Wakeup Scheme  for Power Saving Mode   */
	struct completion comp;
	
/* BEGIN: 0013584: jihyun.park@lge.com 2011-03-11  */
/* [LTE] Wakeup Scheme  for Power Saving Mode   */
    struct timer_list lte_timer; 
/* END: 0013584: jihyun.park@lge.com 2011-03-11 */   


/* BEGIN: 0018385 seungyeol.seo@lge.com 2011-03-22 */
/* MOD 0018385 : [LTE] Rearrangement of 'Wake Lock' section */
	#ifdef SSY_TEST_SUSPENDED_WORKQUEUE
	struct timer_list rx_workqueue_timer; 
	#endif /* SSY_TEST_SUSPENDED_WORKQUEUE */
/* END: 0018385 seungyeol.seo@lge.com 2011-03-22 */
/* BEGIN: 0018522 jaegyu.lee@lge.com 2011-03-24 */
/* MOD 0018522: [LTE] Kernel crash happen, When the SDIO TX & RX fail in 5sec */
	struct timer_list tx_timeout;
	struct timer_list rx_timeout;
/* END: 0018522 jaegyu.lee@lge.com 2011-03-24 *///seongmook.yim
/* Wake Lock */
	struct wake_lock lte_wake_lock;
/* END: 0013584: seongmook.yim@lge.com 2011-01-05 */   
/* BEGIN: 0017997 jaegyu.lee@lge.com 2011-03-15 */
/* MOD 0017997: [LTE] Wake-lock period rearrange in LTE SDIO driver */
	struct wake_lock tx_data_wake_lock;
	struct wake_lock tx_control_wake_lock;
	struct wake_lock rx_wake_lock;	
/* END: 0017997 jaegyu.lee@lge.com 2011-03-15 */	
	struct sdio_func *func;

	struct tty_struct *tty;	/* pointer to the tty */
	#ifdef LTE_ROUTING
	struct tty_struct *tty_lldm;	/* pointer to the tty */
	#endif /*LTE_ROUTING*/
	struct semaphore sem;	/* lock this struct */
	int opened_count;
	int msr;				/* MSR shadow */
	int mcr;				/* MCR shadow */

	#ifdef LTE_ROUTING
	int lldm_flag;
	#endif /*LTE_ROUTING*/
#ifdef SDIO_TX_TIMER
	struct timer_list *timer;
	int time_tick;
#endif

#ifdef SDIO_TX_TIMER
	/* TX thread */
	struct task_struct *tx_thread;
	struct semaphore tx_sem;
#endif
	struct mutex tx_lock_mutex;

	tx_packet_list *tx_packet_head;
	int tx_list_size;
	spinlock_t lock;
#ifndef SDIO_TX_TIMER
	struct work_struct tx_worker;
	struct workqueue_struct *tx_workqueue;
	char tx_wq_name[32];
#endif

/*BEGIN: 0014536 daeok.kim@lge.com 2011-01-22 */
/*MOD 0014536: [LTE] Rx thread is changed by Single thread workqueue in SDIO driver */
	/* RX thread */
//	struct task_struct *rx_thread;
//	struct semaphore rx_sem;
	struct mutex rx_lock_mutex;

	struct work_struct rx_worker;
	struct workqueue_struct *rx_workqueue;
	char rx_wq_name[32];	
	
	unsigned char *rx_buff;
	unsigned int rx_size;

	lte_him_cb_fn_t callback_from_ved;
/*END: 0014536 daeok.kim@lge.com 2011-01-22 */
	
/* for test */
#ifdef SDIO_HIM_TEST
	struct task_struct *test_thread;
	struct semaphore test_sem;
	unsigned int test_packet_size;
	unsigned int test_packet_count;
#endif

/* status of SDIO */	
#ifdef SDIO_CAL_THROUGHPUT
	unsigned int rx_byte_per_sec;
	unsigned int tx_byte_per_sec;
#endif
	unsigned int him_rx_cnt;
	unsigned int him_tx_cnt;
/* BEGIN: 0011698 jaegyu.lee@lge.com 2010-12-01 */
/* ADD 0011698: [LTE] ADD : Debug message for him packet */
	unsigned int him_dropped_ip_cnt;
/* END : 0011698 jaegyu.lee@lge.com 2010-12-01 */

/*BEGIN: 0017497 daeok.kim@lge.com 2011-03-05 */
/*MOD 0017497: [LTE] LTE SW Assert stability is update: blocking of tx operation */
	unsigned char flag_tx_blocking;
/*END: 0017497 daeok.kim@lge.com 2011-03-05 */



/* BEGIN: 0018522 jaegyu.lee@lge.com 2011-03-24 */
/* MOD 0018522: [LTE] Kernel crash happen, When the SDIO TX & RX fail in 5sec */
	unsigned char flag_gpio_l2k_host_status;
/* END: 0018522 jaegyu.lee@lge.com 2011-03-24 */



} LTE_SDIO_INFO, *PLTE_SDIO_INFO;

int lte_sdio_enable_function(void);
int lte_sdio_disable_function(void);

int lte_sdio_start_thread(void);
int lte_sdio_end_thread(void);

int lte_sdio_drv_init(void);
int lte_sdio_drv_deinit(void);

/* BEGIN: 0013584: seongmook.yim@lge.com 2011-01-05  */
/* [LTE] Wakeup Scheme  for Power Saving Mode   */
#ifdef LTE_WAKE_UP
int lte_sdio_wake_up_tx(void *tmp_him_blk_ptr, unsigned int write_length);
void lte_sdio_wake_up_rx();
#endif
/* END: 0013584: seongmook.yim@lge.com 2011-01-05 */   

#endif //_LTE_SDIO_API_H_
