/* drivers/media/video/msm/mt9m113_reg.h 
*
* This software is for APTINA 1.3M sensor 
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

#include "mt9m113.h"
#include <linux/kernel.h>

/* chanhee.park@lge.com 
   temp : we define the delay time on MSM as 0xFFFE address 
*/
/************************************************************
; slave address 0x7A
;	MCK:32M	PCK:60M
;
; Preview : 640x480
; Capture : 1280x960
; Preview frame rate 22fps
; 
;************************************************************/
struct register_address_value_pair const mt9m113_reg_init_settings[] = {
//[Reset]
{0x001C, 0x0001}, 	// MCU_BOOT_MODE
{0x001C, 0x0000}, 	// MCU_BOOT_MODE
{0xFFFE,10},	//Delay=10 ms
//[Power up]
{0x0016, 0x00FF}, 	// CLOCKS_CONTROL
{0x0018, 0x0008}, 	// STANDBY_CONTROL
{0x301A, 0x12C8}, 	// RESERVED_CORE_301A
{0xFFFE,100},   //Delay=100 ms
{0x301A, 0x12CC}, 	// RESERVED_CORE_301A
// init start
//[start sequence]
{0xFFFE,300},     //DELAY=300 ms
{0x0016, 0x00FF}, 	// CLOCKS_CONTROL
{0x0018, 0x0028}, 	// STANDBY_CONTROL	
//[timming pll]
{0x0014, 0x2147}, 	// PLL_CONTROL
{0x0014, 0x2143}, 	// PLL_CONTROL
{0x0014, 0x2145}, 	// PLL_CONTROL
{0x0010, 0x031E}, //PLL Dividers = 2128
{0x0012, 0x1FF1}, //PLL P Dividers = 8177
{0x0014, 0x2545}, 	   //PLL control: TEST_BYPASS on = 9541
{0x0014, 0x2547}, 	   //PLL control: PLL_ENABLE on = 9543
{0x0014, 0x3447}, 	   //PLL control: SEL_LOCK_DET on = 13383
{0xFFFE, 0x000A}, 	   // Allow PLL to lock
{0x0014, 0x3047}, //PLL control: TEST_BYPASS off = 12359
{0x0014, 0x3046}, //PLL control: PLL_BYPASS off = 12358
{0xFFFE, 100}, //DELAY = 100 // Allow PLL to lock

//[timming]
{0x0014, 0x3047}, 	   //PLL control: TEST_BYPASS off = 12359
{0x0014, 0x3046}, 	   //PLL control: PLL_BYPASS off = 12358
{0x001E, 0x0702}, 		//Slew
{0x098C, 0x2703}, //Output Width (A)
{0x0990, 0x0280}, //		= 640
{0x098C, 0x2705}, //Output Height (A)
{0x0990, 0x01E0}, //		= 480
{0x098C, 0x2707}, //Output Width (B)
{0x0990, 0x0500}, //		= 1280
{0x098C, 0x2709}, //Output Height (B)
{0x0990, 0x03C0}, //		= 960
{0x098C, 0x270D}, //Row Start (A)
{0x0990, 0x0000}, //		= 0
{0x098C, 0x270F}, //Column Start (A)
{0x0990, 0x0000}, //		= 0
{0x098C, 0x2711}, //Row End (A)
{0x0990, 0x03CD}, //		= 973
{0x098C, 0x2713}, //Column End (A)
{0x0990, 0x050D}, //		= 1293
{0x098C, 0x2715}, //Row Speed (A)
{0x0990, 0x2111}, //		= 8465
{0x098C, 0x2717}, //Read Mode (A)
{0x0990, 0x046C}, //		= 1132
{0x098C, 0x2719}, //sensor_fine_correction (A)
{0x0990, 0x00AC}, //		= 172
{0x098C, 0x271B}, //sensor_fine_IT_min (A)
{0x0990, 0x01F1}, //		= 497
{0x098C, 0x271D}, //sensor_fine_IT_max_margin (A)
{0x0990, 0x013F}, //		= 319
{0x098C, 0x271F}, //Frame Lines (A)
{0x0990, 0x0239}, //		= 620
{0x098C, 0x2721}, //Line Length (A)
{0x0990, 0x095C}, //		= 2196
{0x098C, 0x2723}, //Row Start (B)
{0x0990, 0x0004}, //		= 4
{0x098C, 0x2725}, //Column Start (B)
{0x0990, 0x0004}, //		= 4
{0x098C, 0x2727}, //Row End (B)
{0x0990, 0x040B}, //		= 1035
{0x098C, 0x2729}, //Column End (B)
{0x0990, 0x050B}, //		= 1291
{0x098C, 0x272B}, //Row Speed (B)
{0x0990, 0x2111}, //		= 8465
{0x098C, 0x272D}, //Read Mode (B)
{0x0990, 0x0024}, //		= 36
{0x098C, 0x272F}, //sensor_fine_correction (B)
{0x0990, 0x004C}, //		= 76
{0x098C, 0x2731}, //sensor_fine_IT_min (B)
{0x0990, 0x00F9}, //		= 249
{0x098C, 0x2733}, //sensor_fine_IT_max_margin (B)
{0x0990, 0x00A7}, //		= 167
{0x098C, 0x2735}, //Frame Lines (B)
{0x0990, 0x045B}, //		= 1115
{0x098C, 0x2737}, //Line Length (B)
{0x0990, 0x0894}, //		= 2196
{0x098C, 0x2739}, //Crop_X0 (A)
{0x0990, 0x0000}, //		= 0
{0x098C, 0x273B}, //Crop_X1 (A)
{0x0990, 0x027F}, //		= 639
{0x098C, 0x273D}, //Crop_Y0 (A)
{0x0990, 0x0000}, //		= 0
{0x098C, 0x273F}, //Crop_Y1 (A)
{0x0990, 0x01DF}, //		= 479
{0x098C, 0x2747}, //Crop_X0 (B)
{0x0990, 0x0000}, //		= 0
{0x098C, 0x2749}, //Crop_X1 (B)
{0x0990, 0x04FF}, //		= 1279
{0x098C, 0x274B}, //Crop_Y0 (B)
{0x0990, 0x0000}, //		= 0
{0x098C, 0x274D}, //Crop_Y1 (B)
{0x0990, 0x03BF}, //		= 959 
{0x098C, 0x222D}, //R9 Step
{0x0990, 0x0068}, //		= 114
{0x098C, 0xA404}, //search_f1_50
{0x0990, 0x0010}, //		= 27
{0x098C, 0xA408}, //search_f2_50
{0x0990, 0x0019}, //		= 29
{0x098C, 0xA409}, //search_f1_60
{0x0990, 0x001B}, //		= 33
{0x098C, 0xA40A}, //search_f2_60
{0x0990, 0x001E}, //		= 35
{0x098C, 0xA40B}, //R9_Step_60 (A)
{0x0990, 0x0020}, //		= 113
{0x098C, 0x2411}, //R9_Step_50 (A)
{0x0990, 0x0068}, //		= 136
{0x098C, 0x2413}, //R9_Step_60 (B)
{0x0990, 0x007D}, //		= 113
{0x098C, 0x2415}, //R9_Step_50 (B)
{0x0990, 0x0072}, //		= 136
{0x098C, 0x2417}, //FD Mode
{0x0990, 0x0089}, //		= 16
{0x098C, 0xA40D}, //Stat_min
{0x0990, 0x0002}, //		= 2
{0x098C, 0xA40E}, //Stat_max
{0x0990, 0x0003}, //		= 3
{0x098C, 0xA410}, //Min_amplitude
{0x0990, 0x000A}, //		= 10
//[High Power Preview Mode]
{0x098C, 0x275F}, 	// MCU_ADDRESS [RESERVED_MODE_5F]
{0x0990, 0x0596}, 	// MCU_DATA_0
{0x098C, 0x2761}, 	// MCU_ADDRESS [RESERVED_MODE_61]
{0x0990, 0x00f2}, 	// MCU_DATA_0
//[pevuew 0 seq]
{0x098C, 0xA117}, 	// MCU_ADDRESS [SEQ_PREVIEW_1_AE]
{0x0990, 0x0002}, 	// MCU_DATA_0
{0x098C, 0xA118}, 	// MCU_ADDRESS [SEQ_PREVIEW_1_FD]
{0x0990, 0x0001}, 	// MCU_DATA_0
{0x098C, 0xA119}, 	// MCU_ADDRESS [SEQ_PREVIEW_1_AWB]
{0x0990, 0x0001}, 	// MCU_DATA_0
{0x098C, 0xA11a}, 	// MCU_ADDRESS [SEQ_PREVIEW_1_HG]
{0x0990, 0x0001}, 	// MCU_DATA_0
//[pevuew 1 seq]
{0x098C, 0xA11d}, 	// MCU_ADDRESS [SEQ_PREVIEW_1_AE]
{0x0990, 0x0002}, 	// MCU_DATA_0
{0x098C, 0xA11e}, 	// MCU_ADDRESS [SEQ_PREVIEW_1_FD]
{0x0990, 0x0001}, 	// MCU_DATA_0
{0x098C, 0xA11f}, 	// MCU_ADDRESS [SEQ_PREVIEW_1_AWB]
{0x0990, 0x0001}, 	// MCU_DATA_0
{0x098C, 0xA120}, 	// MCU_ADDRESS [SEQ_PREVIEW_1_HG]
{0x0990, 0x0001}, 	// MCU_DATA_0

//[Lens Correction 90% 12/14/09 13:43:28]
{0x364E, 0x01F0}, 	// P_GR_P0Q0
{0x3650, 0x650D}, 	// P_GR_P0Q1
{0x3652, 0x21B1}, 	// P_GR_P0Q2
{0x3654, 0x0F4D}, 	// P_GR_P0Q3
{0x3656, 0x972D}, 	// P_GR_P0Q4
{0x3658, 0x0230}, 	// P_RD_P0Q0
{0x365A, 0x98AE}, 	// P_RD_P0Q1
{0x365C, 0x26B1}, 	// P_RD_P0Q2
{0x365E, 0xDFEB}, 	// P_RD_P0Q3
{0x3660, 0x4F8E}, 	// P_RD_P0Q4
{0x3662, 0x0170}, 	// P_BL_P0Q0
{0x3664, 0x514D}, 	// P_BL_P0Q1
{0x3666, 0x05D1}, 	// P_BL_P0Q2
{0x3668, 0x032C}, 	// P_BL_P0Q3
{0x366A, 0x1EAA}, 	// P_BL_P0Q4
{0x366C, 0x02D0}, 	// P_GB_P0Q0
{0x366E, 0xA26E}, 	// P_GB_P0Q1
{0x3670, 0x1D51}, 	// P_GB_P0Q2
{0x3672, 0x84CD}, 	// P_GB_P0Q3
{0x3674, 0xD7AE}, 	// P_GB_P0Q4
{0x3676, 0x29EB}, 	// P_GR_P1Q0
{0x3678, 0x7D4C}, 	// P_GR_P1Q1
{0x367A, 0x762C}, 	// P_GR_P1Q2
{0x367C, 0xA7CC}, 	// P_GR_P1Q3
{0x367E, 0xA6CE}, 	// P_GR_P1Q4
{0x3680, 0x410C}, 	// P_RD_P1Q0
{0x3682, 0xABAC}, 	// P_RD_P1Q1
{0x3684, 0x77AD}, 	// P_RD_P1Q2
{0x3686, 0x38AC}, 	// P_RD_P1Q3
{0x3688, 0xD8AF}, 	// P_RD_P1Q4
{0x368A, 0xC14C}, 	// P_BL_P1Q0
{0x368C, 0xCD4C}, 	// P_BL_P1Q1
{0x368E, 0xA5CC}, 	// P_BL_P1Q2
{0x3690, 0x9E8D}, 	// P_BL_P1Q3
{0x3692, 0x84EF}, 	// P_BL_P1Q4
{0x3694, 0x8E0C}, 	// P_GB_P1Q0
{0x3696, 0x1A0D}, 	// P_GB_P1Q1
{0x3698, 0x84ED}, 	// P_GB_P1Q2
{0x369A, 0xB2AB}, 	// P_GB_P1Q3
{0x369C, 0xFD0E}, 	// P_GB_P1Q4
{0x369E, 0x6350}, 	// P_GR_P2Q0
{0x36A0, 0xB488}, 	// P_GR_P2Q1
{0x36A2, 0x6231}, 	// P_GR_P2Q2
{0x36A4, 0x500E}, 	// P_GR_P2Q3
{0x36A6, 0xC712}, 	// P_GR_P2Q4
{0x36A8, 0x0D51}, 	// P_RD_P2Q0
{0x36AA, 0xD24F}, 	// P_RD_P2Q1
{0x36AC, 0x1C12}, 	// P_RD_P2Q2
{0x36AE, 0x1350}, 	// P_RD_P2Q3
{0x36B0, 0x8213}, 	// P_RD_P2Q4
{0x36B2, 0x53F0}, 	// P_BL_P2Q0
{0x36B4, 0xA40B}, 	// P_BL_P2Q1
{0x36B6, 0x1971}, 	// P_BL_P2Q2
{0x36B8, 0x14AF}, 	// P_BL_P2Q3
{0x36BA, 0xF171}, 	// P_BL_P2Q4
{0x36BC, 0x6DD0}, 	// P_GB_P2Q0
{0x36BE, 0xEBAF}, 	// P_GB_P2Q1
{0x36C0, 0x6271}, 	// P_GB_P2Q2
{0x36C2, 0x5C4F}, 	// P_GB_P2Q3
{0x36C4, 0xB6F2}, 	// P_GB_P2Q4
{0x36C6, 0xA9AA}, 	// P_GR_P3Q0
{0x36C8, 0xD70A}, 	// P_GR_P3Q1
{0x36CA, 0x3A0E}, 	// P_GR_P3Q2
{0x36CC, 0x79CE}, 	// P_GR_P3Q3
{0x36CE, 0x74AC}, 	// P_GR_P3Q4
{0x36D0, 0x0809}, 	// P_RD_P3Q0
{0x36D2, 0xC78E}, 	// P_RD_P3Q1
{0x36D4, 0xE10E}, 	// P_RD_P3Q2
{0x36D6, 0x25AF}, 	// P_RD_P3Q3
{0x36D8, 0x1DAF}, 	// P_RD_P3Q4
{0x36DA, 0xE90D}, 	// P_BL_P3Q0
{0x36DC, 0xB64F}, 	// P_BL_P3Q1
{0x36DE, 0xF00F}, 	// P_BL_P3Q2
{0x36E0, 0x3950}, 	// P_BL_P3Q3
{0x36E2, 0x06D2}, 	// P_BL_P3Q4
{0x36E4, 0x826E}, 	// P_GB_P3Q0
{0x36E6, 0x682B}, 	// P_GB_P3Q1
{0x36E8, 0x9A8E}, 	// P_GB_P3Q2
{0x36EA, 0xAA2E}, 	// P_GB_P3Q3
{0x36EC, 0x5991}, 	// P_GB_P3Q4
{0x36EE, 0xC8EE}, 	// P_GR_P4Q0
{0x36F0, 0x43CF}, 	// P_GR_P4Q1
{0x36F2, 0x7AD0}, 	// P_GR_P4Q2
{0x36F4, 0x472F}, 	// P_GR_P4Q3
{0x36F6, 0xC075}, 	// P_GR_P4Q4
{0x36F8, 0x400D}, 	// P_RD_P4Q0
{0x36FA, 0x798C}, 	// P_RD_P4Q1
{0x36FC, 0x692E}, 	// P_RD_P4Q2
{0x36FE, 0x3F12}, 	// P_RD_P4Q3
{0x3700, 0xD075}, 	// P_RD_P4Q4
{0x3702, 0x44EB}, 	// P_BL_P4Q0
{0x3704, 0x362E}, 	// P_BL_P4Q1
{0x3706, 0x0672}, 	// P_BL_P4Q2
{0x3708, 0x0E71}, 	// P_BL_P4Q3
{0x370A, 0xB3D5}, 	// P_BL_P4Q4
{0x370C, 0xE86E}, 	// P_GB_P4Q0
{0x370E, 0x194E}, 	// P_GB_P4Q1
{0x3710, 0x5670}, 	// P_GB_P4Q2
{0x3712, 0x73D2}, 	// P_GB_P4Q3
{0x3714, 0xCCB5}, 	// P_GB_P4Q4
{0x3644, 0x0280}, 	// POLY_ORIGIN_C
{0x3642, 0x0224}, 	// POLY_ORIGIN_R
{0x3210, 0x01B8}, 	// COLOR_PIPELINE_CONTROL
//[Color correction matrices]
{0x098C, 0x2306},  	// MCU_ADDRESS [AWB_CCM_L_0]           
{0x0990, 0x00A4},  	// MCU_DATA_0                          
{0x098C, 0x2308},  	// MCU_ADDRESS [AWB_CCM_L_1]           
{0x0990, 0x00D4},  	// MCU_DATA_0                          
{0x098C, 0x230A},  	// MCU_ADDRESS [AWB_CCM_L_2]           
{0x0990, 0xFFBD},  	// MCU_DATA_0                          
{0x098C, 0x230C},  	// MCU_ADDRESS [AWB_CCM_L_3]           
{0x0990, 0xFF68},  	// MCU_DATA_0                          
{0x098C, 0x230E},  	// MCU_ADDRESS [AWB_CCM_L_4]           
{0x0990, 0x0284},  	// MCU_DATA_0                          
{0x098C, 0x2310},  	// MCU_ADDRESS [AWB_CCM_L_5]           
{0x0990, 0xFF70},  	// MCU_DATA_0                          
{0x098C, 0x2312},  	// MCU_ADDRESS [AWB_CCM_L_6]           
{0x0990, 0xFFE5},  	// MCU_DATA_0                          
{0x098C, 0x2314},  	// MCU_ADDRESS [AWB_CCM_L_7]           
{0x0990, 0xFF1B},  	// MCU_DATA_0                          
{0x098C, 0x2316},  	// MCU_ADDRESS [AWB_CCM_L_8]           
{0x0990, 0x0259},  	// MCU_DATA_0                          
{0x098C, 0x2318},  	// MCU_ADDRESS [AWB_CCM_L_9]           
{0x0990, 0x002B},  	// MCU_DATA_0                          
{0x098C, 0x231A},  	// MCU_ADDRESS [AWB_CCM_L_10]          
{0x0990, 0x0042},  	// MCU_DATA_0      
{0x098C, 0x231C},  	// MCU_ADDRESS [AWB_CCM_RL_0]          
{0x0990, 0x00CC},  	// MCU_DATA_0                          
{0x098C, 0x231E},  	// MCU_ADDRESS [AWB_CCM_RL_1]          
{0x0990, 0xFE3C},  	// MCU_DATA_0                          
{0x098C, 0x2320},  	// MCU_ADDRESS [AWB_CCM_RL_2]          
{0x0990, 0x00C5},  	// MCU_DATA_0                          
{0x098C, 0x2322},  	// MCU_ADDRESS [AWB_CCM_RL_3]          
{0x0990, 0x0059},  	// MCU_DATA_0                          
{0x098C, 0x2324},  	// MCU_ADDRESS [AWB_CCM_RL_4]          
{0x0990, 0xFF50},  	// MCU_DATA_0                          
{0x098C, 0x2326},  	// MCU_ADDRESS [AWB_CCM_RL_5]          
{0x0990, 0xFFF9},  	// MCU_DATA_0                          
{0x098C, 0x2328},  	// MCU_ADDRESS [AWB_CCM_RL_6]          
{0x0990, 0x005E},  	// MCU_DATA_0                          
{0x098C, 0x232A},  	// MCU_ADDRESS [AWB_CCM_RL_7]          
{0x0990, 0x0031},  	// MCU_DATA_0                          
{0x098C, 0x232C},  	// MCU_ADDRESS [AWB_CCM_RL_8]          
{0x0990, 0xFF18},  	// MCU_DATA_0                          
{0x098C, 0x232E},  	// MCU_ADDRESS [AWB_CCM_RL_9]          
{0x0990, 0x0008},  	// MCU_DATA_0                          
{0x098C, 0x2330},  	// MCU_ADDRESS [AWB_CCM_RL_10]         
{0x0990, 0xFFE7},  	// MCU_DATA_0      
{0x098C, 0xA348},  	// MCU_ADDRESS [AWB_GAIN_BUFFER_SPEED]    
{0x0990, 0x0008},  	// MCU_DATA_0                             
{0x098C, 0xA349},  	// MCU_ADDRESS [AWB_JUMP_DIVISOR]         
{0x0990, 0x0002},  	// MCU_DATA_0                             
{0x098C, 0xA34A},  	// MCU_ADDRESS [AWB_GAIN_MIN]             
{0x0990, 0x0059},  	// MCU_DATA_0                             
{0x098C, 0xA34B},  	// MCU_ADDRESS [AWB_GAIN_MAX]             
{0x0990, 0x00A6},  	// MCU_DATA_0                             
{0x098C, 0xA34C}, 	// MCU_ADDRESS [AWB_GAIN_MIN]
{0x0990, 0x0059}, 	// MCU_DATA_0
{0x098C, 0xA34D}, 	// MCU_ADDRESS [AWB_GAIN_MAX]
{0x0990, 0x00a6}, 	// MCU_DATA_0
{0x098C, 0xA351}, 	// MCU_ADDRESS [AWB_CCM_POSITION_MIN]
{0x0990, 0x0000}, 	// MCU_DATA_0
{0x098C, 0xA352}, 	// MCU_ADDRESS [AWB_CCM_POSITION_MAX]
{0x0990, 0x007F}, 	// MCU_DATA_0
{0x098C, 0xA354}, 	// MCU_ADDRESS [AWB_SATURATION]
{0x0990, 0x0043}, 	// MCU_DATA_0
{0x098C, 0xA355}, 	// MCU_ADDRESS [AWB_MODE]
{0x0990, 0x0002},  	// MCU_DATA_0                             
{0x098C, 0xA35D}, 	// MCU_ADDRESS [AWB_STEADY_BGAIN_OUT_MIN]
{0x0990, 0x0078}, 	// MCU_DATA_0
{0x098C, 0xA35E}, 	// MCU_ADDRESS [AWB_STEADY_BGAIN_OUT_MAX]
{0x0990, 0x0086}, 	// MCU_DATA_0
{0x098C, 0xA35F}, 	// MCU_ADDRESS [AWB_STEADY_BGAIN_IN_MIN]
{0x0990, 0x007E}, 	// MCU_DATA_0
{0x098C, 0xA360}, 	// MCU_ADDRESS [AWB_STEADY_BGAIN_IN_MAX]
{0x0990, 0x0082}, 	// MCU_DATA_0
{0x098C, 0xA365},  	// MCU_ADDRESS
{0x0990, 0x0010},  	// MCU_DATA_0

//[true gray]
{0x098C, 0xA363},  	// MCU_ADDRESS
{0x0990, 0x00BA},  	// MCU_DATA_0
{0x098C, 0xA364},  	// MCU_ADDRESS
{0x0990, 0x00F0},  	// MCU_DATA_0
//[K FACTOR]
{0x098C, 0xA366},  	// MCU_ADDRESS [AWB_KR_L]        
{0x0990, 0x007D},  	// MCU_DATA_0                    
{0x098C, 0xA367},  	// MCU_ADDRESS [AWB_KG_L]        
{0x0990, 0x0080},  	// MCU_DATA_0                    
{0x098C, 0xA368},  	// MCU_ADDRESS [AWB_KB_L]        
{0x0990, 0x0080},  	// MCU_DATA_0                    
{0x098C, 0xA369},  	// MCU_ADDRESS [AWB_KR_R]        
{0x0990, 0x007F},    //80,  	// (081111 84->7F)MCU_DATA_0     
{0x098C, 0xA36A},  	// MCU_ADDRESS [AWB_KG_R]        
{0x0990, 0x0077},  	// MCU_DATA_0                    
{0x098C, 0xA36B},  	// MCU_ADDRESS [AWB_KB_R]        
{0x0990, 0x0078},         //0x0080  	// MCU_DATA_0                     


//[AFD]
////Auto flicker detection
{0x098C, 0xA11E}, 	// MCU_ADDRESS
{0x0990, 0x0001}, 	// MCU_DATA_0
{0x098C, 0xA404}, 	// MCU_ADDRESS
{0x0990, 0x0000}, 	// MCU_DATA_0

//[virtgain]
{0x098C, 0xA20C}, 	// MCU_ADDRESS
{0x0990, 0x000C},     //0x0010  min 7.5fps   //0x000C min 10fps //0x0018 min5fps 	// MCU_DATA_0
{0x098C, 0xA20D}, 	// MCU_ADDRESS [AE_MIN_VIRTGAIN]
{0x0990, 0x0017}, 	// MCU_DATA_0
{0x098C, 0xA20E}, 	// MCU_ADDRESS [AE_MAX_VIRTGAIN]
{0x0990, 0x0080},    //0x0010     //0x0048 	// MCU_DATA_0
{0x098C, 0xA11D}, 	// MCU_ADDRESS
{0x0990, 0x0002}, 	// MCU_DATA_0


//[color kill]
{0x35a2, 0x00B1},	// 0x94  MCU_ADDRESS
{0x098C, 0x2761}, 	// MCU_ADDRESS [MODE_COMMONMODESETTINGS_FX_SEPIA_SETTINGS]
{0x0990, 0x00B1}, 	// MCU_DATA_0         
//[AE setting] 
{0x098C, 0xA207}, 	// MCU_ADDRESS [AE_GATE]
{0x0990, 0x0004}, 	// MCU_DATA_0
{0x098C, 0xA24F}, 	// MCU_ADDRESS
{0x0990, 0x0040}, 	// MCU_DATA_0
{0x098C, 0x2257}, 	// MCU_ADDRESS [RESERVED_AE_57]
{0x0990, 0x2710}, 	// MCU_DATA_0
{0x098C, 0x2250}, 	// MCU_ADDRESS [RESERVED_AE_50]
{0x0990, 0x1B58}, 	// MCU_DATA_0
{0x098C, 0x2252}, 	// MCU_ADDRESS [RESERVED_AE_52]
{0x0990, 0x32C8}, 	// MCU_DATA_0
{0x098C, 0x2212}, 	// MCU_ADDRESS [AE_MAX_DGAIN_AE1]
{0x0990, 0x0180},	//060	//4E 	// MCU_DATA_0
{0x098C, 0xA20C}, 	// MCU_ADDRESS [AE_MAX_INDEX]
{0x0990, 0x0010}, 	// MCU_DATA_0
{0x098C, 0xA115}, 	// MCU_ADDRESS [SEQ_CAP_MODE]
{0x0990, 0x0002}, 	// MCU_DATA_0  

//[black gamma contrast]
{0x098C, 0x2B1B}, 	// MCU_ADDRESS [HG_BRIGHTNESSMETRIC]
{0x0990, 0x0643}, 	// MCU_DATA_0
{0x098C, 0xAB37}, 	// MCU_ADDRESS [HG_GAMMA_MORPH_CTRL]
{0x0990, 0x0003}, 	// MCU_DATA_0
{0x098C, 0x2B38}, 	// MCU_ADDRESS [HG_GAMMASTARTMORPH]
{0x0990, 0x2800},	        // MCU_DATA_0
{0x098C, 0x2B3A}, 	// MCU_ADDRESS [HG_GAMMASTOPMORPH]
{0x0990, 0x46fe}, 	// MCU_DATA_0
//black 06
{0x098C, 0xAB3C}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_A_0]
{0x0990, 0x0000}, 	// MCU_DATA_0
{0x098C, 0xAB3D}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_A_1]
{0x0990, 0x0008}, 	// MCU_DATA_0
{0x098C, 0xAB3E}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_A_2]
{0x0990, 0x0019}, 	// MCU_DATA_0
{0x098C, 0xAB3F}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_A_3]
{0x0990, 0x0035}, 	// MCU_DATA_0
{0x098C, 0xAB40}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_A_4]
{0x0990, 0x0056}, 	// MCU_DATA_0
{0x098C, 0xAB41}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_A_5]
{0x0990, 0x006F}, 	// MCU_DATA_0
{0x098C, 0xAB42}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_A_6]
{0x0990, 0x0085}, 	// MCU_DATA_0
{0x098C, 0xAB43}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_A_7]
{0x0990, 0x0098}, 	// MCU_DATA_0
{0x098C, 0xAB44}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_A_8]
{0x0990, 0x00A7}, 	// MCU_DATA_0
{0x098C, 0xAB45}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_A_9]
{0x0990, 0x00B4}, 	// MCU_DATA_0
{0x098C, 0xAB46}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_A_10]
{0x0990, 0x00C0}, 	// MCU_DATA_0
{0x098C, 0xAB47}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_A_11]
{0x0990, 0x00CA}, 	// MCU_DATA_0
{0x098C, 0xAB48}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_A_12]
{0x0990, 0x00D4}, 	// MCU_DATA_0
{0x098C, 0xAB49}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_A_13]
{0x0990, 0x00DC}, 	// MCU_DATA_0
{0x098C, 0xAB4A}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_A_14]
{0x0990, 0x00E4}, 	// MCU_DATA_0
{0x098C, 0xAB4B}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_A_15]
{0x0990, 0x00EC}, 	// MCU_DATA_0
{0x098C, 0xAB4C}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_A_16]
{0x0990, 0x00F3}, 	// MCU_DATA_0
{0x098C, 0xAB4D}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_A_17]
{0x0990, 0x00F9}, 	// MCU_DATA_0
{0x098C, 0xAB4E}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_A_18]
{0x0990, 0x00FF}, 	// MCU_DATA_0
//Black 05 contrast 1.35
{0x098C, 0xAB4F}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_0]
{0x0990, 0x0000}, 	// MCU_DATA_0
{0x098C, 0xAB50}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_1]
{0x0990, 0x0006}, 	// MCU_DATA_0
{0x098C, 0xAB51}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_2]
{0x0990, 0x0012}, 	// MCU_DATA_0
{0x098C, 0xAB52}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_3]
{0x0990, 0x002F}, 	// MCU_DATA_0
{0x098C, 0xAB53}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_4]
{0x0990, 0x0053}, 	// MCU_DATA_0
{0x098C, 0xAB54}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_5]
{0x0990, 0x006D}, 	// MCU_DATA_0
{0x098C, 0xAB55}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_6]
{0x0990, 0x0083}, 	// MCU_DATA_0
{0x098C, 0xAB56}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_7]
{0x0990, 0x0096}, 	// MCU_DATA_0
{0x098C, 0xAB57}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_8]
{0x0990, 0x00A6}, 	// MCU_DATA_0
{0x098C, 0xAB58}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_9]
{0x0990, 0x00B3}, 	// MCU_DATA_0
{0x098C, 0xAB59}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_10]
{0x0990, 0x00BF}, 	// MCU_DATA_0
{0x098C, 0xAB5A}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_11]
{0x0990, 0x00CA}, 	// MCU_DATA_0
{0x098C, 0xAB5B}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_12]
{0x0990, 0x00D3}, 	// MCU_DATA_0
{0x098C, 0xAB5C}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_13]
{0x0990, 0x00DC}, 	// MCU_DATA_0
{0x098C, 0xAB5D}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_14]
{0x0990, 0x00E4}, 	// MCU_DATA_0
{0x098C, 0xAB5E}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_15]
{0x0990, 0x00EB}, 	// MCU_DATA_0
{0x098C, 0xAB5F}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_16]
{0x0990, 0x00F2}, 	// MCU_DATA_0
{0x098C, 0xAB60}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_17]
{0x0990, 0x00F9}, 	// MCU_DATA_0
{0x098C, 0xAB61}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_18]
{0x0990, 0x00FF}, 	// MCU_DATA_0
{0x098C, 0xAB04}, 	// MCU_ADDRESS [HG_MAX_DLEVEL]
{0x0990, 0x0040}, 	// MCU_DATA_0
{0x098C, 0xAB06}, 	// MCU_ADDRESS [HG_PERCENT]
{0x0990, 0x0005},		// 3, //0x000A 	// MCU_DATA_0
{0x098C, 0xAB08}, 	// MCU_ADDRESS [HG_DLEVEL]
{0x0990, 0x0010}, 	// MCU_DATA_0
{0x098C, 0x2B28}, 	// MCU_ADDRESS [HG_LL_BRIGHTNESSSTART]
{0x0990, 0x157c},        //0x0a00 //0x157c  //0x0A00 	// MCU_DATA_0
{0x098C, 0x2B2A}, 	// MCU_ADDRESS [HG_LL_BRIGHTNESSSTOP]
{0x0990, 0x37ef},        //0x7000 	// MCU_DATA_0
{0x098C, 0x2755}, //{0x098C, 0x2755   // MCU_ADDRESS [MODE_OUTPUT_FORMAT_A]
{0x0990, 0x0002}, //{0x0990, 0x0002  // MCU_DATA_0
{0x098C, 0x2757}, //{0x098C, 0x2757  // MCU_ADDRESS [MODE_OUTPUT_FORMAT_B]
{0x0990, 0x0002}, //{0x0990, 0x0002   // MCU_DATA_0
//[NR ]
{0x098C, 0xAB20}, 	// MCU_ADDRESS [HG_LL_SAT1]
{0x0990, 0x0063}, 	// MCU_DATA_0
{0x098C, 0xAB21}, 	// MCU_ADDRESS [HG_LL_INTERPTHRESH1]
{0x0990, 0x001D}, 	// MCU_DATA_0
{0x098C, 0xAB22}, 	// MCU_ADDRESS [HG_LL_APCORR1]
{0x0990, 0x0007}, 	// MCU_DATA_0
{0x098C, 0xAB23}, 	// MCU_ADDRESS [HG_LL_APTHRESH1]
{0x0990, 0x0004}, 	// MCU_DATA_0
{0x098C, 0xAB24}, 	// MCU_ADDRESS [HG_LL_SAT2]
{0x0990, 0x0000}, 	// MCU_DATA_0
{0x098C, 0xAB25}, 	// MCU_ADDRESS [HG_LL_INTERPTHRESH2]
{0x0990, 0x00a0}, 	// MCU_DATA_0
{0x098C, 0xAB26}, 	// MCU_ADDRESS [HG_LL_APCORR2]
{0x0990, 0x0003}, 	// MCU_DATA_0
{0x098C, 0xAB27}, 	// MCU_ADDRESS [HG_LL_APTHRESH2]
{0x0990, 0x0010}, 	// MCU_DATA_0
{0x098C, 0xAB2C}, 	// MCU_ADDRESS [HG_NR_START_R]
{0x0990, 0x0006}, 	// MCU_DATA_0
{0x098C, 0xAB2D}, 	// MCU_ADDRESS [HG_NR_START_G]
{0x0990, 0x000E}, 	// MCU_DATA_0
{0x098C, 0xAB2E}, 	// MCU_ADDRESS [HG_NR_START_B]
{0x0990, 0x0006}, 	// MCU_DATA_0
{0x098C, 0xAB2F}, 	// MCU_ADDRESS [HG_NR_START_OL]
{0x0990, 0x0006}, 	// MCU_DATA_0
{0x098C, 0xAB30}, 	// MCU_ADDRESS [HG_NR_STOP_R]
{0x0990, 0x001E}, 	// MCU_DATA_0
{0x098C, 0xAB31}, 	// MCU_ADDRESS [HG_NR_STOP_G]
{0x0990, 0x000E}, 	// MCU_DATA_0
{0x098C, 0xAB32}, 	// MCU_ADDRESS [HG_NR_STOP_B]
{0x0990, 0x001E}, 	// MCU_DATA_0
{0x098C, 0xAB33}, 	// MCU_ADDRESS [HG_NR_STOP_OL]
{0x0990, 0x001E}, 	// MCU_DATA_0
{0x098C, 0xAB34}, 	// MCU_ADDRESS
{0x0990, 0x0008},   //0x001E 	// MCU_DATA_0
{0x098C, 0xAB35}, 	// MCU_ADDRESS
{0x0990, 0x0080},   //0x004D 	// MCU_DATA_0
{0x328E, 0x000C}, 	// THRESH_EDGE_DETECT
//[AE window]
{0x098C, 0xA202}, 	// MCU_ADDRESS [AE_WINDOW_POS]
{0x0990, 0x0022}, 	// MCU_DATA_0
{0x098C, 0xA203}, 	// MCU_ADDRESS [AE_WINDOW_SIZE]
{0x0990, 0x00BB}, 	// MCU_DATA_0

