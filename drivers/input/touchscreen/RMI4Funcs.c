// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ 
// Copyright ?2008 Synaptics Incorporated. All rights reserved. 
// 
// The information in this file is confidential under the terms 
// of a non-disclosure agreement with Synaptics and is provided 
// AS IS. 
// 
// The information in this file shall remain the exclusive property 
// of Synaptics and may be the subject of Synaptics?patents, in 
// whole or part. Synaptics?intellectual property rights in the 
// information in this file are not expressly or implicitly licensed 
// or otherwise transferred to you as a result of such information 
// being made available to you. 
//
// $RCSfile: RMI4Funcs.cpp,v $
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#include <linux/delay.h>
#include <linux/module.h>
#include <asm/gpio.h>

#include "RMI4Funcs.h"
#include "SynaImage.h"  // this contains the firmware to download into the sensor
#include "SynaImage_05.h"  // this contains the firmware to download into the sensor
#include "SynaImage_06.h"  // this contains the firmware to download into the sensor
#include "SynaImage_07.h"  // this contains the firmware to download into the sensor
#include "SynaImage_09.h"  // this contains the firmware to download into the sensor

//extern void   SynaSleep(DWORD mSecs);
//extern int touch_i2c_read(u8 addr);
//extern s8 touch_i2c_write(u8 addr, u8 value);

//#define TOUCH_FD_DEBUG_PRINT (1)
#define TOUCH_FD_ERROR_PRINT (1)

