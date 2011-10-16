/* drivers/media/video/msm/isx006_tun.c
*
* This software is for SONY 5M sensor 
*  
* Copyright (C) 2010-2011 LGE Inc.  
*
*
* This software is licensed under the terms of the GNU General Public  
* License version 2, as published by the Free Software Foundation, and  
* may be copied, distributed, and modified under those terms.  
*  
* This program is distributed in the hope that it will be useful,  
* but WITHOUT ANY WARRANTY; without even the implied warranty of  
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the  
* GNU General Public License for more details. 
*/

#include <linux/delay.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <media/msm_camera.h>
#include <mach/gpio.h>
#include <mach/vreg.h>
#include <linux/byteorder/little_endian.h>
#include "isx006_tun.h"
#include "isx006.h"
#include <linux/slab.h>

#if IMAGE_TUNING_SET
static struct isx006_register_address_value_pair  ext_reg_settings[4000] = {0,};
#endif


/* =====================================================================================
   isx006 IMAGE tunning                                                                 
   =====================================================================================*/
#if IMAGE_TUNING_SET
#define LOOP_INTERVAL		20
#define IS_NUM(c)			((0x30<=c)&&(c<=0x39))
#define IS_CHAR_C(c)		((0x41<=c)&&(c<=0x46))						// Capital Letter
#define IS_CHAR_S(c)		((0x61<=c)&&(c<=0x66))						// Small Letter
#define IS_VALID(c)			(IS_NUM(c)||IS_CHAR_C(c)||IS_CHAR_S(c))		// NUM or CHAR
#define TO_BE_NUM_OFFSET(c)	(IS_NUM(c) ? 0x30 : (IS_CHAR_C(c) ? 0x37 : 0x57))	
#define TO_BE_READ_SIZE		 3300*40 // 4000*40	 								// 8pages (4000x8)

char *file_buf_alloc_pages=NULL;


extern int32_t isx006_i2c_write(unsigned short saddr,
	unsigned short waddr, unsigned short wdata, enum isx006_width width);


