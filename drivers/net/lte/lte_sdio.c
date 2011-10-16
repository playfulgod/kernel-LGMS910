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

#include <linux/workqueue.h>
#include <linux/slab.h>

#include "lte_sdio.h"
#include "lte_debug.h"

/* BEGIN: 0013584: jihyun.park@lge.com 2011-01-05  */
/* [LTE] Wakeup Scheme  for Power Saving Mode   */
#include <linux/gpio.h>
#include <linux/unistd.h>
#include <linux/timer.h>
/* END: 0013584: jihyun.park@lge.com 2011-01-05 */   			

#define LTE_STATIC_BUFFER

extern PLTE_SDIO_INFO gLte_sdio_info;

static void lte_sdio_isr(struct sdio_func *func);
#ifdef SDIO_TX_TIMER
static int lte_sdio_tx_thread(void *unused);
#endif

/*BEGIN: 0014536 daeok.kim@lge.com 2011-01-22 */
/*MOD 0014536: [LTE] Rx thread is changed by Single thread workqueue in SDIO driver */
//static int lte_sdio_rx_thread(void *unused);
static void lte_sdio_tx(void *data);
static void lte_sdio_rx(void *data);
/*END: 0014536 daeok.kim@lge.com 2011-01-22 */

#ifdef SDIO_HIM_TEST
static int lte_sdio_test_thread(void *unused);
#endif

/* BEGIN: 0013584: jihyun.park@lge.com 2011-01-05  */
/* [LTE] Wakeup Scheme  for Power Saving Mode   */
extern int irq_tx, irq_rx;	
extern int g_tx_int_enable;
/* END: 0013584: jihyun.park@lge.com 2011-01-05 */   			

/* BEGIN: 0013584: jihyun.park@lge.com 2011-03-11  */
/* [LTE] Wakeup Scheme  for Power Saving Mode   */
static int g_tx_loop_skip;  
void lte_sdio_wake_up_tx_skip(unsigned long arg); 
void lte_sdio_wake_up_tx_init(void); 
/* END: 0013584: jihyun.park@lge.com 2011-03-11 */   			

int lte_sdio_enable_function(void)
{
	int error = 0;

	FUNC_ENTER();	

	sdio_claim_host(gLte_sdio_info->func);

	error = sdio_set_block_size(gLte_sdio_info->func, LTE_SDIO_BLK_SIZE);
    if(error != 0)
    {
        LTE_ERROR("Failed to set block size. Error = %x\n", error);
		goto err;
    }

	error = sdio_enable_func(gLte_sdio_info->func);
    if(error != 0)
    {
        LTE_ERROR("Failed to enable function. Error = %x\n", error);
		goto err;
    }

	error = sdio_claim_irq(gLte_sdio_info->func, lte_sdio_isr);
    if(error != 0)
    {
        LTE_ERROR("Failed to claim irq. Error = %x\n", error);
		goto err;
    }

	error = 0;
err:
	sdio_release_host(gLte_sdio_info->func);		
	FUNC_EXIT();
	return error;
}

int lte_sdio_disable_function(void)
{
	int error = 0;
		
	FUNC_ENTER();

	sdio_claim_host(gLte_sdio_info->func);

	error = sdio_release_irq(gLte_sdio_info->func);
    if(error != 0)
    {
        LTE_ERROR("Failed to release irq. Error = %x\n",error);
		goto err;
    }

	error = sdio_disable_func(gLte_sdio_info->func);
    if(error != 0)
    {
        LTE_ERROR("Failed to disable function. Error = %x\n",error);
		goto err;
    }

	error = 0;
err:
	sdio_release_host(gLte_sdio_info->func);		
	FUNC_EXIT();
	return error;
}

#ifdef SDIO_TX_TIMER
void lte_sdio_tx_timer_func(unsigned long arg);

int lte_sdio_register_tx_timer(unsigned long time_over)
{
	init_timer(gLte_sdio_info->timer);
	gLte_sdio_info->timer->expires = jiffies+msecs_to_jiffies(time_over);
	gLte_sdio_info->timer->data = 0;
	gLte_sdio_info->timer->function = lte_sdio_tx_timer_func;
	add_timer(gLte_sdio_info->timer);
}

void lte_sdio_tx_timer_func(unsigned long arg)
{
	gLte_sdio_info->time_tick++;

	lte_sdio_register_tx_timer(SDIO_TX_POLL_TIME_MS);

	up(&gLte_sdio_info->tx_sem);
}
#endif

int lte_sdio_drv_init(void)
{
#ifdef SDIO_CAL_THROUGHPUT	
	gLte_sdio_info->rx_byte_per_sec = 0;
	gLte_sdio_info->tx_byte_per_sec = 0;
#endif
	gLte_sdio_info->him_rx_cnt = 1;
	gLte_sdio_info->him_tx_cnt = 1;

/* BEGIN: 0011698 jaegyu.lee@lge.com 2010-12-01 */
/* ADD 0011698: [LTE] ADD : Debug message for him packet */
	gLte_sdio_info->him_dropped_ip_cnt = 0;
/* END : 0011698 jaegyu.lee@lge.com 2010-12-01 */

#ifdef SDIO_TX_TIMER
	sema_init(&gLte_sdio_info->tx_sem, 0);
#endif
/*BEGIN: 0014536 daeok.kim@lge.com 2011-01-22 */
/*ADD 0014536: [LTE] Rx thread is changed by Single thread workqueue in SDIO driver */
	//sema_init(&gLte_sdio_info->rx_sem, 0);
/*END: 0014536 daeok.kim@lge.com 2011-01-22 */

	init_MUTEX(&gLte_sdio_info->tx_lock_mutex);
	init_MUTEX(&gLte_sdio_info->rx_lock_mutex);	

#ifdef SDIO_HIM_TEST
	sema_init(&gLte_sdio_info->test_sem, 0);
#endif

	gLte_sdio_info->tx_packet_head = (tx_packet_list *)kzalloc(sizeof(tx_packet_list), GFP_KERNEL);
	if(gLte_sdio_info->tx_packet_head==0)
	{
		LTE_ERROR("Failed to allocate memory for tx_list\n");	
		return -1;
	}

	gLte_sdio_info->tx_packet_head->tx_packet = 0;
	gLte_sdio_info->tx_packet_head->size = 0;
	
	INIT_LIST_HEAD(&gLte_sdio_info->tx_packet_head->list);
	spin_lock_init(&gLte_sdio_info->lock);
	gLte_sdio_info->tx_list_size = 0;

/* BEGIN: 0017997 jaegyu.lee@lge.com 2011-03-15 */
/* MOD 0017997: [LTE] Wake-lock period rearrange in LTE SDIO driver */
	wake_lock_init(&gLte_sdio_info->lte_wake_lock, WAKE_LOCK_SUSPEND, "LTE");

	wake_lock_init(&gLte_sdio_info->tx_data_wake_lock, WAKE_LOCK_SUSPEND, "TX_DATA");
	wake_lock_init(&gLte_sdio_info->tx_control_wake_lock, WAKE_LOCK_SUSPEND, "TX_CONTROL");
	wake_lock_init(&gLte_sdio_info->rx_wake_lock, WAKE_LOCK_SUSPEND, "RX");
/* END: 0017997 jaegyu.lee@lge.com 2011-03-15 */   
#ifdef SDIO_TX_TIMER
	gLte_sdio_info->timer = (struct timer_list *)kzalloc(sizeof(struct timer_list), GFP_KERNEL);
	if(gLte_sdio_info->timer==0)
	{
		LTE_ERROR("Failed to allocate memory for tx_timer\n");
		return -1;
	}
	LTE_INFO("Timer created!\n");
#endif

#ifndef SDIO_TX_TIMER
	INIT_WORK(&gLte_sdio_info->tx_worker, lte_sdio_tx);

	gLte_sdio_info->tx_workqueue = create_singlethread_workqueue("lte_tx");
	if (NULL == gLte_sdio_info->tx_workqueue)
	{
		LTE_ERROR("Failed to create work queue\n");
		return -ENOMEM;
	}

/*BEGIN: 0014536 daeok.kim@lge.com 2011-01-22 */
/*ADD 0014536: [LTE] Rx thread is changed by Single thread workqueue in SDIO driver */
	INIT_WORK(&gLte_sdio_info->rx_worker, lte_sdio_rx);

	gLte_sdio_info->rx_workqueue = create_singlethread_workqueue("lte_rx");
	if (NULL == gLte_sdio_info->rx_workqueue)
	{
		LTE_ERROR("Failed to create work queue\n");
		return -ENOMEM;
	}
/*END: 0014536 daeok.kim@lge.com 2011-01-22 */
	
#endif

	/* BEGIN: 0018522 jaegyu.lee@lge.com 2011-03-24 */
	/* MOD 0018522: [LTE] Kernel crash happen, When the SDIO TX & RX fail in 5sec */
	gLte_sdio_info->flag_gpio_l2k_host_status = TRUE;
	/* END: 0018522 jaegyu.lee@lge.com 2011-03-24 */
	


/*BEGIN: 0017497 daeok.kim@lge.com 2011-03-05 */
/*MOD 0017497: [LTE] LTE SW Assert stability is update: blocking of tx operation */
	gLte_sdio_info->flag_tx_blocking =FALSE;
/*END: 0017497 daeok.kim@lge.com 2011-03-05 */	
	return 0;
}

