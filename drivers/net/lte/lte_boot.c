/*===========================================================================
											lte_boot.c

DESCRIPTION

	 This file contains LTE boot image tranfer operation.

===========================================================================*/
/*===========================================================================
	
								 EDIT HISTORY FOR FILE
								 
	when			   who			   what, where, why
	---------- 	 ---------	 ---------------------------------------------------------- 
	2010/06/09	 Daeok Kim   Creation.
===========================================================================*/

#include <linux/slab.h>

#include "lte_boot.h"
#include "lte_boot_debug.h"
/*BEGIN: 0015094 daeok.kim@lge.com 2011-01-31 */
/*ADD: 0015094: [LTE] To resolve INTR handler initial callinig problem in case of the GPIO_2, GPIO_2's status register should be cleared */
#include <linux/irq.h>
/*END: 0015094 daeok.kim@lge.com 2011-01-31 */

/*BEGIN: 0013724 daeok.kim@lge.com 2011-01-08 */
/*ADD 0013724: [LTE] LTE HW Watchdog Timer expired handling is added */ 
#if defined (LTE_HW_WDT_RST_TEST) && defined (CONFIG_LGE_LTE_ERS)
extern int lte_crash_log(void *buffer, unsigned int size, unsigned int reserved);
int gIrq_lte_hw_wdt;
#endif
/*END: 0013724 daeok.kim@lge.com 2011-01-08 */

extern PLTE_SDIO_INFO gLte_sdio_info;

unsigned int WordToByte =4;

/////////////////////////////////////////////////////////////////
// Function for File Read on the Kernel space
/////////////////////////////////////////////////////////////////
int lte_sdio_boot_fread(struct file *g_filp, char *g_buf, int g_len)
{
  int g_readlen;
  mm_segment_t oldfs;
 
  if (g_filp == NULL)
  {
  	return -ENOENT;
  }
  if (g_filp->f_op->read == NULL) 
  {
	return -ENOSYS;
  }
  oldfs = get_fs();
  set_fs(KERNEL_DS);
  g_readlen = g_filp->f_op->read(g_filp, g_buf, g_len, &g_filp->f_pos);
  set_fs(oldfs);
 
  return g_readlen;	
}

/////////////////////////////////////////////////////////////////
// Function for transfering LTE image through SDIO
/////////////////////////////////////////////////////////////////
LTE_BOOT_STATUS lte_sdio_transfer_image(unsigned char* buff, unsigned int block_cnt,
										unsigned int remained_bytes)
{
  unsigned int	i = 0;
 // LTE_SDIO_STATUS status;
  unsigned int t_remained;
  int error = 0;
  t_remained = remained_bytes;
  

  for(i =0; i <block_cnt; i++)
  {
    sdio_claim_host(gLte_sdio_info->func);		
	error = sdio_memcpy_toio(gLte_sdio_info->func, 0, (void*)buff, LTE_BOOT_SDIO_BLK_SIZE);
	sdio_release_host(gLte_sdio_info->func);
	
	if (error != LTE_SDIO_SUCCESS)
	{
		LBOOT_ERROR("lte_sdio_write failed(512)!\n");
		return error;
	}
	buff = (unsigned char*)(buff + LTE_BOOT_SDIO_BLK_SIZE);
  }
  if(t_remained >= 256)
  {
    sdio_claim_host(gLte_sdio_info->func);		
	error = sdio_memcpy_toio(gLte_sdio_info->func, 0, (void*)buff, 256);
	sdio_release_host(gLte_sdio_info->func);
	
	if (error != LTE_SDIO_SUCCESS)
	{
		LBOOT_ERROR("lte_sdio_write failed(256)!\n");
		return error;
	}
	t_remained = t_remained - 256;
	buff = (unsigned char*)(buff + 256);
  }
  if(t_remained >= 128)
  {
    sdio_claim_host(gLte_sdio_info->func);		
	error = sdio_memcpy_toio(gLte_sdio_info->func, 0, (void*)buff, 128);
	sdio_release_host(gLte_sdio_info->func);
	
	if (error != LTE_SDIO_SUCCESS)
	{
		LBOOT_ERROR("lte_sdio_write failed(128)!\n");
		return error;
	}
	t_remained = t_remained - 128;
	buff = (unsigned char*)(buff + 128);
  }
  if(t_remained >= 64)
  {
    sdio_claim_host(gLte_sdio_info->func);		
	error = sdio_memcpy_toio(gLte_sdio_info->func, 0, (void*)buff, 64);
	sdio_release_host(gLte_sdio_info->func);

	if (error != LTE_SDIO_SUCCESS)
	{
		LBOOT_ERROR("lte_sdio_write failed(64)!\n");
		return error;
	}
	t_remained = t_remained - 64;
	buff = (unsigned char*)(buff + 64);
  }
  if(t_remained >= 32)
  {
    sdio_claim_host(gLte_sdio_info->func);		
	error = sdio_memcpy_toio(gLte_sdio_info->func, 0, (void*)buff, 32);
	sdio_release_host(gLte_sdio_info->func);

	if (error != LTE_SDIO_SUCCESS)
	{
		LBOOT_ERROR("lte_sdio_write failed(32)!\n");
		return error;
	}
	t_remained = t_remained - 32;
	buff = (unsigned char*)(buff + 32);
  }
  if(t_remained >= 16)
  {
    sdio_claim_host(gLte_sdio_info->func);		
	error = sdio_memcpy_toio(gLte_sdio_info->func, 0, (void*)buff, 16);
	sdio_release_host(gLte_sdio_info->func);

	if (error != LTE_SDIO_SUCCESS)
	{
		LBOOT_ERROR("lte_sdio_write failed(16)!\n");
		return error;
	}
	t_remained = t_remained - 16;
	buff = (unsigned char*)(buff + 16);
  }
  if(t_remained >= 8)
  {
    sdio_claim_host(gLte_sdio_info->func);		
	error = sdio_memcpy_toio(gLte_sdio_info->func, 0, (void*)buff, 8);
	sdio_release_host(gLte_sdio_info->func);

	if (error != LTE_SDIO_SUCCESS)
	{
		LBOOT_ERROR("lte_sdio_write failed(8)!\n");
		return error;
	}
	t_remained = t_remained - 8;
	buff = (unsigned char*)(buff + 8);
  }
  if(t_remained == 4)
  {
    sdio_claim_host(gLte_sdio_info->func);		
	error = sdio_memcpy_toio(gLte_sdio_info->func, 0, (void*)buff, 4);
	sdio_release_host(gLte_sdio_info->func);

	if (error != LTE_SDIO_SUCCESS)
	{
		LBOOT_ERROR("lte_sdio_write failed(4)!\n");
		return error;
	}
	t_remained = t_remained - 4;
  }
  if(t_remained != 0)
  {
	LBOOT_ERROR("LTE Byte allign problem! \n");
  }
	
  return LTE_BOOT_NO_ERROR; 
}

////unsigned char LTE_GPR_TEST =0;
/////////////////////////////////////////////////////////////////
//
// PBC Transfer
//
/////////////////////////////////////////////////////////////////

////static const char SUPP_CONFIG_FILE[]    = "/data/misc/wifi/wpa_supplicant.conf";
/* BEGIN : daeok.kim@lge.com 20110206 */
/* ADD 0015440: [LTE] LTE image (singed_app.bin) partition is changed to system from persist */
#if defined (CONFIG_LGE_LTE_IMAGE_IN_PERSIST)
static const char LTE_PBC[]         = "/persist/lte_images/Normal_pbc.bin";
#elif defined (CONFIG_LGE_LTE_IMAGE_IN_DATA)
static const char LTE_PBC[]         = "/data/data/lte_images/Normal_pbc.bin";
#else
static const char LTE_PBC[]         = "/etc/lte_images/Normal_pbc.bin";
#endif
/* END : daeok.kim@lge.com 20110206 */

