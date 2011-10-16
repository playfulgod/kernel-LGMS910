/* drivers/media/video/msm/isx006.c 
*
* This software is for SONY 5M sensor 
*  
* Copyright (C) 2010-2011 LGE Inc.  
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
#include <linux/list.h>
#include <linux/kthread.h>
#include <linux/miscdevice.h>
#include <media/msm_camera.h>
#include <mach/gpio.h>
#include <mach/vreg.h>
#include <linux/byteorder/little_endian.h>
#include <linux/slab.h>
#include "isx006_flash.h"
#include "isx006_tun.h"
#include "isx006.h"


/*---------------------------------------------------------------------------
    ISX006 Registers
---------------------------------------------------------------------------*/
#define ISX006_REG_AF_EXT			0x000A
#define ISX006_REG_MODESEL			0x0011
#define ISX006_REG_MONI_REFRESH		0x0012
#define ISX006_REG_CAP_NUM			0x0014
#define ISX006_REG_AF_REFRESH		0x0015
#define ISX006_REG_FMTTYPE_MONI		0x001C
#define ISX006_REG_OUTFMT_CAP		0x001D
#define ISX006_REG_READVECT_CAP		0x0020
#define ISX006_REG_HSIZE_MONI		0x0022
#define ISX006_REG_HSIZE_CAP		0x0024
#define ISX006_REG_VSIZE_CAP		0x002A
#define ISX006_REG_VSIZE_MONI		0x0028
#define ISX006_REG_AFMODE_MONI		0x002E
#define ISX006_REG_INTSTS0			0x00F8
#define ISX006_REG_INTCLR0			0x00FC
#define ISX006_REG_SENSMODE_MONI	0x0380
#define ISX006_REG_FPSTYE_MONI		0x0383
#define ISX006_REG_AF_MANUAL_GO		0x4850
#define ISX006_REG_AF_MANUAL_POS	0x4852
#define ISX006_REG_AF_CANCEL		0x4885
#define ISX006_REG_AF_STATE			0x6D76
#define ISX006_REG_AF_RESULT		0x6D77
#define ISX006_REG_AF_LENSPOS		0x6D7A

#define ISX006_MAX_NUM_QUEUE		64

#define USE_INIT_THREAD 0 // sungmin.cho@lge.com 

enum{
  ISX006_FPS_5 = 5,
  ISX006_FPS_6 = 6,	
  ISX006_FPS_7 = 7,
  ISX006_FPS_10 = 10,
  ISX006_FPS_15 = 15,
  ISX006_FPS_30 = 30,
};

enum{
  ISX006_FPS_RANGE_5000 = 5000,
  ISX006_FPS_RANGE_6000 = 6000,	
  ISX006_FPS_RANGE_7000 = 7000,
  ISX006_FPS_RANGE_10000 = 10000,
  ISX006_FPS_RANGE_14000 = 14000,
  ISX006_FPS_RANGE_15000 = 15000,
  ISX006_FPS_RANGE_16000 = 16000,
  ISX006_FPS_RANGE_30000 = 30000,
};


struct isx006_queue_cmd {
	struct list_head list;
	struct sensor_cfg_data cfg_data;
};

struct isx006_device_queue {
	struct list_head list;
	spinlock_t lock;
	int max;
	int len;
};

struct isx006_ctrl_t {
	const struct msm_camera_sensor_info *sensordata;
	struct isx006_device_queue tuning_q;
	struct isx006_queue_cmd	  *queue_cmd;	 
		
	uint8_t qfactor;
	int8_t  previous_mode;
	int8_t  prev_af_mode;
  
    uint8_t brightness;
    int8_t  effect;
	int8_t  wb;
	int8_t  scene;
	int8_t  af;
	int8_t  video_mode_fps;
	int8_t  iso;

    /* for register write */
	int16_t write_byte;
	int16_t write_word;

	int32_t af_lock_count;
	
};

/*---------------------------------------------------------------------------
    EXTERNAL DECLARATIONS
---------------------------------------------------------------------------*/

DEFINE_MUTEX(isx006_mutex);
DEFINE_MUTEX(isx006_tuning_mutex);

extern int mclk_rate;

extern struct isx006_reg isx006_regs;


/*---------------------------------------------------------------------------
    INTERNAL  DECLARATIONS
---------------------------------------------------------------------------*/

static struct isx006_ctrl_t *isx006_ctrl = NULL;

static struct i2c_client  *isx006_client;

static atomic_t pll_mode;

#if USE_INIT_THREAD
static int tuning_thread_run;
#endif
static uint16_t msleepCount = 0;
static int isx006_set_fps_range(int fps);

/*---------------------------------------------------------------------------
    isx006 i2c read/write 
   ---------------------------------------------------------------------------*/
int32_t isx006_i2c_txdata(unsigned short saddr,
	unsigned char *txdata, int length)
{
	struct i2c_msg msg[] = {
		{
			.addr = saddr,
			.flags = 0,
			.len = length,
			.buf = txdata,
		},
	};

	if (i2c_transfer(isx006_client->adapter, msg, 1) < 0 ) {
		CAM_ERR("isx006_i2c_txdata failed\n");
		return -EIO;
	}

	return 0;
}

int32_t isx006_i2c_write(unsigned short saddr,
	unsigned short waddr, unsigned short wdata, enum isx006_width width)
{
	int32_t rc = -EIO;
	unsigned char buf[4];

	memset(buf, 0, sizeof(buf));
	switch (width) {
	case WORD_LEN: {
		buf[0] = (waddr & 0xFF00)>>8;
		buf[1] = (waddr & 0x00FF);
		buf[3] = (wdata & 0xFF00)>>8;   // endian change
		buf[2] = (wdata & 0x00FF);

		rc = isx006_i2c_txdata(saddr, buf, 4);
	}
	break;

	case BYTE_LEN: {
		buf[0] = (waddr & 0xFF00)>>8;
		buf[1] = (waddr & 0x00FF);
		buf[2] =  wdata;
		rc = isx006_i2c_txdata(saddr, buf, 3);
	}
	break;
	}

	if (rc < 0){
		CAM_ERR("i2c_write failed, addr = 0x%x, val = 0x%x!\n",waddr, wdata);
	}
	
	return rc;
	
}

int32_t isx006_i2c_write_table(
	struct isx006_i2c_reg_conf const *reg_conf_tbl,
	int num_of_items_in_table)
{
	int i = 0, fail_count = 0;
	int retry ;
	int32_t rc = -EIO;
	
	for (i = 0; i < num_of_items_in_table; i++) {
		rc = isx006_i2c_write(isx006_client->addr,
		reg_conf_tbl->waddr, reg_conf_tbl->wdata,
		reg_conf_tbl->width);
		
		if (reg_conf_tbl->mdelay_time != 0)
			mdelay(reg_conf_tbl->mdelay_time);
             
		if(rc < 0){
    		for(retry = 0; retry < 3; retry++){
   				rc = isx006_i2c_write(isx006_client->addr,
		    			reg_conf_tbl->waddr, reg_conf_tbl->wdata,
		    			reg_conf_tbl->width);
           
            	if(rc >= 0)
               	retry = 3;
            
            	fail_count = fail_count + 1;

         	}
         	reg_conf_tbl++;
		}else
         	reg_conf_tbl++;

	}
	
	return rc;
	
}

int32_t isx006_i2c_rxdata(unsigned short saddr,
	unsigned char *rxdata, int length)
{
	struct i2c_msg msgs[] = {
	{
		.addr   = saddr,
		.flags = 0,
		.len   = 2,
		.buf   = rxdata,
	},
	{
		.addr   = saddr,
		.flags = I2C_M_RD,
		.len   = length,
		.buf   = rxdata,
	},
	};

	if (i2c_transfer(isx006_client->adapter, msgs, 2) < 0) {
		CAM_ERR("isx006_i2c_rxdata failed!\n");
		return -EIO;
	}

	return 0;
}

int32_t isx006_i2c_read(unsigned short saddr,unsigned short raddr, 
	                        unsigned short *rdata,enum isx006_width width)
{
	int32_t rc = 0;
	unsigned char buf[4];

	if (!rdata)
		return -EIO;

	memset(buf, 0, sizeof(buf));

	switch (width) {
	case WORD_LEN: {
		buf[0] = (raddr & 0xFF00)>>8;
		buf[1] = (raddr & 0x00FF);

		rc = isx006_i2c_rxdata(saddr, buf, 2);
		if (rc < 0)
			return rc;

		//*rdata = buf[0] << 8 | buf[1]; endian-change
		*rdata = buf[1] << 8 | buf[0];
		
		}
		break;
   	
   case BYTE_LEN:{
		buf[0] = (raddr & 0xFF00)>>8;
		buf[1] = (raddr & 0x00FF);

		rc = isx006_i2c_rxdata(saddr, buf, 2);
		if (rc < 0)
			return rc;
		
		*rdata = buf[0];
		}
		break;
	}

	if (rc < 0){
		CAM_ERR("isx006_i2c_read failed!\n");
	}
   
	return rc;
	
}