//[Refresh] --> refresh code move to init write part.	
{0x098C,0xA103},//{0x098C, 0xA103},	// MCU_ADDRESS [SEQ_CMD]
{0x0990,0x0006},//{0x0990, 0x0006},	// MCU_DATA_0
{0xFFFE,200},   //Delay=200
{0x098C, 0xA103},//{0x098C, 0xA103},	// MCU_ADDRESS [SEQ_CMD]
{0x0990, 0x0005},//{0x0990, 0x0005},	// MCU_DATA_0
{0xFFFE,200}   //Delay=200
};


/************************************************************
; slave address 0x7A
;	MCK:31M	PCK:60M
;
; Preview : 640x480
; Capture : 1280x960
; Preview frame rate MAX. 30fps (variable frame rate)
; 
;************************************************************/
struct register_address_value_pair const mt9m113_reg_init_vt_settings[] = {
//[Reset]
{0x001C, 0x0001}, 	// MCU_BOOT_MODE
{0x001C, 0x0000}, 	// MCU_BOOT_MODE
{0xFFFE,10},	//Delay=10 ms
//[Power up]
{0x0016, 0x00FF}, 	// CLOCKS_CONTROL
{0x0018, 0x0008}, 	// STANDBY_CONTROL
{0x301A, 0x12C8}, 	// RESERVED_CORE_301A
{0xFFFE,100},   //Delay=100 ms
{0x301A, 0x12CC}, 	// RESERVED_CORE_301A
// init start
//[start sequence]
{0xFFFE,100},
{0xFFFE,100},
{0xFFFE,100},//DELAY=300 ms
{0x0016, 0x00FF}, 	// CLOCKS_CONTROL
{0x0018, 0x0028}, 	// STANDBY_CONTROL	
//[timming pll]
{0x0014, 0x2147}, 	// PLL_CONTROL
{0x0014, 0x2143}, 	// PLL_CONTROL
{0x0014, 0x2145}, 	// PLL_CONTROL
{0x0010, 0x0E74}, //PLL Dividers = 2128
{0x0012, 0x1FF1}, //PLL P Dividers = 8177
{0x0014, 0x2545}, 	   //PLL control: TEST_BYPASS on = 9541
{0x0014, 0x2547}, 	   //PLL control: PLL_ENABLE on = 9543
{0x0014, 0x3447}, 	   //PLL control: SEL_LOCK_DET on = 13383
{0xFFFE, 0x000A}, 	   // Allow PLL to lock
{0x0014, 0x3047}, //PLL control: TEST_BYPASS off = 12359
{0x0014, 0x3046}, //PLL control: PLL_BYPASS off = 12358
{0xFFFE, 100}, //DELAY = 100 // Allow PLL to lock
//[timming]
{0x0014, 0x3047}, 	   //PLL control: TEST_BYPASS off = 12359
{0x0014, 0x3046}, 	   //PLL control: PLL_BYPASS off = 12358
{0x001E, 0x0702}, 		//Slew
{0x098C, 0x2703}, //Output Width (A)
{0x0990, 0x0280}, //		= 640
{0x098C, 0x2705}, //Output Height (A)
{0x0990, 0x01E0}, //		= 480
{0x098C, 0x2707}, //Output Width (B)
{0x0990, 0x0500}, //		= 1280
{0x098C, 0x2709}, //Output Height (B)
{0x0990, 0x03C0}, //		= 960
{0x098C, 0x270D}, //Row Start (A)
{0x0990, 0x0000}, //		= 0
{0x098C, 0x270F}, //Column Start (A)
{0x0990, 0x0000}, //		= 0
{0x098C, 0x2711}, //Row End (A)
{0x0990, 0x03CD}, //		= 973
{0x098C, 0x2713}, //Column End (A)
{0x0990, 0x050D}, //		= 1293
{0x098C, 0x2715}, //Row Speed (A)
{0x0990, 0x2111}, //		= 8465
{0x098C, 0x2717}, //Read Mode (A)
{0x0990, 0x046C}, //		= 1132
{0x098C, 0x2719}, //sensor_fine_correction (A)
{0x0990, 0x00AC}, //		= 172
{0x098C, 0x271B}, //sensor_fine_IT_min (A)
{0x0990, 0x01F1}, //		= 497
{0x098C, 0x271D}, //sensor_fine_IT_max_margin (A)
{0x0990, 0x013F}, //		= 319
{0x098C, 0x271F}, //Frame Lines (A)
{0x0990, 0x0239}, //		= 620
{0x098C, 0x2721}, //Line Length (A)
{0x0990, 0x06DB}, //		= 2196
{0x098C, 0x2723}, //Row Start (B)
{0x0990, 0x0004}, //		= 4
{0x098C, 0x2725}, //Column Start (B)
{0x0990, 0x0004}, //		= 4
{0x098C, 0x2727}, //Row End (B)
{0x0990, 0x040B}, //		= 1035
{0x098C, 0x2729}, //Column End (B)
{0x0990, 0x050B}, //		= 1291
{0x098C, 0x272B}, //Row Speed (B)
{0x0990, 0x2111}, //		= 8465
{0x098C, 0x272D}, //Read Mode (B)
{0x0990, 0x0024}, //		= 36
{0x098C, 0x272F}, //sensor_fine_correction (B)
{0x0990, 0x004C}, //		= 76
{0x098C, 0x2731}, //sensor_fine_IT_min (B)
{0x0990, 0x00F9}, //		= 249
{0x098C, 0x2733}, //sensor_fine_IT_max_margin (B)
{0x0990, 0x00A7}, //		= 167
{0x098C, 0x2735}, //Frame Lines (B)
{0x0990, 0x045B}, //		= 1115
{0x098C, 0x2737}, //Line Length (B)
{0x0990, 0x0891}, //		= 2196
{0x098C, 0x2739}, //Crop_X0 (A)
{0x0990, 0x0000}, //		= 0
{0x098C, 0x273B}, //Crop_X1 (A)
{0x0990, 0x027F}, //		= 639
{0x098C, 0x273D}, //Crop_Y0 (A)
{0x0990, 0x0000}, //		= 0
{0x098C, 0x273F}, //Crop_Y1 (A)
{0x0990, 0x01DF}, //		= 479
{0x098C, 0x2747}, //Crop_X0 (B)
{0x0990, 0x0000}, //		= 0
{0x098C, 0x2749}, //Crop_X1 (B)
{0x0990, 0x04FF}, //		= 1279
{0x098C, 0x274B}, //Crop_Y0 (B)
{0x0990, 0x0000}, //		= 0
{0x098C, 0x274D}, //Crop_Y1 (B)
{0x0990, 0x03FF}, //		= 959
{0x098C, 0x222D}, //R9 Step
{0x0990, 0x008E}, //		= 114
{0x098C, 0xA404}, //search_f1_50
{0x0990, 0x0010}, //		= 27
{0x098C, 0xA408}, //search_f2_50
{0x0990, 0x0022}, //		= 29
{0x098C, 0xA409}, //search_f1_60
{0x0990, 0x0024}, //		= 33
{0x098C, 0xA40A}, //search_f2_60
{0x0990, 0x0029}, //		= 35
{0x098C, 0xA40B}, //R9_Step_60 (A)
{0x0990, 0x002B}, //		= 113
{0x098C, 0x2411}, //R9_Step_50 (A)
{0x0990, 0x008E}, //		= 136
{0x098C, 0x2413}, //R9_Step_60 (B)
{0x0990, 0x00AB}, //		= 113
{0x098C, 0x2415}, //R9_Step_50 (B)
{0x0990, 0x0072}, //		= 136
{0x098C, 0x2417}, //FD Mode
{0x0990, 0x0089}, //		= 16
{0x098C, 0xA40D}, //Stat_min
{0x0990, 0x0002}, //		= 2
{0x098C, 0xA40E}, //Stat_max
{0x0990, 0x0003}, //		= 3
{0x098C, 0xA410}, //Min_amplitude
{0x0990, 0x000A}, //		= 10
//[High Power Preview Mode]
{0x098C, 0x275F}, 	// MCU_ADDRESS [RESERVED_MODE_5F]
{0x0990, 0x0596}, 	// MCU_DATA_0
{0x098C, 0x2761}, 	// MCU_ADDRESS [RESERVED_MODE_61]
{0x0990, 0x0093}, 	// MCU_DATA_0
//[pevuew 0 seq]
{0x098C, 0xA111}, 	// MCU_ADDRESS re vsync
{0x0990, 0x000a}, 	// MCU_DATA_0
{0x098C, 0xA117}, 	// MCU_ADDRESS [SEQ_PREVIEW_1_AE]
{0x0990, 0x0002}, 	// MCU_DATA_0
{0x098C, 0xA118}, 	// MCU_ADDRESS [SEQ_PREVIEW_1_FD]
{0x0990, 0x0001}, 	// MCU_DATA_0
{0x098C, 0xA119}, 	// MCU_ADDRESS [SEQ_PREVIEW_1_AWB]
{0x0990, 0x0001}, 	// MCU_DATA_0
{0x098C, 0xA11a}, 	// MCU_ADDRESS [SEQ_PREVIEW_1_HG]
{0x0990, 0x0001}, 	// MCU_DATA_0
//[pevuew 1 seq]
{0x098C, 0xA11d}, 	// MCU_ADDRESS [SEQ_PREVIEW_1_AE]
{0x0990, 0x0002}, 	// MCU_DATA_0
{0x098C, 0xA11e}, 	// MCU_ADDRESS [SEQ_PREVIEW_1_FD]
{0x0990, 0x0001}, 	// MCU_DATA_0
{0x098C, 0xA11f}, 	// MCU_ADDRESS [SEQ_PREVIEW_1_AWB]
{0x0990, 0x0001}, 	// MCU_DATA_0
{0x098C, 0xA120}, 	// MCU_ADDRESS [SEQ_PREVIEW_1_HG]
{0x0990, 0x0001}, 	// MCU_DATA_0	
//offset
{0x098C, 0xA75D}, 	// MCU_ADDRESS [MODE_Y_RGB_OFFSET_A]
{0x0990, 0x0010}, //0x0010, //0x0010	// MCU_DATA_0
{0x098C, 0xA75E}, // MCU_ADDRESS [MODE_Y_RGB_OFFSET_B]
{0x0990, 0x0010}, //0x0010,//0x0010	// MCU_DATA_0
//[Lens Correction 95% 11/26/09 12:23:07]
{0x364E, 0x0730}, 	// P_GR_P0Q0
{0x3650, 0x1E4E}, 	// P_GR_P0Q1
{0x3652, 0x14F1}, 	// P_GR_P0Q2
{0x3654, 0x32CF}, 	// P_GR_P0Q3
{0x3656, 0x734F}, 	// P_GR_P0Q4
{0x3658, 0x0330}, 	// P_RD_P0Q0
{0x365A, 0xF1AD}, 	// P_RD_P0Q1
{0x365C, 0x2731}, 	// P_RD_P0Q2
{0x365E, 0x550E}, 	// P_RD_P0Q3
{0x3660, 0x6F8F}, 	// P_RD_P0Q4
{0x3662, 0x0250}, 	// P_BL_P0Q0
{0x3664, 0x01AE}, 	// P_BL_P0Q1
{0x3666, 0x7FD0}, 	// P_BL_P0Q2
{0x3668, 0x03EF}, 	// P_BL_P0Q3
{0x366A, 0x442F}, 	// P_BL_P0Q4
{0x366C, 0x0250}, 	// P_GB_P0Q0
{0x366E, 0x82EE}, 	// P_GB_P0Q1
{0x3670, 0x0591}, 	// P_GB_P0Q2
{0x3672, 0x128E}, 	// P_GB_P0Q3
{0x3674, 0x540F}, 	// P_GB_P0Q4
{0x3676, 0x650D}, 	// P_GR_P1Q0
{0x3678, 0x714C}, 	// P_GR_P1Q1
{0x367A, 0x272E}, 	// P_GR_P1Q2
{0x367C, 0x09CA}, 	// P_GR_P1Q3
{0x367E, 0xD12E}, 	// P_GR_P1Q4
{0x3680, 0x652D}, 	// P_RD_P1Q0
{0x3682, 0x9C0D}, 	// P_RD_P1Q1
{0x3684, 0x172F}, 	// P_RD_P1Q2
{0x3686, 0xB64D}, 	// P_RD_P1Q3
{0x3688, 0xB3EF}, 	// P_RD_P1Q4
{0x368A, 0x1F6D}, 	// P_BL_P1Q0
{0x368C, 0xAA2D}, 	// P_BL_P1Q1
{0x368E, 0x68CA}, 	// P_BL_P1Q2
{0x3690, 0x300D}, 	// P_BL_P1Q3
{0x3692, 0x826E}, 	// P_BL_P1Q4
{0x3694, 0x13CD}, 	// P_GB_P1Q0
{0x3696, 0x37AB}, 	// P_GB_P1Q1
{0x3698, 0x2B2D}, 	// P_GB_P1Q2
{0x369A, 0xB52A}, 	// P_GB_P1Q3
{0x369C, 0x874F}, 	// P_GB_P1Q4
{0x369E, 0x5390}, 	// P_GR_P2Q0
{0x36A0, 0x0370}, 	// P_GR_P2Q1
{0x36A2, 0x3812}, 	// P_GR_P2Q2
{0x36A4, 0xFAB0}, 	// P_GR_P2Q3
{0x36A6, 0xBE13}, 	// P_GR_P2Q4
{0x36A8, 0x7F90}, 	// P_RD_P2Q0
{0x36AA, 0xA5AC}, 	// P_RD_P2Q1
{0x36AC, 0x0393}, 	// P_RD_P2Q2
{0x36AE, 0x1DAD}, 	// P_RD_P2Q3
{0x36B0, 0xF693}, 	// P_RD_P2Q4
{0x36B2, 0x3C90}, 	// P_BL_P2Q0
{0x36B4, 0x5CCF}, 	// P_BL_P2Q1
{0x36B6, 0x3E72}, 	// P_BL_P2Q2
{0x36B8, 0x8F70}, 	// P_BL_P2Q3
{0x36BA, 0xABB3}, 	// P_BL_P2Q4
{0x36BC, 0x5390}, 	// P_GB_P2Q0
{0x36BE, 0x06AE}, 	// P_GB_P2Q1
{0x36C0, 0x23F2}, 	// P_GB_P2Q2
{0x36C2, 0x9FB0}, 	// P_GB_P2Q3
{0x36C4, 0x93F3}, 	// P_GB_P2Q4
{0x36C6, 0xA0AA}, 	// P_GR_P3Q0
{0x36C8, 0x43CD}, 	// P_GR_P3Q1
{0x36CA, 0x4E0A}, 	// P_GR_P3Q2
{0x36CC, 0xE750}, 	// P_GR_P3Q3
{0x36CE, 0xDAF1}, 	// P_GR_P3Q4
{0x36D0, 0x50EB}, 	// P_RD_P3Q0
{0x36D2, 0x88CB}, 	// P_RD_P3Q1
{0x36D4, 0xAE0C}, 	// P_RD_P3Q2
{0x36D6, 0x256E}, 	// P_RD_P3Q3
{0x36D8, 0xA36F}, 	// P_RD_P3Q4
{0x36DA, 0xC06E}, 	// P_BL_P3Q0
{0x36DC, 0x844D}, 	// P_BL_P3Q1
{0x36DE, 0x8CED}, 	// P_BL_P3Q2
{0x36E0, 0x9110}, 	// P_BL_P3Q3
{0x36E2, 0xADD1}, 	// P_BL_P3Q4
{0x36E4, 0xEA2D}, 	// P_GB_P3Q0
{0x36E6, 0x1BAE}, 	// P_GB_P3Q1
{0x36E8, 0xD86E}, 	// P_GB_P3Q2
{0x36EA, 0xBF0D}, 	// P_GB_P3Q3
{0x36EC, 0xED30}, 	// P_GB_P3Q4
{0x36EE, 0x224F}, 	// P_GR_P4Q0
{0x36F0, 0xCC10}, 	// P_GR_P4Q1
{0x36F2, 0xAAB2}, 	// P_GR_P4Q2
{0x36F4, 0x510E}, 	// P_GR_P4Q3
{0x36F6, 0xDC54}, 	// P_GR_P4Q4
{0x36F8, 0x29B0}, 	// P_RD_P4Q0
{0x36FA, 0xE5ED}, 	// P_RD_P4Q1
{0x36FC, 0x90F3}, 	// P_RD_P4Q2
{0x36FE, 0xA811}, 	// P_RD_P4Q3
{0x3700, 0xE814}, 	// P_RD_P4Q4
{0x3702, 0x3810}, 	// P_BL_P4Q0
{0x3704, 0xFE2F}, 	// P_BL_P4Q1
{0x3706, 0xA1B2}, 	// P_BL_P4Q2
{0x3708, 0x328D}, 	// P_BL_P4Q3
{0x370A, 0xBC34}, 	// P_BL_P4Q4
{0x370C, 0x2D4F}, 	// P_GB_P4Q0
{0x370E, 0xEA70}, 	// P_GB_P4Q1
{0x3710, 0xBCF1}, 	// P_GB_P4Q2
{0x3712, 0x7BD1}, 	// P_GB_P4Q3
{0x3714, 0xED54}, 	// P_GB_P4Q4
{0x3644, 0x0280}, 	// POLY_ORIGIN_C
{0x3642, 0x01C0}, 	// POLY_ORIGIN_R
{0x3210, 0x01A8}, 	// COLOR_PIPELINE_CONTROL
//[Color correction matrices]
{0x098C, 0x2306}, 	// MCU_ADDRESS [AWB_CCM_L_0]
{0x0990, 0x00A4}, 	// MCU_DATA_0
{0x098C, 0x2308}, 	// MCU_ADDRESS [AWB_CCM_L_1]
{0x0990, 0x00D4}, 	// MCU_DATA_0
{0x098C, 0x230A}, 	// MCU_ADDRESS [AWB_CCM_L_2]
{0x0990, 0xFFBD}, 	// MCU_DATA_0
{0x098C, 0x230C}, 	// MCU_ADDRESS [AWB_CCM_L_3]
{0x0990, 0xFF68}, 	// MCU_DATA_0
{0x098C, 0x230E}, 	// MCU_ADDRESS [AWB_CCM_L_4]
{0x0990, 0x0284}, 	// MCU_DATA_0
{0x098C, 0x2310}, 	// MCU_ADDRESS [AWB_CCM_L_5]
{0x0990, 0xFF70}, 	// MCU_DATA_0
{0x098C, 0x2312}, 	// MCU_ADDRESS [AWB_CCM_L_6]
{0x0990, 0xFFE5}, 	// MCU_DATA_0
{0x098C, 0x2314}, 	// MCU_ADDRESS [AWB_CCM_L_7]
{0x0990, 0xFF1B}, 	// MCU_DATA_0
{0x098C, 0x2316}, 	// MCU_ADDRESS [AWB_CCM_L_8]
{0x0990, 0x0259}, 	// MCU_DATA_0
{0x098C, 0x2318}, 	// MCU_ADDRESS [AWB_CCM_L_9]
{0x0990, 0x002B}, 	// MCU_DATA_0
{0x098C, 0x231A}, 	// MCU_ADDRESS [AWB_CCM_L_10]
{0x0990, 0x0042}, 	// MCU_DATA_0
{0x098C, 0x231C}, 	// MCU_ADDRESS [AWB_CCM_RL_0]		   
{0x0990, 0x002D}, 	// MCU_DATA_0
{0x098C, 0x231E}, 	// MCU_ADDRESS
{0x0990, 0xFF32}, 	// MCU_DATA_0
{0x098C, 0x2320}, 	// MCU_ADDRESS
{0x0990, 0x0045}, 	// MCU_DATA_0
{0x098C, 0x2322}, 	// MCU_ADDRESS
{0x0990, 0x0034}, 	// MCU_DATA_0
{0x098C, 0x2324}, 	// MCU_ADDRESS
{0x0990, 0xFF52}, 	// MCU_DATA_0
{0x098C, 0x2326}, 	// MCU_ADDRESS
{0x0990, 0x002A}, 	// MCU_DATA_0
{0x098C, 0x2328}, 	// MCU_ADDRESS
{0x0990, 0xFFF8}, 	// MCU_DATA_0
{0x098C, 0x232A}, 	// MCU_ADDRESS
{0x0990, 0x0052}, 	// MCU_DATA_0
{0x098C, 0x232C}, 	// MCU_ADDRESS
{0x0990, 0xFF61}, 	// MCU_DATA_0
{0x098C, 0x232E}, 	// MCU_ADDRESS [AWB_CCM_RL_9]
{0x0990, 0x0008}, 	// MCU_DATA_0
{0x098C, 0x2330}, 	// MCU_ADDRESS [AWB_CCM_RL_10]
{0x0990, 0xFFE7}, 	// MCU_DATA_0
{0x098C, 0xA348}, 	// MCU_ADDRESS [AWB_GAIN_BUFFER_SPEED]
{0x0990, 0x0008}, 	// MCU_DATA_0
{0x098C, 0xA349}, 	// MCU_ADDRESS [AWB_JUMP_DIVISOR]
{0x0990, 0x0002}, 	// MCU_DATA_0
{0x098C, 0xA34A}, 	// MCU_ADDRESS [AWB_GAIN_MIN]
{0x0990, 0x0059}, 	// MCU_DATA_0
{0x098C, 0xA34B}, 	// MCU_ADDRESS [AWB_GAIN_MAX]
{0x0990, 0x00A6}, 	// MCU_DATA_0
{0x098C, 0xA34C}, 	// MCU_ADDRESS [AWB_GAIN_MIN]
{0x0990, 0x0059}, 	// MCU_DATA_0
{0x098C, 0xA34D}, 	// MCU_ADDRESS [AWB_GAIN_MAX]
{0x0990, 0x00a6}, 	// MCU_DATA_0
{0x098C, 0xA351}, 	// MCU_ADDRESS [AWB_CCM_POSITION_MIN]
{0x0990, 0x0000}, 	// MCU_DATA_0
{0x098C, 0xA352}, 	// MCU_ADDRESS [AWB_CCM_POSITION_MAX]
{0x0990, 0x007F}, 	// MCU_DATA_0
{0x098C, 0xA354}, 	// MCU_ADDRESS [AWB_SATURATION]
{0x0990, 0x0043}, 	// MCU_DATA_0
{0x098C, 0xA355}, 	// MCU_ADDRESS [AWB_MODE]
{0x0990, 0x0002}, 	// MCU_DATA_0							  
{0x098C, 0xA35D}, 	// MCU_ADDRESS [AWB_STEADY_BGAIN_OUT_MIN]
{0x0990, 0x0078}, 	// MCU_DATA_0
{0x098C, 0xA35E}, 	// MCU_ADDRESS [AWB_STEADY_BGAIN_OUT_MAX]
{0x0990, 0x0086}, 	// MCU_DATA_0
{0x098C, 0xA35F}, 	// MCU_ADDRESS [AWB_STEADY_BGAIN_IN_MIN]
{0x0990, 0x007E}, 	// MCU_DATA_0
{0x098C, 0xA360}, 	// MCU_ADDRESS [AWB_STEADY_BGAIN_IN_MAX]
{0x0990, 0x0082}, 	// MCU_DATA_0
{0x098C, 0xA365}, 	// MCU_ADDRESS
{0x0990, 0x0010}, 	// MCU_DATA_0
//[true gray]
{0x098C, 0xA363}, 	// MCU_ADDRESS
{0x0990, 0x00C7}, 	//CB,		// C7,//BA, 	// MCU_DATA_0
{0x098C, 0xA364}, 	// MCU_ADDRESS
{0x0990, 0x00F0}, 	// MCU_DATA_0
//[K FACTOR]
{0x098C, 0xA366}, 	// MCU_ADDRESS [AWB_KR_L]		 
{0x0990, 0x007D},   //80, 	// MCU_DATA_0					 
{0x098C, 0xA367}, 	// MCU_ADDRESS [AWB_KG_L]		 
{0x0990, 0x0080}, 	// MCU_DATA_0					 
{0x098C, 0xA368}, 	// MCU_ADDRESS [AWB_KB_L]		 
{0x0990, 0x0080}, 	// MCU_DATA_0					 
{0x098C, 0xA369}, 	// MCU_ADDRESS [AWB_KR_R]		 
{0x0990, 0x0080}, 	// (081111 84->7F)MCU_DATA_0	 
{0x098C, 0xA36A}, 	// MCU_ADDRESS [AWB_KG_R]		 
{0x0990, 0x0077},   //78, 	// MCU_DATA_0					 
{0x098C, 0xA36B}, 	// MCU_ADDRESS [AWB_KB_R]		 
{0x0990, 0x0078},   //79, //0x0080	// MCU_DATA_0					 
//[Clearing AWB shaking in Lowlight]
{0x35A2, 0x00A4},  //0x0014,
{0x3240, 0xC802},  //0xC814, 	
//[AFD]
////Auto flicker detection
{0x098C, 0xA11E}, 	// MCU_ADDRESS
{0x0990, 0x0001}, 	// MCU_DATA_0
{0x098C, 0xA404}, 	// MCU_ADDRESS
{0x0990, 0x0000}, 	// MCU_DATA_0
//[virtgain]
{0x098C, 0xA20C}, 	// MCU_ADDRESS
{0x0990, 0x000C}, 	//0x0010  min 7.5fps   //0x000C min 10fps //0x0018 min5fps	// MCU_DATA_0
{0x098C, 0x2212}, 	// MCU_ADDRESS [AE_Dgain1_12]
{0x0990, 0x01A0}, //0x0080	// MCU_DATA_0
{0x098C, 0xA20D}, 	// MCU_ADDRESS [AE_MIN_VIRTGAIN]
{0x0990, 0x0017}, 	// MCU_DATA_0
{0x098C, 0xA20E}, 	// MCU_ADDRESS [AE_MAX_VIRTGAIN]
{0x0990, 0x0080},    //0x0010 	//0x0048	// MCU_DATA_0
{0x098C, 0xA216}, 	// MCU_ADDRESS [AE_MAXGAIN23]
{0x0990, 0x0060}, 	// MCU_DATA_0
{0x098C, 0xA11D}, 	// MCU_ADDRESS
{0x0990, 0x0002}, 	// MCU_DATA_0	
//[no »Ç¼¥ gain up3]
{0x098C, 0xAB2C}, 	// MCU_ADDRESS
{0x0990, 0x0006},  //0x001A	// MCU_DATA_0
{0x098C, 0xAB2D}, 	// MCU_ADDRESS
{0x0990, 0x000e},   //0x001F	// MCU_DATA_0
{0x098C, 0xAB2E}, 	// MCU_ADDRESS
{0x0990, 0x0006},    //0x001A 	// MCU_DATA_0
{0x098C, 0xAB2F}, 	// MCU_ADDRESS
{0x0990, 0x0006},    //0x002E 	// MCU_DATA_0
{0x098C, 0xAB30}, 	// MCU_ADDRESS
{0x0990, 0x001e},   //0x002D	// MCU_DATA_0
{0x098C, 0xAB31}, 	// MCU_ADDRESS
{0x0990, 0x000e},   //0x002D	// MCU_DATA_0
{0x098C, 0xAB32}, 	// MCU_ADDRESS
{0x0990, 0x001e},    //0x002D 	// MCU_DATA_0
{0x098C, 0xAB33}, 	// MCU_ADDRESS
{0x0990, 0x001e},   //0x002D	// MCU_DATA_0
{0x098C, 0xAB34}, 	// MCU_ADDRESS
{0x0990, 0x0008},   //0x001E	// MCU_DATA_0
{0x098C, 0xAB35}, 	// MCU_ADDRESS
{0x0990, 0x0080},   //0x004D	// MCU_DATA_0	
//[AE setting]
{0x098C, 0xA207}, 	// MCU_ADDRESS [AE_GATE]
{0x0990, 0x0004}, 	// MCU_DATA_0
{0x098C, 0xA24F}, 	// MCU_ADDRESS
{0x0990, 0x0036}, //36,	// MCU_DATA_0
{0x098C, 0x2257}, 	// MCU_ADDRESS [RESERVED_AE_57]
{0x0990, 0x2710}, 	// MCU_DATA_0
{0x098C, 0x2250}, 	// MCU_ADDRESS [RESERVED_AE_50]
{0x0990, 0x1B58}, 	// MCU_DATA_0
{0x098C, 0x2252}, 	// MCU_ADDRESS [RESERVED_AE_52]
{0x0990, 0x32C8}, 	// MCU_DATA_0
//[black gamma contrast]
{0x098C, 0x2B1B}, 	// MCU_ADDRESS [HG_BRIGHTNESSMETRIC]
{0x0990, 0x0643}, 	// MCU_DATA_0
{0x098C, 0xAB37}, 	// MCU_ADDRESS [HG_GAMMA_MORPH_CTRL]
{0x0990, 0x0003}, 	// MCU_DATA_0
{0x098C, 0x2B38}, 	// MCU_ADDRESS [HG_GAMMASTARTMORPH]
{0x0990, 0x2800}, 		// MCU_DATA_0
{0x098C, 0x2B3A}, 	// MCU_ADDRESS [HG_GAMMASTOPMORPH]
{0x0990, 0x46fe}, 	// MCU_DATA_0
//black 06
{0x098C, 0xAB3C}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_A_0]
{0x0990, 0x0000}, 	// MCU_DATA_0
{0x098C, 0xAB3D}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_A_1]
{0x0990, 0x0008}, 	// MCU_DATA_0
{0x098C, 0xAB3E}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_A_2]
{0x0990, 0x0019}, 	// MCU_DATA_0
{0x098C, 0xAB3F}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_A_3]
{0x0990, 0x0035}, 	// MCU_DATA_0
{0x098C, 0xAB40}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_A_4]
{0x0990, 0x0056}, 	// MCU_DATA_0
{0x098C, 0xAB41}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_A_5]
{0x0990, 0x006F}, 	// MCU_DATA_0
{0x098C, 0xAB42}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_A_6]
{0x0990, 0x0085}, 	// MCU_DATA_0
{0x098C, 0xAB43}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_A_7]
{0x0990, 0x0098}, 	// MCU_DATA_0
{0x098C, 0xAB44}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_A_8]
{0x0990, 0x00A7}, 	// MCU_DATA_0
{0x098C, 0xAB45}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_A_9]
{0x0990, 0x00B4}, 	// MCU_DATA_0
{0x098C, 0xAB46}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_A_10]
{0x0990, 0x00C0}, 	// MCU_DATA_0
{0x098C, 0xAB47}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_A_11]
{0x0990, 0x00CA}, 	// MCU_DATA_0
{0x098C, 0xAB48}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_A_12]
{0x0990, 0x00D4}, 	// MCU_DATA_0
{0x098C, 0xAB49}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_A_13]
{0x0990, 0x00DC}, 	// MCU_DATA_0
{0x098C, 0xAB4A}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_A_14]
{0x0990, 0x00E4}, 	// MCU_DATA_0
{0x098C, 0xAB4B}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_A_15]
{0x0990, 0x00EC}, 	// MCU_DATA_0
{0x098C, 0xAB4C}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_A_16]
{0x0990, 0x00F3}, 	// MCU_DATA_0
{0x098C, 0xAB4D}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_A_17]
{0x0990, 0x00F9}, 	// MCU_DATA_0
{0x098C, 0xAB4E}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_A_18]
{0x0990, 0x00FF}, 	// MCU_DATA_0
//Black 05 contrast 1.35
{0x098C, 0xAB4F}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_0]
{0x0990, 0x0000}, 	// MCU_DATA_0
{0x098C, 0xAB50}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_1]
{0x0990, 0x0006}, 	// MCU_DATA_0
{0x098C, 0xAB51}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_2]
{0x0990, 0x0012}, 	// MCU_DATA_0
{0x098C, 0xAB52}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_3]
{0x0990, 0x002F}, 	// MCU_DATA_0
{0x098C, 0xAB53}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_4]
{0x0990, 0x0053}, 	// MCU_DATA_0
{0x098C, 0xAB54}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_5]
{0x0990, 0x006D}, 	// MCU_DATA_0
{0x098C, 0xAB55}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_6]
{0x0990, 0x0083}, 	// MCU_DATA_0
{0x098C, 0xAB56}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_7]
{0x0990, 0x0096}, 	// MCU_DATA_0
{0x098C, 0xAB57}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_8]
{0x0990, 0x00A6}, 	// MCU_DATA_0
{0x098C, 0xAB58}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_9]
{0x0990, 0x00B3}, 	// MCU_DATA_0
{0x098C, 0xAB59}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_10]
{0x0990, 0x00BF}, 	// MCU_DATA_0
{0x098C, 0xAB5A}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_11]
{0x0990, 0x00CA}, 	// MCU_DATA_0
{0x098C, 0xAB5B}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_12]
{0x0990, 0x00D3}, 	// MCU_DATA_0
{0x098C, 0xAB5C}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_13]
{0x0990, 0x00DC}, 	// MCU_DATA_0
{0x098C, 0xAB5D}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_14]
{0x0990, 0x00E4}, 	// MCU_DATA_0
{0x098C, 0xAB5E}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_15]
{0x0990, 0x00EB}, 	// MCU_DATA_0
{0x098C, 0xAB5F}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_16]
{0x0990, 0x00F2}, 	// MCU_DATA_0
{0x098C, 0xAB60}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_17]
{0x0990, 0x00F9}, 	// MCU_DATA_0
{0x098C, 0xAB61}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_18]
{0x0990, 0x00FF}, 	// MCU_DATA_0
//[LL-mode-modified]
{0x098C, 0xAB04}, 	// MCU_ADDRESS [HG_MAX_DLEVEL]
{0x0990, 0x0040}, 	// MCU_DATA_0
{0x098C, 0xAB06}, 	// MCU_ADDRESS [HG_PERCENT]
{0x0990, 0x0005}, 	// 3, //0x000A	// MCU_DATA_0
{0x098C, 0xAB08}, 	// MCU_ADDRESS [HG_DLEVEL]
{0x0990, 0x0010}, 	// MCU_DATA_0
{0x098C, 0xAB20}, 	// MCU_ADDRESS [HG_LL_SAT1]
{0x0990, 0x0050},   //63, 	// MCU_DATA_0
{0x098C, 0xAB21}, 	// MCU_ADDRESS [RESERVED_HG_21]
{0x0990, 0x001d}, 	// MCU_DATA_0
{0x098C, 0xAB22}, 	// MCU_ADDRESS [RESERVED_HG_22]
{0x0990, 0x0007}, 	// MCU_DATA_0
{0x098C, 0xAB23}, 	// MCU_ADDRESS [RESERVED_HG_23]
{0x0990, 0x0004}, //0x000A	// MCU_DATA_0
{0x098C, 0xAB24}, 	// MCU_ADDRESS [HG_LL_SAT2]
{0x0990, 0x0000}, 	// MCU_DATA_0
{0x098C, 0xAB25}, 	// MCU_ADDRESS [RESERVED_HG_25]
{0x0990, 0x00a0}, 	//0x0014	// MCU_DATA_0
{0x098C, 0xAB26}, 	// MCU_ADDRESS [RESERVED_HG_26]
{0x0990, 0x0005}, 	// MCU_DATA_0
{0x098C, 0xAB27}, 	// MCU_ADDRESS [RESERVED_HG_27]
{0x0990, 0x0010}, 	// MCU_DATA_0
{0x098C, 0x2B28}, 	// MCU_ADDRESS [HG_LL_BRIGHTNESSSTART]
{0x0990, 0x157c}, //0x0a00 //0x157c  //0x0A00 	// MCU_DATA_0
{0x098C, 0x2B2A}, 	// MCU_ADDRESS [HG_LL_BRIGHTNESSSTOP]
{0x0990, 0x37ef}, 	//0x7000	// MCU_DATA_0	
{0x098C, 0x2755},//{0x098C, 0x2755},   // MCU_ADDRESS [MODE_OUTPUT_FORMAT_A]
{0x0990, 0x0002},//{0x0990, 0x0002},  // MCU_DATA_0
{0x098C, 0x2757},//{0x098C, 0x2757},  // MCU_ADDRESS [MODE_OUTPUT_FORMAT_B]
{0x0990, 0x0002},//{0x0990, 0x0002},   // MCU_DATA_0
/*----------------*/
//[30Frame fixed]
{0x098C, 0x271F },	// MCU_ADDRESS [MODE_SENSOR_FRAME_LENGTH_A]
{0x0990, 0x0239 },	// MCU_DATA_0
{0x098C, 0xA20C }, 	// MCU_ADDRESS [AE_MAX_INDEX]
{0x0990, 0x0004 },	// MCU_DATA_0
{0x098C, 0xA21B },	// MCU_ADDRESS [AE_INDEX]
{0x0990, 0x0004 },	// MCU_DATA_0
/*----------------*/
//[Refresh] --> refresh code move to init write part.	
{0x098C,0xA103},//{0x098C, 0xA103},	// MCU_ADDRESS [SEQ_CMD]
{0x0990,0x0006},//{0x0990, 0x0006},	// MCU_DATA_0
{0xFFFE,200},   //Delay=200
{0x098C,0xA103},//{0x098C, 0xA103},	// MCU_ADDRESS [SEQ_CMD]
{0x0990,0x0005},//{0x0990, 0x0005},	// MCU_DATA_0
{0xFFFE,200},   //Delay=200
};


