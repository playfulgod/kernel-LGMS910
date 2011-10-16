/*
 * LGE L2000 HIM layer
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
	
#include <linux/slab.h>

#include "lte_sdio.h"
#include "lte_debug.h"
#include "lte_him.h"

/* BEGIN: 0013584: jihyun.park@lge.com 2011-01-05  */
/* [LTE] Wakeup Scheme  for Power Saving Mode   */
#include <linux/gpio.h>
#include <linux/unistd.h>
/* END: 0013584: jihyun.park@lge.com 2011-01-05 */   			

#ifdef CONFIG_LGE_LTE_ERS
#include <mach/lge_mem_misc.h>
#undef LGE_LTE_ERS_DBG
#endif

extern PLTE_SDIO_INFO gLte_sdio_info;

#ifdef RACE_LTE_TEST
typedef struct _lte_hi_command_header
{
	unsigned short cmd;  	/* command id */
	unsigned short len;		/* command length */
} lte_hi_command_header;

typedef struct _lte_dummy
{
	lte_hi_command_header	header;
	unsigned int		param1;
	unsigned int		param2;
	unsigned int		param3;
	unsigned int		param4;
	unsigned int		param5;
} lte_dummy_info;
#endif /* RACE_LTE_TEST */

#ifdef CONFIG_LGE_LTE_ERS
/* BEGIN: 0013575 seongmook.yim@lge.com 2011-01-05 */
/* ADD 0013575: [LTE] LTE S/W assert case handling */
extern int lte_crash_log(void *buffer, unsigned int size, unsigned int reserved);
extern int lte_crash_log_dump(void *buffer, unsigned int size, unsigned int file_num, unsigned int file_seq, unsigned int file_attr);
/* END: 0013575 seongmook.yim@lge.com 2011-01-05 */   
#endif /* LG_FW_COMPILE_ERROR */

static unsigned int lte_him_calc_dummy(unsigned int packet_size)
{
	unsigned int temp_align, dummy_size, total_packet_size = 0;
	
	temp_align = packet_size % HIM_BLOCK_BYTE_ALIGN;
	
	if(temp_align)	
		dummy_size = HIM_BLOCK_BYTE_ALIGN - temp_align;
/* defect */
/* when temp_align is false(0) dummy_size should defined */
	else
		dummy_size = 0;
/* defect */	
	total_packet_size = packet_size + dummy_size ;

	return	total_packet_size;
}

int lte_him_register_cb_from_ved(lte_him_cb_fn_t cb)
{
	if (gLte_sdio_info == NULL) {
		return -ENXIO;
	}
	
	gLte_sdio_info->callback_from_ved = cb;

	return 0;
}