int lte_sdio_drv_deinit(void)
{
#ifdef SDIO_TX_TIMER
	del_timer(gLte_sdio_info->timer);
	kfree(gLte_sdio_info->timer);
#endif

	/* free all tx list */

/*BEGIN: 0014536 daeok.kim@lge.com 2011-01-22 */
/*ADD 0014536: [LTE] Rx thread is changed by Single thread workqueue in SDIO driver */
    /* MUTEX Locked*/
	init_MUTEX_LOCKED(&gLte_sdio_info->tx_lock_mutex);
	init_MUTEX_LOCKED(&gLte_sdio_info->rx_lock_mutex);
/*END: 0014536 daeok.kim@lge.com 2011-01-22 */

	/* free tx list head */
	kfree(gLte_sdio_info->tx_packet_head);

/* BEGIN: 0013584: jihyun.park@lge.com 2011-01-05  */
/* [LTE] Wakeup Scheme  for Power Saving Mode   */
	wake_lock_destroy(&gLte_sdio_info->lte_wake_lock);
/* END: 0013584: jihyun.park@lge.com 2011-01-05 */   			


	/* BEGIN: 0018385 seungyeol.seo@lge.com 2011-03-22 */
	/* MOD 0018385 : [LTE] Rearrangement of 'Wake Lock' section */
	#ifdef SSY_TEST_SUSPENDED_WORKQUEUE
	del_timer(&gLte_sdio_info->rx_workqueue_timer); 
	#endif /*SSY_TEST_SUSPENDED_WORKQUEUE*/
	/* END: 0018385 seungyeol.seo@lge.com 2011-03-22 */



/* BEGIN: 0013584: jihyun.park@lge.com 2011-03-11  */
/* [LTE] Wakeup Scheme  for Power Saving Mode   */
    del_timer(&gLte_sdio_info->lte_timer); 
/* END: 0013584: jihyun.park@lge.com 2011-03-11 */   			
/* BEGIN: 0017997 jaegyu.lee@lge.com 2011-03-15 */
/* MOD 0017997: [LTE] Wake-lock period rearrange in LTE SDIO driver */
	wake_lock_destroy(&gLte_sdio_info->tx_data_wake_lock);
	wake_lock_destroy(&gLte_sdio_info->tx_control_wake_lock);
	wake_lock_destroy(&gLte_sdio_info->rx_wake_lock);	
/* END: 0017997 jaegyu.lee@lge.com 2011-03-15 */
	
#ifndef SDIO_TX_TIMER
	/* release workqueue */
	if (gLte_sdio_info->tx_workqueue != NULL)
	{
		flush_workqueue(gLte_sdio_info->tx_workqueue);
		destroy_workqueue(gLte_sdio_info->tx_workqueue);
		gLte_sdio_info->tx_workqueue = NULL;
	}

/*BEGIN: 0014536 daeok.kim@lge.com 2011-01-22 */
/*ADD 0014536: [LTE] Rx thread is changed by Single thread workqueue in SDIO driver */
	if (gLte_sdio_info->rx_workqueue != NULL)
	{
		flush_workqueue(gLte_sdio_info->rx_workqueue);
		destroy_workqueue(gLte_sdio_info->rx_workqueue);
		gLte_sdio_info->rx_workqueue = NULL;
	}
/*END: 0014536 daeok.kim@lge.com 2011-01-22 */

#endif
}

int lte_sdio_read_transfer_count(unsigned int *count)
{
	int error = 0;
	unsigned char tmp_count = 0;
	
	tmp_count = sdio_readb(gLte_sdio_info->func, AHB_TRANSFER_COUNT, &error);
    if(error != 0)
	{
		LTE_ERROR("Failed to read transfer count. Error = 0x%x\n", error);
		goto err;
	}
	*count=(unsigned int)(tmp_count&0xff);

	tmp_count = sdio_readb(gLte_sdio_info->func, AHB_TRANSFER_COUNT+1, &error);
    if(error != 0)
	{
		LTE_ERROR("Failed to read transfer count. Error = 0x%x\n", error);
		goto err;
	}
	*count = *count | ((unsigned int)(tmp_count & 0xff) << 8);

	tmp_count = sdio_readb(gLte_sdio_info->func, AHB_TRANSFER_COUNT+2, &error);
    if(error != 0)
	{
		LTE_ERROR("Failed to read transfer count. Error = 0x%x\n", error);
		goto err;
	}
	*count = *count | ((unsigned int)(tmp_count & 0xff) << 16);

	tmp_count = sdio_readb(gLte_sdio_info->func, AHB_TRANSFER_COUNT+3, &error);
    if(error != 0)
	{
		LTE_ERROR("Failed to read transfer count. Error = 0x%x\n", error);
		goto err;
	}
	*count = *count | ((unsigned int)(tmp_count & 0xff) << 24);

	*count = *count & AHB_TRANSFER_COUNT_BIT_MASK;

	error = 0;

err:
	return error;
}


int lte_sdio_start_thread(void)
{
	FUNC_ENTER();

#ifdef SDIO_TX_TIMER
	gLte_sdio_info->tx_thread = kthread_run(lte_sdio_tx_thread, NULL, "SDIO_TX");
#endif
/*BEGIN: 0014536 daeok.kim@lge.com 2011-01-22 */
/*MOD 0014536: [LTE] Rx thread is changed by Single thread workqueue in SDIO driver */
//	gLte_sdio_info->rx_thread = kthread_run(lte_sdio_rx_thread, NULL, "SDIO_RX");
/*END: 0014536 daeok.kim@lge.com 2011-01-22 */

#ifdef SDIO_HIM_TEST
	gLte_sdio_info->test_thread = kthread_run(lte_sdio_test_thread, NULL, "SDIO_TEST");
#endif
	FUNC_EXIT();

	return 0;
}

int lte_sdio_end_thread()
{
	FUNC_ENTER();

#ifdef SDIO_TX_TIMER
	up(&gLte_sdio_info->tx_sem);
	kthread_stop(gLte_sdio_info->tx_thread);
	up(&gLte_sdio_info->tx_sem);
#endif

/*BEGIN: 0014536 daeok.kim@lge.com 2011-01-22 */
/*MOD 0014536: [LTE] Rx thread is changed by Single thread workqueue in SDIO driver */
//	up(&gLte_sdio_info->rx_sem);
//	kthread_stop(gLte_sdio_info->rx_thread);
//	up(&gLte_sdio_info->rx_sem);
/*END: 0014536 daeok.kim@lge.com 2011-01-22 */

#ifdef SDIO_HIM_TEST
	up(&gLte_sdio_info->test_sem);
	kthread_stop(gLte_sdio_info->test_thread);
	up(&gLte_sdio_info->test_sem);
#endif
	FUNC_EXIT();
	
	return 0;
}