/*effect*/
static const struct register_address_value_pair const effect_off_reg_settings_array[] = {	
//[OFF]
{0x098C, 0x2759}, 	// MCU_ADDRESS [MODE_SPEC_EFFECTS_A]
{0x0990, 0x6440}, 	// MCU_DATA_0
{0x098C, 0x275B}, 	// MCU_ADDRESS [MODE_SPEC_EFFECTS_B]
{0x0990, 0x6440}, 	// MCU_DATA_0
{0x098C, 0xA103}, 	// MCU_ADDRESS [SEQ_CMD]
{0x0990, 0x0005}, 	// MCU_DATA_0
{0xFFFE, 300},      //Delay=300 
};
static const struct register_address_value_pair const effect_mono_reg_settings_array[] = {	
//[MONO]
{0x098C, 0x2759}, 	// MCU_ADDRESS [MODE_SPEC_EFFECTS_A]
{0x0990, 0x6441}, 	// MCU_DATA_0
{0x098C, 0x275B}, 	// MCU_ADDRESS [MODE_SPEC_EFFECTS_B]
{0x0990, 0x6441}, 	// MCU_DATA_0
{0x098C, 0xA103}, 	// MCU_ADDRESS [SEQ_CMD]
{0x0990, 0x0005}, 	// MCU_DATA_0
{0xFFFE, 300},      //Delay=300 
};
static const struct register_address_value_pair const effect_sepia_reg_settings_array[] = {	
//[Sepia]
{0x098C, 0x2759}, 	// MCU_ADDRESS [MODE_SPEC_EFFECTS_A]
{0x0990, 0x6442}, 	// MCU_DATA_0
{0x098C, 0x275B}, 	// MCU_ADDRESS [MODE_SPEC_EFFECTS_B]
{0x0990, 0x6442}, 	// MCU_DATA_0
{0x098C, 0x2763}, 	// MCU_ADDRESS [MODE_COMMONMODESETTINGS_FX_SEPIA_SETTINGS]
{0x0990, 0xDD18}, 	// MCU_DATA_0
{0x098C, 0xA103}, 	// MCU_ADDRESS [SEQ_CMD]
{0x0990, 0x0005}, 	// MCU_DATA_0
{0xFFFE, 300},		//Delay=300
};
static const struct register_address_value_pair const effect_negative_reg_settings_array[] = {	
 //[Negative]
{0x098C, 0x2759}, 	// MCU_ADDRESS [MODE_SPEC_EFFECTS_A]
{0x0990, 0x6443}, 	// MCU_DATA_0
{0x098C, 0x275B}, 	// MCU_ADDRESS [MODE_SPEC_EFFECTS_B]
{0x0990, 0x6443}, 	// MCU_DATA_0
{0x098C, 0xA103}, 	// MCU_ADDRESS [SEQ_CMD]
{0x0990, 0x0005}, 	// MCU_DATA_0
{0xFFFE, 300},		//Delay=300 
};
static const struct register_address_value_pair const effect_solarize_reg_settings_array[] = {	
 //[Solaize]
{0x098C, 0x2759}, 	// MCU_ADDRESS [MODE_SPEC_EFFECTS_A]
{0x0990, 0x6D44}, 	// MCU_DATA_0
{0x098C, 0x275B}, 	// MCU_ADDRESS [MODE_SPEC_EFFECTS_B]
{0x0990, 0x6D44}, 	// MCU_DATA_0
{0x098C, 0xA103}, 	// MCU_ADDRESS [SEQ_CMD]
{0x0990, 0x0005}, 	// MCU_DATA_0
{0xFFFE, 300},		//Delay=300
};
static const struct register_address_value_pair const effect_blue_reg_settings_array[] = {	
 //[Aqua]
{0x098C, 0x2759}, 	// MCU_ADDRESS [MODE_SPEC_EFFECTS_A]
{0x0990, 0x6442}, 	// MCU_DATA_0
{0x098C, 0x275B}, 	// MCU_ADDRESS [MODE_SPEC_EFFECTS_B]
{0x0990, 0x6442}, 	// MCU_DATA_0
{0x098C, 0x2763}, 	// MCU_ADDRESS [MODE_COMMONMODESETTINGS_FX_SEPIA_SETTINGS]
{0x0990, 0x21D4}, 	// MCU_DATA_0
{0x098C, 0xA103}, 	// MCU_ADDRESS [SEQ_CMD]
{0x0990, 0x0005}, 	// MCU_DATA_0
{0xFFFE, 300},		//Delay=300
};