int lte_him_enqueue_ip_packet(unsigned char *data, int size, int pdn)
{
	int error = 0;
	unsigned int packet_length, curr_pdn = 0;
	unsigned long flags;
	unsigned char *tmp_packet;	
	tx_packet_list *curr_item;


/* BEGIN: 0018522 jaegyu.lee@lge.com 2011-03-24 */
/* MOD 0018522: [LTE] Kernel crash happen, When the SDIO TX & RX fail in 5sec */
	if(gLte_sdio_info->flag_gpio_l2k_host_status == FALSE){
		/* BEGIN: 0018684 seungyeol.seo@lge.com 2011-03-30 */
		/* MOD 0018684: [LTE] Remove a debug code for tracking the unwoken host */
		// LTE_INFO(" *********** MSM SDCC doesn't resume *********** \n");
		/* END: 0018684 seungyeol.seo@lge.com 2011-03-30 */
		return -EIO;
	}
/* END: 0018522 jaegyu.lee@lge.com 2011-03-24 */

	if (gLte_sdio_info == NULL) {
		return -ENXIO;
	}
	if(size==0 || data == NULL)
	{
		return -EINVAL;
	}

/*BEGIN: 0017497 daeok.kim@lge.com 2011-03-05 */
/*MOD 0017497: [LTE] LTE SW Assert stability is update: blocking of tx operation */
	if (gLte_sdio_info->flag_tx_blocking == TRUE)
	{
		LTE_ERROR("[LTE_ASSERT] SDIO Tx(IP data) blocking, return -EIO \n");
		return -EIO;
	}
/*END: 0017497 daeok.kim@lge.com 2011-03-05 */

	mutex_lock(&gLte_sdio_info->tx_lock_mutex);	

	/* packet_length = 4 byte aligned HIM packet size */
	packet_length = lte_him_calc_dummy(HIM_PACKET_HEADER_SIZE + size);

	/* memory allocation for tx packet list */
	curr_item = (tx_packet_list *)kzalloc(sizeof(tx_packet_list), GFP_KERNEL);
	if(!curr_item)
	{
/* BEGIN: 0011698 jaegyu.lee@lge.com 2010-12-01 */
/* ADD 0011698: [LTE] ADD : Debug message for him packet */
		LTE_ERROR("Failed to allocate memory\n");
/* END : 0011698 jaegyu.lee@lge.com 2010-12-01 */
		mutex_unlock(&gLte_sdio_info->tx_lock_mutex);			
		return -ENOMEM;
	}

	/* memory allocation for tx packet */
	tmp_packet = (unsigned char *)kzalloc(packet_length, GFP_KERNEL);
	if(!tmp_packet)
	{
/* BEGIN: 0011698 jaegyu.lee@lge.com 2010-12-01 */
/* ADD 0011698: [LTE] ADD : Debug message for him packet */

		LTE_ERROR("Failed to allocate memory\n");
/* END : 0011698 jaegyu.lee@lge.com 2010-12-01 */
		kfree(curr_item);
		mutex_unlock(&gLte_sdio_info->tx_lock_mutex);			
		return -ENOMEM;
	}

	curr_item->tx_packet = tmp_packet;
	curr_item->size = packet_length;	

	switch(pdn)
	{
		case 0:
			curr_pdn = HIM_NIC_1;
			break;
		case 1:
			curr_pdn = HIM_NIC_2;			
			break;
		case 2:
			curr_pdn = HIM_NIC_3;			
			break;
		case 3:
			curr_pdn = HIM_IP_PACKET_1;
			break;
		case 4:
			curr_pdn = HIM_IP_PACKET_2;			
			break;
		case 5:
			curr_pdn = HIM_IP_PACKET_3;
			break;
		case 6:
			curr_pdn = HIM_IP_PACKET_4;			
			break;
		case 7:
			curr_pdn = HIM_IP_PACKET_5;	
			break;
		case 8:
			curr_pdn = HIM_IP_PACKET_6;			
			break;
		case 9:
			curr_pdn = HIM_IP_PACKET_7;	
			break;
		case 10:
			curr_pdn = HIM_IP_PACKET_8;	
			break;
		default:
/* defect */
/* When default case, free the memory */
			mutex_unlock(&gLte_sdio_info->tx_lock_mutex);				
			kfree(curr_item);
/* defect */

/* BEGIN: 0011698 jaegyu.lee@lge.com 2010-12-01 */
/* ADD 0011698: [LTE] ADD : Debug message for him packet */
			LTE_ERROR("Unsupported PDN\n");
/* END : 0011698 jaegyu.lee@lge.com 2010-12-01 */
			return -EIO;
	}


	((him_packet_header *)(tmp_packet))->payload_size = (unsigned short)size;	
	((him_packet_header *)(tmp_packet))->packet_type = curr_pdn;

	memcpy(tmp_packet + HIM_PACKET_HEADER_SIZE, data, size);

	list_add_tail(&curr_item->list, &gLte_sdio_info->tx_packet_head->list);
	gLte_sdio_info->tx_list_size += packet_length;

	mutex_unlock(&gLte_sdio_info->tx_lock_mutex);	

#ifndef SDIO_TX_TIMER
	queue_work(gLte_sdio_info->tx_workqueue, &gLte_sdio_info->tx_worker);
#endif

	return error;
}