int32_t isx006_i2c_write_ext(unsigned short waddr, unsigned short wdata, 
								 enum isx006_width width)
{
	int32_t rc = 0;
	
	rc = isx006_i2c_write(isx006_client->addr,waddr,wdata,width);
	
	return rc;

}
int32_t isx006_i2c_read_ext(unsigned short raddr,unsigned short *rdata,
								 enum isx006_width width)
{
	int32_t rc = 0;

	rc = isx006_i2c_read(isx006_client->addr,raddr,rdata,width);

	return rc;
}

/*---------------------------------------------------------------------------
    isx006 queue for tuning registers
   ---------------------------------------------------------------------------*/
void isx006_queue_init(struct isx006_device_queue *queue)
{
	spin_lock_init(&queue->lock);
	queue->len = 0;
	queue->max = 0;
	INIT_LIST_HEAD(&queue->list);
}

void isx006_cmd_enqueue(struct isx006_device_queue *queue,
		struct list_head *entry)
{
	unsigned long flags;
	spin_lock_irqsave(&queue->lock, flags);
	queue->len++;
	if (queue->len > queue->max) {
		queue->max = queue->len;
		CAM_MSG("%s: queue new max is %d\n", __func__,queue->max);
	}
	
	list_add_tail(entry, &queue->list);
	spin_unlock_irqrestore(&queue->lock, flags);
}

struct isx006_queue_cmd *isx006_cmd_dequeue(struct isx006_device_queue *queue)
{
	unsigned long flags;
	struct isx006_device_queue *__q = (queue);			
	struct isx006_queue_cmd *qcmd = 0;			
	
	spin_lock_irqsave(&queue->lock, flags);
	if (!list_empty(&__q->list)) {				
			__q->len--; 				
			qcmd = list_first_entry(&__q->list, 	
					struct isx006_queue_cmd, list);
			
			CAM_MSG("%s: __q->len [%d]\n", __func__,__q->len);
			if (qcmd)	
				list_del_init(&qcmd->list);			
	}
	
	spin_unlock_irqrestore(&queue->lock, flags);

	return qcmd;
}

int32_t isx006_set_preview_fps(uint16_t fps)
{
	uint16_t reg_value = 0;  
	uint32_t fps_range = 0;
	int32_t rc = 0;

	CAM_MSG("isx006: isx006_set_preview_fps : fps = %d, current fps = %d\n",fps,isx006_ctrl->video_mode_fps);

	if(fps > ISX006_FPS_30)
		fps = ISX006_FPS_30;

	if(isx006_ctrl->video_mode_fps==fps){
		return rc;
	}	

	fps_range = (((uint32_t)fps) << 16) | fps;
	rc = isx006_set_fps_range(fps_range);

#if 0
	switch(fps){
	case ISX006_FPS_5:
		reg_value = 0x07;
		break; 
	case ISX006_FPS_6:
		reg_value = 0x06;
		break;
	case ISX006_FPS_7:
		reg_value = 0x05;
		break;
	case ISX006_FPS_10:
		reg_value = 0x04;
		break;
	case ISX006_FPS_15:
		reg_value = 0x03;
		break;
	case ISX006_FPS_30:
		reg_value = 0x02;
		break;
	default: 
		reg_value  = 0x02; 
		fps = ISX006_FPS_30;
		break;
	}

	rc = isx006_i2c_write(isx006_client->addr, ISX006_REG_FPSTYE_MONI, reg_value, BYTE_LEN);
	if (rc < 0)
		return rc;

	rc = isx006_i2c_write(isx006_client->addr, ISX006_REG_MONI_REFRESH, 0x01, BYTE_LEN);
#endif
	if (rc < 0)
		return rc;


	 isx006_ctrl->video_mode_fps = fps;	

	return rc;
	
}
// START : ADD : sungmin.cho@lge.com 2011.06.20 for CTS
static int isx006_set_fps_range(int fps)
{
	printk(KERN_ERR " >> %s START \n", __func__);
	uint16_t reg_value = 0;  
	int rc = 0;
	uint16_t min_fps = (((uint32_t) fps) & 0xFFFF0000) >> 16;
	uint16_t max_fps = (fps & 0x0000FFFF);

	printk("isx005 : isx006_set_fps_range : min_fps = %d, max_fps = %d", min_fps, max_fps); 

	if(!atomic_read(&pll_mode)== CAMERA_VIDEO_MODE_ON) // snapshot mode
	{
		//if(((min_fps == ISX006_FPS_RANGE_14000) && (max_fps == ISX006_FPS_RANGE_16000))
		//	|| ((min_fps == ISX006_FPS_RANGE_15000) && (max_fps == ISX006_FPS_RANGE_15000)))
			reg_value = 0x02;
	}
	else // video mode
	{
		if((min_fps == ISX006_FPS_RANGE_15000) && (max_fps == ISX006_FPS_RANGE_15000))
			reg_value = 0x03;
		else if((min_fps == ISX006_FPS_RANGE_30000) && (max_fps == ISX006_FPS_RANGE_30000))
			reg_value = 0x02;
		else // fixed 15fps
			reg_value = 0x03;
	}

	rc = isx006_i2c_write(isx006_client->addr, ISX006_REG_FPSTYE_MONI, reg_value, BYTE_LEN);
	if (rc < 0)
		return rc;

	rc = isx006_i2c_write(isx006_client->addr, ISX006_REG_MONI_REFRESH, 0x01, BYTE_LEN);
	if (rc < 0)
		return rc;

	printk(KERN_ERR " << %s END (rc = %d)\n", __func__, rc);
	return rc;
}
// END : sungmin.cho@lge.com 2011.06.20 for CTS


int32_t isx006_cancel_focus(int mode)
{
	int32_t rc = 0;
	int32_t lense_po_back = 0;
	
	CAM_MSG("isx006: cancel focus - mode = %d\n",mode);

	/* single AF cancel */
	rc = isx006_i2c_write(isx006_client->addr, ISX006_REG_AF_CANCEL, 0x01, BYTE_LEN);
	if (rc < 0)
		return rc;
		
	rc = isx006_i2c_write(isx006_client->addr, ISX006_REG_MODESEL, 0x00, BYTE_LEN);
	if (rc < 0)
		return rc;
	
	switch(mode){
	case AUTO_FOCUS:  
   		CAM_MSG("back to the infinity\n");
		lense_po_back = 0x00c8;
		break;
	
	case MACRO_FOCUS:  
	   	CAM_MSG("back to the macro\n");
		lense_po_back = 0x0258;
		break;
	}
			
	rc = isx006_i2c_write(isx006_client->addr,
		 ISX006_REG_AFMODE_MONI, 0x02, BYTE_LEN);
	if (rc < 0)
		return rc;

	rc = isx006_i2c_write(isx006_client->addr,
		 ISX006_REG_AF_MANUAL_POS,lense_po_back,WORD_LEN);
	if (rc < 0)
		return rc;
	
	rc = isx006_i2c_write(isx006_client->addr,
		 ISX006_REG_MONI_REFRESH, 0x01, BYTE_LEN);
	if (rc < 0)
		return rc;
	
	rc = isx006_i2c_write(isx006_client->addr,
		 ISX006_REG_AF_MANUAL_GO, 0x01, BYTE_LEN);
	if (rc < 0)
		return rc;

	rc = isx006_i2c_write(isx006_client->addr,
			ISX006_REG_INTCLR0, 0x1F, BYTE_LEN);

	if (rc < 0)
		return rc;
	return rc;
}

int32_t isx006_check_af_lock(void)
{
	int32_t rc;
	int i;
	unsigned short af_lock;

	/*  check AF lock status : lock */
	for(i=0; i<10; i++){
		/*INT state read -*/
		rc = isx006_i2c_read(isx006_client->addr,
			 ISX006_REG_INTSTS0, &af_lock, BYTE_LEN);
		if (rc < 0){
			CAM_ERR("isx006: reading af_lock fail\n");
			return rc;
		}

		/* af interruption lock state read compelete */
		if((af_lock & 0x10)== 0x10){  
			CAM_MSG(" af_lock is locking\n");
			break;
		}

		CAM_MSG("isx006: af_lock( 0x%x ) is not ready yet\n", af_lock);

		msleep(10); 
		
	}

	/* INT clear */
	rc = isx006_i2c_write(isx006_client->addr,
		 ISX006_REG_INTCLR0, 0x10, BYTE_LEN); 
	if (rc < 0)
		return rc;
	
	
	/* check AF lock status : clear */
	for(i=0; i<10; i++){
		
		rc = isx006_i2c_read(isx006_client->addr,
			 ISX006_REG_INTSTS0,&af_lock, BYTE_LEN);
		if (rc < 0){
			CAM_ERR("isx006: reading af_lock fail\n");
			return rc;
		}
	
		if((af_lock & 0x10)== 0x00){
			CAM_MSG("af_lock is released....i[%d]\n",i);
			break;
		}

		CAM_MSG("isx006: af_lock( 0x%x ) is not clear yet\n", af_lock);

		msleep(70);
	}	

	return rc;

}