static long isx006_read_ext_reg(char *filename)
{	
	long value=0, length=ADDRESS_TUNE, read_idx=0, i=0, j=0, k=0;
	struct file *phMscd_Filp = NULL;
	mm_segment_t old_fs=get_fs();
	phMscd_Filp = filp_open(filename, O_RDONLY |O_LARGEFILE, 0);

	printk("%s : enter this function!\n", __func__);

	if (IS_ERR(phMscd_Filp)) {
		printk("%s : open error!\n", __func__);
		return 0;
	}

	file_buf_alloc_pages = kmalloc(TO_BE_READ_SIZE, GFP_KERNEL);	
	
	if(!file_buf_alloc_pages) {
		printk("%s : mem alloc error!\n", __func__);
		return 0;
	}

	set_fs(get_ds());
	phMscd_Filp->f_op->read(phMscd_Filp, file_buf_alloc_pages, TO_BE_READ_SIZE-1, &phMscd_Filp->f_pos);
	set_fs(old_fs);

	do
	{		
		if (file_buf_alloc_pages[read_idx]=='0' && file_buf_alloc_pages[read_idx+1]=='x' 
			&& file_buf_alloc_pages[read_idx + 2] != ' ') {	// skip : 0x
			read_idx += 2;			

			if(length == ADDRESS_TUNE)
			{
				value = (file_buf_alloc_pages[read_idx]-TO_BE_NUM_OFFSET(file_buf_alloc_pages[read_idx]))*0x1000 \
						+ (file_buf_alloc_pages[read_idx+1]-TO_BE_NUM_OFFSET(file_buf_alloc_pages[read_idx+1]))*0x100\
						+ (file_buf_alloc_pages[read_idx+2]-TO_BE_NUM_OFFSET(file_buf_alloc_pages[read_idx+2]))*0x10 \
							+ (file_buf_alloc_pages[read_idx+3]-TO_BE_NUM_OFFSET(file_buf_alloc_pages[read_idx+3]));

				read_idx = read_idx + 4;

				ext_reg_settings[i++].register_address = value;
#if IMAGE_TUNING_LOG
				printk("%s : length == ADDRESS_TUNE, i = %d\n", __func__, i);
#endif
			}
			else if(length == BYTE_LEN)
			{
				value = (file_buf_alloc_pages[read_idx]-TO_BE_NUM_OFFSET(file_buf_alloc_pages[read_idx]))*0x10 \
							+ (file_buf_alloc_pages[read_idx+1]-TO_BE_NUM_OFFSET(file_buf_alloc_pages[read_idx+1]));

				read_idx = read_idx + 2;

				ext_reg_settings[j].register_value = value;
				ext_reg_settings[j++].register_length = length;
#if IMAGE_TUNING_LOG
				printk("%s : length == BYTE_LEN, j = %d\n", __func__, j);
#endif
			}
			else
			{
				value = (file_buf_alloc_pages[read_idx]-TO_BE_NUM_OFFSET(file_buf_alloc_pages[read_idx]))*0x1000 \
						+ (file_buf_alloc_pages[read_idx+1]-TO_BE_NUM_OFFSET(file_buf_alloc_pages[read_idx+1]))*0x100\
						+ (file_buf_alloc_pages[read_idx+2]-TO_BE_NUM_OFFSET(file_buf_alloc_pages[read_idx+2]))*0x10 \
							+ (file_buf_alloc_pages[read_idx+3]-TO_BE_NUM_OFFSET(file_buf_alloc_pages[read_idx+3]));

				read_idx = read_idx + 4;

				ext_reg_settings[j].register_value = value;
				ext_reg_settings[j++].register_length = length;
#if IMAGE_TUNING_LOG
				printk("%s : length == WORD_LEN, j = %d\n", __func__, j);
#endif
			}

			if(length == ADDRESS_TUNE)
			{			
				for(k=0; k < LOOP_INTERVAL; k++)
				{
					if(file_buf_alloc_pages[read_idx + k] == 'W')
					{
						length = WORD_LEN;
						break;
					}
					else if(file_buf_alloc_pages[read_idx + k] == 'Y')
					{
						length = BYTE_LEN;
						break;
					}						
				}
			}	
			else
			{
				length = ADDRESS_TUNE;
			}
		}
		else
		{
			++read_idx;
		}	

#if IMAGE_TUNING_LOG		
		printk("%s : read external file! position : %d\n", __func__, read_idx);
#endif
	}while(file_buf_alloc_pages[read_idx] != '$');

	kfree(file_buf_alloc_pages);
	file_buf_alloc_pages=NULL;
	filp_close(phMscd_Filp,NULL);

	return i;
}

static long isx006_reg_write_ext_table(uint16_t array_length)
{
	long i, rc = 0;

#if IMAGE_TUNING_LOG
	printk("%s : write registers from external file!\n", __func__);
#endif

	for (i = 0; i < array_length; i++) {
		rc = isx006_i2c_write_ext(
		  ext_reg_settings[i].register_address,
		  ext_reg_settings[i].register_value,
		  ext_reg_settings[i].register_length);

#if IMAGE_TUNING_LOG
		printk("isx006_reg_write_ext_table, addr = 0x%x, val = 0x%x, width = %d\n",
			    ext_reg_settings[i].register_address, ext_reg_settings[i].register_value, ext_reg_settings[i].register_length);
#endif

		if (rc < 0) {
			printk("I2C Fail\n");
			return rc;
		}
	}

	return 1;
}
long isx006_reg_pll_ext(void)
{
	uint16_t length = 0;	

	printk("%s : isx006_reg_pll_ext enter\n", __func__);
	length = isx006_read_ext_reg("/mnt/sdcard/_ExternalSD/pll_settings_array.txt");
	printk("%s : length = %d!\n", __func__, length);

	if (!length)
		return 0;
	else	
		return isx006_reg_write_ext_table(length);
}
long isx006_reg_init_ext(void)
{	
	uint16_t length = 0;

	printk("%s : isx006_reg_init_ext enter\n", __func__);
	length = isx006_read_ext_reg("/mnt/sdcard/_ExternalSD/init_settings_array.txt");
	printk("%s : length = %d!\n", __func__, length);

	if (!length)
		return 0;
	else
		return isx006_reg_write_ext_table(length);

}
#endif //IMAGE_TUNING_SET