/*wb*/
static const struct register_address_value_pair const wb_auto_reg_settings_array[] = {	
//[AWB]
{0x098C, 0xA34A},	// MCU_ADDRESS
{0x0990, 0x0059},	// AWB_GAIN_MIN
{0x098C, 0xA34B},	// MCU_ADDRESS
{0x0990, 0x00A6},	// AWB_GAIN_MAX
{0x098C, 0xA34C},	// MCU_ADDRESS
{0x0990, 0x0059},	// AWB_GAINMIN_B
{0x098C, 0xA34D},	// MCU_ADDRESS
{0x0990, 0x00A6},	// AWB_GAINMAX_B
{0x098C, 0xA351},	// MCU_ADDRESS
{0x0990, 0x0000},	// AWB_CCM_POSITION_MIN
{0x098C, 0xA352},	// MCU_ADDRESS
{0x0990, 0x007F},	// AWB_CCM_POSITION_MAX
{0x098C, 0xA103},	// MCU_ADDRESS [SEQ_CMD]
{0x0990, 0x0005},	// MCU_DATA_0
{0xFFFE, 300},		//delay=50
};
static const struct register_address_value_pair const wb_incandescent_reg_settings_array[] = {
//[MWB- Incandescent]
{0x098C, 0xA34A},	// MCU_ADDRESS
{0x0990, 0x0069},	// AWB_GAIN_MIN
{0x098C, 0xA34B},	// MCU_ADDRESS
{0x0990, 0x0074},	// AWB_GAIN_MAX
{0x098C, 0xA34C},	// MCU_ADDRESS
{0x0990, 0x0072},	// AWB_GAINMIN_B
{0x098C, 0xA34D},	// MCU_ADDRESS
{0x0990, 0x007E},	// AWB_GAINMAX_B
{0x098C, 0xA351},	// MCU_ADDRESS
{0x0990, 0x0010},	// AWB_CCM_POSITION_MIN
{0x098C, 0xA352},	// MCU_ADDRESS
{0x0990, 0x0020},  // AWB_CCM_POSITION_MAX
{0x098C, 0xA103},	// MCU_ADDRESS [SEQ_CMD]
{0x0990, 0x0005},	// MCU_DATA_0
{0xFFFE, 300},		//delay=50
};
static const struct register_address_value_pair const wb_fluorescent_reg_settings_array[] = {
//[MWB- Flourscent]
{0x098C, 0xA34A},	// MCU_ADDRESS
{0x0990, 0x0071},	// AWB_GAIN_MIN
{0x098C, 0xA34B},	// MCU_ADDRESS
{0x0990, 0x0080},	// AWB_GAIN_MAX
{0x098C, 0xA34C},	// MCU_ADDRESS
{0x0990, 0x0078},	// AWB_GAINMIN_B
{0x098C, 0xA34D},	// MCU_ADDRESS
{0x0990, 0x0085},	// AWB_GAINMAX_B
{0x098C, 0xA351},	// MCU_ADDRESS
{0x0990, 0x0050},	// AWB_CCM_POSITION_MIN
{0x098C, 0xA352},	// MCU_ADDRESS
{0x0990, 0x0060},  // AWB_CCM_POSITION_MAX
{0x098C, 0xA103},	// MCU_ADDRESS [SEQ_CMD]
{0x0990, 0x0005},	// MCU_DATA_0
{0xFFFE, 300},		//delay=50
};
static const struct register_address_value_pair const wb_sunny_reg_settings_array[] = {
//[MWB- Sunny]
{0x098C, 0xA34A}, // MCU_ADDRESS
{0x0990, 0x0088}, // AWB_GAIN_MIN
{0x098C, 0xA34B}, // MCU_ADDRESS
{0x0990, 0x0093}, // AWB_GAIN_MAX
{0x098C, 0xA34C}, // MCU_ADDRESS
{0x0990, 0x0078}, // AWB_GAINMIN_B
{0x098C, 0xA34D}, // MCU_ADDRESS
{0x0990, 0x0080}, // AWB_GAINMAX_B
{0x098C, 0xA351}, // MCU_ADDRESS
{0x0990, 0x0065}, // AWB_CCM_POSITION_MIN
{0x098C, 0xA352}, // MCU_ADDRESS
{0x0990, 0x0070},  // AWB_CCM_POSITION_MAX
{0x098C, 0xA103},	// MCU_ADDRESS [SEQ_CMD]
{0x0990, 0x0005},	// MCU_DATA_0
{0xFFFE, 300},		//delay=50
};
static const struct register_address_value_pair const wb_cloudy_reg_settings_array[] = {
//[MWB- Cloudy]
{0x098C, 0xA34A},	// MCU_ADDRESS
{0x0990, 0x0098},	// AWB_GAIN_MIN
{0x098C, 0xA34B},	// MCU_ADDRESS
{0x0990, 0x00A0},	// AWB_GAIN_MAX
{0x098C, 0xA34C},	// MCU_ADDRESS
{0x0990, 0x0067},	// AWB_GAINMIN_B
{0x098C, 0xA34D},	// MCU_ADDRESS
{0x0990, 0x0070},	// AWB_GAINMAX_B
{0x098C, 0xA351},	// MCU_ADDRESS
{0x0990, 0x006c},	// AWB_CCM_POSITION_MIN
{0x098C, 0xA352},	// MCU_ADDRESS
{0x0990, 0x007f},  // AWB_CCM_POSITION_MAX
{0x098C, 0xA103},	// MCU_ADDRESS [SEQ_CMD]
{0x0990, 0x0005},	// MCU_DATA_0
{0xFFFE, 300},		//delay=50
};