#ifdef SDIO_HIM_TEST
static int lte_sdio_test_thread(void *unused)
{
	unsigned char *tmp_ip_packet;
	unsigned int packet_size, packet_count;
	int i;

	while(down_interruptible(&gLte_sdio_info->test_sem) == 0)
	{
		if(kthread_should_stop())
			break;

		packet_size = gLte_sdio_info->test_packet_size;
		packet_count = gLte_sdio_info->test_packet_count;

		tmp_ip_packet = kzalloc(packet_size, GFP_KERNEL);
		if (!tmp_ip_packet)
		{	
	        LTE_ERROR("Failed to allocate memory for SDIO\n");
			return -ENOMEM;
		}

		for(i=0; i<packet_size; i++)
		{
			*(tmp_ip_packet+i) = i;
		}

		for(i=0; i<packet_count; i++)
		{
			*(tmp_ip_packet) = i;			
			lte_him_enqueue_ip_packet(tmp_ip_packet, packet_size, 3);
		}

		kfree(tmp_ip_packet);
	}
}
#endif


#ifdef SDIO_TX_TEST
typedef struct
{
	him_block_header blk_hdr; //24
	him_packet_header p_hdr1; //14->16
	char payload1[1996]; // 
	him_packet_header p_hdr2; // 14->16
	char payload2[1996]; //
	him_packet_header p_hdr3; //14->16
	char payload3[1996]; //
} tx_him_blk; // 6060

#endif
/* BEGIN: 0018522 jaegyu.lee@lge.com 2011-03-24 */
/* MOD 0018522: [LTE] Kernel crash happen, When the SDIO TX & RX fail in 5sec */
void lte_sdio_tx_timeout_func(unsigned long arg);
void lte_sdio_rx_timeout_func(unsigned long arg);

int lte_sdio_start_tx_timeout(void)
{
	init_timer(&gLte_sdio_info->tx_timeout);
	/* BEGIN: 0018745 seungyeol.seo@lge.com 2011-04-06 */
	/* MOD 0018745: [LTE] To prevent from late response of L2000 */
	gLte_sdio_info->tx_timeout.expires = jiffies + 10*HZ;
	/* END: 0018745 seungyeol.seo@lge.com 2011-04-06 */   
	gLte_sdio_info->tx_timeout.data = 0;
	gLte_sdio_info->tx_timeout.function = lte_sdio_tx_timeout_func;
	add_timer(&gLte_sdio_info->tx_timeout);
}

int lte_sdio_start_rx_timeout(void)
{
	init_timer(&gLte_sdio_info->rx_timeout);
	/* BEGIN: 0018745 seungyeol.seo@lge.com 2011-04-06 */
	/* MOD 0018745: [LTE] To prevent from late response of L2000 */
	gLte_sdio_info->rx_timeout.expires = jiffies + 10*HZ;
	/* END: 0018745 seungyeol.seo@lge.com 2011-04-06 */   
	gLte_sdio_info->rx_timeout.data = 0;
	gLte_sdio_info->rx_timeout.function = lte_sdio_rx_timeout_func;
	add_timer(&gLte_sdio_info->rx_timeout);
}

void lte_sdio_tx_timeout_func(unsigned long arg)
{
    int* abc;

	LTE_ERROR("SDIO TX failed, because of L2000 doesn't respond\n");

    abc = NULL;
    *abc= 0;	
}

void lte_sdio_rx_timeout_func(unsigned long arg)
{
    int* abc;

	LTE_ERROR("SDIO RX failed, because of L2000 doesn't respond\n");

    abc = NULL;
    *abc= 0;	
}
/* END: 0018522 jaegyu.lee@lge.com 2011-03-24 */   
#ifndef SDIO_TX_TIMER
static void lte_sdio_tx(void *data)
{
	int error, him_block_cnt = 0;
	unsigned long flags;
	unsigned int tmp_him_blk_size = 0, him_packet_num = 0, blk_count = 0, write_length = 0, temp_curr_packet_size = 0, list_count = 0;
	unsigned char *tmp_him_blk, *tmp_him_blk_ptr;
	struct list_head *c_tiem, *c_temp;
	tx_packet_list *current_tx_packet;
#ifdef LTE_STATIC_BUFFER
	static unsigned char *c_tx_buff=0;

	if(!c_tx_buff) {
		c_tx_buff=kzalloc(HIM_BLOCK_TX_MAX_SIZE, GFP_KERNEL);
	}
#endif /* LTE_STATIC_BUFFER */

	mutex_lock(&gLte_sdio_info->tx_lock_mutex);

	/* BEGIN: 0018709: seungyeol.seo@lge.com 2011-03-31  */
	/* MOD 0018709 : [LTE] Wake-lock period rearrangement */
	wake_lock(&gLte_sdio_info->tx_data_wake_lock);
	/* END: 0018709: seungyeol.seo@lge.com 2011-03-31 */   			
/* BEGIN: 0013584: jihyun.park@lge.com 2011-01-05  */
/* [LTE] Wakeup Scheme  for Power Saving Mode   */
#ifdef LTE_WAKE_UP
	gpio_set_value(GPIO_L2K_LTE_WAKEUP,TRUE);			//SET LTE_WU
#endif

/* BEGIN: 0017997 jaegyu.lee@lge.com 2011-03-15 */
/* MOD 0017997: [LTE] Wake-lock period rearrange in LTE SDIO driver */
	/* Wake lock for 2 sec */
//	wake_lock_timeout(&gLte_sdio_info->lte_wake_lock, HZ * 2);
/* END: 0013584: jihyun.park@lge.com 2011-01-05 */   			
//	wake_lock(&gLte_sdio_info->tx_data_wake_lock);
/* END: 0017997 jaegyu.lee@lge.com 2011-03-15 */
	while(him_block_cnt < 5)
	{
		him_block_cnt ++;

		if(!list_empty(&gLte_sdio_info->tx_packet_head->list))		
		{
			tmp_him_blk_size = HIM_BLOCK_HEADER_SIZE;
			him_packet_num = 0;
			blk_count = 0;
			write_length = 0;
			list_count = 0;	

#ifdef LTE_STATIC_BUFFER
			tmp_him_blk	= c_tx_buff;
#else
			/* memory allocation for HIM block */
			tmp_him_blk	= kzalloc(HIM_BLOCK_TX_MAX_SIZE, GFP_KERNEL);
#endif /* LTE_STATIC_BUFFER */

			if(!tmp_him_blk)
			{	
			    LTE_ERROR("Failed to allocate memory\n");

/* BEGIN: 0013584: jihyun.park@lge.com 2011-01-05  */
/* [LTE] Wakeup Scheme  for Power Saving Mode   */			    
#ifdef LTE_WAKE_UP
	                    gpio_set_value(GPIO_L2K_LTE_WAKEUP,FALSE);	//SET LTE_WU
#endif			    
/* END: 0013584: jihyun.park@lge.com 2011-01-05 */   			

				/* BEGIN: 0018385 seungyeol.seo@lge.com 2011-03-22 */
				/* MOD 0018385 : [LTE] Rearrangement of 'Wake Lock' section */
				wake_unlock(&gLte_sdio_info->tx_data_wake_lock);
				mutex_unlock(&gLte_sdio_info->tx_lock_mutex);
				/* END: 0018385 seungyeol.seo@lge.com 2011-03-22 */

				return;
			}

			/* keep HIM block address */
			tmp_him_blk_ptr = tmp_him_blk;

			/* calculate address for HIM packet */
			tmp_him_blk += HIM_BLOCK_HEADER_SIZE;

			/* search tx list */		
			list_for_each_safe(c_tiem, c_temp, &gLte_sdio_info->tx_packet_head->list)
			{
				/* get HIM packet address from list */
				current_tx_packet = (tx_packet_list *)list_entry(c_tiem, tx_packet_list, list);

				/* calculate HIM block size */
				tmp_him_blk_size += current_tx_packet->size; // 4byte aligned HIM packet size

				/* if HIM block size is bigger than max HIM block size, stop! */
				if(tmp_him_blk_size > HIM_BLOCK_TX_MAX_SIZE)
				{
					tmp_him_blk_size -=current_tx_packet->size;
					break;
				}

				list_count++;
				
				/* copy HIM packet from tx list to HIM block */
				memcpy(tmp_him_blk, current_tx_packet->tx_packet, current_tx_packet->size);

				/* calculate size of tx list */
				gLte_sdio_info->tx_list_size -= current_tx_packet->size;

				/* calculate pointer of next HIM packet */
				tmp_him_blk += current_tx_packet->size;

				him_packet_num++;
				list_del(c_tiem);
				kfree(current_tx_packet->tx_packet);
				kfree(current_tx_packet);
			}

			/* fill HIM block header */
			((him_block_header *)(tmp_him_blk_ptr))->seq_num = gLte_sdio_info->him_tx_cnt;	
			((him_block_header *)(tmp_him_blk_ptr))->packet_num = him_packet_num;
			((him_block_header *)(tmp_him_blk_ptr))->total_block_size = tmp_him_blk_size;	
			((him_block_header *)(tmp_him_blk_ptr))->him_block_type = HIM_BLOCK_IP;

//			LTE_INFO("Blcok count = %d\n", him_block_cnt);
//			LTE_INFO("List count = %d\n", list_count);
//			LTE_INFO("HIM block size = %d\n", tmp_him_blk_size);
//			LTE_INFO("HIM block seq = %d\n", gLte_sdio_info->him_tx_cnt);

			if((tmp_him_blk_size % LTE_SDIO_BLK_SIZE) != 0)
			{
				blk_count = (tmp_him_blk_size / LTE_SDIO_BLK_SIZE);
				write_length = (LTE_SDIO_BLK_SIZE * blk_count) + LTE_SDIO_BLK_SIZE;
			}
			else
			{
				write_length = tmp_him_blk_size;
			}

/* BEGIN: 0013584: jihyun.park@lge.com 2011-01-05  */
/* [LTE] Wakeup Scheme  for Power Saving Mode   */	
#ifdef LTE_WAKE_UP

			//printk(" *********** lte_sdio_tx 1*********** \n");	
                        error = lte_sdio_wake_up_tx((void *)tmp_him_blk_ptr, write_length);			
			//printk(" *********** lte_sdio_tx 2*********** \n");	
#else
			sdio_claim_host(gLte_sdio_info->func);
			error = sdio_memcpy_toio(gLte_sdio_info->func, DATA_PORT, tmp_him_blk_ptr, write_length);
			sdio_release_host(gLte_sdio_info->func);
#endif
/* END: 0013584: jihyun.park@lge.com 2011-01-05 */   			

#ifdef LTE_STATIC_BUFFER
#else
			kfree(tmp_him_blk_ptr);
#endif /* LTE_STATIC_BUFFER */

			if(error != 0)
			{
//				LTE_ERROR("Failed to wrtie data to LTE. Error = 0x%x\n", error);

/* BEGIN: 0013584: jihyun.park@lge.com 2011-01-05  */
/* [LTE] Wakeup Scheme  for Power Saving Mode   */
#ifdef LTE_WAKE_UP
	                        gpio_set_value(GPIO_L2K_LTE_WAKEUP,FALSE);	//SET LTE_WU
#endif			    				
				/* BEGIN: 0018385 seungyeol.seo@lge.com 2011-03-22 */
				/* MOD 0018385 : [LTE] Rearrangement of 'Wake Lock' section */
				wake_unlock(&gLte_sdio_info->tx_data_wake_lock);
				/* END: 0018385 seungyeol.seo@lge.com 2011-03-22 */
/* END: 0013584: jihyun.park@lge.com 2011-01-05 */   			
				mutex_unlock(&gLte_sdio_info->tx_lock_mutex);				

				return;
			}

			if(gLte_sdio_info->him_tx_cnt == 0xFFFFFFFF)
				gLte_sdio_info->him_tx_cnt = 1;
			else
				gLte_sdio_info->him_tx_cnt++;
		}

	}

/* BEGIN: 0013584: jihyun.park@lge.com 2011-01-05  */
/* [LTE] Wakeup Scheme  for Power Saving Mode   */	
#ifdef LTE_WAKE_UP
	gpio_set_value(GPIO_L2K_LTE_WAKEUP,FALSE);	//SET LTE_WU
#endif	
/* END: 0013584: jihyun.park@lge.com 2011-01-05 */   			
		    

	if(!list_empty(&gLte_sdio_info->tx_packet_head->list))		
		queue_work(gLte_sdio_info->tx_workqueue, &gLte_sdio_info->tx_worker);

	/* BEGIN: 0018385 seungyeol.seo@lge.com 2011-03-22 */
	/* MOD 0018385 : [LTE] Rearrangement of 'Wake Lock' section */
	wake_unlock(&gLte_sdio_info->tx_data_wake_lock);
	mutex_unlock(&gLte_sdio_info->tx_lock_mutex);
	/* END: 0018385 seungyeol.seo@lge.com 2011-03-22 */

}
#endif