int32_t isx006_check_focus(int *lock)
{
	int32_t rc;
	unsigned short af_status, af_result;

	CAM_MSG("isx006_check_focus \n");

	/*af status check  0:load, 1: init,  8: af_lock*/
	rc = isx006_i2c_read(isx006_client->addr,
        		ISX006_REG_AF_STATE, &af_status, BYTE_LEN);
	
	if(af_status != 0x8){
		isx006_ctrl->af_lock_count++;
		if(isx006_ctrl->af_lock_count > 100){
			CAM_ERR("%s: af STATE UNLOCK...isx006_ctrl->af_lock_count[%d]\n",__func__,isx006_ctrl->af_lock_count);
			isx006_ctrl->af_lock_count = 0;
		}
		else{		
			CAM_MSG("%s:af_status != 0x8:%d...af_lock_count[%d]...\n",__func__,af_status,isx006_ctrl->af_lock_count);
			return -ETIME;
		}
	}
	
	rc = isx006_check_af_lock();
	if (rc < 0){
		CAM_ERR("isx006: check_af_lock_and_clear fail\n");
		return rc;
	}

	/* af result read : success or fail*/	
	rc = isx006_i2c_read(isx006_client->addr, ISX006_REG_AF_RESULT, &af_result, BYTE_LEN);
	if (rc < 0){
		CAM_ERR("isx006: reading af_result fail\n");
		return rc;
	}

#if 0 //When flash is supported that the sensor is change into half mode and this code doesn't need. 
	CAM_MSG("[1]isx006: single AF off : Flash Not supported\n");

	/* single AF off */
	rc = isx006_i2c_write(isx006_client->addr, ISX006_REG_AFMODE_MONI, 0x03, BYTE_LEN);
	if (rc < 0)
		return rc;

	/* single AF refresh */
	rc = isx006_i2c_write(isx006_client->addr, ISX006_REG_MONI_REFRESH, 0x01, BYTE_LEN);
	if (rc < 0)
		return rc;
#else
	CAM_MSG("[2]isx006: single AF off : Flash supported \n");

	/* single AF off */
	rc = isx006_i2c_write(isx006_client->addr, ISX006_REG_MODESEL, 0x00, BYTE_LEN);
	if (rc < 0)
		return rc;
#endif

	if(af_result == 1){
		*lock = CFG_AF_LOCKED;	// success
		CAM_MSG("isx006_check_focus : CFG_AF_LOCKED\n");
	} else {
		*lock = CFG_AF_UNLOCKED; //0: focus fail or 2: during focus
		CAM_MSG("isx006_check_focus : CFG_AF_UNLOCKED\n");
	}

	return rc;

}

/* 	single AF 
*/
int32_t isx006_set_focus(void)
{
	int32_t rc;

	rc = isx006_i2c_write_table(&isx006_regs.AF_reg_settings[0], isx006_regs.AF_reg_settings_size);

	if(rc<0){
		CAM_ERR("[isx006.c]%s: fail in writing for focus\n",__func__);
		return rc;
	}

	return rc;
}

/* AF menu 1 : auto mode 
*/
int32_t isx006_set_auto_focus(void)
{
	int32_t rc =0;

	CAM_MSG("isx006: auto focus\n");

	isx006_ctrl->prev_af_mode = AUTO_FOCUS;
	
	rc = isx006_i2c_write_table(&isx006_regs.AF_nomal_reg_settings[0],
			isx006_regs.AF_nomal_reg_settings_size);
	
	if(rc<0){
		CAM_ERR("isx006: AF auto focus writing fail!\n");
		return rc;
	}

    	msleep(70);  // 1 frame skip

	return rc;
	
}


/* AF menu 2 : macro mode 
*/
int32_t isx006_set_macro_focus(void)
{
	int32_t rc;
	
	CAM_MSG("isx006: macro focus\n");

	isx006_ctrl->prev_af_mode = MACRO_FOCUS;
	
	rc = isx006_i2c_write_table(&isx006_regs.AF_macro_reg_settings[0],
			isx006_regs.AF_macro_reg_settings_size);
	
	if(rc<0){
		CAM_ERR("isx006: AF macro focus writing fail!\n");
		return rc;
	}

	return rc;
}

/* AF menu 3 : manual mode 
*/
int32_t isx006_set_move_focus(int32_t steps)
{

	int32_t rc;
	unsigned short af_lenspos;
	unsigned short cm_changed_sts;	
	int i, af_manual_pos=0;
	unsigned short current_pos = 0;

	printk("%s: request steps[%d]\n",__func__,steps);

	////////////////////////////////////////////////////////
	
	switch(steps){
		case 10: 
			current_pos = 0x0208; // position : 520
			break;
		
		case 9:  
			current_pos = 0x0190; // position : 400 15cm
			break;
	
		case 8:  
			current_pos = 0x0168; // position : 360 20cm
			break;
		
		case 7:  
			current_pos = 0x0154; // position : 340 25cm
			break;
	
		case 6:  
			current_pos = 0x0140; // position : 320 30cm
			break;
		
		case 5:  
			current_pos = 0x0136; // position : 310 40cm
			break;
	
		case 4:  
			current_pos = 0x012C; // position : 300 50cm
			break;
		
		case 3:  
			current_pos = 0x0118; // position : 280 65cm
			break;
	
		case 2:  
			current_pos = 0x0110; // position : 272 80cm
			break;
		
		case 1:  
			current_pos = 0x0100; // position : 256 1m
			break;
	
		case 0:  
			current_pos = 0x00C8; // position : 200 infinity
			break;
	
		default: 
			CAM_ERR("\n focus step(%d) is not supported \n", steps);
			return -EINVAL;
	}

	
	rc = isx006_i2c_read(isx006_client->addr,
				 ISX006_REG_AF_LENSPOS, &af_lenspos, WORD_LEN);

	if((current_pos == af_lenspos) && (isx006_ctrl->prev_af_mode == MANUAL_FOCUS)){
		return 0;
	}

	////////////////////////////////////////////////////////////////
	
	
	isx006_ctrl->prev_af_mode = MANUAL_FOCUS;

	rc = isx006_i2c_write_table(&isx006_regs.manual_focus_reg_settings[0],
			isx006_regs.manual_focus_reg_settings_size);

	if(rc<0){
		CAM_ERR("[isx006.c]%s: fail in writing for move focus\n",__func__);
		return rc;
	}

	/* check cm_changed_sts */
	for(i=0; i<24; i++){
		rc = isx006_i2c_read(isx006_client->addr,
				ISX006_REG_INTSTS0, &cm_changed_sts, BYTE_LEN);
		if (rc < 0){
			CAM_ERR("isx006: reading cm_changed_sts fail\n");
			return rc;
		}
	
		if((cm_changed_sts & 0x02)==0x02){
			CAM_MSG("cm_changed_sts -> 0x02\n");
			break;
		}
		
		CAM_MSG("isx006: cm_changed_sts( 0x%x ) for MF is not ready yet\n", cm_changed_sts);
		
		msleep(10); 
	}	
	
	/* clear the interrupt register */
	rc = isx006_i2c_write(isx006_client->addr,
			ISX006_REG_INTCLR0, 0x1F, BYTE_LEN);
	if (rc < 0)
		return rc;

	/* check cm_changed_clr */
	for(i=0; i<24; i++){
		rc = isx006_i2c_read(isx006_client->addr,
				ISX006_REG_INTSTS0, &cm_changed_sts, BYTE_LEN);
		if (rc < 0){
			CAM_ERR("isx006: reading cm_changed_clr fail\n");
			return rc;
		}
	
		if((cm_changed_sts & 0x1F)==0x00){
			CAM_ERR("cm_changed_clr -> 0\n");
			break;
		}
		
		CAM_MSG("isx006: cm_changed_clr( 0x%x ) for MF is not ready yet\n", cm_changed_sts);
		
		msleep(10); 
	}

	CAM_MSG("\n move focus: step is %d\n", steps);

	switch(steps){
	case 10: 
   		af_manual_pos = 0x0208; // position : 520
		break;
	
	case 9:  
		af_manual_pos = 0x0190; // position : 400 15cm
		break;

	case 8:  
   		af_manual_pos = 0x0168; // position : 360 20cm
		break;
	
	case 7:  
		af_manual_pos = 0x0154; // position : 340 25cm
		break;

	case 6:  
   		af_manual_pos = 0x0140; // position : 320 30cm
		break;
	
	case 5:  
		af_manual_pos = 0x0136; // position : 310 40cm
		break;

	case 4:  
   		af_manual_pos = 0x012C;	// position : 300 50cm
		break;
	
	case 3:  
		af_manual_pos = 0x0118;	// position : 280 65cm
		break;

	case 2:  
   		af_manual_pos = 0x0110; // position : 272 80cm
		break;
	
	case 1:  
		af_manual_pos = 0x0100;	// position : 256 1m
		break;

	case 0:  
		af_manual_pos = 0x00C8;	// position : 200 infinity
		break;

	default: 
		CAM_ERR("\n focus step(%d) is not supported \n", steps);
		return -EINVAL;
	}

	CAM_MSG("af_manual_pos = 0x%x\n", af_manual_pos);

	rc = isx006_i2c_write(isx006_client->addr,
		 ISX006_REG_AF_MANUAL_POS, af_manual_pos, WORD_LEN);
	if (rc < 0)
		return rc;

	rc = isx006_i2c_write(isx006_client->addr,
		 ISX006_REG_AF_MANUAL_GO, 0x01, BYTE_LEN);
	if (rc < 0)
		return rc;
	rc = isx006_i2c_write(isx006_client->addr,
		 ISX006_REG_AF_REFRESH, 0x01, BYTE_LEN);
	if (rc < 0)
		return rc;

	mdelay(30);

	isx006_check_af_lock();
	
	/* check lens position */
	for(i=0; i<24; i++){
		rc = isx006_i2c_read(isx006_client->addr,
			 ISX006_REG_AF_LENSPOS, &af_lenspos, WORD_LEN);
		if (rc < 0){
			CAM_ERR("isx006: reading af_lenspos fail\n");
			return rc;
		}

		if(af_lenspos == af_manual_pos){
            CAM_MSG("af_lenspos is equal with af_manual_pos\n");
			break;
		}
		
	    CAM_MSG("isx006: lens position ( 0x%x )is not ready yet\n", af_lenspos);

		msleep(10);
	}	
	
	return rc;
	
}