/*fps range for snapshot mode*/
static const struct register_address_value_pair const fps_range_7p5_22_reg_settings_array[] = {
//[Piture_max 22~ 7.5fps]
{0x098C, 0x2721},	  // MCU_ADDRESS [MODE_SENSOR_FRAME_LENGTH_A]
{0x0990, 0x095C},	  // MCU_DATA_0
{0x098C, 0xA20C},	  // MCU_ADDRESS [AE_MAX_INDEX]
{0x0990, 0x0010},	  // MCU_DATA_0
{0x098C, 0xA21B},	  // MCU_ADDRESS [AE_INDEX]
{0x0990, 0x0010},	  // MCU_DATA_0
{0x098C, 0xA103},	  // MCU_ADDRESS [SEQ_CMD]
{0x0990, 0x0006},	  // MCU_DATA_0
};

static const struct register_address_value_pair const fps_range_15_reg_settings_array[] = {
//[15Frame fixed]
{0x098C, 0x2721}, 	// MCU_ADDRESS [MODE_SENSOR_FRAME_LENGTH_A]
{0x0990, 0x0DBA}, 	// MCU_DATA_0
{0x098C, 0xA20C}, 	// MCU_ADDRESS [AE_MAX_INDEX]
{0x0990, 0x0004}, 	// MCU_DATA_0
{0x098C, 0xA21B}, 	// MCU_ADDRESS [AE_INDEX]
{0x0990, 0x0004}, 	// MCU_DATA_0
{0x098C, 0xA103}, 	// MCU_ADDRESS [SEQ_CMD]
{0x0990, 0x0006}, 	// MCU_DATA_0
};

