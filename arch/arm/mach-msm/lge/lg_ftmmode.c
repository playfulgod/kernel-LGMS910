//
// lg_ftmmode.c
//

#include <linux/kernel.h>
#include "../proc_comm.h"

// BEGIN 0012299: eundeok.bae@lge.com 2010-12-13 FTM MODE RESET
// [KERNEL] ADDED "FTM BOOT RESET" Function For Test Mode
#include <mach/lg_comdef.h>
// END 0012299: eundeok.bae@lge.com 2010-12-13

// BEGIN 0011665: eundeok.bae@lge.com FTM MODE FOR ONLY KERNEL BOOTING
// [KERNEL] Added source code For Sleep Mode Test, Test Mode V8.3 [250-42] 
// LGE_CHANGE [dojip.kim@lge.com] 2010-09-28, FTM boot
void remote_set_ftm_boot(uint32 info)
{
  uint32 onoff = info;
  msm_proc_comm(PCOM_OEM_SET_FTM_MODE_CMD, &onoff, NULL);		
  printk("##MSM_PROC_COMM, REMOTE_SET_FTM_BOOT[%d]##\n", onoff); 
}
EXPORT_SYMBOL(remote_set_ftm_boot);

void remote_get_ftm_boot(uint32 *info)
{
  uint32 *ftmboot = info;
  msm_proc_comm(PCOM_OEM_GET_FTM_MODE_CMD, ftmboot, NULL);		
  printk("##MSM_PROC_COMM, REMOTE_GET_FTM_BOOT[%d]##\n", *ftmboot); 
}
EXPORT_SYMBOL(remote_get_ftm_boot);

void remote_set_operation_mode(uint32 info)
{
  uint32 onoff = info;
  msm_proc_comm(PCOM_OEM_SET_OPERATION_MODE_CMD, &onoff, NULL);		
  printk("##MSM_PROC_COMM, REMOTE_SET_OPERATION_MODE[%d]##\n", onoff); 
}
EXPORT_SYMBOL(remote_set_operation_mode);
// END 0011665: eundeok.bae@lge.com FTM MODE FOR ONLY KERNEL BOOTING

// BEGIN 0012299: eundeok.bae@lge.com 2010-12-13 FTM MODE RESET
// [KERNEL] ADDED "FTM BOOT RESET" Function For Test Mode
void remote_set_ftmboot_reset(uint32 info)
{
  uint32 reset_on = info;
  msm_proc_comm(PCOM_OEM_SET_FTMBOOT_RESET_CMD, &reset_on, NULL);		
  printk("##MSM_PROC_COMM, PCOM_OEM_SET_FTMBOOT_RESET_CMD[%d]##\n", reset_on); 
}
EXPORT_SYMBOL(remote_set_ftmboot_reset);
// END 0012299: eundeok.bae@lge.com 2010-12-13