int32_t isx006_focus_config(int mode)
{

	int32_t rc=0;

	CAM_MSG("isx006_focus_config mode= %d, prev_af_mode=%d\n", mode, isx006_ctrl->prev_af_mode);
	
	/* INT clear */
	rc = isx006_i2c_write(isx006_client->addr, ISX006_REG_INTCLR0, 0x1F, BYTE_LEN);

	if(isx006_ctrl->prev_af_mode == mode)
	{
		isx006_ctrl->af_lock_count = 0;
		rc = isx006_set_focus();
	}
	else
	{
	    switch(mode){		
		case AUTO_FOCUS:
			isx006_ctrl->af_lock_count = 0;
			rc = isx006_set_auto_focus();
			break;
		
		case MACRO_FOCUS:
			rc = isx006_set_macro_focus();
			break;

		case MANUAL_FOCUS:
			rc = isx006_i2c_write_table(&isx006_regs.manual_focus_reg_settings[0],
							isx006_regs.manual_focus_reg_settings_size);
			break;
		
		default:
			CAM_MSG("[isx006.c] isx006_focus_config: invalid af %d\n",mode);
			return -EINVAL;
		}
		
		rc = isx006_set_focus();
		
	}

	isx006_ctrl->prev_af_mode = mode;

	return rc;		
	
}

/*  Auto Focus : default mode
*/
int32_t isx006_default_focus(void)
{
	int32_t rc = 0;

	CAM_MSG("\nisx006: defalut focus\n");
	
	rc = isx006_cancel_focus(isx006_ctrl->prev_af_mode);
	if(rc<0){
		CAM_MSG("[isx006.c]%s: fail in cancel_focus\n",__func__);
		return rc;
	}

	rc = isx006_i2c_write_table(&isx006_regs.AF_nomal_reg_settings[0],
			isx006_regs.AF_nomal_reg_settings_size);

	isx006_ctrl->prev_af_mode = AUTO_FOCUS;

	isx006_ctrl->af_lock_count = 0;

	if(rc<0){
		CAM_MSG(" isx006 : AF writing fail!\n");
		return rc;
	}
    
    msleep(70); // 1 frame skip

	isx006_check_focus(&rc);

	return rc;
}

int32_t isx006_set_effect(int effect)
{
	int32_t rc = 0;

   switch (effect) {
	case CAMERA_EFFECT_OFF: 
		rc = isx006_i2c_write(isx006_client->addr,
				0x005F, 0x00, BYTE_LEN);
		if (rc < 0)
			return rc;
	
		rc = isx006_i2c_write(isx006_client->addr,
				0x021E, 0x1169, WORD_LEN);
		if (rc < 0)
			return rc;
	
      	break;

	case CAMERA_EFFECT_MONO: 
		rc = isx006_i2c_write(isx006_client->addr,
				0x005F, 0x04, BYTE_LEN);
		if (rc < 0)
			return rc;
		rc = isx006_i2c_write(isx006_client->addr,
				0x021E, 0x1169, WORD_LEN);
		if (rc < 0)
			return rc;
		break;

   case CAMERA_EFFECT_SEPIA: 
		rc = isx006_i2c_write(isx006_client->addr,
				0x005F, 0x03, BYTE_LEN);
		if (rc < 0)			
			return rc;
		rc = isx006_i2c_write(isx006_client->addr,
				0x021E, 0x1169, WORD_LEN);
		if (rc < 0)
			return rc;
		break;

	case CAMERA_EFFECT_NEGATIVE: 
		rc = isx006_i2c_write(isx006_client->addr,
				0x005F, 0x02, BYTE_LEN);
		if (rc < 0)
			return rc;
		rc = isx006_i2c_write(isx006_client->addr,
				0x021E, 0x1169, WORD_LEN);
		if (rc < 0)
			return rc;
		break;

   case CAMERA_EFFECT_NEGATIVE_SEPIA:
		rc = isx006_i2c_write(isx006_client->addr,
				0x005F, 0x02, BYTE_LEN);
		if (rc < 0)
			return rc;
		rc = isx006_i2c_write(isx006_client->addr,
				0x021E, 0x116D, WORD_LEN);
		if (rc < 0)
			return rc;
		break;

   case CAMERA_EFFECT_BLUE:
   		rc = isx006_i2c_write(isx006_client->addr,
			0x005F, 0x03, BYTE_LEN);
		if (rc < 0)
			return rc;
		rc = isx006_i2c_write(isx006_client->addr,
				0x021E, 0x116D, WORD_LEN);
		if (rc < 0)
			return rc;
		break;

   case CAMERA_EFFECT_SOLARIZE: 
		
		rc = isx006_i2c_write(isx006_client->addr,
				0x005F, 0x01, BYTE_LEN);
		if (rc < 0)
			return rc;
		rc = isx006_i2c_write(isx006_client->addr,
				0x021E, 0x1169, WORD_LEN);
		if (rc < 0)
			return rc;
		break;

   case CAMERA_EFFECT_PASTEL: 
		
		rc = isx006_i2c_write(isx006_client->addr,
				0x005F, 0x05, BYTE_LEN);
		if (rc < 0)
			return rc;
		rc = isx006_i2c_write(isx006_client->addr,
				0x021E, 0x1169, WORD_LEN);
		if (rc < 0)
			return rc;
		break;

   default: 
		CAM_MSG("isx006_set_effect: wrong effect mode\n");
		return -EINVAL;	
	}
	
	return 0;
}

int32_t isx006_set_wb(int8_t wb)
{
	int32_t rc = 0;
   
	switch (wb) {
	case CAMERA_WB_AUTO:		
		rc = isx006_i2c_write(isx006_client->addr,
			0x4453, 0x7B, BYTE_LEN);
		if (rc < 0)
			return rc;
		
		rc = isx006_i2c_write(isx006_client->addr,
			0x0102, 0x20, BYTE_LEN);
		if (rc < 0)
			return rc;
		break;

	case CAMERA_WB_INCANDESCENT:		
		rc = isx006_i2c_write(isx006_client->addr,
				0x4453,0x3B, BYTE_LEN);
		if (rc < 0)
			return rc;
		
		rc = isx006_i2c_write(isx006_client->addr,
			0x0102,0x28, BYTE_LEN);
		if (rc < 0)
			return rc;
		break;

	case CAMERA_WB_FLUORESCENT:		
		rc = isx006_i2c_write(isx006_client->addr,
				0x4453,0x3B, BYTE_LEN);
		if (rc < 0)
			return rc;
		
		rc = isx006_i2c_write(isx006_client->addr,
			0x0102,0x27, BYTE_LEN);
		if (rc < 0)
			return rc;
		break;

	case CAMERA_WB_DAYLIGHT:
		
		rc = isx006_i2c_write(isx006_client->addr,
				0x4453,0x3B, BYTE_LEN);
		if (rc < 0)
			return rc;
		
		rc = isx006_i2c_write(isx006_client->addr,
			0x0102,0x24, BYTE_LEN);
		if (rc < 0)
			return rc;
		break;

	case CAMERA_WB_CLOUDY_DAYLIGHT:
		
		rc = isx006_i2c_write(isx006_client->addr,
				0x4453, 0x3B, BYTE_LEN);
		if (rc < 0)
			return rc;

		rc = isx006_i2c_write(isx006_client->addr,
			0x0102,0x26, BYTE_LEN);
		if (rc < 0)
			return rc;
		break;

	default:
		CAM_MSG("isx006: wrong white balance value\n");
		return -EFAULT;
	}

	isx006_ctrl->wb = wb;

	return 0;
	
}

