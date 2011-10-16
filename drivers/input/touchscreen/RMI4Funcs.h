// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ 
// Copyright ? 2008 Synaptics Incorporated. All rights reserved. 
// 
// The information in this file is confidential under the terms 
// of a non-disclosure agreement with Synaptics and is provided 
// AS IS. 
// 
// The information in this file shall remain the exclusive property 
// of Synaptics and may be the subject of Synaptics? patents, in 
// whole or part. Synaptics? intellectual property rights in the 
// information in this file are not expressly or implicitly licensed 
// or otherwise transferred to you as a result of such information 
// being made available to you. 
//
// $RCSfile: RMI4Funcs.h,v $
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifndef RMI4Funcs_H
#define RMI4Funcs_H

#define ESuccess                     0
#define I2C_SUCCESS 			0
#define EErrorTimeout               -1
#define EErrorFlashNotDisabled           -2
#define EErrorBootID                -3
#define EErrorFunctionNotSupported  -4
#define EErrorFlashImageCorrupted   -5
#define I2C_FAILURE -6
/*
struct RMI4FunctionDescriptor {
  u8 m_QueryBase;
  u8 m_CommandBase;
  u8 m_ControlBase;
  u8 m_DataBase;
  u8 m_IntSourceCount;
  u8 m_ID;
};

struct ConfigDataBlock {
  u8 m_Address;
  u8 m_Data;
};
*/
struct RMI4FunctionDescriptor {
  u8 m_QueryBase;
  u8 m_CommandBase;
  u8 m_ControlBase;
  u8 m_DataBase;
  u8 m_IntSourceCount;
  u8 m_ID;
};

struct ConfigDataBlock {
  u16 m_Address;
  u8 m_Data;
};
enum {
  i2c,
  spi
} protocolType;

typedef enum {
  EAttnNone,
  EAttnLow,
  EAttnHigh,
  EAttnHighAndLow,
} EAttention;

typedef enum {
  Ei2c,
  Espi,
} EProtocol;

struct RMI4FunctionDescriptor m_PdtF34Flash;
struct RMI4FunctionDescriptor m_PdtF01Common;
struct RMI4FunctionDescriptor m_BaseAddresses;
struct RMI4FunctionDescriptor g_RMI4FunctionDescriptor;

//u8 m_uQuery_Base;
u8 m_uQuery_Base;

unsigned int m_lengthWritten, m_lengthRead;

struct ConfigDataBlock g_ConfigDataList[0x2d - 4];
struct ConfigDataBlock g_ConfigDataBlock;

unsigned int g_ConfigDataCount;

bool            m_bAttenAsserted;
u8          m_ret;
bool  m_bFlashProgOnStartup;
bool  m_bUnconfigured;


//u8 m_BootloadID;
//u8 m_BootID_Addr;
u16 m_BootloadID;
u16 m_BootID_Addr;

// Image file 
unsigned long m_checkSumImg;

// buffer for flash images ... tomv
//u8 FirmwareImage[16000];  // make smaller and dynamic
//u8 ConfigImage[16000];  // make smaller and dynamic
  
u8 FirmwareImage[16000];  // make smaller and dynamic
u8 ConfigImage[16000];  // make smaller and dynamic
  
u16 m_bootloadImgID;
u8 m_firmwareImgVersion;
u8 *m_firmwareImgData;
u8 *m_configImgData;
u16 m_firmwareBlockSize;
u16 m_firmwareBlockCount;
u16 m_configBlockSize;
u16 m_configBlockCount;

// Registers for configuration flash
u8 m_uPageData[0x200];
u8 m_uStatus;
unsigned long m_firmwareImgSize;
unsigned long m_configImgSize;
unsigned long m_fileSize;

u8 m_uF01RMI_CommandBase;
u8 m_uF01RMI_DataBase;
u8 m_uF01RMI_QueryBase;
u8 m_uF01RMI_IntStatus;