/* BEGIN: 0017997 jaegyu.lee@lge.com 2011-03-15 */
/* MOD 0017997: [LTE] Wake-lock period rearrange in LTE SDIO driver */
int lte_sdio_check_card_status(void)
{
	struct mmc_command cmd;
	int err;

	memset(&cmd, 0, sizeof(struct mmc_command));

	cmd.opcode = SD_IO_RW_DIRECT;
	cmd.arg = 0x00000000;
	cmd.arg |= 0x07 << 9;
	cmd.flags = ((1 << 7)|(1 << 8)) | ((1 << 0)|(1 << 2)|(1 << 4)) | (0 << 5);

	err = mmc_wait_for_cmd(gLte_sdio_info->func->card->host, &cmd, 0);
	if (err)
		return 0;

//	printk("Status=0x%x\n",cmd.resp[0]);

	if((cmd.resp[0] & 0x3000) == 0x2000)
		return 1;
	return 0;

}
/* END: 0017997 jaegyu.lee@lge.com 2011-03-15 */
/* BEGIN: 0013584: jihyun.park@lge.com 2011-01-05  */
/* [LTE] Wakeup Scheme  for Power Saving Mode   */
#ifdef LTE_WAKE_UP
int lte_sdio_wake_up_tx(void * tmp_him_blk_ptr, unsigned int write_length)
{
	int ret = 0, result=0, ret_tx, i;
	
	/* BEGIN: 0018852: seungyeol.seo@lge.com 2011-04-22  */
	/* [LTE] Read done checking & L2K WAKEUP retry for 5 times  */
	int status_checking_cycle=0;
	status_checking_cycle = jiffies + 1*HZ;
	/* END: 0018852: seungyeol.seo@lge.com 2011-04-22 */   			
/* BEGIN: 0013584: jihyun.park@lge.com 2011-03-11  */
/* [LTE] Wakeup Scheme  for Power Saving Mode   */	
	g_tx_loop_skip = 0; 
    lte_sdio_wake_up_tx_init(); 
    add_timer(&gLte_sdio_info->lte_timer); 
/* END: 0013584: jihyun.park@lge.com 2011-03-11 */   			

        do {
	    ret = gpio_get_value(GPIO_L2K_LTE_STATUS);			//READ LTE_ST
	    //printk(" *********** LTE_STATUS= %d, %x*********** \n", ret, ret);	
            if (ret) break;
            
	    //ret_tx = gpio_tlmm_config(GPIO_CFG(GPIO_L2K_LTE_STATUS, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	    //gpio_direction_input(GPIO_L2K_LTE_STATUS);	
	    //usleep(1); ///// add - 100us delay
		/* BEGIN: 0018852: seungyeol.seo@lge.com 2011-04-22  */
//MS910_TEST 60 times retry		/* [LTE] Read done checking & L2K WAKEUP retry for 5 times  */
		if(jiffies > status_checking_cycle || jiffies == status_checking_cycle){
			printk("(%d jiffies) Periodic Checking S \n",jiffies);
			gpio_set_value(GPIO_L2K_LTE_WAKEUP,FALSE);
			mdelay(1);
			gpio_set_value(GPIO_L2K_LTE_WAKEUP,TRUE);
			status_checking_cycle = jiffies + HZ*1;
			//printk("(%d jiffies) Periodic Checking E (target is %d)\n",jiffies,status_checking_cycle);
		}
		/* END: 0018852: seungyeol.seo@lge.com 2011-04-22 */  

/* BEGIN: 0013584: jihyun.park@lge.com 2011-03-11  */
/* [LTE] Wakeup Scheme  for Power Saving Mode   */	
        if( g_tx_loop_skip == 2)
			break;
/* END: 0013584: jihyun.park@lge.com 2011-03-11 */   			
			
        } while(1);

/* BEGIN: 0013584: jihyun.park@lge.com 2011-03-11  */
/* [LTE] Wakeup Scheme  for Power Saving Mode   */	
    del_timer(&gLte_sdio_info->lte_timer); 
    if( g_tx_loop_skip == 2) 
		return result; 
/* END: 0013584: jihyun.park@lge.com 2011-03-11 */   			
/* BEGIN: 0018522 jaegyu.lee@lge.com 2011-03-24 */
/* MOD 0018522: [LTE] Kernel crash happen, When the SDIO TX & RX fail in 5sec */
//	printk("a");
	sdio_claim_host(gLte_sdio_info->func);		
//	printk("b");
	lte_sdio_start_tx_timeout();
    result=sdio_memcpy_toio(gLte_sdio_info->func, DATA_PORT, tmp_him_blk_ptr, write_length);
    del_timer(&gLte_sdio_info->tx_timeout);
	if(result != 0)
	{
		LTE_ERROR("Failed to wrtie data to LTE. Error = 0x%x\n", result);
	}	
/* END: 0018522 jaegyu.lee@lge.com 2011-03-24 */
//	printk("c");
/* BEGIN: 0017997 jaegyu.lee@lge.com 2011-03-15 */
/* MOD 0017997: [LTE] Wake-lock period rearrange in LTE SDIO driver */
		/* BEGIN: 0018852: seungyeol.seo@lge.com 2011-04-22  */
		/* [LTE] Read done checking & L2K WAKEUP retry for 5 times   */
		while(lte_sdio_check_card_status())
			udelay(100);
		/* END: 0018852: seungyeol.seo@lge.com 2011-04-22 */  
/* END: 0017997 jaegyu.lee@lge.com 2011-03-15 */
		sdio_release_host(gLte_sdio_info->func);
//	printk("d\n");
	//printk(" *********** TRANSFER_DATA *********** \n");							 		

	return result;
}