int32_t isx006_set_iso(int8_t iso)
{
	int32_t rc;

	isx006_ctrl->iso = iso;

	switch (iso) {
	case CAMERA_ISO_AUTO:
		
		rc = isx006_i2c_write_table(&isx006_regs.iso_auto_reg_settings[0],
            isx006_regs.iso_auto_reg_settings_size);
		break;                                                    
                                                            
	case CAMERA_ISO_100:
			
		rc = isx006_i2c_write_table(&isx006_regs.iso_100_reg_settings[0],
			isx006_regs.iso_100_reg_settings_size);
		break;

	case CAMERA_ISO_200:
			
		rc = isx006_i2c_write_table(&isx006_regs.iso_200_reg_settings[0],
			isx006_regs.iso_200_reg_settings_size);
		break;

	case CAMERA_ISO_400:
			
		rc = isx006_i2c_write_table(&isx006_regs.iso_400_reg_settings[0],
			isx006_regs.iso_400_reg_settings_size);	
		break;
   
	default:
		CAM_MSG("isx006: wrong iso value\n");
		return -EINVAL;
	}

	if(rc<0)
		return rc;
	
	return 0;
}

int32_t isx006_set_scene_mode(int8_t mode)
{
	int32_t rc = 0;
	
	switch (mode) {
	case CAMERA_SCENEMODE_NORMAL:
		if(atomic_read(&pll_mode)== CAMERA_VIDEO_MODE_ON){				
			rc = isx006_i2c_write_table(&isx006_regs.scene_normal_vt_reg_settings[0],
                			isx006_regs.scene_normal_vt_reg_settings_size);
		}
		else{
			rc = isx006_i2c_write_table(&isx006_regs.scene_normal_reg_settings[0],
                			isx006_regs.scene_normal_reg_settings_size);
		}
		break;
	
	case CAMERA_SCENEMODE_PORTRAIT:
		if(atomic_read(&pll_mode)== CAMERA_VIDEO_MODE_ON){	
			rc = isx006_i2c_write_table(&isx006_regs.scene_portrait_vt_reg_settings[0],
                     	isx006_regs.scene_portrait_vt_reg_settings_size);
		}
		else{
			rc = isx006_i2c_write_table(&isx006_regs.scene_portrait_reg_settings[0],
                     	isx006_regs.scene_portrait_reg_settings_size);
		}
     	break;
	
	case CAMERA_SCENEMODE_LANDSCAPE:
		if(atomic_read(&pll_mode)== CAMERA_VIDEO_MODE_ON){
			rc = isx006_i2c_write_table(&isx006_regs.scene_landscape_vt_reg_settings[0],
               			 	isx006_regs.scene_landscape_vt_reg_settings_size);	
		}
		else{
			rc = isx006_i2c_write_table(&isx006_regs.scene_landscape_reg_settings[0],
               			 	isx006_regs.scene_landscape_reg_settings_size);
		}
		break;
	
	case CAMERA_SCENEMODE_SPORT:
	    if(atomic_read(&pll_mode)== CAMERA_VIDEO_MODE_ON){
			rc = isx006_i2c_write_table(&isx006_regs.scene_sport_vt_reg_settings[0],
               		 		isx006_regs.scene_sport_vt_reg_settings_size);
	    }
		else{
			rc = isx006_i2c_write_table(&isx006_regs.scene_sport_reg_settings[0],
               		 		isx006_regs.scene_sport_reg_settings_size);
		}
		break;
	
	case CAMERA_SCENEMODE_SUNSET:
		if(atomic_read(&pll_mode)== CAMERA_VIDEO_MODE_ON){
			rc = isx006_i2c_write_table(&isx006_regs.scene_sunset_vt_reg_settings[0],
                			isx006_regs.scene_sunset_vt_reg_settings_size);
		}
		else{
			rc = isx006_i2c_write_table(&isx006_regs.scene_sunset_reg_settings[0],
                			isx006_regs.scene_sunset_reg_settings_size);
		}
		break;
	
	case CAMERA_SCENEMODE_NIGHT:
		if(atomic_read(&pll_mode)== CAMERA_VIDEO_MODE_ON){
			rc = isx006_i2c_write_table(&isx006_regs.scene_night_vt_reg_settings[0],
                			isx006_regs.scene_night_vt_reg_settings_size);	
		}
		else{
			rc = isx006_i2c_write_table(&isx006_regs.scene_night_reg_settings[0],
                			isx006_regs.scene_night_reg_settings_size);
		}
		break;
	
	default:
		CAM_MSG("isx006: wrong scene mode value, set to the normal\n");
		return -EINVAL;
	}   
	
	if (rc < 0)
		return rc;

	isx006_ctrl->scene = mode;

	return rc;
   
}

/*---------------------------------------------------------------------------
    isx006_set_brightness
   ---------------------------------------------------------------------------*/
int32_t isx006_set_brightness(int8_t brightness)
{

	int32_t rc = 0;

	CAM_MSG("isx006_set_brightness: called: ev %u\n", brightness);	

// START : [MS910] sungmin.cho@lge.com 2011.05.18 android original parameter. change from luma-adaptation to exposure-compensation.
#if 1
	switch (brightness) {
	case -5:
		rc = isx006_i2c_write(isx006_client->addr,
				0x0060, 0x80, BYTE_LEN);

		rc = isx006_i2c_write(isx006_client->addr,
				0x0061, 0x50, BYTE_LEN);		
		break;

	case -4:
		rc = isx006_i2c_write(isx006_client->addr,
				0x0060, 0x80, BYTE_LEN);

		rc = isx006_i2c_write(isx006_client->addr,
				0x0061, 0x60, BYTE_LEN);

		break;

	case -3:
		rc = isx006_i2c_write(isx006_client->addr,
				0x0060, 0x80, BYTE_LEN);

		rc = isx006_i2c_write(isx006_client->addr,
				0x0061, 0x70, BYTE_LEN);

		break;

	case -2:
		rc = isx006_i2c_write(isx006_client->addr,
				0x0060, 0xCD, BYTE_LEN);

		rc = isx006_i2c_write(isx006_client->addr,
				0x0061, 0x80, BYTE_LEN);

		break;

	case -1:
		rc = isx006_i2c_write(isx006_client->addr,
				0x0060, 0xEF, BYTE_LEN);

		rc = isx006_i2c_write(isx006_client->addr,
				0x0061, 0x80, BYTE_LEN);

		break;

	case 0:
		rc = isx006_i2c_write(isx006_client->addr,
				0x0060, 0x00, BYTE_LEN);

		rc = isx006_i2c_write(isx006_client->addr,
				0x0061, 0x80, BYTE_LEN);

		break;

	case 1:
		rc = isx006_i2c_write(isx006_client->addr,
				0x0060, 0x18, BYTE_LEN);

		rc = isx006_i2c_write(isx006_client->addr,
				0x0061, 0x80, BYTE_LEN);

		break;

	case 2:
		rc = isx006_i2c_write(isx006_client->addr,
				0x0060, 0x7F, BYTE_LEN);

		rc = isx006_i2c_write(isx006_client->addr,
				0x0061, 0x8A, BYTE_LEN);

		break;

	case 3:
		rc = isx006_i2c_write(isx006_client->addr,
				0x0060, 0x7F, BYTE_LEN);

		rc = isx006_i2c_write(isx006_client->addr,
				0x0061, 0x9C, BYTE_LEN);

		break;

	case 4:
		rc = isx006_i2c_write(isx006_client->addr,
				0x0060, 0x7F, BYTE_LEN);

		rc = isx006_i2c_write(isx006_client->addr,
				0x0061, 0xAA, BYTE_LEN);

		break;

	case 5:
		rc = isx006_i2c_write(isx006_client->addr,
				0x0060, 0x7F, BYTE_LEN);

		rc = isx006_i2c_write(isx006_client->addr,
				0x0061, 0xC8, BYTE_LEN);

		break;
	
	default:
		CAM_ERR("isx006: wrong ev value, set to the default\n");
		return -EFAULT;
	}

#else
	switch (brightness) {
	case 0:
		rc = isx006_i2c_write(isx006_client->addr,
				0x0060, 0x80, BYTE_LEN);

		rc = isx006_i2c_write(isx006_client->addr,
				0x0061, 0x50, BYTE_LEN);		
		break;

	case 1:
		rc = isx006_i2c_write(isx006_client->addr,
				0x0060, 0x80, BYTE_LEN);

		rc = isx006_i2c_write(isx006_client->addr,
				0x0061, 0x60, BYTE_LEN);

		break;

	case 2:
		rc = isx006_i2c_write(isx006_client->addr,
				0x0060, 0x80, BYTE_LEN);

		rc = isx006_i2c_write(isx006_client->addr,
				0x0061, 0x70, BYTE_LEN);

		break;

	case 3:
		rc = isx006_i2c_write(isx006_client->addr,
				0x0060, 0xCD, BYTE_LEN);

		rc = isx006_i2c_write(isx006_client->addr,
				0x0061, 0x80, BYTE_LEN);

		break;

	case 4:
		rc = isx006_i2c_write(isx006_client->addr,
				0x0060, 0xEF, BYTE_LEN);

		rc = isx006_i2c_write(isx006_client->addr,
				0x0061, 0x80, BYTE_LEN);

		break;

	case 5:
		rc = isx006_i2c_write(isx006_client->addr,
				0x0060, 0x00, BYTE_LEN);

		rc = isx006_i2c_write(isx006_client->addr,
				0x0061, 0x80, BYTE_LEN);

		break;

	case 6:
		rc = isx006_i2c_write(isx006_client->addr,
				0x0060, 0x18, BYTE_LEN);

		rc = isx006_i2c_write(isx006_client->addr,
				0x0061, 0x80, BYTE_LEN);

		break;

	case 7:
		rc = isx006_i2c_write(isx006_client->addr,
				0x0060, 0x7F, BYTE_LEN);

		rc = isx006_i2c_write(isx006_client->addr,
				0x0061, 0x8A, BYTE_LEN);

		break;

	case 8:
		rc = isx006_i2c_write(isx006_client->addr,
				0x0060, 0x7F, BYTE_LEN);

		rc = isx006_i2c_write(isx006_client->addr,
				0x0061, 0x9C, BYTE_LEN);

		break;

	case 9:
		rc = isx006_i2c_write(isx006_client->addr,
				0x0060, 0x7F, BYTE_LEN);

		rc = isx006_i2c_write(isx006_client->addr,
				0x0061, 0xAA, BYTE_LEN);

		break;

	case 10:
		rc = isx006_i2c_write(isx006_client->addr,
				0x0060, 0x7F, BYTE_LEN);

		rc = isx006_i2c_write(isx006_client->addr,
				0x0061, 0xC8, BYTE_LEN);

		break;
	
	default:
		CAM_ERR("isx006: wrong ev value, set to the default\n");
		return -EFAULT;
	}
#endif
// END : [MS910] sungmin.cho@lge.com 2011.05.18 android original parameter. change from luma-adaptation to exposure-compensation.

	if(rc<0)
		return rc;
	
	isx006_ctrl->brightness = brightness;

	return rc;
}