static const struct register_address_value_pair const fps_range_20_reg_settings_array[] = {
//[20Frame fixed]
{0x098C, 0x2721}, 	// MCU_ADDRESS [MODE_SENSOR_FRAME_LENGTH_A]
{0x0990, 0x0A4C}, 	// MCU_DATA_0
{0x098C, 0xA20C}, 	// MCU_ADDRESS [AE_MAX_INDEX]
{0x0990, 0x0004}, 	// MCU_DATA_0
{0x098C, 0xA21B}, 	// MCU_ADDRESS [AE_INDEX]
{0x0990, 0x0004}, 	// MCU_DATA_0
{0x098C, 0xA103}, 	// MCU_ADDRESS [SEQ_CMD]
{0x0990, 0x0006}, 	// MCU_DATA_0
};

static const struct register_address_value_pair const fps_range_30_reg_settings_array[] = {
//[30Frame fixed]
{0x098C, 0x2721}, 	// MCU_ADDRESS [MODE_SENSOR_FRAME_LENGTH_A]
{0x0990, 0x06DD}, 	// MCU_DATA_0
{0x098C, 0xA20C}, 	// MCU_ADDRESS [AE_MAX_INDEX]
{0x0990, 0x0004}, 	// MCU_DATA_0
{0x098C, /*0xA21B*/ 0xA20B}, 	// MCU_ADDRESS [AE_INDEX]
{0x0990, 0x0004}, 	// MCU_DATA_0
{0x098C, 0xA103}, 	// MCU_ADDRESS [SEQ_CMD]
{0x0990, 0x0006}, 	// MCU_DATA_0
};