int lte_him_write_control_packet(const unsigned char *data, int size)
{
	unsigned char *tmp_him_blk;
	int error, blk_count = 0;
	unsigned int him_blk_length, packet_length, write_length = 0;
	unsigned long flags;
	
#ifdef RACE_LTE_TEST
  lte_dummy_info* lte_dummy_packet_ptr = NULL;
#endif /* RACE_LTE_TEST */

	if (gLte_sdio_info == NULL) {
		return -ENXIO;
	}


//	FUNC_ENTER();
	/* BEGIN: 0018385 seungyeol.seo@lge.com 2011-03-22 */
	/* MOD 0018385 : [LTE] Rearrangement of 'Wake Lock' section */
	//mutex_lock(&gLte_sdio_info->tx_lock_mutex);
	/* END: 0018385 seungyeol.seo@lge.com 2011-03-22 */

/* BEGIN: 0013584: jihyun.park@lge.com 2011-01-05  */
/* [LTE] Wakeup Scheme  for Power Saving Mode   */	
#ifdef LTE_WAKE_UP
	gpio_set_value(GPIO_L2K_LTE_WAKEUP,TRUE);			//SET LTE_WU
#endif	
/* BEGIN: 0015024 jaegyu@lge.com 2011-01-29 */
/* MOD 0015024: [LTE] AP power collapse move wake lock point */
	/* Wake lock for 0.5 sec */
/* END: 0015024 jaegyu@lge.com 2011-01-29 */
/* BEGIN: 0017997 jaegyu.lee@lge.com 2011-03-15 */
/* MOD 0017997: [LTE] Wake-lock period rearrange in LTE SDIO driver */
//	wake_lock_timeout(&gLte_sdio_info->lte_wake_lock, HZ / 2);
/* END: 0017997 jaegyu.lee@lge.com 2011-03-15 */   

/* END: 0013584: jihyun.park@lge.com 2011-01-05 */   			

	packet_length = lte_him_calc_dummy(HIM_PACKET_HEADER_SIZE + size);
	him_blk_length = HIM_BLOCK_HEADER_SIZE + packet_length;

#ifdef RACE_LTE_TEST
	lte_dummy_packet_ptr = data;
	LTE_INFO("[REQ] cmd = 0x%x, param1 = 0x%x \n",lte_dummy_packet_ptr->header.cmd,lte_dummy_packet_ptr->param1);
	LTE_INFO("[REQ] HIM Write beform size=%d // pack_size=%d // him_blk_size=%d\n",size,packet_length,him_blk_length);
#endif /* RACE_LTE_TEST */
	
	tmp_him_blk = (unsigned char *)kzalloc(him_blk_length, GFP_KERNEL);
	if(!tmp_him_blk)
	{

/* BEGIN: 0011698 jaegyu.lee@lge.com 2010-12-01 */
/* ADD 0011698: [LTE] ADD : Debug message for him packet */
		LTE_ERROR("Failed to allocate memory\n");
/* END : 0011698 jaegyu.lee@lge.com 2010-12-01 */

/* BEGIN: 0013584: jihyun.park@lge.com 2011-01-05  */
/* [LTE] Wakeup Scheme  for Power Saving Mode   */	
#ifdef LTE_WAKE_UP
	        gpio_set_value(GPIO_L2K_LTE_WAKEUP,FALSE);	//SET LTE_WU
#endif			    
/* END: 0013584: jihyun.park@lge.com 2011-01-05 */   			

		/* BEGIN: 0018385 seungyeol.seo@lge.com 2011-03-22 */
		/* MOD 0018385 : [LTE] Rearrangement of 'Wake Lock' section */
		//mutex_unlock(&gLte_sdio_info->tx_lock_mutex);			
		/* END: 0018385 seungyeol.seo@lge.com 2011-03-22 */
		return -ENOMEM;
	}
	
	((him_block_header *)(tmp_him_blk))->total_block_size = him_blk_length;
	((him_block_header *)(tmp_him_blk))->seq_num = gLte_sdio_info->him_tx_cnt;	
	((him_block_header *)(tmp_him_blk))->packet_num = 1;
/* BEGIN: 0012603 jaegyu.lee@lge.com 2010-12-17 */
/* MOD 0012603: [LTE] HIM block parsing policy change */
	((him_block_header *)(tmp_him_blk))->him_block_type = HIM_BLOCK_MISC;
/* END: 0012603 jaegyu.lee@lge.com 2010-12-17 */

	((him_packet_header *)(tmp_him_blk + HIM_BLOCK_HEADER_SIZE))->payload_size = (unsigned short)size;	
	((him_packet_header *)(tmp_him_blk + HIM_BLOCK_HEADER_SIZE))->packet_type = HIM_CONTROL_PACKET;	

	memcpy(tmp_him_blk + HIM_BLOCK_HEADER_SIZE + HIM_PACKET_HEADER_SIZE, data, size);

	if((him_blk_length % LTE_SDIO_BLK_SIZE) != 0)
	{
		blk_count = (him_blk_length / LTE_SDIO_BLK_SIZE);
		write_length = (LTE_SDIO_BLK_SIZE * blk_count) + LTE_SDIO_BLK_SIZE;
	}
	else
	{
		write_length = him_blk_length;
	}

#ifndef LGE_NO_HARDWARE

/* BEGIN: 0013584: jihyun.park@lge.com 2011-01-05  */
/* [LTE] Wakeup Scheme  for Power Saving Mode   */	
#ifdef LTE_WAKE_UP
	//printk(" ************* lte_him_write_control_packet 1**************\n");		
	error = lte_sdio_wake_up_tx((void *)tmp_him_blk, write_length);					
	//printk(" ************* lte_him_write_control_packet 2**************\n");					
#else
	sdio_claim_host(gLte_sdio_info->func);
	error = sdio_memcpy_toio(gLte_sdio_info->func, DATA_PORT, (void*)tmp_him_blk, write_length);			
	sdio_release_host(gLte_sdio_info->func);
#endif
/* END: 0013584: jihyun.park@lge.com 2011-01-05 */   			

#else
	error = 0;
#endif

	if(error != 0)
	{
		LTE_ERROR("Failed to wrtie data to LTE. Error = 0x%x\n", error);
	}
	else
	{
		if(gLte_sdio_info->him_tx_cnt == 0xFFFFFFFF)
			gLte_sdio_info->him_tx_cnt = 1;
		else
			gLte_sdio_info->him_tx_cnt++;
	}

	kfree(tmp_him_blk);

/* BEGIN: 0013584: jihyun.park@lge.com 2011-01-05  */
/* [LTE] Wakeup Scheme  for Power Saving Mode   */	
#ifdef LTE_WAKE_UP
	gpio_set_value(GPIO_L2K_LTE_WAKEUP,FALSE);	//SET LTE_WU
#endif			    
/* END: 0013584: jihyun.park@lge.com 2011-01-05 */   			

	/* BEGIN: 0018385 seungyeol.seo@lge.com 2011-03-22 */
	/* MOD 0018385 : [LTE] Rearrangement of 'Wake Lock' section */
	//mutex_unlock(&gLte_sdio_info->tx_lock_mutex);
	/* END: 0018385 seungyeol.seo@lge.com 2011-03-22 */

//	FUNC_EXIT();

	return error;
}
#ifdef LTE_ROUTING
int lte_him_write_control_packet_tty_lldm(const unsigned char *data, int size)
{
	//FUNC_ENTER();
	unsigned char *tmp_lldm_him_blk;
	int error, blk_count = 0;
	unsigned int him_blk_length, packet_length, write_length = 0;
	unsigned long flags;

	if (gLte_sdio_info == NULL) {
		return -ENXIO;
	}
	#ifdef LTE_WAKE_UP
	/* SET LTE_WU = HIGH */
	gpio_set_value(GPIO_L2K_LTE_WAKEUP,TRUE);	
	#endif	/* LTE_WAKE_UP */

	packet_length = lte_him_calc_dummy(HIM_PACKET_HEADER_SIZE + size);
	him_blk_length = HIM_BLOCK_HEADER_SIZE + packet_length;

	/* Memery allocation for the HIM packet */
	tmp_lldm_him_blk = (unsigned char *)kzalloc(him_blk_length, GFP_KERNEL);
	if(!tmp_lldm_him_blk)
	{
		LTE_ERROR("Failed to allocate memory\n");
		#ifdef LTE_WAKE_UP
		/* SET LTE_WU = LOW */
		gpio_set_value(GPIO_L2K_LTE_WAKEUP,FALSE);	
		#endif /* LTE_WAKE_UP */		    
		wake_unlock(&gLte_sdio_info->tx_control_wake_lock);
		mutex_unlock(&gLte_sdio_info->tx_lock_mutex);			
		return -ENOMEM;
	}

	/* Packing a HIM packet */	
	((him_block_header *)(tmp_lldm_him_blk))->total_block_size = him_blk_length;
	((him_block_header *)(tmp_lldm_him_blk))->seq_num = gLte_sdio_info->him_tx_cnt;	
	((him_block_header *)(tmp_lldm_him_blk))->packet_num = 1;
	((him_block_header *)(tmp_lldm_him_blk))->him_block_type = HIM_BLOCK_MISC;
	((him_packet_header *)(tmp_lldm_him_blk + HIM_BLOCK_HEADER_SIZE))->payload_size = (unsigned short)size;	
	((him_packet_header *)(tmp_lldm_him_blk + HIM_BLOCK_HEADER_SIZE))->packet_type = HIM_LLDM_PACKET;	

	memcpy(tmp_lldm_him_blk + HIM_BLOCK_HEADER_SIZE + HIM_PACKET_HEADER_SIZE, data, size);

	if((him_blk_length % LTE_SDIO_BLK_SIZE) != 0)
	{
		blk_count = (him_blk_length / LTE_SDIO_BLK_SIZE);
		write_length = (LTE_SDIO_BLK_SIZE * blk_count) + LTE_SDIO_BLK_SIZE;
	}
	else
	{
		write_length = him_blk_length;
	}

	/* Write a packet to SDIO channel */
	#ifndef LGE_NO_HARDWARE
	#ifdef LTE_WAKE_UP
	error = lte_sdio_wake_up_tx((void *)tmp_lldm_him_blk, write_length);					
	#else /* LTE_WAKE_UP */
	sdio_claim_host(gLte_sdio_info->func);
	error = sdio_memcpy_toio(gLte_sdio_info->func, DATA_PORT, (void*)tmp_lldm_him_blk, write_length);
	sdio_release_host(gLte_sdio_info->func);
	#endif /* LTE_WAKE_UP */
	#else /* LGE_NO_HARDWARE */
	error = 0;
	#endif /* LGE_NO_HARDWARE */

	if(error != 0)
	{
		LTE_ERROR("Failed to wrtie data to LTE. Error = 0x%x\n", error);
	}
	else
	{
		if(gLte_sdio_info->him_tx_cnt == 0xFFFFFFFF)
			gLte_sdio_info->him_tx_cnt = 1;
		else
			gLte_sdio_info->him_tx_cnt++;
	}

	kfree(tmp_lldm_him_blk);
	#ifdef LTE_WAKE_UP
	/* SET LTE_WU = LOW */
	gpio_set_value(GPIO_L2K_LTE_WAKEUP,FALSE);	
	#endif /* LTE_WAKE_UP */

	//FUNC_EXIT();
	return error;
}
#endif	/*LTE_ROUTING*/