/* START : [MS910] sungmin.cho@lge.com 2011.04.07 add focus distances for GB */
/*---------------------------------------------------------------------------
    isx006_get_focus_distances
   ---------------------------------------------------------------------------*/
#define MAX_FOCUS_DISTANCES_INDEX 6
uint16_t actuator_position_tbl[MAX_FOCUS_DISTANCES_INDEX] = 
{	
	420, 330, 300, 240, 180, 140
};

float focus_distances_tbl[MAX_FOCUS_DISTANCES_INDEX] = 
{	
	0.10, 0.20, 0.30, 0.40, 0.50, 0.60
};

int32_t isx006_get_focus_distances(struct focus_distances_type *distances)
{
	int32_t rc = 0;
	int8_t i = 0;
	uint16_t output_actuator_position = 0;	

	CAM_MSG("isx006_get_focus_distances: called: \n");	
	rc = isx006_i2c_read(isx006_client->addr, 0x6D7a, &output_actuator_position, WORD_LEN);
	// CAM_MSG("isx006: reading isx006_get_focus_distances() : output_actuator_position = %d\n", output_actuator_position);
	if (rc < 0){
		CAM_ERR("isx006: reading isx006_get_focus_distances() fail\n");
		return rc;
	}

	for(i = 0 ; i < MAX_FOCUS_DISTANCES_INDEX ; i++)
	{
		if(output_actuator_position >= actuator_position_tbl[i])
		{
			distances->near_focus= focus_distances_tbl[0];
			distances->current_focus = focus_distances_tbl[i];
			distances->far_focus = focus_distances_tbl[MAX_FOCUS_DISTANCES_INDEX-1];
			return rc;
		}
	}

	// maximum distance
	distances->near_focus = focus_distances_tbl[0];
	distances->current_focus = focus_distances_tbl[MAX_FOCUS_DISTANCES_INDEX-1];
	distances->far_focus = focus_distances_tbl[MAX_FOCUS_DISTANCES_INDEX-1];
	
	return rc;
}
/* END : [MS910] sungmin.cho@lge.com 2011.04.07 add focus distances for GB */

/* BUG FIX : msm is changed preview mode but sensor is not change preview mode. */
/* so there is case that vfe get capture image on preview mode */
int32_t isx006_check_output_image_mode(int32_t mode)
{
	int32_t i, rc = 0;
	uint16_t  output_img_mode = 0xFF,current_output_mode = 0xFF;

	if((mode == SENSOR_SNAPSHOT_MODE)||(mode == SENSOR_RAW_SNAPSHOT_MODE))
        	current_output_mode = 0x02;
 	else
		current_output_mode = 0x00;
	
	/* check  current_output_mode */
	for(i=0; i< 30; i++){
		rc = isx006_i2c_read(isx006_client->addr,
				0x0004, &output_img_mode, BYTE_LEN);
		if (rc < 0){
			CAM_ERR("isx006: reading cm_changed_sts fail\n");
			return rc;
		}

		
		if((output_img_mode & 0x03)== current_output_mode){  
			CAM_MSG("mode : %d == cm_changed_sts : ( 0x%x )\n", mode, current_output_mode); 
			break;
		}
		
		mdelay(10);
	}

	return rc;
	
}

int32_t isx006_set_preview_mode(int32_t mode,uint16_t width,uint16_t height)
{
   int32_t rc = 0;

    CAM_MSG("isx006_set_sensor_mode: previous_mode[%d]\n",isx006_ctrl->previous_mode);
		
    if((isx006_ctrl->previous_mode == SENSOR_SNAPSHOT_MODE)||
		(isx006_ctrl->previous_mode == SENSOR_RAW_SNAPSHOT_MODE)){

	  rc = isx006_i2c_write(isx006_client->addr,
	  				ISX006_REG_MODESEL,0x00,BYTE_LEN);	

	  rc = isx006_check_output_image_mode(mode);
		   
	  rc = isx006_cancel_focus(isx006_ctrl->prev_af_mode);
		  
    }
#if 0
    else if(isx006_ctrl->previous_mode == SENSOR_PREVIEW_MODE){	
	  rc = isx006_i2c_read(isx006_client->addr,
	  				ISX006_REG_HSIZE_MONI,&reg_width,WORD_LEN);
      rc = isx006_i2c_read(isx006_client->addr,
	  				ISX006_REG_VSIZE_MONI,&reg_height,WORD_LEN);

      CAM_MSG("%s: reg_width[%d] reg_height[%d] \n",__func__,reg_width,reg_height);


	  if((width != reg_width)||(height != reg_height)){
	  	
		 if(width > 0x0280 ){
		    if((width == 0x0500)&&(height == 0x02D0)){
				reg_width = 0x0500;
				reg_height = 0x02D0;
		    }
			else{
				reg_width = 0x0500;
				reg_height = 0x03C0;
			}
		 }
		 else{
		 	reg_width = 0x0280;
			reg_height = 0x01E0;
		 }

		 rc = isx006_i2c_write(isx006_client->addr,
							ISX006_REG_HSIZE_MONI,reg_width,WORD_LEN);
		 rc = isx006_i2c_write(isx006_client->addr,
							ISX006_REG_VSIZE_MONI,reg_height,WORD_LEN);
		 rc = isx006_i2c_write(isx006_client->addr,
		 					ISX006_REG_MONI_REFRESH,0x01,BYTE_LEN);

		 /*for(i = 0; i < 7 ; i++){  // 1 frame skip
		 	msleep(10);
		 }*/
		
	  }

     }
#endif
	  
	 return rc;
	
}

int32_t isx006_set_snapshot_mode(int32_t mode)
{
	int32_t rc = 0;
		
	rc = isx006_i2c_write(isx006_client->addr,
						0x1D,0x00,BYTE_LEN);		
	
	rc = isx006_i2c_write(isx006_client->addr,
					ISX006_REG_MODESEL,0x02,BYTE_LEN);	

	rc = isx006_check_output_image_mode(mode);

	return rc;
			
}

/*---------------------------------------------------------------------------
    isx006_set_sensor_mode
   ---------------------------------------------------------------------------*/
int32_t isx006_set_sensor_mode(int32_t mode,uint16_t width,uint16_t height)
{
   int32_t rc = 0;
   
   switch (mode) {
   	case SENSOR_PREVIEW_MODE:
		rc = isx006_set_preview_mode(mode,width,height);
		break;
 	case SENSOR_SNAPSHOT_MODE:
	case SENSOR_RAW_SNAPSHOT_MODE:		
		rc = isx006_set_snapshot_mode(mode);
		break;
	default:
		return 0;//-EINVAL;
	}

	isx006_ctrl->previous_mode = mode;
   
	return rc;
	
}

/*---------------------------------------------------------------------------
    isx006_sensor_config
   ---------------------------------------------------------------------------*/
void isx006_enqueue_cfg(struct sensor_cfg_data cfg_data);