unsigned char LTE_GPR_TEST =0;
LTE_BOOT_STATUS lte_sdio_boot_PBC(void)
{
  LTE_BOOT_STATUS status = 0;
  int error = 0;
  
  unsigned char		lte_gpr_value =0;
  unsigned short    time_cnt =0;

  unsigned char		*CTRL_pbc_buff, *H_pbc_buff, *B_pbc_buff;
  int 				readlen =0, H_pbc_rlen =0, B_pbc_rlen =0;
  
// 1.To ready PBC image, check location and size
  unsigned int header_size =0, body_size =0, i =0;
  unsigned int H_block_cnt =0, B_block_cnt =0;
  unsigned int H_remained_bytes =0, B_remained_bytes =0;

  struct file *fp_PBC = NULL;

  FUNC_ENTER();

  
  fp_PBC = filp_open(LTE_PBC, O_RDONLY, 0);
  if (IS_ERR(fp_PBC))
		return PTR_ERR(fp_PBC);

// 2.Write GPR, Initialized
////  lte_sdio_write_gpr(GPR_0,LTE_BOOT_GPR_INITIALIZED);
  sdio_claim_host(gLte_sdio_info->func);	
  sdio_writeb(gLte_sdio_info->func, LTE_BOOT_GPR_INITIALIZED, GPR_0, &error);
  sdio_release_host(gLte_sdio_info->func);	
  if (error) 
  {
  	LBOOT_ERROR("[PBC] Failed to write GPR 0x5A\n");
	goto err;
  }
// 3.Read GPR, LTE initialized or not
  while ( lte_gpr_value != LTE_BOOT_GPR_INITIALIZED)
  {
////    lte_sdio_read_gpr(GPR_2,&lte_gpr_value);
	sdio_claim_host(gLte_sdio_info->func);			
	lte_gpr_value = sdio_readb(gLte_sdio_info->func, GPR_2, &error);
	sdio_release_host(gLte_sdio_info->func);
	if (error) 
    {
  	  LBOOT_ERROR("[PBC] Failed to Read GPR 0x5A\n");
	  goto err;
    }
	
    if( lte_gpr_value ==LTE_BOOT_GPR_INITIALIZED)
      break;

	mdelay(1); // 1ms Unit

    if (time_cnt > 1000) // 1 second
      return LTE_BOOT_ERROR_INIT;

    time_cnt++;
  }

  time_cnt = 0;
// 4.If LTE initialized, Write GPR, Initialized
////  lte_sdio_write_gpr(GPR_0,LTE_BOOT_GPR_INITIALIZED);
  sdio_claim_host(gLte_sdio_info->func);	
  sdio_writeb(gLte_sdio_info->func, LTE_BOOT_GPR_INITIALIZED, GPR_0, &error);
  sdio_release_host(gLte_sdio_info->func);
  if (error) 
  {
  	LBOOT_ERROR("[PBC] Failed to write GPR 0x5A\n");
	goto err;
  }
  
// After delay, Write GPR, Ready
  mdelay(5); // Delay
////  lte_sdio_write_gpr(GPR_0,LTE_BOOT_GPR_READY);
  sdio_claim_host(gLte_sdio_info->func);	
  sdio_writeb(gLte_sdio_info->func, LTE_BOOT_GPR_READY, GPR_0, &error);
  sdio_release_host(gLte_sdio_info->func);
  if (error) 
  {
  	LBOOT_ERROR("[PBC] Failed to write GPR 0x5B\n");
	goto err;
  }
  
  lte_gpr_value = 0;

// 5.Read GPR, LTE Ready or not
  while ( lte_gpr_value != LTE_BOOT_GPR_READY)
  {
////    lte_sdio_read_gpr(GPR_2,&lte_gpr_value);
	sdio_claim_host(gLte_sdio_info->func);			
	lte_gpr_value = sdio_readb(gLte_sdio_info->func, GPR_2, &error);
	sdio_release_host(gLte_sdio_info->func);
	if (error) 
    {
  	  LBOOT_ERROR("[PBC] Failed to Read GPR 0x5B\n");
	  goto err;
    }
	
    if( lte_gpr_value ==LTE_BOOT_GPR_READY)
      break;

    mdelay(1); // 1ms Unit

    if (time_cnt > 1000) // 1 second
      return LTE_BOOT_ERROR_READY;

    time_cnt++;
  }

  time_cnt = 0;

// 6.If LTE Ready Write GPR, CTLBL
////  lte_sdio_write_gpr(GPR_0,LTE_BOOT_GPR_CTLBL);
  sdio_claim_host(gLte_sdio_info->func);	
  sdio_writeb(gLte_sdio_info->func, LTE_BOOT_GPR_CTLBL, GPR_0, &error);
  sdio_release_host(gLte_sdio_info->func);
  if (error) 
  {
  	LBOOT_ERROR("[PBC] Failed to write GPR 0x5C\n");
	goto err;
  }
  
  lte_gpr_value = 0;

// 7.Read GPR, LTE CTLBL or not
  while ( lte_gpr_value != LTE_BOOT_GPR_CTLBL)
  {
////    lte_sdio_read_gpr(GPR_2,&lte_gpr_value);

    sdio_claim_host(gLte_sdio_info->func);			
	lte_gpr_value = sdio_readb(gLte_sdio_info->func, GPR_2, &error);
	sdio_release_host(gLte_sdio_info->func);
	if (error) 
    {
  	  LBOOT_ERROR("[PBC] Failed to Read GPR 0x5C\n");
	  goto err;
    }
	
    if( lte_gpr_value ==LTE_BOOT_GPR_CTLBL)
      break;

    mdelay(1); // 1ms Unit

    if (time_cnt > 1000) // 1 second
      return LTE_BOOT_ERROR_CTLBL;

    time_cnt++;
  }

  time_cnt = 0;
// 8.If LTE CTLBL, Transfer CTLBL (size = 512 bytes)
  CTRL_pbc_buff = kzalloc(LTE_BOOT_CTLBL_SIZE, GFP_KERNEL);
  if (CTRL_pbc_buff == NULL)
  {
    LBOOT_ERROR("[PBC] Failed to allocate memory for CTRL_pbc_buff\n");
    return -ENOMEM;
  }

  /* now read len bytes from offset 0 */
  fp_PBC->f_pos = 0;  //start offset
  readlen = lte_sdio_boot_fread(fp_PBC, CTRL_pbc_buff, LTE_BOOT_CTLBL_SIZE);
  if (readlen != LTE_BOOT_CTLBL_SIZE)
  {
    LBOOT_ERROR("fread(CTLBL) failed: %d\n", readlen);
  }

  status = lte_sdio_transfer_image(CTRL_pbc_buff, 1, 0);
////  status = lte_sdio_transfer_image(PBC_CTLBL_start_addr, 1, 0); 

// 9.If Error, Report error
  if (status != LTE_BOOT_NO_ERROR)
  {
    LBOOT_ERROR("[PBC] Transfer PBC CTLBL ERROR\n");
////    lte_sdio_write_gpr(GPR_0,LTE_BOOT_GPR_ROM_ERROR);
	sdio_claim_host(gLte_sdio_info->func);	
    sdio_writeb(gLte_sdio_info->func, LTE_BOOT_GPR_ROM_ERROR, GPR_0, &error);
    sdio_release_host(gLte_sdio_info->func);
    if (error) 
    {
	  LBOOT_ERROR("[PBC] Failed to write GPR 0xA5\n");
  	  goto err;
    }
	  
    return LTE_BOOT_ERROR_CTLBL;
  }
  
  kfree(CTRL_pbc_buff);
  
// 10.If Success of transfer CTLBL, Write GPR, HDR_Start
////  lte_sdio_write_gpr(GPR_0,LTE_BOOT_GPR_HDR_START);
  sdio_claim_host(gLte_sdio_info->func);	
  sdio_writeb(gLte_sdio_info->func, LTE_BOOT_GPR_HDR_START, GPR_0, &error);
  sdio_release_host(gLte_sdio_info->func);
  if (error) 
  {
    LBOOT_ERROR("[PBC] Failed to write GPR 0x5D\n");
  	goto err;
  }
  
  lte_gpr_value = 0;

// 11.Read GPR, LTE HDR_Start or not
  while ( lte_gpr_value != LTE_BOOT_GPR_HDR_START)
  {
////    lte_sdio_read_gpr(GPR_2,&lte_gpr_value);
    sdio_claim_host(gLte_sdio_info->func);			
	lte_gpr_value = sdio_readb(gLte_sdio_info->func, GPR_2, &error);
	sdio_release_host(gLte_sdio_info->func);
	if (error) 
    {
  	  LBOOT_ERROR("[PBC] Failed to Read GPR 0x5D\n");
	  goto err;
    }

    if( lte_gpr_value ==LTE_BOOT_GPR_HDR_START)
      break;

    mdelay(1); // 1ms Unit

    if (time_cnt > 1000) // 1 second
      return LTE_BOOT_ERROR_HDR_START;

    time_cnt++;
  }

  time_cnt = 0;
// 12.If LTE HDR_Start, Transfer PBC Header
  H_pbc_buff = kzalloc(sizeof(PBC_Xheader_t), GFP_KERNEL);
  if (H_pbc_buff == NULL)
  {
    LBOOT_ERROR("[PBC] Failed to allocate memory for H_pbc_buff\n");
    return -ENOMEM;
  }

  readlen = lte_sdio_boot_fread(fp_PBC, H_pbc_buff, sizeof(PBC_Xheader_t));
  if (readlen != sizeof(PBC_Xheader_t))
  {
    LBOOT_ERROR("[PBC] fread(H_pbc) failed: %d\n", readlen);
  }

  H_pbc_rlen = readlen;
  LBOOT_INFO("[PBC] header_size/H_pbc_rlen= %d/%d\n", sizeof(PBC_Xheader_t), readlen);

  header_size = (((PBC_Xheader_t*)H_pbc_buff)->header1.LenInWords) * WordToByte;
  H_block_cnt = (header_size/LTE_BOOT_SDIO_BLK_SIZE);
  H_remained_bytes = header_size - (H_block_cnt * LTE_BOOT_SDIO_BLK_SIZE);

  body_size = (((PBC_Xheader_t*)H_pbc_buff)->header2.LenInWords) * WordToByte;
  B_block_cnt = (body_size/LTE_BOOT_SDIO_BLK_SIZE);
  B_remained_bytes = body_size - (B_block_cnt * LTE_BOOT_SDIO_BLK_SIZE);
  
  status = lte_sdio_transfer_image(H_pbc_buff, H_block_cnt, H_remained_bytes); 

////  status = lte_sdio_transfer_image(PBC_H_start_addr, H_block_cnt, H_remained_bytes); 

// 13.If Error, Report error
  if (status != LTE_BOOT_NO_ERROR)
  {
    LBOOT_ERROR("[PBC] Transfer PBC Header ERROR\n");
////    lte_sdio_write_gpr(GPR_0,LTE_BOOT_GPR_ROM_ERROR);
    sdio_claim_host(gLte_sdio_info->func);	
    sdio_writeb(gLte_sdio_info->func, LTE_BOOT_GPR_ROM_ERROR, GPR_0, &error);
    sdio_release_host(gLte_sdio_info->func);
    if (error) 
    {
      LBOOT_ERROR("[PBC] Failed to write GPR 0xA5\n");
      goto err;
    }
    return LTE_BOOT_ERROR_HDR_PBC;
  }

// 14.If Success of transfer PBC Header, Write GPR, PBC_Start
  LBOOT_INFO("Transfer PBC Header SUCCESS\n");
////  lte_sdio_write_gpr(GPR_0,LTE_BOOT_GPR_PBC_START);
  sdio_claim_host(gLte_sdio_info->func);	
  sdio_writeb(gLte_sdio_info->func, LTE_BOOT_GPR_PBC_START, GPR_0, &error);
  sdio_release_host(gLte_sdio_info->func);
  if (error) 
  {
    LBOOT_ERROR("[PBC] Failed to write GPR 0x5E\n");
    goto err;
  }
  lte_gpr_value = 0;

// 15.Read GPR, LTE PBC_Start or not
  while ( lte_gpr_value != LTE_BOOT_GPR_PBC_START)
  {
////    lte_sdio_read_gpr(GPR_2,&lte_gpr_value);
    sdio_claim_host(gLte_sdio_info->func);			
	lte_gpr_value = sdio_readb(gLte_sdio_info->func, GPR_2, &error);
	sdio_release_host(gLte_sdio_info->func);
	if (error) 
    {
  	  LBOOT_ERROR("[PBC] Failed to Read GPR 0x5E\n");
	  goto err;
    }
    if( lte_gpr_value ==LTE_BOOT_GPR_PBC_START)
      break;

    mdelay(1); // 1ms Unit

    if (time_cnt > 1000) // 1 second
      return LTE_BOOT_ERROR_PBC_START;

    time_cnt++;
  }

// 16.If LTE PBC_Start, Transfer PBC body by block unit
  for(i=0; i<B_block_cnt; i++)
  {
    B_pbc_buff = kzalloc(LTE_BOOT_SDIO_BLK_SIZE, GFP_KERNEL);
    if (H_pbc_buff == NULL)
    {
      LBOOT_ERROR("[PBC] Failed to allocate memory for B_pbc_buff\n");
      return -ENOMEM;
    }

    readlen = lte_sdio_boot_fread(fp_PBC, B_pbc_buff, LTE_BOOT_SDIO_BLK_SIZE);
    if (readlen != LTE_BOOT_SDIO_BLK_SIZE)
    {
      LBOOT_ERROR("[PBC] fread(B_pbc) failed: %d\n", readlen);
    }
    B_pbc_rlen +=readlen;
    status = lte_sdio_transfer_image(B_pbc_buff, 1, 0); 
    // 17.If Error, Report error
    if (status != LTE_BOOT_NO_ERROR)
    {
      LBOOT_ERROR("[PBC] Transfer PBC Body[%d] ERROR\n", B_block_cnt);
////    lte_sdio_write_gpr(GPR_0,LTE_BOOT_GPR_ROM_ERROR);
      sdio_claim_host(gLte_sdio_info->func);      
      sdio_writeb(gLte_sdio_info->func, LTE_BOOT_GPR_ROM_ERROR, GPR_0, &error);
      sdio_release_host(gLte_sdio_info->func);
      if (error) 
      {
        LBOOT_ERROR("[PBC] Failed to write GPR 0xA5\n");
        goto err;
      }
      return LTE_BOOT_ERROR_PBC;
    }
    kfree(B_pbc_buff);
  }
  B_pbc_buff = kzalloc(B_remained_bytes, GFP_KERNEL);
  if (B_pbc_buff == NULL)
  {
    LBOOT_ERROR("[PBC] Failed to allocate memory for B_pbc_buff\n");
    return -ENOMEM;
  }
  readlen = lte_sdio_boot_fread(fp_PBC, B_pbc_buff, B_remained_bytes);
  if (readlen != B_remained_bytes)
  {
    LBOOT_ERROR("[PBC] fread(B_pbc) failed: %d\n", readlen);
  }
  
  B_pbc_rlen += readlen; 
  LBOOT_INFO("[PBC] body_size/B_pbc_rlen=%d/%d\n", body_size, B_pbc_rlen);
  
  status = lte_sdio_transfer_image(B_pbc_buff, 0, B_remained_bytes);
  
////  status = lte_sdio_transfer_image(PBC_B_start_addr, B_block_cnt, B_remained_bytes); 

// 17.If Error, Report error
  if (status != LTE_BOOT_NO_ERROR)
  {
    LBOOT_ERROR("[PBC] Transfer PBC Body ERROR\n");
////    lte_sdio_write_gpr(GPR_0,LTE_BOOT_GPR_ROM_ERROR);
    sdio_claim_host(gLte_sdio_info->func);	
    sdio_writeb(gLte_sdio_info->func, LTE_BOOT_GPR_ROM_ERROR, GPR_0, &error);
    sdio_release_host(gLte_sdio_info->func);
    if (error) 
    {
      LBOOT_ERROR("[PBC] Failed to write GPR 0xA5\n");
      goto err;
    }
    return LTE_BOOT_ERROR_PBC;
  }
  kfree(B_pbc_buff);
  kfree(H_pbc_buff);
  filp_close(fp_PBC, NULL);
  
// 18.If PBC body done, Write GPR, PBC body done
    
////  lte_sdio_write_gpr(GPR_0,LTE_BOOT_GPR_DONE);
  sdio_claim_host(gLte_sdio_info->func);	
  sdio_writeb(gLte_sdio_info->func, LTE_BOOT_GPR_DONE, GPR_0, &error);
  sdio_release_host(gLte_sdio_info->func);
  if (error) 
  {
    LBOOT_ERROR("[PBC] Failed to write GPR 0x5F\n");
    goto err;
  }

#if 0
  lte_sdio_read_gpr(GPR_0,&lte_gpr_value);
  LTE_GPR_TEST = lte_gpr_value;
//  while(1);
#endif

#if 1
  lte_gpr_value = 0;
// 19.Read GPR, LTE PBC body done or not
  while ( lte_gpr_value != LTE_BOOT_GPR_DONE)
  {
////    lte_sdio_read_gpr(GPR_2,&lte_gpr_value);
    sdio_claim_host(gLte_sdio_info->func);			
	lte_gpr_value = sdio_readb(gLte_sdio_info->func, GPR_2, &error);
	sdio_release_host(gLte_sdio_info->func);
	if (error) 
    {
  	  LBOOT_ERROR("[PBC] Failed to Read GPR 0x5F\n");
	  goto err;
    }
    if( lte_gpr_value ==LTE_BOOT_GPR_DONE)
      break;

    mdelay(1); // 1ms Unit

    if (time_cnt > 1000) // 1 second
      return LTE_BOOT_ERROR_PBC_DONE;

    time_cnt++;
  }
#endif
  LBOOT_INFO("PBC Download End !! \n");	
  // We should use free(buffer)  
	return LTE_BOOT_NO_ERROR;

err:
	FUNC_EXIT();
	return error;
  
}


  
/////////////////////////////////////////////////////////////////
//
// CPU Transfer
//
/////////////////////////////////////////////////////////////////
LTE_BOOT_STATUS lte_sdio_boot_main_image(void)
{
  LTE_BOOT_STATUS	status = 0;
  int error = 0;
  
  unsigned char		lte_gpr_value = 0;
  unsigned short    time_cnt = 0, i =0, j =0; 

  unsigned char		*tmp_H_cpu_buff1, *tmp_H_cpu_buff2, *H_cpu_buff, *B_cpu_buff;
  int 				readlen =0, H_cpu_rlen =0, B_cpu_rlen =0;
  
// 1.To ready CPU image, check Header and body's location and size
  unsigned int		header_size =0;
  unsigned int		body_size[CPU_IMG_MAX_RECORDS] ={};
  unsigned int   	H_block_cnt =0;
  unsigned int		B_block_cnt[CPU_IMG_MAX_RECORDS] ={};
  unsigned int   	H_remained_bytes =0;
  unsigned int		B_remained_bytes[CPU_IMG_MAX_RECORDS] ={};
  unsigned int   	NumRecords =0; // The Number of images(=body)
  
  struct file *fp_CPU = NULL;
/* BEGIN : daeok.kim@lge.com 20110206 */
/* ADD 0015440: [LTE] LTE image (singed_app.bin) partition is changed to system from persist */ 
  unsigned int 		flag_lte_parition =0;
  struct file *fp_LTE_exist = NULL;
  #if defined(ENG) || defined(USERDEBUG) || defined(USER)
    flag_lte_parition   =1;
#if defined (CONFIG_LGE_LTE_IMAGE_IN_PERSIST)
#ifdef CONFIG_LGE_USB_GADGET_LLDM_DRIVER
	static const char LTE_CPU_persist[] 	= "/persist/lte_images/signed_app_lldm.bin";
	static const char LTE_CPU[] 	= "/etc/lte_images/signed_app_lldm.bin";
	LBOOT_ERROR("[LTE_IMAGE] ENG or USERDEBUG, persist lldm routing \n");
#else
  	static const char LTE_CPU_persist[]		= "/persist/lte_images/signed_app.bin";
  	static const char LTE_CPU[] 	= "/etc/lte_images/signed_app.bin";
  	LBOOT_ERROR("[LTE_IMAGE] ENG or USERDEBUG, persist \n");
#endif	
#elif defined (CONFIG_LGE_LTE_IMAGE_IN_DATA)
  	static const char LTE_CPU_persist[] 	= "/data/data/lte_images/signed_app.bin";
  	static const char LTE_CPU[] 	= "/etc/lte_images/signed_app.bin";
  	LBOOT_ERROR("[LTE_IMAGE] ENG or USERDEBUG, data/data/lte_images \n");
#else
  	static const char LTE_CPU_persist[] 	= "/data/data/lte_images/signed_app.bin";
  	static const char LTE_CPU[] 	= "/etc/lte_images/signed_app.bin";
  	LBOOT_ERROR("[LTE_IMAGE] ENG or USERDEBUG, data/data/lte_images \n");
#endif
  	fp_LTE_exist = filp_open(LTE_CPU_persist, O_RDONLY, 0);
  	if (IS_ERR(fp_LTE_exist)) // LTE image is not existed in persist partition
  	{
		flag_lte_parition =0;
		LBOOT_ERROR("[LTE_IMAGE] ENG or USERDEBUG, system \n");
  	}
  #endif

#ifdef LG_FW_LTE_COMPILE_ERROR
  #if defined(USER)
    flag_lte_parition =0;
    static const char LTE_CPU_persist[]		= "/persist/lte_images/signed_app.bin";
  	static const char LTE_CPU[]		= "/etc/lte_images/signed_app.bin";
    LBOOT_ERROR("[LTE_IMAGE] USER, system \n");
  #endif
#endif /* LG_FW_LTE_COMPILE_ERROR */
  
    FUNC_ENTER();
  
  if (flag_lte_parition ==1) // LTE CPU image is in the persist partition
  {
  	  fp_CPU = filp_open(LTE_CPU_persist, O_RDONLY, 0);
  }
  else // LTE CPU image is in the system partition
  {
	  fp_CPU = filp_open(LTE_CPU, O_RDONLY, 0);
  }
/* END : daeok.kim@lge.com 20110206 */  
  if (IS_ERR(fp_CPU))
		return PTR_ERR(fp_CPU);


// 2.Write GPR, BL_Initialized
////  lte_sdio_write_gpr(GPR_0,LTE_BOOT_GPR_BL_INITIALIZED);
  sdio_claim_host(gLte_sdio_info->func);	
  sdio_writeb(gLte_sdio_info->func, LTE_BOOT_GPR_BL_INITIALIZED, GPR_0, &error);
  sdio_release_host(gLte_sdio_info->func);
  if (error) 
  {
    LBOOT_ERROR("[CPU] Failed to write GPR 0x7A\n");
    goto err;
  }

// 3.Read GPR, LTE BL_initialized or not
  while ( lte_gpr_value != LTE_BOOT_GPR_BL_INITIALIZED)
  {
////    lte_sdio_read_gpr(GPR_2,&lte_gpr_value);
    sdio_claim_host(gLte_sdio_info->func);			
    lte_gpr_value = sdio_readb(gLte_sdio_info->func, GPR_2, &error);
    sdio_release_host(gLte_sdio_info->func);
    if (error) 
    {
      LBOOT_ERROR("[CPU] Failed to Read GPR 0x7A\n");
      goto err;
    }
    if( lte_gpr_value ==LTE_BOOT_GPR_BL_INITIALIZED)
      break;
    
    mdelay(1); // 1ms Unit

    if (time_cnt > 4000) // 4 seconds
    {
	  LTE_GPR_TEST = lte_gpr_value;
      return LTE_BOOT_ERROR_INIT;
    }

    time_cnt++;
  }

  time_cnt = 0;
// 4.If LTE initialized, Write GPR, BL_Initialized
////  lte_sdio_write_gpr(GPR_0,LTE_BOOT_GPR_BL_INITIALIZED);
  sdio_claim_host(gLte_sdio_info->func);	
  sdio_writeb(gLte_sdio_info->func, LTE_BOOT_GPR_BL_INITIALIZED, GPR_0, &error);
  sdio_release_host(gLte_sdio_info->func);
  if (error) 
  {
    LBOOT_ERROR("[CPU] Failed to write GPR 0x7A\n");
    goto err;
  }
  
// 5.After delay, Write GPR, BL_Ready
  mdelay(5); // Delay
////  lte_sdio_write_gpr(GPR_0,LTE_BOOT_GPR_BL_READY);
  sdio_claim_host(gLte_sdio_info->func);	
  sdio_writeb(gLte_sdio_info->func, LTE_BOOT_GPR_BL_READY, GPR_0, &error);
  sdio_release_host(gLte_sdio_info->func);
  if (error) 
  {
    LBOOT_ERROR("[CPU] Failed to write GPR 0x7B\n");
    goto err;
  }
  lte_gpr_value = 0;

// 6.Read GPR, LTE BL_Ready or not
  while ( lte_gpr_value != LTE_BOOT_GPR_BL_READY)
  {
////    lte_sdio_read_gpr(GPR_2,&lte_gpr_value);
    sdio_claim_host(gLte_sdio_info->func);			
    lte_gpr_value = sdio_readb(gLte_sdio_info->func, GPR_2, &error);
    sdio_release_host(gLte_sdio_info->func);
    if (error) 
    {
      LBOOT_ERROR("[CPU] Failed to Read GPR 0x7B\n");
      goto err;
    }
    if( lte_gpr_value ==LTE_BOOT_GPR_BL_READY)
      break;

    mdelay(1); // 1ms Unit

    if (time_cnt > 4000) // 4 seconds
      return LTE_BOOT_ERROR_READY;

    time_cnt++;
  }

  time_cnt = 0;

// 7.If LTE BL_Ready, Write GPR2, Hearder size (32byte Unit) 
  tmp_H_cpu_buff1 = kzalloc(sizeof(VRL_Header_t), GFP_KERNEL);
  if (tmp_H_cpu_buff1 == NULL)
  {
    LBOOT_ERROR("[CPU] Failed to allocate memory for tmp_H_cpu_buff\n");
    return -ENOMEM;
  }

  /* now read len bytes from offset 0 */
  fp_CPU->f_pos = 0;  //start offset
  readlen = lte_sdio_boot_fread(fp_CPU, tmp_H_cpu_buff1, sizeof(VRL_Header_t));
  if (readlen != sizeof(VRL_Header_t))
  {
    LBOOT_ERROR("[CPU] fread(tmp_H_cpu1) failed: %d\n", readlen);
  }
  
  NumRecords = (((VRL_Header_t*)tmp_H_cpu_buff1)->NumOfRecords);
  header_size = sizeof(VRL_Header_t) + (sizeof(VRL_RecordInfo_t) * NumRecords) + sizeof(VRL_Sign_t);
///  LBOOT_INFO("[CPU] NumRecords = %d\n", NumRecords);
///  LBOOT_INFO("[CPU] Header size = %d\n", header_size);

////  lte_sdio_write_gpr(GPR_1,header_size/WordToByte); // Add Function at lte_sdio
  sdio_claim_host(gLte_sdio_info->func);	
  sdio_writeb(gLte_sdio_info->func, header_size/WordToByte, GPR_1, &error);
  sdio_release_host(gLte_sdio_info->func);
  if (error) 
  {
    LBOOT_ERROR("[CPU] Failed to write GPR Header size\n");
    goto err;
  }
  
// 8.Write GPR, BL_PRE_HDR
////  lte_sdio_write_gpr(GPR_0,LTE_BOOT_GPR_BL_PRE_HDR);
  sdio_claim_host(gLte_sdio_info->func);	
  sdio_writeb(gLte_sdio_info->func, LTE_BOOT_GPR_BL_PRE_HDR, GPR_0, &error);
  sdio_release_host(gLte_sdio_info->func);
  if (error) 
  {
    LBOOT_ERROR("[CPU] Failed to write GPR 0x85\n");
    goto err;
  }
  lte_gpr_value = 0;

// 9.Read GPR, LTE BL_PRE_HDR or not
  while ( lte_gpr_value != LTE_BOOT_GPR_BL_PRE_HDR)
  {
////    lte_sdio_read_gpr(GPR_2,&lte_gpr_value);
    sdio_claim_host(gLte_sdio_info->func);			
    lte_gpr_value = sdio_readb(gLte_sdio_info->func, GPR_2, &error);
    sdio_release_host(gLte_sdio_info->func);
    if (error) 
    {
      LBOOT_ERROR("[CPU] Failed to Read GPR 0x85\n");
      goto err;
    }
    if( lte_gpr_value ==LTE_BOOT_GPR_BL_PRE_HDR)
      break;

    mdelay(1); // 1ms Unit

    if (time_cnt > 4000) // 4 seconds
      return LTE_BOOT_ERROR_PRE_HDR;

    time_cnt++;
  }

  time_cnt = 0;

// 10.IF LTE BL_PRE_HDR, Write GPR, BL_HDR_Start -->Return point
////  lte_sdio_write_gpr(GPR_0,LTE_BOOT_GPR_BL_HDR_START);
  sdio_claim_host(gLte_sdio_info->func);	
  sdio_writeb(gLte_sdio_info->func, LTE_BOOT_GPR_BL_HDR_START, GPR_0, &error);
  sdio_release_host(gLte_sdio_info->func);
  if (error) 
  {
    LBOOT_ERROR("[CPU] Failed to write GPR 0x7C\n");
    goto err;
  }
  
  lte_gpr_value = 0;

// 11.Read GPR, LTE BL_HDR_Start or not
  while ( lte_gpr_value != LTE_BOOT_GPR_BL_HDR_START)
  {
////    lte_sdio_read_gpr(GPR_2,&lte_gpr_value);
    sdio_claim_host(gLte_sdio_info->func);			
    lte_gpr_value = sdio_readb(gLte_sdio_info->func, GPR_2, &error);
    sdio_release_host(gLte_sdio_info->func);
    if (error) 
    {
      LBOOT_ERROR("[CPU] Failed to Read GPR 0x7C\n");
      goto err;
    }

    if( lte_gpr_value ==LTE_BOOT_GPR_BL_HDR_START)
      break;

    mdelay(1); // 1ms Unit

    if (time_cnt > 4000) // 4 seconds
      return LTE_BOOT_ERROR_HDR_START;

    time_cnt++;
  }

  time_cnt = 0;

// 12.If LTE BL_HDR_Start, Transfer CPU Header
  tmp_H_cpu_buff2 = kzalloc(header_size, GFP_KERNEL);
  if (tmp_H_cpu_buff2 == NULL)
  {
    LBOOT_ERROR("[CPU] Failed to allocate memory for tmp_H_cpu_buff\n");
    return -ENOMEM;
  }

  /* now read len bytes from offset 0 */
  fp_CPU->f_pos = 0;  //start offset
  readlen = lte_sdio_boot_fread(fp_CPU, tmp_H_cpu_buff2, header_size);
  if (readlen != header_size)
  {
    LBOOT_ERROR("[CPU] fread(tmp_H_cpu2) failed: %d\n", readlen);
  }

  //NumRecords = (((CPU_Xheader_t*)tmp_H_cpu_buff2)->header1.NumOfRecords);
  //header_size = sizeof(VRL_Header_t) + (sizeof(VRL_RecordInfo_t) * NumRecords) + sizeof(VRL_Sign_t);

  H_block_cnt = (header_size/LTE_BOOT_SDIO_BLK_SIZE);
  H_remained_bytes = header_size - (H_block_cnt * LTE_BOOT_SDIO_BLK_SIZE);
///  LBOOT_INFO("[CPU] H_block_cnt = %d\n", H_block_cnt);
///  LBOOT_INFO("[CPU] H_remained_bytes = %d\n", H_remained_bytes);
  
  for(i =0; i < NumRecords; i++)
  {
    body_size[i] = (((CPU_Xheader_t*)tmp_H_cpu_buff2)->header2[i].LenInWords) * WordToByte;
    B_block_cnt[i] = (body_size[i]/LTE_BOOT_SDIO_BLK_SIZE);
    B_remained_bytes[i] = body_size[i] - (B_block_cnt[i] * LTE_BOOT_SDIO_BLK_SIZE);
///	LBOOT_INFO("body_size[%d]= %d,B_block_cnt[%d]= %d,B_remained_bytes[%d]= %d\n",i,body_size[i],i,B_block_cnt[i],i,B_remained_bytes[i]);
  }

  
  for(i=0; i<H_block_cnt; i++)
  {
    H_cpu_buff = kzalloc(LTE_BOOT_SDIO_BLK_SIZE, GFP_KERNEL);
    if (H_cpu_buff == NULL)
    {
      LBOOT_ERROR("[CPU] Failed to allocate memory for H_cpu_buff\n");
      return -ENOMEM;
    }

	fp_CPU->f_pos = 0;  //start offset
    readlen = lte_sdio_boot_fread(fp_CPU, H_cpu_buff, LTE_BOOT_SDIO_BLK_SIZE);
    if (readlen != LTE_BOOT_SDIO_BLK_SIZE)
    {
      LBOOT_ERROR("[CPU] fread(H_cpu) failed: %d\n", readlen);
    }
    H_cpu_rlen +=readlen;
    status = lte_sdio_transfer_image(H_cpu_buff, 1, 0); 
    // 13.If Error, Report error
    if (status != LTE_BOOT_NO_ERROR)
    {
      LBOOT_ERROR("[CPU] Transfer Main Image Header ERROR\n");
////    lte_sdio_write_gpr(GPR_0,LTE_BOOT_GPR_ROM_ERROR);
      sdio_claim_host(gLte_sdio_info->func);      
      sdio_writeb(gLte_sdio_info->func, LTE_BOOT_GPR_BLOADER_ERROR, GPR_0, &error);
      sdio_release_host(gLte_sdio_info->func);
      if (error) 
      {
        LBOOT_ERROR("[CPU] Failed to write GPR 0xA5\n");
        goto err;
      }
      return LTE_BOOT_ERROR_HDR_CPU;
    }
    kfree(H_cpu_buff);
  }
  H_cpu_buff = kzalloc(H_remained_bytes, GFP_KERNEL);
  if (H_cpu_buff == NULL)
  {
    LBOOT_ERROR("[CPU] Failed to allocate memory for H_cpu_buff\n");
    return -ENOMEM;
  }
  readlen = lte_sdio_boot_fread(fp_CPU, H_cpu_buff, H_remained_bytes);
  if (readlen != H_remained_bytes)
  {
    LBOOT_ERROR("[CPU] fread(H_cpu) failed: %d\n", readlen);
  }
  
  H_cpu_rlen += readlen; 
  LBOOT_INFO("[CPU] header_size/H_cpu_rlen=%d/%d\n", header_size, H_cpu_rlen);
  
  status = lte_sdio_transfer_image(H_cpu_buff, 0, H_remained_bytes);
////  status = lte_sdio_transfer_image(CPU_H_start_addr, H_block_cnt, H_remained_bytes); 

// 13.If Error, Report error
  if (status != LTE_BOOT_NO_ERROR)
    {
      LBOOT_ERROR("[CPU] Transfer Main Image Header ERROR\n");
////	  lte_sdio_write_gpr(GPR_0,LTE_BOOT_GPR_BLOADER_ERROR);
      sdio_claim_host(gLte_sdio_info->func);	
      sdio_writeb(gLte_sdio_info->func, LTE_BOOT_GPR_BLOADER_ERROR, GPR_0, &error);
      sdio_release_host(gLte_sdio_info->func);
      if (error) 
      {
        LBOOT_ERROR("[CPU] Failed to write GPR 0xA5\n");
        goto err;
      }
      return LTE_BOOT_ERROR_HDR_CPU;
    }

  kfree(H_cpu_buff);
  
  for(i =0; i <NumRecords; i++)
  {
// 14.If Success of transfer CPU Header or BL_Ready, Write GPR, BL_CPU_Start
////    lte_sdio_write_gpr(GPR_0,LTE_BOOT_GPR_BL_CPU_START);
    sdio_claim_host(gLte_sdio_info->func);	
    sdio_writeb(gLte_sdio_info->func, LTE_BOOT_GPR_BL_CPU_START, GPR_0, &error);
    sdio_release_host(gLte_sdio_info->func);
    if (error) 
    {
      LBOOT_ERROR("[CPU] Failed to write GPR 0x7D\n");
      goto err;
    }
    lte_gpr_value = 0;

// 15.Read GPR, LTE CPU_Start or not
     while ( lte_gpr_value != LTE_BOOT_GPR_BL_CPU_START)
    {
////      lte_sdio_read_gpr(GPR_2,&lte_gpr_value);
      sdio_claim_host(gLte_sdio_info->func);			
      lte_gpr_value = sdio_readb(gLte_sdio_info->func, GPR_2, &error);
      sdio_release_host(gLte_sdio_info->func);
      if (error) 
      {
        LBOOT_ERROR("[CPU] Failed to Read GPR 0x7D\n");
        goto err;
      }
      if( lte_gpr_value ==LTE_BOOT_GPR_BL_CPU_START)
        break;

      mdelay(1); // 1ms Unit

      if (time_cnt > 4000) // 4 seconds
        return LTE_BOOT_ERROR_CPU_START;

      time_cnt++;
    }

    time_cnt = 0;
  
// 16.If LTE BL_CPU_Start, Transfer CPU body by block unit
    for(j=0; j<B_block_cnt[i]; j++)
    {
      B_cpu_buff = kzalloc(LTE_BOOT_SDIO_BLK_SIZE, GFP_KERNEL);
      if (B_cpu_buff == NULL)
      {
        LBOOT_ERROR("[CPU] Failed to allocate memory for B_cpu_buff\n");
        return -ENOMEM;
      }

      readlen = lte_sdio_boot_fread(fp_CPU, B_cpu_buff, LTE_BOOT_SDIO_BLK_SIZE);
      if (readlen != LTE_BOOT_SDIO_BLK_SIZE)
      {
        LBOOT_ERROR("[CPU] fread(B_cpu) failed: %d\n", readlen);
      }
      B_cpu_rlen +=readlen;
      status = lte_sdio_transfer_image(B_cpu_buff, 1, 0); 
      // 17.If Error, Report error
      if (status != LTE_BOOT_NO_ERROR)
      {
        LBOOT_ERROR("[CPU] Transfer CPU Body ERROR, Record Number = %d \n", i);
////    lte_sdio_write_gpr(GPR_0,LTE_BOOT_GPR_ROM_ERROR);
        sdio_claim_host(gLte_sdio_info->func);      
        sdio_writeb(gLte_sdio_info->func, LTE_BOOT_GPR_BLOADER_ERROR, GPR_0, &error);
        sdio_release_host(gLte_sdio_info->func);
        if (error) 
        {
          LBOOT_ERROR("[CPU] Failed to write GPR 0xA5\n");
          goto err;
        }
        return LTE_BOOT_ERROR_CPU;
      }
      kfree(B_cpu_buff);
    }
    B_cpu_buff = kzalloc(B_remained_bytes[i], GFP_KERNEL);
    if (B_cpu_buff == NULL)
    {
      LBOOT_ERROR("[CPU] Failed to allocate memory for B_cpu_buff[i]\n", i);
      return -ENOMEM;
    }
    readlen = lte_sdio_boot_fread(fp_CPU, B_cpu_buff, B_remained_bytes[i]);
    if (readlen != B_remained_bytes[i])
    {
      LBOOT_ERROR("[CPU] fread(B_cpu) failed: %d\n", readlen);
    }
  
    B_cpu_rlen += readlen; 
    LBOOT_INFO("[CPU] body_size[%d]/B_cpu_rlen=%d/%d\n", i, body_size[i], B_cpu_rlen);
  
    status = lte_sdio_transfer_image(B_cpu_buff, 0, B_remained_bytes[i]);

////    status = lte_sdio_transfer_image(CPU_B_start_addr[i], B_block_cnt[i], B_remained_bytes[i]); 

// 17.If Error, Report error
    if (status != LTE_BOOT_NO_ERROR)
    {
      LBOOT_ERROR("[CPU] Transfer CPU Body ERROR, Record Number = %d \n", i);
////      lte_sdio_write_gpr(GPR_0,LTE_BOOT_GPR_BLOADER_ERROR);
      sdio_claim_host(gLte_sdio_info->func);	
      sdio_writeb(gLte_sdio_info->func, LTE_BOOT_GPR_BLOADER_ERROR, GPR_0, &error);
      sdio_release_host(gLte_sdio_info->func);
      if (error) 
      {
        LBOOT_ERROR("[CPU] Failed to write GPR 0xA5\n");
        goto err;
      }
      return LTE_BOOT_ERROR_CPU;
    }
	
	B_cpu_rlen =0;
	kfree(B_cpu_buff);
    
// 18.If Success of transfer CPU Body, Write GPR, BL_Ready
////	lte_sdio_write_gpr(GPR_0,LTE_BOOT_GPR_BL_READY);
    sdio_claim_host(gLte_sdio_info->func);	
    sdio_writeb(gLte_sdio_info->func, LTE_BOOT_GPR_BL_READY, GPR_0, &error);
    sdio_release_host(gLte_sdio_info->func);
    if (error) 
    {
      LBOOT_ERROR("[CPU] Failed to write GPR 0x7B\n");
      goto err;
    }
  	lte_gpr_value = 0;

// 19.Read GPR, LTE BL_Ready or not
    while ( lte_gpr_value != LTE_BOOT_GPR_BL_READY)
    {
////      lte_sdio_read_gpr(GPR_2,&lte_gpr_value);
      sdio_claim_host(gLte_sdio_info->func);			
      lte_gpr_value = sdio_readb(gLte_sdio_info->func, GPR_2, &error);
      sdio_release_host(gLte_sdio_info->func);
      if (error) 
      {
        LBOOT_ERROR("[CPU] Failed to Read GPR 0x7B\n");
        goto err;
      }
      if( lte_gpr_value ==LTE_BOOT_GPR_BL_READY)
        break;
  
      mdelay(1); // 1ms Unit
  
      if (time_cnt > 4000) // 4 seconds
        return LTE_BOOT_ERROR_READY;
     time_cnt++;
    }
  
    time_cnt = 0;
  
  }
  // We should use free(buffer)
  kfree(tmp_H_cpu_buff1);
  kfree(tmp_H_cpu_buff2);
  //kfree(B_cpu_buff);
  filp_close(fp_CPU, NULL);
// 20.If CPU body done, Check Send image count is lower than Total image count
// 21.If Lower, Return Write GPR, HDR_Start
// 22.IF Same, Write GPR, Done
////  lte_sdio_write_gpr(GPR_0,LTE_BOOT_GPR_BL_DONE);
  sdio_claim_host(gLte_sdio_info->func);	
  sdio_writeb(gLte_sdio_info->func, LTE_BOOT_GPR_BL_DONE, GPR_0, &error);
  sdio_release_host(gLte_sdio_info->func);
  if (error) 
  {
    LBOOT_ERROR("[CPU] Failed to write GPR 0x7E\n");
    goto err;
  }
  
  LBOOT_INFO("CPU Download Complete !! \n");	
  
	return LTE_BOOT_NO_ERROR;
err:
	FUNC_EXIT();
	return error;
  
}