int lte_him_parsing_blk(void)
{
	unsigned char *rx_buff, *packet_buff;
	unsigned int him_blk_size = 0, him_blk_curr_seq_num = 0, him_blk_seq_num = 0, packet_count = 0, total_packet_size = 0, i = 0;
/* BEGIN: 0012603 jaegyu.lee@lge.com 2010-12-17 */
/* MOD 0012603: [LTE] HIM block parsing policy change */
	unsigned int him_blk_type = 0, real_packet_size = 0, him_packet_type = 0, curr_pdn = 0;
/* END: 0012603 jaegyu.lee@lge.com 2010-12-17 */
	int error;

#ifdef CONFIG_LGE_LTE_ERS
	S_L2K_RAM_DUMP_HDR_TYPE *l2k_ram_dump_pkt = NULL;
#endif

#ifdef LTE_ROUTING
	int control_count = FALSE;
	int lldm_count = FALSE;
#endif  /* LTE_ROUTING */

#ifdef RACE_LTE_TEST
	lte_dummy_info* lte_dummy_packet_ptr = NULL;
#endif /* RACE_LTE_TEST */
	
	rx_buff = gLte_sdio_info->rx_buff;
	him_blk_size = ((him_block_header *)rx_buff)->total_block_size;
	him_blk_curr_seq_num = ((him_block_header *)rx_buff)->seq_num;
	him_blk_seq_num = gLte_sdio_info->him_rx_cnt;
	packet_count = ((him_block_header *)rx_buff)->packet_num;
	him_blk_type = ((him_block_header *)rx_buff)->him_block_type;


/* BEGIN: 0011323 jaegyu.lee@lge.com 2010-11-24 */  
/* MOD 0011323: [LTE] VED callback value = 0 Issue  */  
#if 0
	if(him_blk_curr_seq_num != him_blk_seq_num)
	{
		error = -1;
		LTE_ERROR("HIM rx seq num is wrong! current = %d, received = %d\n", him_blk_seq_num, him_blk_curr_seq_num);
		goto err;
	}
#endif
/* END: 0011213 jaegyu.lee@lge.com 2010-11-24 */ 

/* BEGIN: 0012603 jaegyu.lee@lge.com 2010-12-17 */
/* MOD 0012603: [LTE] HIM block parsing policy change */
	if(him_blk_type == HIM_BLOCK_MISC)
	{	/* Control and ETC. */
		rx_buff += HIM_BLOCK_HEADER_SIZE;

		for(i=0; i < packet_count; i++)
		{
			real_packet_size = ((him_packet_header *)rx_buff)->payload_size;
			him_packet_type = ((him_packet_header *)rx_buff)->packet_type;

			LTE_INFO("MISC packet type = 0x%x\n", him_packet_type);
			LTE_INFO("MISC packet size = %d\n", real_packet_size);
#ifdef LTE_ROUTING
			if(real_packet_size > 12762)
				{
					LTE_ERROR("Invaild packet size (bigger than 12762)\n");
					return;
				}
#endif	/*LTE_ROUTING*/


#ifdef RACE_LTE_TEST
			lte_dummy_packet_ptr = rx_buff + HIM_PACKET_HEADER_SIZE;
			LTE_INFO("[RESP] cmd = 0x%x, param1 = 0x%x, param2 = 0x%x \n",lte_dummy_packet_ptr->header.cmd,lte_dummy_packet_ptr->param1,lte_dummy_packet_ptr->param2);
			LTE_INFO("[RESP] param3 = 0x%x, param4 = 0x%x, param5 = 0x%x \n",lte_dummy_packet_ptr->param3,lte_dummy_packet_ptr->param4,lte_dummy_packet_ptr->param5);
#endif /* RACE_LTE_TEST */

			if(him_packet_type == HIM_CONTROL_PACKET)
			{
				/* Fill the flip buffer */
#ifdef LTE_ROUTING
				control_count = TRUE;
				LTE_INFO("control_count = %d\n",control_count);
#endif  /* LTE_ROUTING */
				tty_insert_flip_string(gLte_sdio_info->tty, rx_buff + HIM_PACKET_HEADER_SIZE, (size_t)real_packet_size);
			}
#ifdef LTE_ROUTING
			else if(him_packet_type == HIM_LLDM_PACKET)
			{
#ifdef LTE_ROUTING
				lldm_count = TRUE;
				LTE_INFO("lldm_count = %d\n",lldm_count);
#endif  /* LTE_ROUTING */
				//LTE_INFO("HIM_LLDM_PACKET\n");
				if(gLte_sdio_info->lldm_flag == TRUE)
				{
					/* Fill the flip buffer */
					tty_insert_flip_string(gLte_sdio_info->tty_lldm, rx_buff + HIM_PACKET_HEADER_SIZE, (size_t)real_packet_size);
				}
				else
				{
					LTE_ERROR("TTY LLDM Port is not open\n");
					LTE_ERROR("lldm_flag = %d\n",gLte_sdio_info->lldm_flag);
				}
				
			}
#endif	/*LTE_ROUTING*/
			else
			{
				LTE_INFO("Not a control packet!\n");
			}

			total_packet_size = lte_him_calc_dummy(HIM_PACKET_HEADER_SIZE + ((him_packet_header *)rx_buff)->payload_size);
			rx_buff += total_packet_size;
		}

#ifdef LTE_ROUTING
		/* wake up tty core */
		//if(him_packet_type == HIM_CONTROL_PACKET)
		if(control_count == TRUE)
		{
			LTE_INFO("contorl packet push to tty \n");
			tty_flip_buffer_push(gLte_sdio_info->tty);
			control_count = FALSE;
		}

		//else if(him_packet_type == HIM_LLDM_PACKET)
		else if(lldm_count == TRUE)
		{
			
			if(gLte_sdio_info->lldm_flag == TRUE)
			{
			LTE_INFO("LLDM packet push to tty_lldm \n");
			tty_flip_buffer_push(gLte_sdio_info->tty_lldm);
			}

			else
			{
					LTE_ERROR("TTY LLDM Port is not open\n");
					LTE_ERROR("lldm_flag = %d\n",gLte_sdio_info->lldm_flag);
			}
			lldm_count = FALSE;
		}
		else
		LTE_INFO("Not a Control and LLDM Packet!!\n");

#endif	/*LTE_ROUTING*/
	}

	else if(him_blk_type == HIM_BLOCK_IP)
	{	/* IP packet to VED */
		// Parsing HIM block and call VED CB fucntion

		rx_buff += HIM_BLOCK_HEADER_SIZE;
		
/* BEGIN: 0011323 jaegyu.lee@lge.com 2010-11-24 */  
/* MOD 0011323: [LTE] VED callback value = 0 Issue  */  
#if 0
		if(gLte_sdio_info->callback_from_ved == 0)
		{
			error = -2;
			LTE_ERROR("Callback from VED is NULL!\n");
			goto err;
		}
#endif
/* END: 0011213 jaegyu.lee@lge.com 2010-11-24 */ 		

/* END: 0012603 jaegyu.lee@lge.com 2010-12-17 */
		for(i=0; i < packet_count; i++)
		{
			/* Copy IP packet from HIM blk to each packet buffer */
			real_packet_size = ((him_packet_header *)rx_buff)->payload_size;
			packet_buff = (unsigned char *)kzalloc(real_packet_size, GFP_KERNEL);
			if(!packet_buff)
			{
/* BEGIN: 0011698 jaegyu.lee@lge.com 2010-12-01 */
/* ADD 0011698: [LTE] ADD : Debug message for him packet */
				LTE_ERROR("Failed to allocate memory\n");
/* END : 0011698 jaegyu.lee@lge.com 2010-12-01 */
				return -ENOMEM;
			}
			
			memcpy(packet_buff, rx_buff + HIM_PACKET_HEADER_SIZE, real_packet_size);			

			switch(((him_packet_header *)rx_buff)->packet_type)
			{
				case HIM_NIC_1:
					curr_pdn = 0;
					break;
				case HIM_NIC_2:
					curr_pdn = 1;			
					break;
				case HIM_NIC_3:
					curr_pdn = 2;			
					break;
				case HIM_IP_PACKET_1:
					curr_pdn = 3;
					break;
				case HIM_IP_PACKET_2:
					curr_pdn = 4;			
					break;
				case HIM_IP_PACKET_3:
					curr_pdn = 5;			
					break;
				case HIM_IP_PACKET_4:
					curr_pdn = 6;
					break;
				case HIM_IP_PACKET_5:
					curr_pdn = 7;			
					break;
				case HIM_IP_PACKET_6:
					curr_pdn = 8;			
					break;
				case HIM_IP_PACKET_7:
					curr_pdn = 9;			
					break;
				case HIM_IP_PACKET_8:
					curr_pdn = 10;			
					break;
				default:
/* defect */
/* When default case, free the memory */
					kfree(packet_buff);					
/* defect */
					return -1;
			}

//			LTE_INFO("Size = %d\n", real_packet_size);
//			LTE_INFO("PDN = %d\n", curr_pdn);

/* BEGIN: 0011323 jaegyu.lee@lge.com 2010-11-24 */  
/* MOD 0011323: [LTE] VED callback value = 0 Issue  */  
			if(gLte_sdio_info->callback_from_ved != 0)
			{
				(gLte_sdio_info->callback_from_ved)(packet_buff, real_packet_size, curr_pdn);			
			}
/* END: 0011213 jaegyu.lee@lge.com 2010-11-24 */ 

/* BEGIN: 0011698 jaegyu.lee@lge.com 2010-12-01 */
/* ADD 0011698: [LTE] ADD : Debug message for him packet */
			else
			{
				gLte_sdio_info->him_dropped_ip_cnt ++;
				LTE_INFO("Dropped IP packet cnt = %d\n", gLte_sdio_info->him_dropped_ip_cnt);
			}
/* END : 0011698 jaegyu.lee@lge.com 2010-12-01 */
			kfree(packet_buff);
			
			total_packet_size = lte_him_calc_dummy(HIM_PACKET_HEADER_SIZE + ((him_packet_header *)rx_buff)->payload_size);
			rx_buff += total_packet_size;			
		}
		
	}
/* BEGIN: 0012603 jaegyu.lee@lge.com 2010-12-17 */
/* ADD 0012603: [LTE] HIM block parsing policy change */

/* BEGIN: 0013575 seongmook.yim@lge.com 2011-01-05 */
/* ADD 0013575: [LTE] LTE S/W assert case handling */
	else if(him_blk_type == HIM_BLOCK_ERROR)
	{
		/* LTE error handling */
		//printk("[LTE_ASSERT] LTE S/W Assert Case\n");
/*BEGIN: 0017497 daeok.kim@lge.com 2011-03-05 */
/*MOD 0017497: [LTE] LTE SW Assert stability is update: blocking of tx operation */
		/* SDIO Tx Blocking procedure*/
#if 0 //not available
		if (gLte_sdio_info->tx_workqueue != NULL)
		{
//			flush_workqueue(gLte_sdio_info->tx_workqueue);
			destroy_workqueue(gLte_sdio_info->tx_workqueue);
			gLte_sdio_info->tx_workqueue = NULL;
		}
#endif
		gLte_sdio_info->flag_tx_blocking =TRUE;
/*END: 0017497 daeok.kim@lge.com 2011-03-05 */	


#ifdef LGE_LTE_ERS_DBG
		printk("\n\n");
		for(i=HIM_BLOCK_HEADER_SIZE+HIM_PACKET_HEADER_SIZE; i < HIM_BLOCK_HEADER_SIZE + HIM_PACKET_HEADER_SIZE + sizeof(S_L2K_RAM_DUMP_HDR_TYPE) + 32; i++)
		{
			printk("0x%2X ", *(rx_buff+i));
			if((i+3)%8 == 0)
				printk("\n");
		}
		printk("\n\n");
#endif

		rx_buff += HIM_BLOCK_HEADER_SIZE;

		him_packet_type = ((him_packet_header *)rx_buff)->packet_type;
		
		// check 0xF0E1 for lte_ers_panic_prev
		if(him_packet_type == HIM_LTE_EM)
		{
			printk("[LTE_ASSERT] LTE S/W Assert Case, packet_type : 0x%X\n", him_packet_type);
			
			for(i=0; i < packet_count; i++)
			{
				real_packet_size = ((him_packet_header *)rx_buff)->payload_size;
				packet_buff = (unsigned char *)kzalloc(real_packet_size, GFP_KERNEL);
				if(!packet_buff)
				{
					LTE_ERROR("Failed to allocate for Error Manager\n");
					return -ENOMEM;
				}
				memcpy(packet_buff,rx_buff + HIM_PACKET_HEADER_SIZE,real_packet_size);
			
				printk("[LTE_ASSERT] lte_ers_panic dump start\n");
#ifdef CONFIG_LGE_LTE_ERS
				lte_crash_log(packet_buff,real_packet_size,NULL);
#endif
		
				kfree(packet_buff);
			}
		}
#ifdef CONFIG_LGE_LTE_ERS
		// check 0xF0E2 for lte_total_dump
		else if(him_packet_type == HIM_LTE_CRASH_DUMP)
		{
			rx_buff += HIM_PACKET_HEADER_SIZE;
			l2k_ram_dump_pkt = (S_L2K_RAM_DUMP_HDR_TYPE *)rx_buff;
			
			if(l2k_ram_dump_pkt->cmd == S_L2K_RAM_DUMP_CMD_SAVE)
			{
				if((l2k_ram_dump_pkt->attribute) & S_L2K_RAM_DUMP_ATTR_START)
				{
					printk("[LTE_ASSERT] LTE S/W Assert Case, packet_type : 0x%X, first packet to dump\n", him_packet_type);
					printk(KERN_INFO "[LTE_ASSERT] attribute : 0x%x, file_num : %d, file_seq : %d\n", l2k_ram_dump_pkt->attribute, l2k_ram_dump_pkt->file_num, l2k_ram_dump_pkt->file_seq);					
				}
				if((l2k_ram_dump_pkt->attribute) & S_L2K_RAM_DUMP_ATTR_END)
				{
					printk("[LTE_ASSERT] LTE S/W Assert Case, packet_type : 0x%X, last packet to dump\n", him_packet_type);
					printk(KERN_INFO "[LTE_ASSERT] attribute : 0x%x, file_num : %d, file_seq : %d\n", l2k_ram_dump_pkt->attribute, l2k_ram_dump_pkt->file_num, l2k_ram_dump_pkt->file_seq);					
				}

				/*
				real_packet_size = l2k_ram_dump_pkt->raw_data_size;
				packet_buff = (unsigned char *)kzalloc(real_packet_size, GFP_KERNEL);
				if(!packet_buff)
				{
					LTE_ERROR("Failed to allocate for Error Manager\n");
					return -ENOMEM;
				}
				memcpy(packet_buff, rx_buff+sizeof(S_L2K_RAM_DUMP_HDR_TYPE) ,real_packet_size);
				lte_crash_log_dump(packet_buff,real_packet_size,l2k_ram_dump_pkt->file_num, l2k_ram_dump_pkt->file_seq, l2k_ram_dump_pkt->attribute);
				kfree(packet_buff);
				*/
				lte_crash_log_dump(rx_buff+sizeof(S_L2K_RAM_DUMP_HDR_TYPE),l2k_ram_dump_pkt->raw_data_size,l2k_ram_dump_pkt->file_num, l2k_ram_dump_pkt->file_seq, l2k_ram_dump_pkt->attribute);
			}
			else
			{
				printk(KERN_ERR "[LTE_ASSERT] unknown cmd : 0x%X\n", l2k_ram_dump_pkt->cmd);
			}
		}
		else
		{
			printk(KERN_ERR "[LTE_ASSERT] unknown error packet_type\n");
		}
#endif
		
	}
/* END: 0013575 seongmook.yim@lge.com 2011-01-05 */   
	else
	{	/* Invalid HIM block type */
		LTE_ERROR("Invalid HIM block type! 0x%x\n", him_blk_type);
	}
/* END: 0012603 jaegyu.lee@lge.com 2010-12-17 */
	gLte_sdio_info->him_rx_cnt++;

	error = 0;

err:
	return error;
}

EXPORT_SYMBOL(lte_him_register_cb_from_ved);
EXPORT_SYMBOL(lte_him_enqueue_ip_packet);

