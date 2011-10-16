#ifndef LTE_BOOT_H
#define LTE_BOOT_H

/*===========================================================================
											lte_boot.h

DESCRIPTION

	 This file contains LTE boot image tranfer operation.

===========================================================================*/
/*===========================================================================
	
								 EDIT HISTORY FOR FILE
								 
	when			   who			   what, where, why
	---------- 	 ---------	 ---------------------------------------------------------- 
	2010/05/28	 Daeok Kim   Creation.
===========================================================================*/
////#include "uta_os.h"
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/delay.h>

#include "lte_sdio.h"

#include <asm/uaccess.h>
#include <asm/io.h>

/*BEGIN: 0013724 daeok.kim@lge.com 2011-01-08 */
/*ADD 0013724: [LTE] LTE HW Watchdog Timer expired handling is added */
#define LTE_HW_WDT_RST_TEST
#ifdef LTE_HW_WDT_RST_TEST
#define GPIO_LTE_HW_WDT_RST 2
/* BEGIN: 0018470 jaegyu.lee@lge.com 2011-03-23 */
/* MOD 0018470 : [LTE] LTE ASSERT CASE report available in SDIO not working */
#define LTE_WDT_LOG_SIZE 50
/* END: 0018470 jaegyu.lee@lge.com 2011-03-23 */
#define Reserved_LTE_WDT_RST 0x0101
#include <asm/irq.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <mach/gpio.h>
static irqreturn_t lte_cb_fcn_hw_wdt(int irq, void *dev_id);
static void lte_reg_intr_hw_wdt(void);
#endif
/*END: 0013724 daeok.kim@lge.com 2011-01-08 */

////#define LTE_BOOT_PBC_ADDR 0x61F00000
////#define LTE_BOOT_CPU_ADDR 0x62000000

#define LTE_BOOT_SDIO_BLK_SIZE 512
#define LTE_BOOT_CTLBL_SIZE 512

typedef enum {
	 LTE_SDIO_SUCCESS = 0,
	 LTE_SDIO_INVALID_PARM
}LTE_SDIO_STATUS;

typedef enum {
	LTE_BOOT_NO_ERROR = 0,
  	LTE_BOOT_ERROR_INIT,		// 1
    LTE_BOOT_ERROR_READY,		// 2
    LTE_BOOT_ERROR_CTLBL, 		// 3
  	LTE_BOOT_ERROR_HDR_START, 	// 4
  	LTE_BOOT_ERROR_HDR_PBC, 	// 5
  	LTE_BOOT_ERROR_PBC_START,	// 6 
  	LTE_BOOT_ERROR_PBC,			// 7
  	LTE_BOOT_ERROR_PBC_DONE,	// 8
  	LTE_BOOT_ERROR_PRE_HDR,		// 9
  	LTE_BOOT_ERROR_HDR_CPU,		// 10
  	LTE_BOOT_ERROR_CPU_START,	// 11
  	LTE_BOOT_ERROR_CPU,			// 12
  	LTE_BOOT_ERROR_HIM_SDIO_DONE// 13
}LTE_BOOT_STATUS;

typedef enum{
	LTE_BOOT_GPR_INITIALIZED    = 0x5A,
	LTE_BOOT_GPR_READY          = 0x5B,
	LTE_BOOT_GPR_CTLBL     		= 0x5C,
	LTE_BOOT_GPR_HDR_START      = 0x5D,
	LTE_BOOT_GPR_PBC_START      = 0x5E,
	LTE_BOOT_GPR_DONE           = 0x5F,
	LTE_BOOT_GPR_ROM_ERROR      = 0xA5,
}S_LTE_BOOT_GPR_STATUS_VALUE;

typedef enum{
	LTE_BOOT_GPR_BL_INITIALIZED    = 0x7A,
	LTE_BOOT_GPR_BL_READY          = 0x7B,
	LTE_BOOT_GPR_BL_HDR_START      = 0x7C,
	LTE_BOOT_GPR_BL_CPU_START      = 0x7D,
	LTE_BOOT_GPR_BL_DONE           = 0x7E,
	LTE_BOOT_GPR_BL_PRE_HDR        = 0x85,
	LTE_BOOT_GPR_BLOADER_ERROR     = 0xA5,	
	LTE_BOOT_GPR_HOST_HIM_SDIO_DONE= 0xCF,
	LTE_BOOT_GPR_HIM_SDIO_DONE	   = 0x30
}S_LTE_BOOT_GPR_STATUS_BL_VALUE;

/*BEGIN: 0014137 daeok.kim@lge.com 2011-01-15 */
/*ADD 0014137: [LTE] LTE USB enable/disable according to USB cable (130K/ 56K/ 910K LT, user cable) */
typedef enum{
    LTE_BOOT_USB_DISABLE   			= 0x18,
    LTE_BOOT_USB_ENABLE   			= 0x19,
    LTE_BOOT_USB_ENABLE_BY_NV_CONFIG= 0x1B,
}S_LTE_BOOT_GPR_STATUS_USB_VALUE;
/*END: 0014137 daeok.kim@lge.com 2011-01-15 */

#define  CPU_IMG_MAX_RECORDS  10

typedef unsigned long HASH_Rst_t[8];

/* 1 . the VRL structure */
typedef struct 
{
	unsigned long 	MagicNumber;		//magic
	unsigned long	LenInWords;			//header length
	unsigned long	SecondaryVrlPresent;
	unsigned long	SecondaryVrlAddr;
	unsigned long	MemCopySizeInBytes;	//image size
	unsigned long	NumOfRecords;

}VRL_Header_t;
   
/* 2. the record structure */
typedef struct
{
	unsigned long	Addr;
	unsigned long	LenInWords;
	unsigned long	CheckSwOnWakeUpMode;
	HASH_Rst_t	 SwHashResult;  
}VRL_RecordInfo_t; 

/* 3. secondary info on the primary structure */
/* 4. the signature */
typedef struct
{
	unsigned long	H[64];
	unsigned long	N[64];
}VRL_Sign_t;

typedef struct
{
	VRL_Header_t		header1;
	VRL_RecordInfo_t	header2;
	VRL_Sign_t		unused;
}PBC_Xheader_t;


typedef struct
{
	VRL_Header_t		header1;
	VRL_RecordInfo_t	header2[CPU_IMG_MAX_RECORDS];
	VRL_Sign_t			unused;
}CPU_Xheader_t;



LTE_BOOT_STATUS	lte_sdio_boot(void);


#endif //LTE_BOOT_H