/*fps range for video mode*/
static const struct register_address_value_pair const fps_range_15_video_reg_settings_array[] = {
//[VT 15Frame fixed]
{0x098C, 0x2721}, 	// MCU_ADDRESS [MODE_SENSOR_FRAME_LENGTH_A]
{0x0990, 0x0DB7}, 	// MCU_DATA_0
{0x098C, 0xA20C}, 	// MCU_ADDRESS [AE_MAX_INDEX]
{0x0990, 0x0004}, 	// MCU_DATA_0
{0x098C, 0xA21B}, 	// MCU_ADDRESS [AE_INDEX]
{0x0990, 0x0004}, 	// MCU_DATA_0
{0x098C, 0xA103}, 	// MCU_ADDRESS [SEQ_CMD]
{0x0990, 0x0006}, 	// MCU_DATA_0
};

static const struct register_address_value_pair const fps_range_20_video_reg_settings_array[] = {
//[VT 20Frame fixed]
{0x098C, 0x2721}, 	// MCU_ADDRESS [MODE_SENSOR_FRAME_LENGTH_A]
{0x0990, 0x0A49}, 	// MCU_DATA_0
{0x098C, 0xA20C}, 	// MCU_ADDRESS [AE_MAX_INDEX]
{0x0990, 0x0004}, 	// MCU_DATA_0
{0x098C, 0xA21B}, 	// MCU_ADDRESS [AE_INDEX]
{0x0990, 0x0004}, 	// MCU_DATA_0
{0x098C, 0xA103}, 	// MCU_ADDRESS [SEQ_CMD]
{0x0990, 0x0006}, 	// MCU_DATA_0
};