u8 m_uF34Reflash_DataReg;
u8 m_uF34Reflash_BlockNum;
u8 m_uF34Reflash_BlockData;
u8 m_uF34Reflash_FlashControl;
u8 m_uF34ReflashQuery_BootID;
u8 m_uF34ReflashQuery_FirmwareBlockSize;
u8 m_uF34ReflashQuery_FirmwareBlockCount;
u8 m_uF34ReflashQuery_ConfigBlockSize;
u8 m_uF34ReflashQuery_ConfigBlockCount;
u8 m_uF34ReflashQuery_FlashPropertyQuery;

unsigned long m_FirmwareImgFile_checkSum;
  
//  Constants
static const u8 s_uF34ReflashCmd_FirmwareCrc   = 0x01;
static const u8 s_uF34ReflashCmd_FirmwareWrite = 0x02;
static const u8 s_uF34ReflashCmd_EraseAll      = 0x03;
static const u8 s_uF34ReflashCmd_ConfigRead    = 0x05;
static const u8 s_uF34ReflashCmd_ConfigWrite   = 0x06;
static const u8 s_uF34ReflashCmd_ConfigErase   = 0x07;
static const u8 s_uF34ReflashCmd_Enable        = 0x0f;
static const u8 s_uF34ReflashCmd_NormalResult  = 0x80; //Read the Flash control register. The result should be 
                                                                  //$80: Flash Command = Idle ($0), Flash Status = Success ($0), 
                                                                  //Program Enabled = Enabled (1). Any other value indicates an error.


// Device
void RMI4ReadPageDescriptionTable(void);
void RMI4ResetDevice(void);
 
//**************************************************************
// This function polls the registers to ensure the device is in idled state
// Parameter errCount is the max times to read a register to make sure the state is ready
// Default number of checks is 3
//**************************************************************
//  bool UsePolling() { return (eAttention==EAttnNone) ? true : false; }

//void RMI4WaitATTN(int errorCount=300);
  
u8 RMI4ReadBootloadID(void);

u8 RMI4WriteBootloadID(void);
 
bool RMI4ValidateBootloadID(u16 bootloadID);
  
u8 RMI4IssueEnableFlashCommand(void);

u8 RMI4IssueEraseCommand(u8 *command);

u8 RMI4IssueFlashControlCommand( u8 *command);

// Configuration
void RMI4ReadConfigInfo(void);
 
// Firmware (UI)
//**************************************************************
// Determine firmware organization - read firmware block size and firmware size
//**************************************************************
void RMI4ReadFirmwareInfo(void);

//**************************************************************
// Write firmware to device one block at a time
//**************************************************************
u8 RMI4FlashFirmwareWrite(void);

//**************************************************************
// Calculates checksum for a given data block
//**************************************************************
void RMI4CalculateChecksum(u16 * data, u16 len, unsigned long *dataBlock);


void RMI4RMIInit(EProtocol pt, u8 i2cAddr, EAttention eAttn, unsigned int byteDelay,
                 unsigned int bitRate, unsigned long timeOut);

void RMI4RMIInitI2C(u8 i2cAddr, EAttention i2cAttn);

void RMI4RMIInitSPI(unsigned int byteDelay, unsigned int bitRate, unsigned long timeOut);
     
void RMI4WritePage(void);

// Flash entry and exit
void RMI4EnableFlashing(void);
  
//**************************************************************
// Resets device, wait for ATTN and ensure that $F01 flash prog is '0' - meaning
// the new firmware is valid and executing
//**************************************************************
u8 RMI4DisableFlash(void);


// Configuration
u16 GetConfigSize(void);
 
// Firmware (UI)
u16 GetFirmwareSize(void);
 
void RMI4ProgramFirmware(void);
  
void RMI4ProgramConfiguration(void);

// Image file
void RMI4Init(void);

// Device
bool RMI4isExpectedRegFormat(void);

void RMI4setFlashAddrForDifFormat(void);
#endif