int32_t isx006_sensor_config(void __user *argp)
{
	struct sensor_cfg_data cfg_data;
	int32_t   rc = 0;
	 
	if (copy_from_user(&cfg_data,(void *)argp,sizeof(struct sensor_cfg_data)))
		return -EFAULT;

	CAM_MSG("[isx006_sensor_config] isx006_ioctl, cfgtype = %d, mode = %d\n",
					cfg_data.cfgtype, cfg_data.mode);

#if !IMAGE_TUNING_SET
#if USE_INIT_THREAD
	mutex_lock(&isx006_tuning_mutex);
	if (tuning_thread_run == 1) {
		isx006_enqueue_cfg(cfg_data);
		mutex_unlock(&isx006_tuning_mutex);
		return rc;
	}
	mutex_unlock(&isx006_tuning_mutex);
#endif
#endif
	 
	mutex_lock(&isx006_mutex);

	switch (cfg_data.cfgtype) {
	case CFG_SET_MODE:			
		rc = isx006_set_sensor_mode(cfg_data.mode,cfg_data.width,cfg_data.height);  
		break;

#if !IMAGE_TUNING_SET
	case CFG_SET_EFFECT:			
		rc = isx006_set_effect(cfg_data.cfg.effect);
		break;
		
  	case CFG_SET_WB:			
		rc = isx006_set_wb(cfg_data.wb);
		break;
		
	case CFG_SET_ISO:
		rc = isx006_set_iso(cfg_data.iso);
		break;

	case CFG_SET_SCENE_MODE:
		rc = isx006_set_scene_mode(cfg_data.scene_mode);
		break;

	case CFG_SET_BRIGHTNESS:
		rc = isx006_set_brightness(cfg_data.brightness);
		break;
#endif

	case CFG_SET_DEFAULT_FOCUS:		   
		rc = isx006_default_focus();
		break;

	case CFG_SET_PARM_AF_MODE:
		rc = isx006_focus_config(cfg_data.cfg.focus.mode);
		break;

	case CFG_CHECK_AF_DONE:
		rc = isx006_check_focus(&cfg_data.cfg.focus.mode);

		if (copy_to_user((void *)argp, &cfg_data,sizeof(struct sensor_cfg_data)))
			rc = -EFAULT;
		break;

	case CFG_MOVE_FOCUS:
		rc = isx006_set_move_focus(cfg_data.cfg.focus.steps);
		break;	

	case CFG_SET_CANCEL_FOCUS:		   
        rc = isx006_cancel_focus(cfg_data.cfg.focus.mode);
		break;
		 	
    case CFG_GET_AF_MAX_STEPS:
		cfg_data.max_steps = 20;
		if (copy_to_user((void *)argp,&cfg_data, sizeof(struct sensor_cfg_data)))
			rc = -EFAULT;
		break;
	case CFG_SET_FLASH_LED_MODE:
		rc = isx006_set_flash_ctrl(ISX006_FLASH_LED_MODE,cfg_data.led_mode);
		break;
		
	case CFG_SET_FLASH_STATE:
		rc = isx006_update_flash_state(cfg_data.flash_state, isx006_ctrl->iso);
		break;

	case CFG_SET_FPS:
	  	 if(atomic_read(&pll_mode)== CAMERA_VIDEO_MODE_ON) 
		 	rc = isx006_set_preview_fps(cfg_data.cfg.fps.fps_div);
		 break;	

/* START : [MS910] sungmin.cho@lge.com 2011.04.07 add focus distances for GB */
	case CFG_GET_FOCUS_DISTANCES:			
		rc = isx006_get_focus_distances(&cfg_data.focus_distances);
		if (copy_to_user((void *)argp, &cfg_data,sizeof(struct sensor_cfg_data)))
			rc = -EFAULT;
		break;
/* END : [MS910] sungmin.cho@lge.com 2011.04.07 add focus distances for GB */

// START : ADD : sungmin.cho@lge.com 2011.06.20 for CTS
	case CFG_SET_FPS_RANGE:
		rc = isx006_set_fps_range(cfg_data.mode);
		break;
// END : sungmin.cho@lge.com 2011.06.20 for CTS

	default:
		rc = 0;
		break;
	}

	mutex_unlock(&isx006_mutex);

	if (rc < 0){
		CAM_ERR("isx006: ERROR in sensor_config: type[%d] rc[%ld]\n",cfg_data.cfgtype,rc);
	}
	
	return rc;
 	
}

int32_t isx006_dequeue_sensor_config(struct sensor_cfg_data cfg_data)
{
	int32_t rc;

	switch (cfg_data.cfgtype) {
	case CFG_SET_MODE:
		rc = isx006_set_sensor_mode(cfg_data.mode,cfg_data.width,cfg_data.height); 
		break;

	case CFG_SET_EFFECT:
		rc = isx006_set_effect(cfg_data.cfg.effect);
		break;

	case CFG_MOVE_FOCUS:
		rc = isx006_set_move_focus(cfg_data.cfg.focus.steps);
		break;

	case CFG_SET_DEFAULT_FOCUS:
		rc = isx006_default_focus();
		break;

	case CFG_SET_WB:
		rc = isx006_set_wb(cfg_data.wb);
		break;

	case CFG_SET_ISO:
		rc = isx006_set_iso(cfg_data.iso);
		break;
	
	case CFG_SET_SCENE_MODE:
		rc = isx006_set_scene_mode(cfg_data.scene_mode);
		break;

	case CFG_SET_BRIGHTNESS:
		rc = isx006_set_brightness(cfg_data.brightness);
		break;

	case CFG_SET_FPS:
	  	 if(atomic_read(&pll_mode)== CAMERA_VIDEO_MODE_ON) 
		 	rc = isx006_set_preview_fps(cfg_data.cfg.fps.fps_div);
		 break;	

	default:
		rc = 0;
		break;
	}

	return rc;
}

/*---------------------------------------------------------------------------
    isx006 sensor init. code
   ---------------------------------------------------------------------------*/
void isx006_enqueue_cfg(struct sensor_cfg_data cfg_data)
{
	int32_t index = 0;

	if(!isx006_ctrl->queue_cmd)
		return;

    index = isx006_ctrl->tuning_q.max;
	if(index > ISX006_MAX_NUM_QUEUE)
		return;

	memcpy(&isx006_ctrl->queue_cmd[index].cfg_data,&cfg_data,sizeof(struct sensor_cfg_data));

    isx006_cmd_enqueue(&isx006_ctrl->tuning_q,&isx006_ctrl->queue_cmd[index].list);	
	
}
void isx006_dequeue_cfg(struct isx006_device_queue *queue)
{
	struct isx006_queue_cmd *queue_cmd = NULL;
	int32_t i;
	
	for(i= 0 ; i < queue->max ; i++){
	   	queue_cmd = isx006_cmd_dequeue(queue);
		if(queue_cmd)
			isx006_dequeue_sensor_config(queue_cmd->cfg_data);
		
   }
}   

#if USE_INIT_THREAD
int32_t isx006_init_sensor_tuning_reg(void)
{
	int32_t rc = 0;

	mutex_lock(&isx006_tuning_mutex);
	isx006_queue_init(&isx006_ctrl->tuning_q);
	
	if(isx006_ctrl->queue_cmd)
		kfree(isx006_ctrl->queue_cmd);	
	isx006_ctrl->queue_cmd = kmalloc(sizeof(struct isx006_queue_cmd) * ISX006_MAX_NUM_QUEUE, GFP_KERNEL);
	//tuning_thread_run = 1;

	mutex_unlock(&isx006_tuning_mutex);

	rc = isx006_i2c_write_table(&isx006_regs.init[0],
         	 isx006_regs.init_size);
	if(rc < 0){
		CAM_ERR("%s:init writing failed\n",__func__);
		return rc;
	}

	mutex_lock(&isx006_tuning_mutex);
	isx006_dequeue_cfg(&isx006_ctrl->tuning_q);
    if(isx006_ctrl->queue_cmd){
		kfree(isx006_ctrl->queue_cmd);
		isx006_ctrl->queue_cmd = NULL;
    }
	
	tuning_thread_run = 0;
	mutex_unlock(&isx006_tuning_mutex);

	return rc;

}
#endif

int32_t isx006_init_sensor_pll_reg(void)
{
	int32_t rc = 0;

	if(atomic_read(&pll_mode)== CAMERA_VIDEO_MODE_ON){
		printk("%s: CAMERA_TYPE_MAIN_FOR_VT\n",__func__);
		
		rc = isx006_i2c_write_table(&isx006_regs.pll_vt_mode[0],
				 isx006_regs.pll_vt_mode_size); 	
	}
	else{
		printk("%s: CAMERA_TYPE_MAIN \n",__func__);
		
		rc = isx006_i2c_write_table(&isx006_regs.pll[0],
				 isx006_regs.pll_size);
	}
		
	if(rc < 0){
		CAM_ERR("isx006: pll writing fail\n");
	}
		
	return rc;
	
}


/*---------------------------------------------------------------------------
	 isx006 sysfs
   ---------------------------------------------------------------------------*/
static ssize_t isx006_brightness_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	int value;

	sscanf(buf, "%d", &value);
	
	if(value == 0)
		isx006_set_flash_ctrl(ISX006_FLASH_TEST_MODE_LED_MODE,LED_MODE_OFF);
	else
		isx006_set_flash_ctrl(ISX006_FLASH_TEST_MODE_LED_MODE,LED_MODE_ON);
	
	return size;

}

//2011.07.28 - CTS FAIL android.permission.cts.FileSystemPermissionTest#testAllBlockDevicesAreNotReadableWritable
static DEVICE_ATTR(brightness, 0664 , NULL, isx006_brightness_store);
//static DEVICE_ATTR(brightness, S_IRUGO|S_IWUGO, NULL, isx006_brightness_store);