static const struct register_address_value_pair const fps_range_30_video_reg_settings_array[] = {
//[VT 30fps fixxed]
{0x098C, 0x2721}, 	// MCU_ADDRESS [MODE_SENSOR_FRAME_LENGTH_A]
{0x0990, 0x06DB}, 	// MCU_DATA_0
{0x098C, 0xA20C}, 	// MCU_ADDRESS [AE_MAX_INDEX]
{0x0990, 0x0004}, 	// MCU_DATA_0
{0x098C, 0xA21B}, 	// MCU_ADDRESS [AE_INDEX]
{0x0990, 0x0004}, 	// MCU_DATA_0
{0x098C, 0xA103}, 	// MCU_ADDRESS [SEQ_CMD]
{0x0990, 0x0006}, 	// MCU_DATA_0
};


  

struct mt9m113_reg mt9m113_regs = {
   /* VGA 22fps for normal mode */	
  .init_reg_settings 	  = mt9m113_reg_init_settings,  
  .init_reg_settings_size = ARRAY_SIZE(mt9m113_reg_init_settings),

   /* VGA 30fps for VT mode */ 
  .init_reg_vt_settings  = mt9m113_reg_init_vt_settings,  
  .init_reg_vt_settings_size = ARRAY_SIZE(mt9m113_reg_init_vt_settings),
   
	/*effect*/
  .effect_off_reg_settings 	  			= effect_off_reg_settings_array,
  .effect_off_reg_settings_size  		= ARRAY_SIZE(effect_off_reg_settings_array),
  .effect_mono_reg_settings 	  		= effect_mono_reg_settings_array,
  .effect_mono_reg_settings_size  		= ARRAY_SIZE(effect_mono_reg_settings_array),
  .effect_sepia_reg_settings 	  		= effect_sepia_reg_settings_array,
  .effect_sepia_reg_settings_size  		= ARRAY_SIZE(effect_sepia_reg_settings_array),
  .effect_negative_reg_settings 	  	= effect_negative_reg_settings_array,
  .effect_negative_reg_settings_size  	= ARRAY_SIZE(effect_negative_reg_settings_array),
  .effect_solarize_reg_settings 	  	= effect_solarize_reg_settings_array,
  .effect_solarize_reg_settings_size  	= ARRAY_SIZE(effect_solarize_reg_settings_array),
  .effect_blue_reg_settings 			= effect_blue_reg_settings_array,
  .effect_blue_reg_settings_size		= ARRAY_SIZE(effect_blue_reg_settings_array),

	/*wb*/
  .wb_auto_reg_settings 	  			= wb_auto_reg_settings_array,
  .wb_auto_reg_settings_size  			= ARRAY_SIZE(wb_auto_reg_settings_array),
  .wb_incandescent_reg_settings 	  	= wb_incandescent_reg_settings_array,
  .wb_incandescent_reg_settings_size  	= ARRAY_SIZE(wb_incandescent_reg_settings_array),
  .wb_fluorescent_reg_settings 	  		= wb_fluorescent_reg_settings_array,
  .wb_fluorescent_reg_settings_size  	= ARRAY_SIZE(wb_fluorescent_reg_settings_array),
  .wb_sunny_reg_settings 	  			= wb_sunny_reg_settings_array,
  .wb_sunny_reg_settings_size  			= ARRAY_SIZE(wb_sunny_reg_settings_array),
  .wb_cloudy_reg_settings 	  			= wb_cloudy_reg_settings_array,
  .wb_cloudy_reg_settings_size  		= ARRAY_SIZE(wb_cloudy_reg_settings_array),

	/*fps range for snapshot mode*/
  .fps_range_7p5_22_reg_settings		= fps_range_7p5_22_reg_settings_array,
  .fps_range_7p5_22_reg_settings_size	= ARRAY_SIZE(fps_range_7p5_22_reg_settings_array),
  .fps_range_15_reg_settings			= fps_range_15_reg_settings_array,
  .fps_range_15_reg_settings_size		= ARRAY_SIZE(fps_range_15_reg_settings_array),
  .fps_range_20_reg_settings			= fps_range_20_reg_settings_array,
  .fps_range_20_reg_settings_size		= ARRAY_SIZE(fps_range_20_reg_settings_array),
  .fps_range_30_reg_settings			= fps_range_30_reg_settings_array,
  .fps_range_30_reg_settings_size		= ARRAY_SIZE(fps_range_30_reg_settings_array),

	/*fps range for video mode*/
  .fps_range_15_video_reg_settings			= fps_range_15_video_reg_settings_array,
  .fps_range_15_video_reg_settings_size		= ARRAY_SIZE(fps_range_15_video_reg_settings_array),
  .fps_range_20_video_reg_settings			= fps_range_20_video_reg_settings_array,
  .fps_range_20_video_reg_settings_size		= ARRAY_SIZE(fps_range_20_video_reg_settings_array),
  .fps_range_30_video_reg_settings			= fps_range_30_video_reg_settings_array,
  .fps_range_30_video_reg_settings_size		= ARRAY_SIZE(fps_range_30_video_reg_settings_array),
};