void lte_sdio_wake_up_rx()
{
	int ret = 0;
	ret = gpio_get_value(GPIO_L2K_HOST_STATUS);			//Read HOST_ST
}

/* BEGIN: 0013584: jihyun.park@lge.com 2011-03-11  */
/* [LTE] Wakeup Scheme  for Power Saving Mode   */	
void lte_sdio_wake_up_tx_skip(unsigned long arg)
{
    int* abc;
    // use a global variable for indicating escape a while loop
    g_tx_loop_skip = 2;
	printk(" *********** LTE doesn't respond ! *********** \n");							 		
    // Crash AP with NULL pointer
    abc = NULL;
    *abc= 0;
    
}

void lte_sdio_wake_up_tx_init()
{

    init_timer(&gLte_sdio_info->lte_timer); 
		/* BEGIN: 0018852: seungyeol.seo@lge.com 2011-04-22  */
		/* [LTE] Read done checking & L2K WAKEUP retry for 5 times   */
//MS910_TEST	gLte_sdio_info->lte_timer.expires = jiffies +  5*HZ+1; //5sec + 10msec
		gLte_sdio_info->lte_timer.expires = jiffies +  60*HZ+1; //60sec + 10msec
		/* END: 0018852: seungyeol.seo@lge.com 2011-04-22 */  
	gLte_sdio_info->lte_timer.data = NULL; 
	gLte_sdio_info->lte_timer.function = lte_sdio_wake_up_tx_skip; 
	
}
/* END: 0013584: jihyun.park@lge.com 2011-03-11 */   			

#endif
/* END: 0013584: jihyun.park@lge.com 2011-01-05 */   			