#ifdef LG_FW_LTE_COMPILE_ERROR
/*BEGIN: 0014137 daeok.kim@lge.com 2011-01-15 */
/*ADD 0014137: [LTE] LTE USB enable/disable according to USB cable (130K/ 56K/ 910K LT, user cable) */
extern int udc_cable;
/*END: 0014137 daeok.kim@lge.com 2011-01-15 */
#endif /* LG_FW_LTE_COMPILE_ERROR */

LTE_BOOT_STATUS	lte_sdio_boot(void)
{
	int error = 0, i =0;
	unsigned char       lte_gpr_value = 0;
	unsigned short		time_cnt = 0;
	LTE_BOOT_STATUS		status =0;
	
/*BEGIN: 0013724 daeok.kim@lge.com 2011-01-08 */
/*ADD 0013724: [LTE] LTE HW Watchdog Timer expired handling is added */
#ifdef LTE_HW_WDT_RST_TEST
  /* Initial setting of LTE HW Wachdog Interrupt GPIO*/
  gpio_request(GPIO_LTE_HW_WDT_RST, "LTE_HW_WD_RST");
  gpio_tlmm_config(GPIO_CFG(GPIO_LTE_HW_WDT_RST, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
  msleep(10);
  gpio_direction_input(GPIO_LTE_HW_WDT_RST);
    printk("[LTE_HW_WDT] GPIO_2 value = %d before LTE Booting \n", gpio_get_value(GPIO_LTE_HW_WDT_RST));
#endif
/*END: 0013724 daeok.kim@lge.com 2011-01-08 */

  // PBC transfer
  status = lte_sdio_boot_PBC();
  if ( status != LTE_BOOT_NO_ERROR)
  { 	
  	LBOOT_ERROR("lte_sdio_boot_PBC FAILED!!");
	LBOOT_ERROR("Error Number = %d", status);
  	return LTE_BOOT_ERROR_PBC;
  }

  // CPU transfer
  status = lte_sdio_boot_main_image();
  if ( status != LTE_BOOT_NO_ERROR)
  {  	
  	LBOOT_ERROR("lte_sdio_boot_main_image FAILED!!");
	LBOOT_ERROR("Error Number = %d", status);
	return LTE_BOOT_ERROR_CPU;
  }

  // IF LTE HIM/SDIO INIT Done, Write HOST HIM/SDIO INIT Done (0xCF)   
////  lte_sdio_write_gpr(GPR_1,LTE_BOOT_GPR_HOST_HIM_SDIO_DONE);
  sdio_claim_host(gLte_sdio_info->func);	
  sdio_writeb(gLte_sdio_info->func, LTE_BOOT_GPR_HOST_HIM_SDIO_DONE, GPR_1, &error);
  sdio_release_host(gLte_sdio_info->func);
  if (error) 
  {
    LBOOT_ERROR("[HOST] Failed to write GPR 0xCF\n");
  }

  LBOOT_INFO("HOST HIM/SDIO INIT Done (0xCF)!! \n");
  
  // Wait LTE HIM/SDIO INIT Done (0x30) 
  while ( lte_gpr_value != LTE_BOOT_GPR_HIM_SDIO_DONE)
  {
////    lte_sdio_read_gpr(GPR_3,&lte_gpr_value);
    sdio_claim_host(gLte_sdio_info->func);			
    lte_gpr_value = sdio_readb(gLte_sdio_info->func, GPR_3, &error);
    sdio_release_host(gLte_sdio_info->func);
    if (error) 
    {
      LBOOT_ERROR("[LTE]Failed to Read GPR 0x30\n");
    }
    if( lte_gpr_value == LTE_BOOT_GPR_HIM_SDIO_DONE)
      break;
  
    mdelay(1); // 1ms Unit
  }

  LBOOT_INFO("LTE HIM_SDIO INIT DONE (0x30)!!Good!!\n");


#ifdef LG_FW_LTE_COMPILE_ERROR
/*BEGIN: 0014137 daeok.kim@lge.com 2011-01-15 */
/*ADD 0014137: [LTE] LTE USB enable/disable according to USB cable (130K/ 56K/ 910K LT, user cable) */
  // Decision of LTE USB connection in Factory process
  /* USB cable detection API & Write GPR*/
  sdio_claim_host(gLte_sdio_info->func);	
  if (udc_cable ==4) //130K LT: LTE USB enable, 0x19
  {
	  sdio_writeb(gLte_sdio_info->func, LTE_BOOT_USB_ENABLE, GPR_0, &error);
	  LBOOT_INFO("[LTE] Write LTE_BOOT_USB_ENABLE (0x19) \n");
  }
  else if ((udc_cable ==3) || (udc_cable ==5)) // 56K/910K LT: LTE USB disable, 0x18
  {
	  sdio_writeb(gLte_sdio_info->func, LTE_BOOT_USB_DISABLE, GPR_0, &error);
	  LBOOT_INFO("[LTE] Write LTE_BOOT_USB_DISABLE (0x18) \n");
  }
  else // According to NV config, LTE USB enable/disable, 0x1B
  {
	  sdio_writeb(gLte_sdio_info->func, LTE_BOOT_USB_ENABLE_BY_NV_CONFIG, GPR_0, &error);
	  LBOOT_INFO("[LTE] Write LTE_BOOT_USB_ENABLE_BY_NV_CONFIG (0x1B) \n");
  }
  sdio_release_host(gLte_sdio_info->func);
/*END: 0014137 daeok.kim@lge.com 2011-01-15 */
#else
	sdio_claim_host(gLte_sdio_info->func);	
	sdio_writeb(gLte_sdio_info->func, LTE_BOOT_USB_ENABLE, GPR_0, &error);
	LBOOT_INFO("[LTE] Write LTE_BOOT_USB_ENABLE (0x19) \n");
	sdio_release_host(gLte_sdio_info->func);
#endif /* LG_FW_LTE_COMPILE_ERROR */

/*BEGIN: 0013724 daeok.kim@lge.com 2011-01-08 */
/*ADD 0013724: [LTE] LTE HW Watchdog Timer expired handling is added */
#ifdef LTE_HW_WDT_RST_TEST
	/* Registeration lte hw wdt interrupt and disable irq */
	for(i=0; i<200; i++)
	{
		if(gpio_get_value(GPIO_LTE_HW_WDT_RST) ==1)
		{
			printk("[LTE_HW_WDT] GPIO 2 value = %d, %d ms after LTE Booting \n", gpio_get_value(GPIO_LTE_HW_WDT_RST), (10*i));
			printk("[LTE_HW_WDT] IRQ Registration and enable\n");
			printk("[LTE_HW_WDT] IRQ MSM#2-LTE#9, Falling edge trigger \n");
			lte_reg_intr_hw_wdt();
			break;
		}
		mdelay(10);	
		printk("[LTE_HW_WDT] GPIO_2 value = %d - GPIO is not drived HIgh value \n",gpio_get_value(GPIO_LTE_HW_WDT_RST));
	}
#endif
/*END: 0013724 daeok.kim@lge.com 2011-01-08 */

// set event of LTE Boot terminate!!!!!!!!!!!!
//??  UtaOsEventGroupSet(&whim_evt, LTE_WHIM_SDIO_DONE_EVT, UTA_OS_EVENTGROUP_SET_OR);
//??  UtaOsThreadTerminate(&g_lte_boot_thread);
  

	return	LTE_BOOT_NO_ERROR;
}

/*BEGIN: 0013724 daeok.kim@lge.com 2011-01-08 */
/*ADD 0013724: [LTE] LTE HW Watchdog Timer expired handling is added */
#ifdef LTE_HW_WDT_RST_TEST
#if 0
static irqreturn_t lte_cb_fcn_rising(int irq, void *dev_id)
{
	int ret =0;
	disable_irq(gIrq_lte_hw_wdt);
	free_irq(gIrq_lte_hw_wdt, NULL);
	ret = request_irq(gIrq_lte_hw_wdt, lte_cb_fcn_hw_wdt, IRQF_TRIGGER_FALLING, "LTE_HW_WD_RST", NULL);
	return IRQ_HANDLED;
}
#endif

static irqreturn_t lte_cb_fcn_hw_wdt(int irq, void *dev_id)
{
#ifdef LG_FW_COMPILE_ERROR
	unsigned char *error_log_lte_hw_wdt;

	/* 1st IRQ Skip process: WORK AROUND*/
	if(gpio_get_value(GPIO_LTE_HW_WDT_RST) ==1)
	{
		printk("[LTE_HW_WDT] Skip 1st IRQ!!, GPIO_2 value = %d \n",gpio_get_value(2));
		return IRQ_HANDLED;
	}

/* BEGIN: 0018470 jaegyu.lee@lge.com 2011-03-23 */
/* MOD 0018470 : [LTE] LTE ASSERT CASE report available in SDIO not working */
	if(gpio_get_value(GPIO_L2K_LTE_STATUS) ==1)
	{
		error_log_lte_hw_wdt = kzalloc(LTE_WDT_LOG_SIZE, GFP_KERNEL);
		error_log_lte_hw_wdt = "LTE assert! But receiving asset log has failed!\n";

		printk("[LTE_ASSERT] LTE assert! But receiving assert log has failed!\n");
	}
	else
	{
		error_log_lte_hw_wdt = kzalloc(LTE_WDT_LOG_SIZE, GFP_KERNEL); // LTE_WDT_LOG_SIZE =50
		error_log_lte_hw_wdt = "LTE HW WDT RST is activated!!"; // You can loot at this message in data/lte_ers_panic
		
		printk("[LTE_HW_WDT] GPIO_2 value = %d in in INTR CB FCN \n",gpio_get_value(2));
		printk("[LTE_HW_WDT] IRQ Detecting & Device RESET Activation \n");
		printk("[LTE_HW_WDT] Look at data/lte_ers_panic \n");
	}
/* END: 0018470 jaegyu.lee@lge.com 2011-03-23 */
	/* Device reset activation*/
	lte_crash_log(error_log_lte_hw_wdt, LTE_WDT_LOG_SIZE, Reserved_LTE_WDT_RST);
	kfree(error_log_lte_hw_wdt);
	return IRQ_HANDLED;
#else
#if defined (LTE_HW_WDT_RST_TEST) && defined (CONFIG_LGE_LTE_ERS)
	unsigned char *error_log_lte_hw_wdt;
#endif
	/* 1st IRQ Skip process: WORK AROUND*/
	if(gpio_get_value(GPIO_LTE_HW_WDT_RST) ==1)
	{
		printk("[LTE_HW_WDT] Skip 1st IRQ!!, GPIO_2 value = %d \n",gpio_get_value(2));
		return IRQ_HANDLED;
	}

/* BEGIN: 0018470 jaegyu.lee@lge.com 2011-03-23 */
/* MOD 0018470 : [LTE] LTE ASSERT CASE report available in SDIO not working */
	if(gpio_get_value(GPIO_L2K_LTE_STATUS) ==1)
	{
#if defined (LTE_HW_WDT_RST_TEST) && defined (CONFIG_LGE_LTE_ERS)
		error_log_lte_hw_wdt = kzalloc(LTE_WDT_LOG_SIZE, GFP_KERNEL);
		error_log_lte_hw_wdt = "LTE assert! But receiving asset log has failed!\n";
#endif

		printk("[LTE_ASSERT] LTE assert! But receiving assert log has failed!\n");
	}
	else
	{		
#if defined (LTE_HW_WDT_RST_TEST) && defined (CONFIG_LGE_LTE_ERS)
		error_log_lte_hw_wdt = kzalloc(LTE_WDT_LOG_SIZE, GFP_KERNEL); // LTE_WDT_LOG_SIZE =50
		error_log_lte_hw_wdt = "LTE HW WDT RST is activated!!"; // You can loot at this message in data/lte_ers_panic
#endif
		
		printk("[LTE_HW_WDT] GPIO_2 value = %d in in INTR CB FCN \n",gpio_get_value(2));
		printk("[LTE_HW_WDT] IRQ Detecting & Device RESET Activation \n");
		printk("[LTE_HW_WDT] Look at data/lte_ers_panic \n");
	}
/* END: 0018470 jaegyu.lee@lge.com 2011-03-23 */
	/* Device reset activation*/
#if defined (LTE_HW_WDT_RST_TEST) && defined (CONFIG_LGE_LTE_ERS)
	lte_crash_log(error_log_lte_hw_wdt, LTE_WDT_LOG_SIZE, Reserved_LTE_WDT_RST);
	kfree(error_log_lte_hw_wdt);
#endif
	return IRQ_HANDLED;
#endif /* LG_FW_COMPILE_ERROR */
}

static void lte_reg_intr_hw_wdt(void)
{
	int ret =0;
	/*BEGIN: 0015094 daeok.kim@lge.com 2011-01-31 */
	/*ADD: 0015094: [LTE] To resolve INTR handler initial callinig problem in case of the GPIO_2, GPIO_2's status register should be cleared */
	void __iomem * ptr;

	/* GPIO_2 should be cleared by setting '1' to GPIO_INT_CLEAR register*/
	set_irq_type(MSM_GPIO_TO_INT(GPIO_LTE_HW_WDT_RST), IRQF_TRIGGER_FALLING); /* GPIO status register is cleared when '1' is set to the GPIO clear register and the GPIO detection register is set to edge(falling or rising) */
	ptr = ioremap(0xAC001000, 0x1000); /* return the virtual address of GPIO1 Shadow 2 Base address */ 
	writel(0x4, ptr + 0x90); /* writing '1' to the bit 2 of GPI_INT_CLEAR_0 register that is correspond to GPIO 2 */ 
	iounmap(ptr);
	/*END: 0015094 daeok.kim@lge.com 2011-01-31 */
	
	/* Registration of ISR*/
	gIrq_lte_hw_wdt = gpio_to_irq(GPIO_LTE_HW_WDT_RST);
	ret = request_irq(gIrq_lte_hw_wdt, lte_cb_fcn_hw_wdt, IRQF_TRIGGER_FALLING, "LTE_HW_WD_RST", NULL);
	if(ret < 0) 
	{
	    printk("[LTE_HW_WDT] Failed to request LTE HW WDT interrupt handler\n");
	    return -1;  // error occurs when initializing
	}

	#if 0
 	/* Control irq power management wakeup */	
    ret = set_irq_wake(irq_rx, 1);
	if(ret < 0) 
	{
	    printk("Failed to wakeup rx_interrupt handler\n");
	    return -1;  // error occurs when initializing
	}        
	#endif
	/* Disable of lte hw wdt rst irq*/
	//disable_irq(gIrq_lte_hw_wdt);

	return 0;
}
#endif
/*END: 0013724 daeok.kim@lge.com 2011-01-08 */