static ssize_t isx006_flash_enable_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	int value;

	sscanf(buf, "%d", &value);
	
	isx006_set_flash_ctrl(ISX006_FLASH_TEST_MODE_ENABLE,(int8_t)value);

	if(isx006_get_flash_ctrl(ISX006_FLASH_TEST_MODE_ENABLE) == 0){
		isx006_set_flash_ctrl(ISX006_FLASH_TEST_MODE_LED_MODE,LED_MODE_OFF);
		isx006_set_flash_ctrl(ISX006_FLASH_PREV_FLASH_STATE,FLASH_STATE_OFF);
	}

	
	return size;
}

//2011.07.28 - CTS FAIL android.permission.cts.FileSystemPermissionTest#testAllBlockDevicesAreNotReadableWritable
static DEVICE_ATTR(flash_enable, 0664 , NULL, isx006_flash_enable_store);
//static DEVICE_ATTR(flash_enable, S_IRUGO|S_IWUGO, NULL, isx006_flash_enable_store);

static ssize_t isx006_flash_state_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	int value;

	sscanf(buf, "%d", &value);
	
	isx006_set_led_state(value);
		
	return size;

}
//2011.07.28 - CTS FAIL android.permission.cts.FileSystemPermissionTest#testAllBlockDevicesAreNotReadableWritable
static DEVICE_ATTR(flash_state, 0664 , NULL, isx006_flash_state_store); 
//static DEVICE_ATTR(flash_state, S_IRUGO|S_IWUGO, NULL, isx006_flash_state_store); 

static ssize_t isx006_mclk_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	printk("[isx006] mclk_rate = %d\n", mclk_rate);
	return 0;
}

static ssize_t isx006_mclk_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	int value;

	sscanf(buf, "%d", &value);
	mclk_rate = value;

	printk("[isx006] mclk_rate = %d\n", mclk_rate);
	return size;
}
//2011.07.28 - CTS FAIL android.permission.cts.FileSystemPermissionTest#testAllBlockDevicesAreNotReadableWritable
 static DEVICE_ATTR(mclk, 0775 , isx006_mclk_show, isx006_mclk_store);
// static DEVICE_ATTR(mclk, S_IRWXUGO, isx006_mclk_show, isx006_mclk_store);
 
 static ssize_t isx006_init_code_store(struct device* dev, struct device_attribute* attr, const char* buf, size_t n)
{
	int val;

	if (isx006_ctrl == NULL)
		return 0;

	sscanf(buf,"%x",&val);

	CAM_MSG("isx006: init code type [0x%x] \n",val);

	/* 		0 : 5M YUV snapshot(preview with 720P, 15fps)
    	    1 : support the preview with 720P, 30fps
	*/
	if(val < 0 || val >= CAMERA_VIDEO_MODE_MAX ) {
		CAM_ERR("isx006: invalid pll type[%d] \n",val);
		val = CAMERA_VIDEO_MODE_OFF;
	}

	atomic_set(&pll_mode,val);
	
	return n;
	
}

//2011.07.28 - CTS FAIL android.permission.cts.FileSystemPermissionTest#testAllBlockDevicesAreNotReadableWritable
static DEVICE_ATTR(init_code, 0664 , NULL, isx006_init_code_store);
//static DEVICE_ATTR(init_code, S_IRUGO|S_IWUGO, NULL, isx006_init_code_store);

static struct attribute* isx006_sysfs_attrs[] = {
	&dev_attr_brightness.attr,
	&dev_attr_flash_enable.attr,
	&dev_attr_flash_state.attr,
	&dev_attr_mclk.attr,
	&dev_attr_init_code.attr,
	NULL
};

static int isx006_sysfs_add(struct kobject* kobj)
{
	int i, n, ret;
	
	n = ARRAY_SIZE(isx006_sysfs_attrs);
	for(i = 0; i < n; i++){
		if(isx006_sysfs_attrs[i]){
			ret = sysfs_create_file(kobj, isx006_sysfs_attrs[i]);
			if(ret < 0){
				CAM_ERR("isx006 sysfs is not created\n");
			}
		}
	}
	return 0;
}

int isx006_sensor_init(const struct msm_camera_sensor_info *data)
{
	int rc = 0;
	int num = 0;
	struct task_struct *p = NULL;

	if (data)
		isx006_ctrl->sensordata = data;
	else
		goto init_fail; 

	data->pdata->camera_power_on();
	mdelay(8);	// T2

#if IMAGE_TUNING_SET
	isx006_reg_pll_ext();

	mdelay(16);  // T3+T4

	isx006_reg_init_ext();

#else

       rc = isx006_init_sensor_pll_reg();
       if(rc < 0){
           for(num = 0 ; num < 5 ; num++){
               msleep(2);
               
               rc = isx006_init_sensor_pll_reg();
               if(rc < 0){
                   printk("[%s] initial register fail: %d \n",__func__,num);	
               }
               else{
                   printk("[%s] retry num[%d] initial register success\n",__func__,num);	
                   break;
               }
           }
       }	

#if USE_INIT_THREAD
	mdelay(16);  // T3+T4

	msleepCount = 0;
	while(tuning_thread_run){
		msleep(5);
		printk("[%s] init thread is running : msleepCount = %d \n",__func__,msleepCount);
		if(msleepCount++ > 500)
			break;
	}	

	tuning_thread_run = 1;

	p = kthread_run(isx006_init_sensor_tuning_reg, 0, "sensor_tuning");
	if (IS_ERR(p))
		return PTR_ERR(p);
#else
	mdelay(16);  // T3+T4

	rc = isx006_i2c_write_table(&isx006_regs.init[0],
         	 isx006_regs.init_size);
	if(rc < 0){
		CAM_ERR("%s:init writing failed\n",__func__);
		return rc;
	}
#endif

	
#endif

	isx006_ctrl->previous_mode = SENSOR_PREVIEW_MODE;
	isx006_ctrl->prev_af_mode = -1;
	isx006_ctrl->video_mode_fps = ISX006_FPS_30;

	printk("%s: isx006_sensor_init successful \n",__func__);
	return rc;

init_fail:
	CAM_ERR("%s: isx006_sensor_init failed\n",__func__);
	return rc;

}

int isx006_sensor_release(void)
{
    isx006_ctrl->sensordata->pdata->camera_power_off();
	isx006_ctrl->af_lock_count = 0;
	isx006_ctrl->video_mode_fps = ISX006_FPS_30;
	isx006_init_flash_ctrl();

	atomic_set(&pll_mode,0);

	return 0;	
}

static int isx006_i2c_probe(struct i2c_client *client,const struct i2c_device_id *id)
{
	int rc = 0;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		rc = -ENOTSUPP;
		goto probe_failure;
	}

	isx006_client = client;
	rc = isx006_sysfs_add(&client->dev.kobj);
	if (rc < 0){
		CAM_ERR("isx006: fail isx006_i2c_probe\n");
	}
	
	return rc;

probe_failure:
	return rc;
	
}

static int isx006_i2c_remove(struct i2c_client *client)
{
	return 0;
}

static const struct i2c_device_id isx006_i2c_ids[] = {
	{"isx006", 0},
	{ /* end of list */},
};

static struct i2c_driver isx006_i2c_driver = {
	.probe  = isx006_i2c_probe,
	.remove = isx006_i2c_remove, 
	.id_table = isx006_i2c_ids,	
	.driver   = {
		.name =  "isx006",
		.owner= THIS_MODULE,	
    },
};

static int isx006_sensor_probe(const struct msm_camera_sensor_info *info,
	                                 struct msm_sensor_ctrl *s)
{
	int rc = 0;

	rc = i2c_add_driver(&isx006_i2c_driver);
	if (rc < 0) {
		rc = -ENOTSUPP;
		CAM_ERR("[isx006_sensor_probe-isx006] return value :ENOTSUPP\n");
		goto probe_done;
	}

	isx006_ctrl = kzalloc(sizeof(struct isx006_ctrl_t), GFP_KERNEL);  
	if (!isx006_ctrl) {
		CAM_ERR("isx006_init failed!\n");
		rc = -ENOMEM;
		goto probe_done;
	}
	
	atomic_set(&pll_mode,0);

	s->s_init    = isx006_sensor_init;
	s->s_release = isx006_sensor_release;
	s->s_config  = isx006_sensor_config;
	s->s_camera_type = BACK_CAMERA_2D;
	s->s_mount_angle = 0;

    return 0;

probe_done:
	kfree(isx006_ctrl);
	return rc;
	
}

static int __isx006_probe(struct platform_device *pdev)
{	
	return msm_camera_drv_start(pdev, isx006_sensor_probe);
}

static struct platform_driver msm_camera_driver = {
	.probe = __isx006_probe,
	.driver = {
		.name = "msm_camera_isx006",
		.owner = THIS_MODULE,
	},
};

static int __init isx006_init(void)
{
	return platform_driver_register(&msm_camera_driver);  
}

module_init(isx006_init);