#ifdef SDIO_TX_TIMER
static int lte_sdio_tx_thread(void *unused)
{
#ifdef SDIO_TX_TEST
	int error;
	tx_him_blk *pTemp_him_blk;
#ifdef SDIO_CAL_THROUGHPUT
	struct timespec t_tx, current_t_tx;	
#endif
	pTemp_him_blk = kzalloc(sizeof(tx_him_blk), GFP_KERNEL);
	if (!pTemp_him_blk)
	{	
        LTE_ERROR("Failed to memory allocation for SDIO\n");
		return -ENOMEM;
	}

	pTemp_him_blk->blk_hdr.him_block_type = HIM_BLOCK_IP;
	pTemp_him_blk->blk_hdr.packet_num = 3;
	pTemp_him_blk->blk_hdr.total_block_size = sizeof(tx_him_blk);
//	pTemp_him_blk->blk_hdr.seq_num = 1;
	pTemp_him_blk->p_hdr1.packet_type =  HIM_IP_PACKET_1;
	pTemp_him_blk->p_hdr1.payload_size = 1998;

	pTemp_him_blk->p_hdr2.packet_type =  HIM_IP_PACKET_1;
	pTemp_him_blk->p_hdr2.payload_size = 1998;

	pTemp_him_blk->p_hdr3.packet_type =  HIM_IP_PACKET_1;
	pTemp_him_blk->p_hdr3.payload_size = 1998;
	pTemp_him_blk->payload3[1993] = 'a';	
	pTemp_him_blk->payload3[1994] = 'b';	
	pTemp_him_blk->payload3[1995] = 'c';	
	pTemp_him_blk->payload3[1996] = 'd';	

	msleep_interruptible(3000);

	lte_sdio_register_tx_timer(SDIO_TX_POLL_TIME_MS);

#ifdef SDIO_CAL_THROUGHPUT
	jiffies_to_timespec(jiffies, &current_t_tx);
#endif
	while(down_interruptible(&gLte_sdio_info->tx_sem) == 0)
	{
		if(kthread_should_stop())
			break;

		mutex_lock(&gLte_sdio_info->tx_lock_mutex);

		pTemp_him_blk->blk_hdr.seq_num = gLte_sdio_info->him_tx_cnt;

#ifndef LGE_NO_HARDWARE
		LTE_INFO("Claim\n");
		sdio_claim_host(gLte_sdio_info->func);		
		error = sdio_memcpy_toio(gLte_sdio_info->func, DATA_PORT, pTemp_him_blk, 6144);
		sdio_release_host(gLte_sdio_info->func);
		LTE_INFO("Release\n");
#else
		msleep(LGE_NO_HARDWARE_TX_DELAY);
#endif
		if(error != 0)
		{
			LTE_ERROR("Failed to wrtie data to LTE. Error = 0x%x\n", error);
			break;
		}

#ifdef SDIO_CAL_THROUGHPUT
		gLte_sdio_info->tx_byte_per_sec = gLte_sdio_info->tx_byte_per_sec + 6144;
#endif
		if(gLte_sdio_info->him_tx_cnt == 0xFFFFFFFF)
			gLte_sdio_info->him_tx_cnt = 1;
		else
			gLte_sdio_info->him_tx_cnt++;

		mutex_unlock(&gLte_sdio_info->tx_lock_mutex);		

#ifdef SDIO_CAL_THROUGHPUT
		jiffies_to_timespec(jiffies, &t_tx);
		
		if(t_tx.tv_sec > current_t_tx.tv_sec)
		{
		
			LTE_INFO("TX Throughput = %dBps\n", gLte_sdio_info->tx_byte_per_sec);
			gLte_sdio_info->tx_byte_per_sec = 0;
			current_t_tx.tv_sec ++;
		}
#endif		
	}
#else
	int error, i, him_block_cnt = 0;
	unsigned long flags;
	unsigned int tmp_him_blk_size = 0, him_packet_num = 0, blk_count = 0, write_length = 0, temp_curr_packet_size = 0, list_count = 0;
	unsigned char *tmp_him_blk, *tmp_him_blk_ptr;
	struct list_head *c_tiem, *c_temp;
	tx_packet_list *current_tx_packet;

#ifdef SDIO_TX_TIMER	
	lte_sdio_register_tx_timer(SDIO_TX_POLL_TIME_MS);
#endif

#ifdef SDIO_CAL_THROUGHPUT
	struct timespec t_tx, current_t_tx;	
	jiffies_to_timespec(jiffies, &current_t_tx);
#endif

	while(down_interruptible(&gLte_sdio_info->tx_sem) == 0)
	{
		if(kthread_should_stop())
			break;

		mutex_lock(&gLte_sdio_info->tx_lock_mutex);

		him_block_cnt = 0;

		while(him_block_cnt < 2)
		{
			him_block_cnt++;
			
			if(!list_empty(&gLte_sdio_info->tx_packet_head->list))
			{
				tmp_him_blk_size = HIM_BLOCK_HEADER_SIZE;
				him_packet_num = 0;
				blk_count = 0;
				write_length = 0;
				list_count = 0;	
				
				/* memory allocation for HIM block */
				tmp_him_blk	= kzalloc(HIM_BLOCK_TX_MAX_SIZE, GFP_KERNEL);
				if(!tmp_him_blk)
				{	
				    LTE_ERROR("Failed to allocate memory\n");
					return -ENOMEM;
				}

				/* keep HIM block address */
				tmp_him_blk_ptr = tmp_him_blk;

				/* calculate address for HIM packet */
				tmp_him_blk += HIM_BLOCK_HEADER_SIZE;

				/* search tx list */		
				list_for_each_safe(c_tiem, c_temp, &gLte_sdio_info->tx_packet_head->list)
				{
					/* get HIM packet address from list */
					current_tx_packet = (tx_packet_list *)list_entry(c_tiem, tx_packet_list, list);

					/* calculate HIM block size */
					tmp_him_blk_size += current_tx_packet->size; // 4byte aligned HIM packet size

					/* if HIM block size is bigger than max HIM block size, stop! */
					if(tmp_him_blk_size > HIM_BLOCK_TX_MAX_SIZE)
					{
						tmp_him_blk_size -=current_tx_packet->size;
						break;
					}

					list_count++;

					/* copy HIM packet from tx list to HIM block */
					memcpy(tmp_him_blk, current_tx_packet->tx_packet, current_tx_packet->size);

#if 0
					for(i=0;i<((him_packet_header *)(tmp_him_blk))->payload_size;i++)
						printk("%x ", *(tmp_him_blk+HIM_PACKET_HEADER_SIZE+i));

					printk("\n");
#endif
					/* calculate size of tx list */
					gLte_sdio_info->tx_list_size -= current_tx_packet->size;

					/* calculate pointer of next HIM packet */
					tmp_him_blk += current_tx_packet->size;

					him_packet_num++;
					list_del(c_tiem);
					kfree(current_tx_packet->tx_packet);
					kfree(current_tx_packet);
				}

				/* fill HIM block header */
				((him_block_header *)(tmp_him_blk_ptr))->seq_num = gLte_sdio_info->him_tx_cnt;	
				((him_block_header *)(tmp_him_blk_ptr))->packet_num = him_packet_num;
				((him_block_header *)(tmp_him_blk_ptr))->total_block_size = tmp_him_blk_size;	
				((him_block_header *)(tmp_him_blk_ptr))->him_block_type = HIM_BLOCK_IP;

//				LTE_INFO("Blcok count = %d\n", him_block_cnt);
//				LTE_INFO("List count = %d\n", list_count);
//				LTE_INFO("HIM block size = %d\n", tmp_him_blk_size);

#if 0
				for(i=0;i<tmp_him_blk_size;i++)
					printk("%x ", *(tmp_him_blk_ptr+i));

				printk("\n");
#endif
				if((tmp_him_blk_size % LTE_SDIO_BLK_SIZE) != 0)
				{
					blk_count = (tmp_him_blk_size / LTE_SDIO_BLK_SIZE);
					write_length = (LTE_SDIO_BLK_SIZE * blk_count) + LTE_SDIO_BLK_SIZE;
				}
				else
				{
					write_length = tmp_him_blk_size;
				}
#ifndef LGE_NO_HARDWARE
				LTE_INFO("Claim\n");				
				sdio_claim_host(gLte_sdio_info->func);
				LTE_INFO("Claimed1\n");
				error = sdio_memcpy_toio(gLte_sdio_info->func, DATA_PORT, tmp_him_blk_ptr, write_length);
				LTE_INFO("Claimed2\n");
				sdio_release_host(gLte_sdio_info->func);
				LTE_INFO("Release\n");
#else
				msleep(LGE_NO_HARDWARE_TX_DELAY);
#endif
				if(error != 0)
				{
					LTE_ERROR("Failed to wrtie data to LTE. Error = 0x%x\n", error);
					break;
				}

				kfree(tmp_him_blk_ptr);

	//			LTE_INFO("SDIO write size = %d\n", write_length);
				
				if(gLte_sdio_info->him_tx_cnt == 0xFFFFFFFF)
					gLte_sdio_info->him_tx_cnt = 1;
				else
					gLte_sdio_info->him_tx_cnt++;
			}

		}
		mutex_unlock(&gLte_sdio_info->tx_lock_mutex);

#ifdef SDIO_CAL_THROUGHPUT
		gLte_sdio_info->tx_byte_per_sec += tmp_him_blk_size;

		jiffies_to_timespec(jiffies, &t_tx);
		
		if(t_tx.tv_sec > current_t_tx.tv_sec)
		{
		
			LTE_INFO("TX Throughput = %dBps\n", gLte_sdio_info->tx_byte_per_sec);
			gLte_sdio_info->tx_byte_per_sec = 0;
			current_t_tx.tv_sec ++;
		}
#endif
	}
	return 0;
#endif	
}
#endif



/* BEGIN: 0018385 seungyeol.seo@lge.com 2011-03-22 */
/* MOD 0018385 : [LTE] Rearrangement of 'Wake Lock' section */

#ifdef SSY_TEST_SUSPENDED_WORKQUEUE
void suspended_rx_workqueue_checking_timer_func(unsigned long arg)
{
 	LTE_INFO(" *********** Rx WorkQueue doesn't respond for 2 sec ! *********** \n");							 		
}