#if defined(TOUCH_FD_DEBUG_PRINT)
#define TOUCH_FDD(fmt, args...) \
	printk(KERN_INFO "D[%-18s:%5d]" \
			fmt, __FUNCTION__, __LINE__, ##args);
#else
#define TOUCH_FDD(fmt, args...)    {};
#endif

#if defined(TOUCH_FD_ERROR_PRINT)
#define TOUCH_FDE(fmt, args...) \
	printk(KERN_ERR "E[%-18s:%5d]" \
			fmt, __FUNCTION__, __LINE__, ##args);
#else
#define TOUCH_FDE(fmt, args...)    {};
#endif
 
extern bool SynaWaitForATTN(int dwMilliseconds);

extern int SynaWriteRegister(u8  uRmiAddress, 
                                u8 * data,
                                unsigned int    length);

extern int SynaReadRegister(u8  uRmiAddress, //register address
                               u8 * data,
                               unsigned int    length); 

#define EError u8
#define I2CAddr7Bit 0x20

void RMI4CheckIfFatalError(EError errCode)
{
	  if(errCode != ESuccess)
	  {
	    TOUCH_FDD("\nFatal error: ");
	    TOUCH_FDD("%d\n", errCode);

//	    Exit();
	  }
}

void SpecialCopyEndianAgnostic(u8 *dest, u16 src) 
{
  dest[0] = src%0x100;  //Endian agnostic method
  dest[1] = src/0x100;  
}

void RMI4FuncsConstructor(void)
{
  // Initialize data members
  m_BaseAddresses.m_QueryBase = 0xff;
  m_BaseAddresses.m_CommandBase = 0xff; 
  m_BaseAddresses.m_ControlBase = 0xff;
  m_BaseAddresses.m_DataBase = 0xff;
  m_BaseAddresses.m_IntSourceCount = 0xff;
  m_BaseAddresses.m_ID = 0xff;

  m_uQuery_Base = 0xffff;
  m_bAttenAsserted = false;
  m_bFlashProgOnStartup = false;
  m_bUnconfigured = false;
  g_ConfigDataList[0x2d - 4].m_Address = 0x0000;
  g_ConfigDataList[0x2d - 4].m_Data = 0x00;
  g_ConfigDataCount = sizeof(g_ConfigDataList) / sizeof(g_ConfigDataBlock);

  m_firmwareImgData = (u8*)NULL;
  m_configImgData = (u8*)NULL;
}


void RMI4Init(void)
{

  // Set up blockSize and blockCount for UI and config
  RMI4ReadConfigInfo();
  RMI4ReadFirmwareInfo();

  // Allocate arrays
  m_firmwareImgData = &FirmwareImage[0];  // new u8 [GetFirmwareSize()];
  m_configImgData = &ConfigImage[0];      // new u8 [GetConfigSize()];
} 

u16 GetConfigSize(void)
{ 
  return m_configBlockSize*m_configBlockCount;
}

u16 GetFirmwareSize(void)
{
  return m_firmwareBlockSize*m_firmwareBlockCount;
}

// Read Bootloader ID from Block Data Registers as a 'key value'
EError RMI4ReadBootloadID(void)
{
  u8 uData[3];
  int state;
  m_ret = SynaReadRegister(m_uF34ReflashQuery_BootID, (u8 *)uData, 2);
  
  state = gpio_get_value(107);
  TOUCH_FDD("state: %d\n", state);

  TOUCH_FDD("reg:0x%x,data[0]:0x%x,data[1]:0x%x\n",m_uF34ReflashQuery_BootID,uData[0],uData[1]);

  m_BootloadID = (unsigned int)uData[0] + (unsigned int)uData[1]*0x100;

  return m_ret;
}


EError RMI4IssueEnableFlashCommand(void)
{
	u8 buf[2] ={0};

	buf[0] = s_uF34ReflashCmd_Enable;
    TOUCH_FDD("%s %d ->buf[0]:0x%x\n ",__FUNCTION__, __LINE__,  buf[0]);
	TOUCH_FDD("reg= 0x%x\n", m_uF34Reflash_FlashControl);
  m_ret = SynaWriteRegister(m_uF34Reflash_FlashControl, (u8 *)&s_uF34ReflashCmd_Enable, 1);
  buf[0] = 0xff;
  buf[1] = 0xff;
  m_ret = SynaReadRegister(m_uF34Reflash_FlashControl, &buf, 1);
  TOUCH_FDD("%s %d ->buf[0]:0x%x\n ",__FUNCTION__, __LINE__,  buf[0]);
  return m_ret;
}


EError RMI4WriteBootloadID(void)
{
  u8 uData[2];
  u8 buf[3]={0};
  SpecialCopyEndianAgnostic(uData, m_BootloadID);

  TOUCH_FDD("\n ey.cho -> 0x%x: udata[0]=0x%x, udata[1]=0x%x\n", m_uF34Reflash_BlockData, uData[0], uData[1]);

  m_ret = SynaWriteRegister(m_uF34Reflash_BlockData, &uData[0], 2);

  SynaReadRegister(m_uF34Reflash_BlockData, &buf[0], 2);
  TOUCH_FDD("\n ey.cho --> 0x%x: buf[0]=0x%x, buf[1]=0x%x\n", m_uF34Reflash_BlockData, buf[0], buf[1]);

  return m_ret;
}

// Wait for ATTN assertion and see if it's idle and flash enabled
void RMI4WaitATTN(int errorCount)
{
  int uErrorCount = 0;

  // To work around the physical address error from Control Bridge
  if (SynaWaitForATTN(1000))   
  {
    RMI4CheckIfFatalError(EErrorTimeout);
    //TOUCH_FDD("Touch FD: %s(%d)\n", __FUNCTION__, __LINE__);
  }

    //TOUCH_FDD("Touch FD: %s(%d)\n", __FUNCTION__, __LINE__);
  do {
    m_ret = SynaReadRegister(m_uF34Reflash_FlashControl, &m_uPageData[0], 1);

    //m_uPageData[0] = touch_i2c_read(m_uF34Reflash_FlashControl);
    // To work around the physical address error from control bridge
    // The default check count value is 3. But the value is larger for erase condition
	//TOUCH_FDD("%d\n", m_ret);
    if((m_ret != ESuccess) && uErrorCount < errorCount)
    {
      uErrorCount++;
      m_uPageData[0] = 0;
      continue;
    }

    RMI4CheckIfFatalError(m_ret);
    
    // Clear the attention assertion by reading the interrupt status register
    m_ret = SynaReadRegister(m_PdtF01Common.m_DataBase + 1, &m_uStatus, 1);

    RMI4CheckIfFatalError(m_ret);
 // TOUCH_FDD("0x%x,0x%x,%d,%d\n", m_uPageData[0], s_uF34ReflashCmd_NormalResult, uErrorCount, errorCount); 
  } while( m_uPageData[0] != s_uF34ReflashCmd_NormalResult && uErrorCount < errorCount); 
 //   TOUCH_FDD("Touch FD: %s(%d)\n", __FUNCTION__, __LINE__);
}


// Enable Flashing programming
void RMI4EnableFlashing(void)
{
  u8 uData[2]={0};
  int uErrorCount = 0;

  // Read bootload ID
  m_ret = RMI4ReadBootloadID();
  RMI4CheckIfFatalError(m_ret);
  TOUCH_FDD("Touch FD: %s(%d)\n", __FUNCTION__, __LINE__);

  // Write bootID to block data registers  
  m_ret = RMI4WriteBootloadID();
  RMI4CheckIfFatalError(m_ret);

  TOUCH_FDD("Touch FD: %s(%d)\n", __FUNCTION__, __LINE__);
  do {
    m_ret = SynaReadRegister(m_uF34Reflash_FlashControl, &m_uPageData[0], 1);

    // To deal with ASIC physic address error from cdciapi lib when device is busy and not available for read
    if((m_ret != ESuccess) && uErrorCount < 300)
    {
      uErrorCount++;
      m_uPageData[0] = 0;
      continue;
    }

    RMI4CheckIfFatalError(m_ret);

    // Clear the attention assertion by reading the interrupt status register
    m_ret = SynaReadRegister(m_PdtF01Common.m_DataBase + 1, &m_uStatus, 1);

    RMI4CheckIfFatalError(m_ret);
  } while(((m_uPageData[0] & 0x0f) != 0x00) && (uErrorCount <= 300)); 

  TOUCH_FDD("Touch FD: %s(%d)\n", __FUNCTION__, __LINE__);
  // Issue Enable flash command
  m_ret = RMI4IssueEnableFlashCommand();
  
  RMI4CheckIfFatalError(m_ret);
  TOUCH_FDD("Touch FD: %s(%d)\n", __FUNCTION__, __LINE__);
  
  RMI4ReadPageDescriptionTable();
  
  // Wait for ATTN and check if flash command state is idle
 // RMI4WaitATTN();
  RMI4WaitATTN(300);
 
  // Read the data block 0 to determine the correctness of the image
  m_ret = SynaReadRegister(m_uF34Reflash_FlashControl, &uData[0], 1);

  RMI4CheckIfFatalError(m_ret);
  TOUCH_FDD("Touch FD: %s(%d) uData:0x%x \n", __FUNCTION__, __LINE__, uData[0]);
  TOUCH_FDD("m_uF34Reflash_FlashControl: 0x%x\n", m_uF34Reflash_FlashControl); 

  if ( uData[0] == 0x80 ) 
  {
    TOUCH_FDD("flash enabled");
  }
  else if ( uData[0] == 0xff )
  {
    TOUCH_FDD("flash failed");
    RMI4CheckIfFatalError( EErrorFlashNotDisabled );
    TOUCH_FDD("Touch FD: %s(%d)\n", __FUNCTION__, __LINE__);
  }
  else {
    TOUCH_FDD("flash failed");
    RMI4CheckIfFatalError( EErrorFlashNotDisabled );
    TOUCH_FDD("Touch FD: %s(%d)\n", __FUNCTION__, __LINE__);
  }
}


// This function gets config block count and config block size
void RMI4ReadConfigInfo(void)
{
  u8 uData[3];
  
  m_ret = SynaReadRegister(m_uF34ReflashQuery_ConfigBlockSize, &uData[0], 2);
  TOUCH_FDD("reg:0x%x,data[0]:0x%x,data[1]:0x%x\n",m_uF34ReflashQuery_ConfigBlockSize,uData[0],uData[1]);

  RMI4CheckIfFatalError(m_ret);
  TOUCH_FDD("Touch FD: %s(%d)\n", __FUNCTION__, __LINE__);
  
  m_configBlockSize = uData[0] | (uData[1] << 8);

  m_ret = SynaReadRegister(m_uF34ReflashQuery_ConfigBlockCount, &uData[0], 2);

  TOUCH_FDD("reg:0x%x,data[0]:0x%x,data[1]:0x%x\n",m_uF34ReflashQuery_ConfigBlockCount,uData[0],uData[1]);
  RMI4CheckIfFatalError(m_ret);
  TOUCH_FDD("Touch FD: %s(%d)\n", __FUNCTION__, __LINE__);
  
  m_configBlockCount = uData[0] | (uData[1] << 8);
  m_configImgSize = m_configBlockSize*m_configBlockCount;
}

//**************************************************************
// This function write config data to device one block at a time
//**************************************************************
void RMI4ProgramConfiguration(void)
{
  u8 uData[3];
  u8 buf[16], i;
  u8 *puData = m_configImgData;
  u16 blockNum = 0;

  TOUCH_FDD("Touch FD: %s(%d)\n", __FUNCTION__, __LINE__);
  for(blockNum = 0 ; blockNum < m_configBlockCount ; blockNum++)
  {
    SpecialCopyEndianAgnostic(&uData[0], blockNum);
    //TOUCH_FDD("blockNum:0x%x \n", blockNum);

    // Write Configuration Block Number
    m_ret = SynaWriteRegister(m_uF34Reflash_BlockNum, &uData[0], 2);
    RMI4CheckIfFatalError(m_ret);
    
    // Write Data Block
	for(i = 0 ; i < m_configBlockSize ; i++)
	{
		buf[i] = *puData;
		puData += 1;
		//TOUCH_FDD("$buf[%d]: 0x%x\t", i, buf[i]);
	}
    m_ret = SynaWriteRegister(m_uF34Reflash_BlockData, &buf, m_configBlockSize);
    RMI4CheckIfFatalError(m_ret);
    
    // Issue Write Configuration Block command to flash command register
    m_bAttenAsserted = false;
    uData[0] = s_uF34ReflashCmd_ConfigWrite;

    m_ret = RMI4IssueFlashControlCommand(&uData[0]);
    RMI4CheckIfFatalError(m_ret);

    // Wait for ATTN
    //RMI4WaitATTN();
    RMI4WaitATTN(300);
  }
    TOUCH_FDD("Touch FD: %s(%d)\n", __FUNCTION__, __LINE__);
}

EError RMI4IssueFlashControlCommand( u8 *command)
{
  m_ret = SynaWriteRegister(m_uF34Reflash_FlashControl, command, 1);
  return m_ret;
}

//**************************************************************
// Issue a reset ($01) command to the $F01 RMI command register
// This tests firmware image and executes it if it's valid
//**************************************************************
void RMI4ResetDevice(void)
{
  u8 m_uF01DeviceControl_CommandReg = m_PdtF01Common.m_CommandBase;
  u8 uData[2];

  uData[0] = 1;
  m_ret = SynaWriteRegister(m_uF01DeviceControl_CommandReg, &uData[0], 1);
  
  RMI4CheckIfFatalError(m_ret);
  TOUCH_FDD("Touch FD: %s(%d)\n", __FUNCTION__, __LINE__);
}

EError RMI4DisableFlash(void)
{
  u8 uData[2];
  unsigned int uErrorCount = 0;

  // Issue a reset command
  RMI4ResetDevice();
  //SynaSleep(200);
	mdelay(400);
  // Wait for ATTN to be asserted to see if device is in idle state
  if (SynaWaitForATTN(300))
  {
    RMI4CheckIfFatalError(EErrorTimeout);
  TOUCH_FDD("Touch FD: %s(%d)\n", __FUNCTION__, __LINE__);
  }

  do {
    m_ret = SynaReadRegister(m_uF34Reflash_FlashControl, &m_uPageData[0], 1);

    // To work around the physical address error from control bridge
    if((m_ret != ESuccess) && uErrorCount < 300)
    {
      uErrorCount++;
      m_uPageData[0] = 0;
      continue;
    }
    TOUCH_FDD("RMI4WaitATTN after errorCount loop, uErrorCount=%d\n", uErrorCount);

  } while(((m_uPageData[0] & 0x0f) != 0x00) && (uErrorCount <= 300));

  RMI4CheckIfFatalError(m_ret);
  TOUCH_FDD("Touch FD: %s(%d)\n", __FUNCTION__, __LINE__);
  
  // Clear the attention assertion by reading the interrupt status register
  m_ret = SynaReadRegister(m_PdtF01Common.m_DataBase + 1, &m_uStatus, 1);

  RMI4CheckIfFatalError(m_ret);
  TOUCH_FDD("Touch FD: %s(%d)\n", __FUNCTION__, __LINE__);
  
  // Read F01 Status flash prog, ensure the 6th bit is '0'
  TOUCH_FDD("Touch FD: %s(%d)\n", __FUNCTION__, __LINE__);
  do
  {
    m_ret = SynaReadRegister(m_uF01RMI_DataBase, &uData[0], 1);

    RMI4CheckIfFatalError(m_ret);

    TOUCH_FDD("F01 data register bit 6: 0x%x, 0x%x\n", m_uF01RMI_DataBase, uData[0]);
  } while((uData[0] & 0x40)!= 0);

  TOUCH_FDD("Touch FD: %s(%d)\n", __FUNCTION__, __LINE__);
  // With a new flash image the page description table could change
  RMI4ReadPageDescriptionTable();

  return ESuccess;
}

void RMI4CalculateChecksum(u16 * data, u16 len, unsigned long * dataBlock)
{
  unsigned long temp = *data++;
  unsigned long sum1;
  unsigned long sum2;

  *dataBlock = 0xffffffff;

  sum1 = *dataBlock & 0xFFFF;
  sum2 = *dataBlock >> 16;

  while (len--)
  {
    sum1 += temp;    
    sum2 += sum1;    
    sum1 = (sum1 & 0xffff) + (sum1 >> 16);    
    sum2 = (sum2 & 0xffff) + (sum2 >> 16);
  }

  *dataBlock = sum2 << 16 | sum1;
}

EError RMI4FlashFirmwareWrite(void)
{
  u8 *puFirmwareData = m_firmwareImgData;
  u8 uData[3];
  u8 buf[16];
  u16 uBlockNum = 0;
  int i;

  TOUCH_FDD("Flash Firmware starts\n");
  
  TOUCH_FDD("Touch FD: %s(%d)\n", __FUNCTION__, __LINE__);
 TOUCH_FDD("*0x%x 0x%x, 0x%x\n", m_uF34Reflash_BlockData, *puFirmwareData, *m_firmwareImgData);
  for(uBlockNum = 0; uBlockNum < m_firmwareBlockCount; ++uBlockNum)
  {
    uData[0] = uBlockNum & 0xff;
    uData[1] = (uBlockNum & 0xff00) >> 8;

 	//TOUCH_FDD(" %s, blockNum: 0x%x\n", __FUNCTION__, uBlockNum);   
    // Write Block Number
    m_ret = SynaWriteRegister(m_uF34Reflash_BlockNum, &uData[0], 2);
    RMI4CheckIfFatalError(m_ret);
	
	for(i = 0 ; i < m_firmwareBlockSize ; i++)
	{
		buf[i] = *puFirmwareData;
		puFirmwareData += 1;
	//	TOUCH_FDD("buf[%d]: 0x%x\t", i, buf[i]);
	}
    //TOUCH_FDD("#0x%x 0x%x, 0x%x\n", m_uF34Reflash_BlockData, *puFirmwareData, *m_firmwareImgData);
    // Write Data Block
    m_ret = SynaWriteRegister(m_uF34Reflash_BlockData, &buf[0], m_firmwareBlockSize);
    //TOUCH_FDD("0x%x 0x%x, 0x%x\n", m_uF34Reflash_BlockData, *puFirmwareData, *m_firmwareImgData);
    //TOUCH_FDD("=>0x%x 0x%x, 0x%x\n", m_uF34Reflash_BlockData, puFirmwareData, m_firmwareImgData);
    RMI4CheckIfFatalError(m_ret);

    // Move to next data block
    //puFirmwareData += m_firmwareBlockSize;

    // Issue Write Firmware Block command
    m_bAttenAsserted = false;
    uData[0] = 2;
    m_ret = SynaWriteRegister(m_uF34Reflash_FlashControl, &uData[0], 1);
    RMI4CheckIfFatalError(m_ret);

    // Wait ATTN. Read Flash Command register and check error
    //RMI4WaitATTN();
    RMI4WaitATTN(300);
  }

  TOUCH_FDD("Flash Firmware done\n");
  
  return ESuccess;
}

void RMI4ProgramFirmware(void)
{
  EError m_ret;
  u8 uData[2];

  if ( !RMI4ValidateBootloadID(m_bootloadImgID) )
  {
    RMI4CheckIfFatalError( EErrorBootID );
  TOUCH_FDD("Touch FD: %s(%d)\n", __FUNCTION__, __LINE__);
  }

  // Write bootID to data block register
  m_ret = RMI4WriteBootloadID();
  RMI4CheckIfFatalError(m_ret);
  TOUCH_FDD("Touch FD: %s(%d)\n", __FUNCTION__, __LINE__);

  // Issue the firmware and configuration erase command
  uData[0]=3;
  //m_ret = RMI4IssueEraseCommand(&uData[0]);
  m_ret = SynaWriteRegister(m_uF34Reflash_FlashControl, &uData[0], 1);

  RMI4CheckIfFatalError(m_ret);
  TOUCH_FDD("Touch FD: %s(%d)\n", __FUNCTION__, __LINE__);
  //RMI4WaitATTN();
  RMI4WaitATTN(300);

  // Write firmware image
  m_ret = RMI4FlashFirmwareWrite();
  RMI4CheckIfFatalError(m_ret);
  TOUCH_FDD("Touch FD: %s(%d)\n", __FUNCTION__, __LINE__);
}

bool RMI4ValidateBootloadID(u16 bootloadID)
{
  TOUCH_FDD("In RMI4ValidateBootloadID\n");
  m_ret = RMI4ReadBootloadID();
  RMI4CheckIfFatalError(m_ret);
  TOUCH_FDD("Bootload ID of device: %X, input bootID: %X\n", m_BootloadID, bootloadID);

  // check bootload ID against the value found in firmware--but only for image file format version 0
  return m_firmwareImgVersion != 0 || bootloadID == m_BootloadID;
}

EError RMI4IssueEraseCommand(u8 *command)
{
  // command = 3 - erase all; command = 7 - erase config
	m_ret = SynaWriteRegister(m_uF34Reflash_FlashControl, command, 1);

  return m_ret;
}


// This function gets the firmware block size and block count
void RMI4ReadFirmwareInfo(void)
{
  u8 uData[3];

  m_ret = SynaReadRegister(m_uF34ReflashQuery_FirmwareBlockSize, &uData[0], 2);
  RMI4CheckIfFatalError(m_ret);
  TOUCH_FDD("0x%x 0x%x, 0x%x\n", m_uF34ReflashQuery_FirmwareBlockSize, uData[0], uData[1]);
  TOUCH_FDD("Touch FD: %s(%d)\n", __FUNCTION__, __LINE__);

  m_firmwareBlockSize = uData[0] | (uData[1] << 8);
  TOUCH_FDD("ey.cho=> BlockSize: 0x%x\n", m_firmwareBlockSize);

  m_ret = SynaReadRegister(m_uF34ReflashQuery_FirmwareBlockCount, &uData[0], 2);
  RMI4CheckIfFatalError(m_ret);
  TOUCH_FDD("Touch FD: %s(%d)\n", __FUNCTION__, __LINE__);

  m_firmwareBlockCount = uData[0] | (uData[1] << 8);
  m_firmwareImgSize = m_firmwareBlockCount*m_firmwareBlockSize;
  
  TOUCH_FDD("m_firmwareBlockSize, m_firmwareBlockCount: %d, %d\n", m_firmwareBlockSize, m_firmwareBlockCount);
}


void RMI4ReadPageDescriptionTable(void) 
{
  struct RMI4FunctionDescriptor Buffer;

  u8 Buf[7]={0};
  u8 uAddress;
  int i = 0;
  // Read config data

  //SynaSleep(20);
  mdelay(20);

  m_PdtF01Common.m_ID = 0;
  m_PdtF34Flash.m_ID = 0;
  m_BaseAddresses.m_ID = 0xff;


  //for(uAddress = 0xe9; uAddress > 10; uAddress -= sizeof(g_RMI4FunctionDescriptor))
  for(uAddress = 0xe9; uAddress > 10; uAddress -= 6)
  {
    m_ret = SynaReadRegister(uAddress, (u8*)&Buffer, sizeof(Buffer));
    RMI4CheckIfFatalError(m_ret);

    TOUCH_FDD("Touch FD: %s(%d) ->0x%x \n", __FUNCTION__, __LINE__, uAddress);
    TOUCH_FDD("Touch FD: %s(%d) ->0x%x \n", __FUNCTION__, __LINE__, Buffer.m_QueryBase);
    TOUCH_FDD("Touch FD: %s(%d) ->0x%x \n", __FUNCTION__, __LINE__, Buffer.m_CommandBase);
    TOUCH_FDD("Touch FD: %s(%d) ->0x%x \n", __FUNCTION__, __LINE__, Buffer.m_ControlBase);
    TOUCH_FDD("Touch FD: %s(%d) ->0x%x \n", __FUNCTION__, __LINE__, Buffer.m_DataBase);
    TOUCH_FDD("Touch FD: %s(%d) ->0x%x \n", __FUNCTION__, __LINE__, Buffer.m_IntSourceCount);
    TOUCH_FDD("Touch FD: %s(%d) ->0x%x \n", __FUNCTION__, __LINE__, Buffer.m_ID);

    if(m_BaseAddresses.m_ID == 0xff)
    {
      m_BaseAddresses = Buffer;
    }

    switch(Buffer.m_ID)
    {
      case 0x34:
        m_PdtF34Flash = Buffer;

        break;
      case 0x01:
       m_PdtF01Common = Buffer;
    	break;
    }

    if(Buffer.m_ID == 0)
    {
      break;
    }
    else
    {
      TOUCH_FDD("Function $%02x found.\n", Buffer.m_ID); 
    }
  }

  // Initialize device related data members
  m_uF01RMI_DataBase = m_PdtF01Common.m_DataBase;
  m_uF01RMI_IntStatus = m_PdtF01Common.m_DataBase + 1;
  m_uF01RMI_CommandBase = m_PdtF01Common.m_CommandBase;
  m_uF01RMI_QueryBase = m_PdtF01Common.m_QueryBase;

  m_uF34Reflash_DataReg = m_PdtF34Flash.m_DataBase;
  m_uF34Reflash_BlockNum = m_PdtF34Flash.m_DataBase;
  m_uF34Reflash_BlockData = m_PdtF34Flash.m_DataBase + 2;
  m_uF34ReflashQuery_BootID = m_PdtF34Flash.m_QueryBase;

  m_uF34ReflashQuery_FlashPropertyQuery = m_PdtF34Flash.m_QueryBase + 2;
  m_uF34ReflashQuery_FirmwareBlockSize = m_PdtF34Flash.m_QueryBase + 3;
  m_uF34ReflashQuery_FirmwareBlockCount = m_PdtF34Flash.m_QueryBase + 5;
  m_uF34ReflashQuery_ConfigBlockSize = m_PdtF34Flash.m_QueryBase + 3;
  m_uF34ReflashQuery_ConfigBlockCount = m_PdtF34Flash.m_QueryBase + 7;
  
  RMI4setFlashAddrForDifFormat();
}

void RMI4WritePage(void)
{
  // Write page
  u8 uPage[2] = {0x00};
  
  u8 uF01_RMI_Data[3];
  u8 m_uStatus[2];
u8 buf[2] = {0};

  m_ret = SynaWriteRegister(0xff, &uPage, 1);
  RMI4CheckIfFatalError(m_ret);
  TOUCH_FDD("Touch FD: %s(%d)\n", __FUNCTION__, __LINE__);
  
m_ret = SynaReadRegister(0xff, &buf, 1);
TOUCH_FDD("Touch FD->buf= 0x%x\n", buf[0]);
  do
  {
    m_ret = SynaReadRegister(0, &m_uStatus, 1);
    RMI4CheckIfFatalError(m_ret);

    if(m_uStatus[0] & 0x40)
    {
      m_bFlashProgOnStartup = true;
    }

    if(m_uStatus[0] & 0x80)
    {
      m_bUnconfigured = true;
      break;
    }

  } while(m_uStatus[0] & 0x40);

  TOUCH_FDD("m_uStatus is 0x%x\n", m_uStatus[0]);
  if(m_bFlashProgOnStartup && ! m_bUnconfigured)
  {
    TOUCH_FDD("Bootloader running\n");
  }
  else if(m_bUnconfigured)
  {
    TOUCH_FDD("UI running\n");
  }

  RMI4ReadPageDescriptionTable();
  
  if(m_PdtF34Flash.m_ID == 0)
  {
    TOUCH_FDD("Function $34 is not supported\n");
    RMI4CheckIfFatalError( EErrorFunctionNotSupported );   
    TOUCH_FDD("Touch FD: %s(%d)\n", __FUNCTION__, __LINE__);
  }

  TOUCH_FDD("Function $34 addresses Control base:$%02x Query base: $%02x.\n", m_PdtF34Flash.m_ControlBase, m_PdtF34Flash.m_QueryBase);
  
  if(m_PdtF01Common.m_ID == 0)
  {
    TOUCH_FDD("Function $01 is not supported\n");
    m_PdtF01Common.m_ID = 0x01;
    m_PdtF01Common.m_DataBase = 0;
    RMI4CheckIfFatalError( EErrorFunctionNotSupported );    
    TOUCH_FDD("Touch FD: %s(%d)\n", __FUNCTION__, __LINE__);
  }
  TOUCH_FDD("Function $01 addresses Control base:$%02x Query base: $%02x.\n", m_PdtF01Common.m_ControlBase, m_PdtF01Common.m_QueryBase);

  // Get device status
  m_ret = SynaReadRegister(m_PdtF01Common.m_DataBase, &uF01_RMI_Data[0], sizeof(uF01_RMI_Data));
  RMI4CheckIfFatalError(m_ret);
  TOUCH_FDD("Touch FD: %s(%d)\n", __FUNCTION__, __LINE__);
  TOUCH_FDD("reg: 0x%x, data:0x%x\n", m_PdtF01Common.m_DataBase, uF01_RMI_Data[0]);
  // Check Device Status
  TOUCH_FDD("Configured: %s\n", uF01_RMI_Data[0] & 0x80 ? "false" : "true");
  TOUCH_FDD("FlashProg:  %s\n", uF01_RMI_Data[0] & 0x40 ? "true" : "false");
  TOUCH_FDD("StatusCode: 0x%x \n", uF01_RMI_Data[0] & 0x0f );
}

void RMI4RMIInit(EProtocol pt, u8 i2cAddr, EAttention eAttn, unsigned int byteDelay,
                 unsigned int bitRate, unsigned long timeOut)
{
   switch (pt)
  {
    case Ei2c:       //i2c
      RMI4RMIInitI2C(i2cAddr, eAttn);
      break;
    case Espi:       //spi
      RMI4RMIInitSPI(byteDelay, bitRate, timeOut);
      break;
  }
}

void RMI4RMIInitI2C(u8 i2cAddr, EAttention i2cAttn)
{
  RMI4WritePage();
}

void RMI4RMIInitSPI(unsigned int byteDelay, unsigned int bitRate, unsigned long timeOut)
{ 
  RMI4WritePage();
}

unsigned long ExtractLongFromHeader(const u8* SynaImage)  // Endian agnostic 
{
  return((unsigned long)SynaImage[0] +
         (unsigned long)SynaImage[1]*0x100 +
         (unsigned long)SynaImage[2]*0x10000 +
         (unsigned long)SynaImage[3]*0x1000000);
}
    
void RMI4ReadFirmwareHeader(int version)
{
	unsigned long checkSumCode;

    TOUCH_FDD("\n***Firmware Version: %d\n\n", version);
	if(version == 5) {

		m_fileSize = sizeof(SynaFirmware5) -1;

		TOUCH_FDD("\nScanning SynaFirmware[], the auto-generated C Header File - len = %d \n\n", m_fileSize);

		checkSumCode         = ExtractLongFromHeader(&(SynaFirmware5[0]));
		m_bootloadImgID      = (unsigned int)SynaFirmware5[4] + (unsigned int)SynaFirmware5[5]*0x100;
		m_firmwareImgVersion = SynaFirmware5[7]; 
		m_firmwareImgSize    = ExtractLongFromHeader(&(SynaFirmware5[8]));
		m_configImgSize      = ExtractLongFromHeader(&(SynaFirmware5[12]));    

		TOUCH_FDD("Target = %s, ",&SynaFirmware5[16]);
		TOUCH_FDD("Cksum = 0x%u, Id = 0x%u, Ver = %d, FwSize = 0x%u, ConfigSize = 0x%u \n",
				checkSumCode, m_bootloadImgID, m_firmwareImgVersion, m_firmwareImgSize, m_configImgSize);

		RMI4ReadFirmwareInfo();   // Determine firmware organization - read firmware block size and firmware size

		RMI4CalculateChecksum((u8*)&(SynaFirmware5[4]), (u8)(m_fileSize-4)>>1,
				&m_FirmwareImgFile_checkSum);

		if (m_fileSize != (0x100+m_firmwareImgSize+m_configImgSize))
		{
			TOUCH_FDD("Error: SynaFirmware[] size = 0x%u, expected 0x%u\n", m_fileSize, (0x100+m_firmwareImgSize+m_configImgSize));
			while(1);   
		}

		if (m_firmwareImgSize != GetFirmwareSize())
		{
			TOUCH_FDD("Firmware image size verfication failed, size in image 0x%X did not match device size 0x%X\n", m_firmwareImgSize, GetFirmwareSize());
			while(1);
		}

		if (m_configImgSize != GetConfigSize())
		{
			TOUCH_FDD("Configuration size verfication failed, size in image 0x%X did not match device size 0x%X\n", m_configImgSize, GetConfigSize());
			while(1); 
		}

		m_firmwareImgData=(u8 *)((&SynaFirmware5[0])+0x100);

		memcpy(m_configImgData, (&SynaFirmware5[0])+0x100+m_firmwareImgSize, m_configImgSize);

		SynaReadRegister(m_uF34Reflash_FlashControl, &m_uPageData[0], 1);

	}
	else if(version == 7){

		m_fileSize = sizeof(SynaFirmware7) -1;

		TOUCH_FDD("\nScanning SynaFirmware[], the auto-generated C Header File - len = %d \n\n", m_fileSize);

		checkSumCode         = ExtractLongFromHeader(&(SynaFirmware7[0]));
		m_bootloadImgID      = (unsigned int)SynaFirmware7[4] + (unsigned int)SynaFirmware7[5]*0x100;
		m_firmwareImgVersion = SynaFirmware7[7]; 
		m_firmwareImgSize    = ExtractLongFromHeader(&(SynaFirmware7[8]));
		m_configImgSize      = ExtractLongFromHeader(&(SynaFirmware7[12]));    

		TOUCH_FDD("Target = %s, ",&SynaFirmware7[16]);
		TOUCH_FDD("Cksum = 0x%u, Id = 0x%u, Ver = %d, FwSize = 0x%u, ConfigSize = 0x%u \n",
				checkSumCode, m_bootloadImgID, m_firmwareImgVersion, m_firmwareImgSize, m_configImgSize);

		RMI4ReadFirmwareInfo();   // Determine firmware organization - read firmware block size and firmware size

		RMI4CalculateChecksum((u8*)&(SynaFirmware7[4]), (u8)(m_fileSize-4)>>1,
				&m_FirmwareImgFile_checkSum);

		if (m_fileSize != (0x100+m_firmwareImgSize+m_configImgSize))
		{
			TOUCH_FDD("Error: SynaFirmware[] size = 0x%u, expected 0x%u\n", m_fileSize, (0x100+m_firmwareImgSize+m_configImgSize));
			while(1);   
		}

		if (m_firmwareImgSize != GetFirmwareSize())
		{
			TOUCH_FDD("Firmware image size verfication failed, size in image 0x%X did not match device size 0x%X\n", m_firmwareImgSize, GetFirmwareSize());
			while(1);
		}

		if (m_configImgSize != GetConfigSize())
		{
			TOUCH_FDD("Configuration size verfication failed, size in image 0x%X did not match device size 0x%X\n", m_configImgSize, GetConfigSize());
			while(1); 
		}

		m_firmwareImgData=(u8 *)((&SynaFirmware7[0])+0x100);

		memcpy(m_configImgData, (&SynaFirmware7[0])+0x100+m_firmwareImgSize, m_configImgSize);

		SynaReadRegister(m_uF34Reflash_FlashControl, &m_uPageData[0], 1);

	} 
	else if(version == 6){

		m_fileSize = sizeof(SynaFirmware6) -1;

		TOUCH_FDD("\nScanning SynaFirmware[], the auto-generated C Header File - len = %d \n\n", m_fileSize);

		checkSumCode         = ExtractLongFromHeader(&(SynaFirmware6[0]));
		m_bootloadImgID      = (unsigned int)SynaFirmware6[4] + (unsigned int)SynaFirmware6[5]*0x100;
		m_firmwareImgVersion = SynaFirmware6[7]; 
		m_firmwareImgSize    = ExtractLongFromHeader(&(SynaFirmware6[8]));
		m_configImgSize      = ExtractLongFromHeader(&(SynaFirmware6[12]));    

		TOUCH_FDD("Target = %s, ",&SynaFirmware6[16]);
		TOUCH_FDD("Cksum = 0x%u, Id = 0x%u, Ver = %d, FwSize = 0x%u, ConfigSize = 0x%u \n",
				checkSumCode, m_bootloadImgID, m_firmwareImgVersion, m_firmwareImgSize, m_configImgSize);

		RMI4ReadFirmwareInfo();   // Determine firmware organization - read firmware block size and firmware size

		RMI4CalculateChecksum((u8*)&(SynaFirmware6[4]), (u8)(m_fileSize-4)>>1,
				&m_FirmwareImgFile_checkSum);

		if (m_fileSize != (0x100+m_firmwareImgSize+m_configImgSize))
		{
			TOUCH_FDD("Error: SynaFirmware[] size = 0x%u, expected 0x%u\n", m_fileSize, (0x100+m_firmwareImgSize+m_configImgSize));
			while(1);   
		}

		if (m_firmwareImgSize != GetFirmwareSize())
		{
			TOUCH_FDD("Firmware image size verfication failed, size in image 0x%X did not match device size 0x%X\n", m_firmwareImgSize, GetFirmwareSize());
			while(1);
		}

		if (m_configImgSize != GetConfigSize())
		{
			TOUCH_FDD("Configuration size verfication failed, size in image 0x%X did not match device size 0x%X\n", m_configImgSize, GetConfigSize());
			while(1); 
		}

		m_firmwareImgData=(u8 *)((&SynaFirmware6[0])+0x100);

		memcpy(m_configImgData, (&SynaFirmware6[0])+0x100+m_firmwareImgSize, m_configImgSize);

		SynaReadRegister(m_uF34Reflash_FlashControl, &m_uPageData[0], 1);

	} 
	else if(version == 9){

		m_fileSize = sizeof(SynaFirmware9) -1;

		TOUCH_FDD("\nScanning SynaFirmware[], the auto-generated C Header File - len = %d \n\n", m_fileSize);

		checkSumCode         = ExtractLongFromHeader(&(SynaFirmware9[0]));
		m_bootloadImgID      = (unsigned int)SynaFirmware9[4] + (unsigned int)SynaFirmware9[5]*0x100;
		m_firmwareImgVersion = SynaFirmware9[7]; 
		m_firmwareImgSize    = ExtractLongFromHeader(&(SynaFirmware9[8]));
		m_configImgSize      = ExtractLongFromHeader(&(SynaFirmware9[12]));    

		TOUCH_FDD("Target = %s, ",&SynaFirmware9[16]);
		TOUCH_FDD("Cksum = 0x%u, Id = 0x%u, Ver = %d, FwSize = 0x%u, ConfigSize = 0x%u \n",
				checkSumCode, m_bootloadImgID, m_firmwareImgVersion, m_firmwareImgSize, m_configImgSize);

		RMI4ReadFirmwareInfo();   // Determine firmware organization - read firmware block size and firmware size

		RMI4CalculateChecksum((u8*)&(SynaFirmware9[4]), (u8)(m_fileSize-4)>>1,
				&m_FirmwareImgFile_checkSum);

		if (m_fileSize != (0x100+m_firmwareImgSize+m_configImgSize))
		{
			TOUCH_FDD("Error: SynaFirmware[] size = 0x%u, expected 0x%u\n", m_fileSize, (0x100+m_firmwareImgSize+m_configImgSize));
			while(1);   
		}

		if (m_firmwareImgSize != GetFirmwareSize())
		{
			TOUCH_FDD("Firmware image size verfication failed, size in image 0x%X did not match device size 0x%X\n", m_firmwareImgSize, GetFirmwareSize());
			while(1);
		}

		if (m_configImgSize != GetConfigSize())
		{
			TOUCH_FDD("Configuration size verfication failed, size in image 0x%X did not match device size 0x%X\n", m_configImgSize, GetConfigSize());
			while(1); 
		}

		m_firmwareImgData=(u8 *)((&SynaFirmware9[0])+0x100);

		memcpy(m_configImgData, (&SynaFirmware9[0])+0x100+m_firmwareImgSize, m_configImgSize);

		SynaReadRegister(m_uF34Reflash_FlashControl, &m_uPageData[0], 1);

	} 
	else {

		m_fileSize = sizeof(SynaFirmware) -1;

		TOUCH_FDD("\nScanning SynaFirmware[], the auto-generated C Header File - len = %d \n\n", m_fileSize);

		checkSumCode         = ExtractLongFromHeader(&(SynaFirmware[0]));
		m_bootloadImgID      = (unsigned int)SynaFirmware[4] + (unsigned int)SynaFirmware[5]*0x100;
		m_firmwareImgVersion = SynaFirmware[7]; 
		m_firmwareImgSize    = ExtractLongFromHeader(&(SynaFirmware[8]));
		m_configImgSize      = ExtractLongFromHeader(&(SynaFirmware[12]));    

		TOUCH_FDD("Target = %s, ",&SynaFirmware[16]);
		TOUCH_FDD("Cksum = 0x%u, Id = 0x%u, Ver = %d, FwSize = 0x%u, ConfigSize = 0x%u \n",
				checkSumCode, m_bootloadImgID, m_firmwareImgVersion, m_firmwareImgSize, m_configImgSize);

		RMI4ReadFirmwareInfo();   // Determine firmware organization - read firmware block size and firmware size

		RMI4CalculateChecksum((u8*)&(SynaFirmware[4]), (u8)(m_fileSize-4)>>1,
				&m_FirmwareImgFile_checkSum);

		if (m_fileSize != (0x100+m_firmwareImgSize+m_configImgSize))
		{
			TOUCH_FDD("Error: SynaFirmware[] size = 0x%u, expected 0x%u\n", m_fileSize, (0x100+m_firmwareImgSize+m_configImgSize));
			while(1);   
		}

		if (m_firmwareImgSize != GetFirmwareSize())
		{
			TOUCH_FDD("Firmware image size verfication failed, size in image 0x%X did not match device size 0x%X\n", m_firmwareImgSize, GetFirmwareSize());
			while(1);
		}

		if (m_configImgSize != GetConfigSize())
		{
			TOUCH_FDD("Configuration size verfication failed, size in image 0x%X did not match device size 0x%X\n", m_configImgSize, GetConfigSize());
			while(1); 
		}

		m_firmwareImgData=(u8 *)((&SynaFirmware[0])+0x100);

		memcpy(m_configImgData, (&SynaFirmware[0])+0x100+m_firmwareImgSize, m_configImgSize);

		SynaReadRegister(m_uF34Reflash_FlashControl, &m_uPageData[0], 1);
	}

	return;
}


bool RMI4isExpectedRegFormat(void)
{ 
  // Flash Properties query 1: registration map format version 1     
  //  0: registration map format version 0
  m_ret = SynaReadRegister(m_uF34ReflashQuery_FlashPropertyQuery, &m_uPageData[0], 1);
  RMI4CheckIfFatalError(m_ret);
  TOUCH_FDD("Touch FD: %s(%d)\n", __FUNCTION__, __LINE__);

  TOUCH_FDD("FlashPropertyQuery = 0x%x\n", m_uPageData[0]);
  return ((m_uPageData[0] & 0x01) == 0x01);
}

void RMI4setFlashAddrForDifFormat(void)
{
  if (RMI4isExpectedRegFormat())
  {
    TOUCH_FDD("Image format 1");
    m_uF34Reflash_FlashControl = m_PdtF34Flash.m_DataBase + m_firmwareBlockSize + 2;
    m_uF34Reflash_BlockNum = m_PdtF34Flash.m_DataBase;
    m_uF34Reflash_BlockData = m_PdtF34Flash.m_DataBase + 2;
  }
  else {
    m_uF34Reflash_FlashControl = m_PdtF34Flash.m_DataBase;
    m_uF34Reflash_BlockNum = m_PdtF34Flash.m_DataBase + 1;
    m_uF34Reflash_BlockData = m_PdtF34Flash.m_DataBase + 3;
  }
}

#if 0
void SynaConvertFirmwareImageToCHeaderFile(char *imgfile)  
{
  FILE * pFile;
  FILE * hFile;
  int c;
  int line = 0;
  int column = 0;

  hFile=fopen ("SynaImage.h","w");  
  pFile=fopen (imgfile,"rb");

  fTOUCH_FDD(hFile,"\n // This is Synaptics Image File Data - Auto-Generated - DO NOT EDIT!!! \n\n\n");
  fTOUCH_FDD(hFile," const u8 SynaFirmware[] = { ");

  TOUCH_FDD("Converting %s to C Header File \n",imgfile);

  if (pFile==NULL) 
    TOUCH_FDD ("\n Error opening file");
  else
  {
    do{
      c = fgetc (pFile);
      if (column == 0)
      {
        TOUCH_FDD("\n /*%04x:*/ ",line);
        fTOUCH_FDD(hFile,"\n /*%04x:*/ ",line);
        line += 16;

      }

      if (c != EOF)
        fTOUCH_FDD(hFile, "0x%02x, ",c); 
      else fTOUCH_FDD(hFile, " 0xFFFF }; \n\n");

      if (c != EOF)
        TOUCH_FDD("0x%02x, ",c); 
      else TOUCH_FDD(" 0xFFFF }; \n\n");       
      
      if (++column >= 16)
        column = 0;
     
    } while (c != EOF);
      
    fclose(pFile);
    fclose(hFile); 
  }
}
#endif
// Scan Page Description Table (PDT) to find all RMI functions presented by this device.
// The Table starts at $00EE. This and every sixth register (decrementing) is a function number
// except when this "function number" is $00, meaning end of PDT.
// In an actual use case this scan might be done only once on first run or before compile.

u8 SynaScanMapToFindFingerData(void)
{
  int i;
  u8 RMIAddress = 0;
  unsigned int RMILength = 0x100;
  u8 PageZero[0x100+3];
  u8 PDT_P00_F11_2D_EXISTS = 0;
  u8 PDT_P00_F11_2D_DATA_BASE;

  SynaReadRegister(RMIAddress, &PageZero[0],RMILength); // read all 256 bytes in page zero

  for (i=0xEE; i>6; i-=6)  // The Table starts at $00EE.
  {
    if (PageZero[i] == 0   )
      break;    // except when this "function number" is $00, meaning end of PDT.
    
    if (PageZero[i] == 0x11)
      PDT_P00_F11_2D_EXISTS = i;  // we found fcn 11!
  } 

  if (PDT_P00_F11_2D_EXISTS) 
  {
    PDT_P00_F11_2D_DATA_BASE = PDT_P00_F11_2D_EXISTS - 2;  // according to the register map rules
    
    return(PageZero[PDT_P00_F11_2D_DATA_BASE]);  // returns index to start of F11_2D_DATA00
  }

  TOUCH_FDD("Error:  Function 11 was not found. \n");
  //getch();

  return -1;
}
      
void  SynaAcquireFingers(void)
{
  unsigned int RMILength = 18;
  //u8 LiveFingers[128];
  u8 LiveFingers[129];
  u8 F11_2D_DATA00;
  TOUCH_FDD("Press enter to read live sensor measurements. \n");
 // getch();

  F11_2D_DATA00 = SynaScanMapToFindFingerData();  // self discovery, or ...
  if (F11_2D_DATA00 == -1)
    return;

  F11_2D_DATA00 = 0x15;  // or you can hard code like this

  TOUCH_FDD("Function 11 DATA starts at Register Addr: %02X \n",F11_2D_DATA00);
  

  while (1)
  {
  //  SynaWaitForATTN((DWORD)64000);
   SynaWaitForATTN(64000);

    SynaReadRegister(F11_2D_DATA00-2, &LiveFingers[0],RMILength);
  }
}


int FirmwareUpgrade_main(int version)
{
  // edit the default values below for your target 
  
  EProtocol protocolType = Ei2c; 
  EAttention attn = EAttnHighAndLow;
//  int i2c_address = 0x20;           // Default slave address  // for tm1228  
  unsigned int byte_delay = 100;    // Default byte delay
  unsigned int bit_rate = 2000;     // Default bit rate
  unsigned int time_out = 3000;     // Default spi mode
  char ImgFile[] =  { "PR618210-tm1416-001.img" };
  int i;

  TOUCH_FDD("RMI4 Reflash Utility:  press enter ...\n");   
  //getch();
  RMI4FuncsConstructor();

  TOUCH_FDD("Touch MAIN FD: %s(%d)\n", __FUNCTION__, __LINE__);
  RMI4RMIInit(protocolType, (u8)I2CAddr7Bit, attn, byte_delay, bit_rate, time_out);
  TOUCH_FDD("Touch MAIN FD: %s(%d)\n", __FUNCTION__, __LINE__);
  RMI4Init();

  TOUCH_FDD("Touch MAIN FD: %s(%d)\n", __FUNCTION__, __LINE__);
  RMI4setFlashAddrForDifFormat();
  TOUCH_FDD("Touch MAIN FD: %s(%d)\n", __FUNCTION__, __LINE__);
  RMI4EnableFlashing();

  TOUCH_FDD("Touch MAIN FD: %s(%d)\n", __FUNCTION__, __LINE__);
  //SynaConvertFirmwareImageToCHeaderFile(ImgFile);  // this is just a utility to generate a header file.

  RMI4ReadFirmwareHeader(version);
  TOUCH_FDD("Touch MAIN FD: %s(%d)\n", __FUNCTION__, __LINE__);
  RMI4ProgramFirmware();  //  issues the "eraseAll" so must call before any write
  TOUCH_FDD("Touch MAIN FD: %s(%d)\n", __FUNCTION__, __LINE__);
  RMI4ProgramConfiguration();  

  TOUCH_FDD("Touch MAIN FD: %s(%d)\n", __FUNCTION__, __LINE__);
  if (RMI4DisableFlash()!= ESuccess )         
  { 
//    EndControlBridge();
    return 1;
  }
     
  TOUCH_FDD("\nOutput: Reflash successful\n");
  
 // SynaAcquireFingers();
 // EndControlBridge();
}
EXPORT_SYMBOL(FirmwareUpgrade_main);