void suspended_rx_workqueue_checking_timer_init()
{
    init_timer(&gLte_sdio_info->rx_workqueue_timer); 
	gLte_sdio_info->rx_workqueue_timer.expires = jiffies + 2*HZ;  //2sec
	gLte_sdio_info->rx_workqueue_timer.data = NULL; 
	gLte_sdio_info->rx_workqueue_timer.function = suspended_rx_workqueue_checking_timer_func;
	add_timer(&gLte_sdio_info->rx_workqueue_timer); 	
}
#endif /*SSY_TEST_SUSPENDED_WORKQUEUE*/
/* END: 0018385 seungyeol.seo@lge.com 2011-03-22 */
/*BEGIN: 0014536 daeok.kim@lge.com 2011-01-22 */
/*ADD 0014536: [LTE] Rx thread is changed by Single thread workqueue in SDIO driver */
static void lte_sdio_rx(void *data)
{
	int error;
	unsigned int size;
	unsigned char reg;
#ifdef LTE_STATIC_BUFFER
	static unsigned char *receive_buffer=0;

	if(!receive_buffer) {
		LTE_INFO("Alocation buffer.\n");
		receive_buffer=kzalloc(HIM_BLOCK_RX_MAX_SIZE, GFP_KERNEL);
	}
#else
	unsigned char *receive_buffer;
#endif /* LTE_STATIC_BUFFER */

/* BEGIN: 0018522 jaegyu.lee@lge.com 2011-03-24 */
/* MOD 0018522: [LTE] Kernel crash happen, When the SDIO TX & RX fail in 5sec */
	if(gLte_sdio_info->flag_gpio_l2k_host_status == FALSE){
		/* BEGIN: 0018684 seungyeol.seo@lge.com 2011-03-30 */
		/* MOD 0018684: [LTE] Remove a debug code for tracking the unwoken host */
		// LTE_INFO(" *********** MSM SDCC doesn't resume *********** \n");
		/* END: 0018684 seungyeol.seo@lge.com 2011-03-30 */		
		queue_work(gLte_sdio_info->rx_workqueue, &gLte_sdio_info->rx_worker);
		wake_unlock(&gLte_sdio_info->rx_wake_lock);		
		return;
	}
/* END: 0018522 jaegyu.lee@lge.com 2011-03-24 */

	mutex_lock(&gLte_sdio_info->rx_lock_mutex);
/* BEGIN: 0018385 seungyeol.seo@lge.com 2011-03-22 */
/* MOD 0018385 : [LTE] Rearrangement of 'Wake Lock' section */
	#ifdef SSY_TEST_SUSPENDED_WORKQUEUE
	del_timer(&gLte_sdio_info->rx_workqueue_timer); 
	#endif /*SSY_TEST_SUSPENDED_WORKQUEUE*/	
/* END: 0018385 seungyeol.seo@lge.com 2011-03-22 */

//#ifndef LGE_NO_HARDWARE
/* BEGIN: 0018522 jaegyu.lee@lge.com 2011-03-24 */
/* MOD 0018522: [LTE] Kernel crash happen, When the SDIO TX & RX fail in 5sec */
//	printk("e");
	sdio_claim_host(gLte_sdio_info->func);				
//	printk("f");
	lte_sdio_start_rx_timeout();	
	error = lte_sdio_read_transfer_count(&size);
    del_timer(&gLte_sdio_info->rx_timeout);
    if(error != 0)
	{
		LTE_ERROR("Failed to read transfer count. Error = 0x%x\n", error);
    }
//	printk("g");
	sdio_release_host(gLte_sdio_info->func);
//	printk("h\n");

//#else
//	    error = -ENXIO;
//	    size = 0;
//#endif
    if(error != 0)
	{
//		LTE_ERROR("Failed to read transfer count. Error = 0x%x\n", error);
/* END: 0018522 jaegyu.lee@lge.com 2011-03-24 */		
/* BEGIN: 0017997 jaegyu.lee@lge.com 2011-03-15 */
/* MOD 0017997: [LTE] Wake-lock period rearrange in LTE SDIO driver */
		wake_unlock(&gLte_sdio_info->rx_wake_lock);		
/* END: 0017997 jaegyu.lee@lge.com 2011-03-15 */


/* BEGIN: 0018385 seungyeol.seo@lge.com 2011-03-22 */
/* MOD 0018385 : [LTE] Rearrangement of 'Wake Lock' section */
		mutex_unlock(&gLte_sdio_info->rx_lock_mutex);
/* END: 0018385 seungyeol.seo@lge.com 2011-03-22 */

		return;
	}

	if(size == 0)
	{
		LTE_ERROR("Read size is zero!!\n");
		
/* BEGIN: 0017997 jaegyu.lee@lge.com 2011-03-15 */
/* MOD 0017997: [LTE] Wake-lock period rearrange in LTE SDIO driver */
		wake_unlock(&gLte_sdio_info->rx_wake_lock);		
/* END: 0017997 jaegyu.lee@lge.com 2011-03-15 */

/* BEGIN: 0018385 seungyeol.seo@lge.com 2011-03-22 */
/* MOD 0018385 : [LTE] Rearrangement of 'Wake Lock' section */
		mutex_unlock(&gLte_sdio_info->rx_lock_mutex);
/* END: 0018385 seungyeol.seo@lge.com 2011-03-22 */

		return;
	}
#ifdef LTE_STATIC_BUFFER
	if(size >HIM_BLOCK_RX_MAX_SIZE) {
		LTE_ERROR("size=%d is more then MAX !!!!!!!!!!!!!!! %s\n",size,__func__);
	}
#else
	receive_buffer = kzalloc(size, GFP_KERNEL);
#endif /* LTE_STATIC_BUFFER */

	if(!receive_buffer)
	{
		LTE_ERROR("Failed to allocate memory. Error = 0x%x\n", (unsigned int)receive_buffer);
/* BEGIN: 0017997 jaegyu.lee@lge.com 2011-03-15 */
/* MOD 0017997: [LTE] Wake-lock period rearrange in LTE SDIO driver */
		wake_unlock(&gLte_sdio_info->rx_wake_lock);			
/* BEGIN: 0018385 seungyeol.seo@lge.com 2011-03-22 */
/* MOD 0018385 : [LTE] Rearrangement of 'Wake Lock' section */
		mutex_unlock(&gLte_sdio_info->rx_lock_mutex);
/* END: 0018385 seungyeol.seo@lge.com 2011-03-22 */
/* END: 0017997 jaegyu.lee@lge.com 2011-03-15 */
/* BEGIN: 0018385 seungyeol.seo@lge.com 2011-03-22 */
/* MOD 0018385 : [LTE] Rearrangement of 'Wake Lock' section */
		return;
/* END: 0018385 seungyeol.seo@lge.com 2011-03-22 */
	}

//#ifndef LGE_NO_HARDWARE

/* BEGIN: 0018385 seungyeol.seo@lge.com 2011-03-22 */
/* MOD 0018385 : [LTE] Rearrangement of 'Wake Lock' section */
		wake_lock_timeout(&gLte_sdio_info->lte_wake_lock, HZ * 2 ); 
		wake_unlock(&gLte_sdio_info->rx_wake_lock);
/* END: 0018385 seungyeol.seo@lge.com 2011-03-22 */
/* BEGIN: 0018522 jaegyu.lee@lge.com 2011-03-24 */
/* MOD 0018522: [LTE] Kernel crash happen, When the SDIO TX & RX fail in 5sec */
//	printk("i");
	sdio_claim_host(gLte_sdio_info->func);				
//	printk("j");
	lte_sdio_start_rx_timeout();		
	error = sdio_memcpy_fromio(gLte_sdio_info->func, (void *)receive_buffer, DATA_PORT, size);
    del_timer(&gLte_sdio_info->rx_timeout);
    if(error != 0)
	{
		LTE_ERROR("Failed to read data from LTE. Error = 0x%x\n", error);
    }
/* END: 0018522 jaegyu.lee@lge.com 2011-03-24 */
//	printk("k");
	sdio_release_host(gLte_sdio_info->func);
//	printk("l\n");

//#else
//	    error = -ENXIO;
//	    size = 0;
//#endif
    if(error != 0)
	{
//		LTE_ERROR("Failed to read data from LTE. Error = 0x%x\n", error);
#ifdef LTE_STATIC_BUFFER
#else
		kfree(receive_buffer);
#endif /* LTE_STATIC_BUFFER */
		mutex_unlock(&gLte_sdio_info->rx_lock_mutex);
/* BEGIN: 0017997 jaegyu.lee@lge.com 2011-03-15 */
/* MOD 0017997: [LTE] Wake-lock period rearrange in LTE SDIO driver */
/* BEGIN: 0018385 seungyeol.seo@lge.com 2011-03-22 */
/* MOD 0018385 : [LTE] Rearrangement of 'Wake Lock' section */
		//wake_unlock(&gLte_sdio_info->rx_wake_lock);			
/* END: 0018385 seungyeol.seo@lge.com 2011-03-22 */

/* END: 0017997 jaegyu.lee@lge.com 2011-03-15 */
		return;
	}
/*
#ifdef SDIO_CAL_THROUGHPUT
	gLte_sdio_info->rx_byte_per_sec = gLte_sdio_info->rx_byte_per_sec + size;

	jiffies_to_timespec(jiffies, &t_rx);
	
	if(t_rx.tv_sec > current_t_rx.tv_sec)
	{
	
		LTE_INFO("RX Throughput = %dBps\n", gLte_sdio_info->rx_byte_per_sec);
		gLte_sdio_info->rx_byte_per_sec = 0;
		current_t_rx.tv_sec ++;
	}
#endif
*/
	gLte_sdio_info->rx_buff = receive_buffer;
	gLte_sdio_info->rx_size = size;

//#ifdef SDIO_LOOPBACK_TEST
//			gLte_sdio_info->him_rx_cnt++;
//#else
	lte_him_parsing_blk();
//#endif
	
#ifdef LTE_STATIC_BUFFER
#else
	kfree(receive_buffer);
#endif /* LTE_STATIC_BUFFER */


	

	mutex_unlock(&gLte_sdio_info->rx_lock_mutex);
	

	return;
}
/*END: 0014536 daeok.kim@lge.com 2011-01-22 */

/*BEGIN: 0014536 daeok.kim@lge.com 2011-01-22 */
/*MOD 0014536: [LTE] Rx thread is changed by Single thread workqueue in SDIO driver */
#if 0
static int lte_sdio_rx_thread(void *unused)
{
	int error;
	unsigned int size;
	unsigned char reg;
	unsigned char *receive_buffer;

#ifdef SDIO_CAL_THROUGHPUT
	struct timespec t_rx, current_t_rx;	
	jiffies_to_timespec(jiffies, &current_t_rx);
#endif

	while(down_interruptible(&gLte_sdio_info->rx_sem) == 0)
	{
		if(kthread_should_stop())
			break;

		mutex_lock(&gLte_sdio_info->rx_lock_mutex);

/* BEGIN: 0013584: jihyun.park@lge.com 2011-01-05  */
/* [LTE] Wakeup Scheme  for Power Saving Mode   */ 
		/* Wake lock for 3 sec */
		wake_lock_timeout(&gLte_sdio_info->lte_wake_lock, HZ / 2);
/* END: 0013584: jihyun.park@lge.com 2011-01-05 */   			

#ifndef LGE_NO_HARDWARE
		sdio_claim_host(gLte_sdio_info->func);				
		error = lte_sdio_read_transfer_count(&size);
		sdio_release_host(gLte_sdio_info->func);
#else
	    error = -ENXIO;
	    size = 0;
#endif
	    if(error != 0)
		{
			LTE_ERROR("Failed to read transfer count. Error = 0x%x\n", error);
			mutex_unlock(&gLte_sdio_info->rx_lock_mutex);
			continue;
		}

		if(size == 0)
		{
			LTE_ERROR("Read size is zero!!\n");
			mutex_unlock(&gLte_sdio_info->rx_lock_mutex);
			continue;
		}

		receive_buffer = kzalloc(size, GFP_KERNEL);
		if(!receive_buffer)
		{
			LTE_ERROR("Failed to allocate memory. Error = 0x%x\n", (unsigned int)receive_buffer);
		}

#ifndef LGE_NO_HARDWARE
		sdio_claim_host(gLte_sdio_info->func);	
		error = sdio_memcpy_fromio(gLte_sdio_info->func, (void *)receive_buffer, DATA_PORT, size);
		sdio_release_host(gLte_sdio_info->func);
#else
	    error = -ENXIO;
	    size = 0;
#endif
	    if(error != 0)
		{
			LTE_ERROR("Failed to read data from LTE. Error = 0x%x\n", error);
			kfree(receive_buffer);
			mutex_unlock(&gLte_sdio_info->rx_lock_mutex);
			continue;
		}

#ifdef SDIO_CAL_THROUGHPUT
		gLte_sdio_info->rx_byte_per_sec = gLte_sdio_info->rx_byte_per_sec + size;

		jiffies_to_timespec(jiffies, &t_rx);
		
		if(t_rx.tv_sec > current_t_rx.tv_sec)
		{
		
			LTE_INFO("RX Throughput = %dBps\n", gLte_sdio_info->rx_byte_per_sec);
			gLte_sdio_info->rx_byte_per_sec = 0;
			current_t_rx.tv_sec ++;
		}
#endif

		gLte_sdio_info->rx_buff = receive_buffer;
		gLte_sdio_info->rx_size = size;

#ifdef SDIO_LOOPBACK_TEST
		gLte_sdio_info->him_rx_cnt++;
#else
		lte_him_parsing_blk();
#endif
		kfree(receive_buffer);

		mutex_unlock(&gLte_sdio_info->rx_lock_mutex);
	}

	return 0;
}
#endif
/*END: 0014536 daeok.kim@lge.com 2011-01-22 */

static void lte_sdio_isr(struct sdio_func *func)
{
	int error;
	unsigned char reg;
	
	
/* BEGIN: 0018385 seungyeol.seo@lge.com 2011-03-22 */
/* MOD 0018385 : [LTE] Rearrangement of 'Wake Lock' section */
	#ifdef SSY_TEST_SUSPENDED_WORKQUEUE
	suspended_rx_workqueue_checking_timer_init();
	#endif /*SSY_TEST_SUSPENDED_WORKQUEUE*/
/* END: 0018385 seungyeol.seo@lge.com 2011-03-22 */
	
	

//	FUNC_ENTER();
/* BEGIN: 0017997 jaegyu.lee@lge.com 2011-03-15 */
/* MOD 0017997: [LTE] Wake-lock period rearrange in LTE SDIO driver */
//	wake_lock_timeout(&gLte_sdio_info->lte_wake_lock, HZ * 2);

	wake_lock(&gLte_sdio_info->rx_wake_lock);
	reg = sdio_readb(gLte_sdio_info->func, INTERRUPT_ID, &error);
    if(error != 0)
	{
		LTE_ERROR("Failed to read interrupt status. Error = 0x%x\n", error);
		wake_unlock(&gLte_sdio_info->rx_wake_lock);
		goto err;
	}
/* END: 0017997 jaegyu.lee@lge.com 2011-03-15 */
/*BEGIN: 0014536 daeok.kim@lge.com 2011-01-22 */
/*MOD 0014536: [LTE] Rx thread is changed by Single thread workqueue in SDIO driver */
	//up(&gLte_sdio_info->rx_sem);
	queue_work(gLte_sdio_info->rx_workqueue, &gLte_sdio_info->rx_worker);
/*END: 0014536 daeok.kim@lge.com 2011-01-22 */

err:
//	FUNC_EXIT();	
	return;
}

/* BEGIN: 0013584: jihyun.park@lge.com 2011-01-05  */
/* [LTE] Wakeup Scheme  for Power Saving Mode   */
int wakeup_rx_resume(void)
{
	gpio_set_value(GPIO_L2K_HOST_STATUS,TRUE);			//Set HOST_ST	
	//disable_irq(irq_rx);   
        //printk(" *********** RX resume *********** \n");	

	/* BEGIN: 0018522 jaegyu.lee@lge.com 2011-03-24 */
	/* MOD 0018522: [LTE] Kernel crash happen, When the SDIO TX & RX fail in 5sec */
	gLte_sdio_info->flag_gpio_l2k_host_status = TRUE;
	/* END: 0018522 jaegyu.lee@lge.com 2011-03-24 */
    
}

int wakeup_rx_suspend(void)
{

/* BEGIN: 0013584: jihyun.park@lge.com 2011-02-11  */
/* [LTE] Wakeup Scheme  for Power Saving Mode   */
	int ret = 0;

        ret = gpio_get_value(GPIO_L2K_HOST_WAKEUP);			//READ HOST_WP	
        if (ret){
	/* Wake lock for 2 sec */
/* BEGIN: 0017997 jaegyu.lee@lge.com 2011-03-15 */
/* MOD 0017997: [LTE] Wake-lock period rearrange in LTE SDIO driver */
	    wake_lock_timeout(&gLte_sdio_info->lte_wake_lock, HZ * 2 ); 
/* END: 0017997 jaegyu.lee@lge.com 2011-03-15 */
	    gpio_set_value(GPIO_L2K_HOST_STATUS,TRUE);			//Set HOST_ST= HIGH
            //printk(" *********** RX suspend *********** \n");			    
		/* BEGIN: 0018522 jaegyu.lee@lge.com 2011-03-24 */
		/* MOD 0018522: [LTE] Kernel crash happen, When the SDIO TX & RX fail in 5sec */
		gLte_sdio_info->flag_gpio_l2k_host_status = TRUE;
		/* END: 0018522 jaegyu.lee@lge.com 2011-03-24 */
	} else {
/* END: 0013584: jihyun.park@lge.com 2011-02-11 */   			

	    gpio_set_value(GPIO_L2K_HOST_STATUS, 0 );			//Set HOST_ST= LOW	
	    



        /* BEGIN: 0018522 jaegyu.lee@lge.com 2011-03-24 */
        /* MOD 0018522: [LTE] Kernel crash happen, When the SDIO TX & RX fail in 5sec */
        gLte_sdio_info->flag_gpio_l2k_host_status = FALSE;
        /* END: 0018522 jaegyu.lee@lge.com 2011-03-24 */

		
	    //enable_irq(irq_rx);
            //printk(" *********** RX suspend *********** \n");		
        }


}
/* END: 0013584: jihyun.park@lge.com 2011-01-05 */   			
