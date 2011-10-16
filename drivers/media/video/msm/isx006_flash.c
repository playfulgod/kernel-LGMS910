/* drivers/media/video/msm/isx006_flash.c
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
#include "isx006.h"
#include "isx006_flash.h"


#define ISX006_REG_AGC_SCL_NOW	  	  0x0278
#define ISX006_REG_AGC_THRESHOLD  	  0x06CC  
#define ISX006_REG_CAP_GAINOFFSET     0x0282
#define	ISX006_REG_CONT_SHIFT		  0x0097
#define ISX006_REG_STB_CONT_SHIFT_R   0x445C
#define ISX006_REG_STB_CONT_SHIFT_B   0x445E

#define ISX006_AGC_THRESHOLD_ISO_OFF	  0x06CC
#define ISX006_AGC_THRESHOLD_ISO_ON		  0x0531

#define AE_OFSETVAL          	  	  0x03E8
#define AE_MAXDIFF           	  	  0x1388
#define	N_RATIO					  	  0x0000	/* luminous ratio of pre flash and fullflash 0:x2, 1:x3, 2:x4, 3:x5, 4:x6*/
#define	AWB_TBLMIN			   	  	  0x0032

#define	STB_CONT_SHIFT_R_DATA_H   	  0x0000		
#define STB_CONT_SHIFT_B_DATA_H   	  0x0000
#define	STB_CONT_SHIFT_R_DATA_M   	  0xFF4C	
#define STB_CONT_SHIFT_B_DATA_M   	  0x02BC
#define	STB_CONT_SHIFT_R_DATA_L   	  0xFF4C	
#define STB_CONT_SHIFT_B_DATA_L   	  0x02BC

#define AE_HIGH					  	  0x0100
#define AE_LOW				      	  0x04C0


enum{	
  ISX006_LED_OFF,
  ISX006_LED_ON
};

struct isx006_flash_t{
	int8_t  led_mode;
	int8_t  prev_flash_state;

	 /*led flash for test mode */
 	int8_t  test_mode_enable;
	int8_t  test_led_mode;
};

/*---------------------------------------------------------------------------
    EXTERNAL DECLARATIONS
---------------------------------------------------------------------------*/
#ifdef CONFIG_LM3559_FLASH
extern int lm3559_flash_set_led_state(int state);
#endif 
/*---------------------------------------------------------------------------
    INTERNAL  DECLARATIONS
---------------------------------------------------------------------------*/

static struct isx006_flash_t isx006_ctrl_flash;


static int32_t	aescl_auto;
static int32_t	errscl_auto;
static int32_t	ratio_r_auto;
static int32_t	ratio_b_auto;


/*---------------------------------------------------------------------------
   AE OFFSET TABLE : main-flash = pre-flashx1.8
---------------------------------------------------------------------------*/

uint16_t isx006_aeoffset_table[] = 
{//1.8
8,16,24,31,39,46,54,61,68,75,
82,89,96,103,110,117,123,130,136,143,
149,155,161,167,173,179,185,191,197,203,
208,214,219,225,230,236,241,246,252,257,
262,267,272,277,282,287,291,296,301,305,
310,315,319,324,328,332,337,341,345,350,
354,358,362,366,370,374,378,382,386,390,
394,397,401,405,408,412,416,419,423,426,
430,433,437,440,443,447,450,453,456,460,
463,466,469,472,475,478,481,484,487,490,
493,496,499,502,504,507,510,513,515,518,
521,523,526,529,531,534,536,539,541,544,
546,549,551,553,556,558,560,563,565,567,
570,572,574,576,578,581,583,585,587,589,
591,593,595,597,599,601,603,605,607,609,
611,613,615,617,618,620,622,624,626,627,
629,631,633,634,636,638,639,641,643,644,
646,648,649,651,652,654,656,657,659,660,
662,663,665,666,668,669,671,672,673,675,
676,678,679,680,682,683,684,686,687,688,
690,691,692,693,695,696,697,698,700,701,
702,703,704,705,707,708,709,710,711,712,
713,715,716,717,718,719,720,721,722,723,
724,725,726,727,728,729,730,731,732,733,
734,735,736,737,738,739,740,740,741,742,
743,744,745,746,747,747,748,749,750,751,
752,752,753,754,755,756,756,757,758,759,
760,760,761,762,763,763,764,765,766,766,
767,768,768,769,770,770,771,772,772,773,
774,774,775,776,776,777,778,778,779,780,
780,781,781,782,783,783,784,784,785,786,
786,787,787,788,788,789,789,790,791,791,
792,792,793,793,794,794,795,795,796,796,
797,797,798,798,799,799,800,800,801,801,
802,802,802,803,803,804,804,805,805,806,
806,806,807,807,808,808,809,809,809,810,
810,811,811,811,812,812,813,813,813,814,
814,814,815,815,816,816,816,817,817,817,
818,818,818,819,819,819,820,820,820,821,
821,821,822,822,822,823,823,823,824,824,
824,825,825,825,825,826,826,826,827,827,
827,827,828,828,828,829,829,829,829,830,
830,830,830,831,831,831,831,832,832,832,
832,833,833,833,833,834,834,834,834,835,
835,835,835,835,836,836,836,836,837,837,
837,837,837,838,838,838,838,838,839,839,
839,839,839,840,840,840,840,840,841,841,
841,841,841,842,842,842,842,842,842,843,
843,843,843,843,844,844,844,844,844,844,
845,845,845,845,845,845,845,846,846,846,
};



#if 0
/*---------------------------------------------------------------------------
   AE OFFSET TABLE : main-flash = pre-flashx2
---------------------------------------------------------------------------*/
uint16_t isx006_aeoffset_table[] = 
{0,10,20,29,39,48,58,67,76,85,
94,102,111,119,128,136,144,152,160,168,
176,184,191,199,206,214,221,228,235,242,
249,256,263,270,276,283,289,296,302,308,
314,321,327,333,339,344,350,356,362,367,
373,378,384,389,395,400,405,410,415,420,
425,430,435,440,445,450,455,459,464,468,
473,478,482,486,491,495,499,504,508,512,
516,520,524,528,532,536,540,544,548,551,
555,559,563,566,570,573,577,581,584,587,
591,594,598,601,604,608,611,614,617,620,
623,627,630,633,636,639,642,645,648,650,
653,656,659,662,665,667,670,673,675,678,
681,683,686,688,691,693,696,698,701,703,
706,708,710,713,715,717,720,722,724,727,
729,731,733,735,737,740,742,744,746,748,
750,752,754,756,758,760,762,764,766,768,
770,771,773,775,777,779,781,782,784,786,
788,789,791,793,794,796,798,799,801,803,
804,806,808,809,811,812,814,815,817,818,
820,821,823,824,826,827,828,830,831,833,
834,835,837,838,839,841,842,843,845,846,
847,848,850,851,852,853,855,856,857,858,
859,861,862,863,864,865,866,867,869,870,
871,872,873,874,875,876,877,878,879,880,
881,882,883,884,885,886,887,888,889,890,
891,892,893,894,895,896,897,898,898,899,
900,901,902,903,904,904,905,906,907,908,
909,909,910,911,912,913,913,914,915,916,
916,917,918,919,919,920,921,922,922,923,
924,924,925,926,926,927,928,929,929,930,
931,931,932,932,933,934,934,935,936,936,
937,937,938,939,939,940,940,941,942,942,
943,943,944,944,945,945,946,947,947,948,
948,949,949,950,950,951,951,952,952,953,
953,954,954,955,955,956,956,957,957,958,
958,958,959,959,960,960,961,961,962,962,
962,963,963,964,964,964,965,965,966,966,
966,967,967,968,968,968,969,969,970,970,
970,971,971,971,972,972,972,973,973,974,
974,974,975,975,975,976,976,976,977,977,
977,978,978,978,978,979,979,979,980,980,
980,981,981,981,982,982,982,982,983,983,
983,984,984,984,984,985,985,985,985,986,
986,986,986,987,987,987,987,988,988,988,
988,989,989,989,989,990,990,990,990,991,
991,991,991,991,992,992,992,992,993,993,
993,993,993,994,994,994,994,994,995,995,
995,995,995,996,996,996,996,996,997,997,
997,997,997,998,998,998,998,998,998,999,999}; 
#endif


/*---------------------------------------------------------------------------
   DELTA  AWB TABLE
---------------------------------------------------------------------------*/
int32_t isx006_delta_awb_table[220][5] = { 
{ 94, 181, 264, 341, 414 }, { 92, 178, 257, 331, 400 },
{ 91, 175, 251, 322, 387 }, { 90, 171, 245, 313, 375 },
{ 89, 168, 240, 304, 363 }, { 88, 165, 234, 296, 352 },
{ 87, 162, 229, 288, 342 }, { 86, 160, 224, 281, 332 },
{ 84, 157, 219, 274, 322 }, { 83, 154, 215, 267, 313 },
{ 82, 151, 210, 261, 305 }, { 81, 149, 206, 254, 297 },
{ 80, 146, 202, 248, 289 }, { 79, 144, 198, 243, 281 },
{ 78, 142, 194, 237, 274 }, { 78, 139, 190, 232, 267 },
{ 77, 137, 186, 227, 261 }, { 76, 135, 183, 222, 255 },
{ 75, 133, 179, 217, 249 }, { 74, 131, 176, 213, 243 },
{ 73, 129, 173, 208, 237 }, { 72, 127, 170, 204, 232 },
{ 71, 125, 166, 200, 227 }, { 71, 123, 164, 196, 222 },
{ 70, 121, 161, 192, 217 }, { 69, 119, 158, 188, 213 },
{ 68, 118, 155, 185, 208 }, { 67, 116, 152, 181, 204 },
{ 67, 114, 150, 178, 200 }, { 66, 113, 147, 174, 196 },
{ 65, 111, 145, 171, 192 }, { 64, 109, 143, 168, 188 },
{ 64, 108, 140, 165, 185 }, { 63, 106, 138, 162, 181 },
{ 62, 105, 136, 159, 178 }, { 62, 103, 134, 157, 174 },
{ 61, 102, 132, 154, 171 }, { 60, 101, 130, 151, 168 },
{ 60, 99, 128, 149, 165 }, { 59, 98, 126, 146, 162 },
{ 58, 97, 124, 144, 159 }, { 58, 95, 122, 142, 157 },
{ 57, 94, 120, 139, 154 }, { 57, 93, 118, 137, 151 },
{ 56, 92, 117, 135, 149 }, { 55, 91, 115, 133, 146 },
{ 55, 89, 113, 131, 144 }, { 54, 88, 112, 129, 142 },
{ 54, 87, 110, 127, 139 }, { 53, 86, 108, 125, 137 },
{ 53, 85, 107, 123, 135 }, { 52, 84, 105, 121, 133 },
{ 52, 83, 104, 119, 131 }, { 51, 82, 103, 118, 129 },
{ 50, 81, 101, 116, 127 }, { 50, 80, 100, 114, 125 },
{ 49, 79, 99, 112, 123 }, { 49, 78, 97, 111, 121 },
{ 48, 77, 96, 109, 119 }, { 48, 76, 95, 108, 118 },
{ 47, 75, 93, 106, 116 }, { 47, 74, 92, 105, 114 },
{ 47, 73, 91, 103, 113 }, { 46, 73, 90, 102, 111 },
{ 46, 72, 89, 101, 109 }, { 45, 71, 88, 99, 108 },
{ 45, 70, 87, 98, 106 }, { 44, 69, 85, 97, 105 },
{ 44, 69, 84, 95, 103 }, { 43, 68, 83, 94, 102 },
{ 43, 67, 82, 93, 101 }, { 43, 66, 81, 92, 99 },
{ 42, 66, 80, 90, 98 }, { 42, 65, 79, 89, 97 },
{ 41, 64, 78, 88, 95 }, { 41, 63, 77, 87, 94 },
{ 41, 63, 77, 86, 93 }, { 40, 62, 76, 85, 92 },
{ 40, 61, 75, 84, 91 }, { 40, 61, 74, 83, 89 },
{ 39, 60, 73, 82, 88 }, { 39, 59, 72, 81, 87 },
{ 38, 59, 71, 80, 86 }, { 38, 58, 70, 79, 85 },
{ 38, 57, 70, 78, 84 }, { 37, 57, 69, 77, 83 },
{ 37, 56, 68, 76, 82 }, { 37, 56, 67, 75, 81 },
{ 36, 55, 67, 74, 80 }, { 36, 55, 66, 73, 79 },
{ 36, 54, 65, 73, 78 }, { 35, 53, 64, 72, 77 },
{ 35, 53, 64, 71, 76 }, { 35, 52, 63, 70, 75 },
{ 34, 52, 62, 69, 74 }, { 34, 51, 62, 69, 73 },
{ 34, 51, 61, 68, 73 }, { 33, 50, 60, 67, 72 },
{ 33, 50, 60, 66, 71 }, { 33, 49, 59, 65, 70 },
{ 33, 49, 58, 65, 69 }, { 32, 48, 58, 64, 69 },
{ 32, 48, 57, 63, 68 }, { 32, 47, 57, 63, 67 },
{ 31, 47, 56, 62, 66 }, { 31, 46, 55, 61, 66 },
{ 31, 46, 55, 61, 65 }, { 31, 45, 54, 60, 64 },
{ 30, 45, 54, 59, 63 }, { 30, 45, 53, 59, 63 },
{ 30, 44, 53, 58, 62 }, { 30, 44, 52, 57, 61 },
{ 29, 43, 51, 57, 61 }, { 29, 43, 51, 56, 60 },
{ 29, 42, 50, 56, 59 }, { 29, 42, 50, 55, 59 },
{ 28, 42, 49, 54, 58 }, { 28, 41, 49, 54, 57 },
{ 28, 41, 48, 53, 57 }, { 28, 40, 48, 53, 56 },
{ 27, 40, 47, 52, 56 }, { 27, 40, 47, 52, 55 },
{ 27, 39, 47, 51, 55 }, { 27, 39, 46, 51, 54 },
{ 26, 39, 46, 50, 53 }, { 26, 38, 45, 50, 53 },
{ 26, 38, 45, 49, 52 }, { 26, 38, 44, 49, 52 },
{ 26, 37, 44, 48, 51 }, { 25, 37, 43, 48, 51 },
{ 25, 36, 43, 47, 50 }, { 25, 36, 43, 47, 50 },
{ 25, 36, 42, 46, 49 }, { 24, 35, 42, 46, 49 },
{ 24, 35, 41, 45, 48 }, { 24, 35, 41, 45, 48 },
{ 24, 35, 41, 45, 47 }, { 24, 34, 40, 44, 47 },
{ 23, 34, 40, 44, 46 }, { 23, 34, 39, 43, 46 },
{ 23, 33, 39, 43, 45 }, { 23, 33, 39, 42, 45 },
{ 23, 33, 38, 42, 45 }, { 22, 32, 38, 42, 44 },
{ 22, 32, 38, 41, 44 }, { 22, 32, 37, 41, 43 },
{ 22, 32, 37, 40, 43 }, { 22, 31, 37, 40, 42 },
{ 22, 31, 36, 40, 42 }, { 21, 31, 36, 39, 42 },
{ 21, 30, 36, 39, 41 }, { 21, 30, 35, 39, 41 },
{ 21, 30, 35, 38, 40 }, { 21, 30, 35, 38, 40 },
{ 21, 29, 34, 37, 40 }, { 20, 29, 34, 37, 39 },
{ 20, 29, 34, 37, 39 }, { 20, 29, 33, 36, 39 },
{ 20, 28, 33, 36, 38 }, { 20, 28, 33, 36, 38 },
{ 20, 28, 33, 35, 38 }, { 19, 28, 32, 35, 37 },
{ 19, 27, 32, 35, 37 }, { 19, 27, 32, 35, 36 },
{ 19, 27, 31, 34, 36 }, { 19, 27, 31, 34, 36 },
{ 19, 27, 31, 34, 35 }, { 18, 26, 31, 33, 35 },
{ 18, 26, 30, 33, 35 }, { 18, 26, 30, 33, 35 },
{ 18, 26, 30, 32, 34 }, { 18, 25, 30, 32, 34 },
{ 18, 25, 29, 32, 34 }, { 18, 25, 29, 32, 33 },
{ 17, 25, 29, 31, 33 }, { 17, 25, 28, 31, 33 },
{ 17, 24, 28, 31, 32 }, { 17, 24, 28, 30, 32 },
{ 17, 24, 28, 30, 32 }, { 17, 24, 28, 30, 32 },
{ 17, 24, 27, 30, 31 }, { 17, 23, 27, 29, 31 },
{ 16, 23, 27, 29, 31 }, { 16, 23, 27, 29, 30 },
{ 16, 23, 26, 29, 30 }, { 16, 23, 26, 28, 30 },
{ 16, 22, 26, 28, 30 }, { 16, 22, 26, 28, 29 },
{ 16, 22, 25, 28, 29 }, { 16, 22, 25, 27, 29 },
{ 15, 22, 25, 27, 29 }, { 15, 21, 25, 27, 28 },
{ 15, 21, 25, 27, 28 }, { 15, 21, 24, 26, 28 },
{ 15, 21, 24, 26, 28 }, { 15, 21, 24, 26, 27 },
{ 15, 21, 24, 26, 27 }, { 15, 20, 24, 26, 27 },
{ 14, 20, 23, 25, 27 }, { 14, 20, 23, 25, 26 },
{ 14, 20, 23, 25, 26 }, { 14, 20, 23, 25, 26 },
{ 14, 20, 23, 25, 26 }, { 14, 19, 22, 24, 26 },
{ 14, 19, 22, 24, 25 }, { 14, 19, 22, 24, 25 },
{ 14, 19, 22, 24, 25 }, { 14, 19, 22, 24, 25 },
{ 13, 19, 22, 23, 25 }, { 13, 19, 21, 23, 24 },
{ 13, 18, 21, 23, 24 }, { 13, 18, 21, 23, 24 },
{ 13, 18, 21, 23, 24 }, { 13, 18, 21, 22, 24 },
{ 13, 18, 21, 22, 23 }, { 13, 18, 20, 22, 23 },
{ 13, 18, 20, 22, 23 }, { 13, 17, 20, 22, 23 },
{ 12, 17, 20, 21, 23 }, { 12, 17, 20, 21, 22 }
}; 

static int16_t	bit_format( int32_t  val )
{
	int16_t ret ;

	if ( val <= -800) 
		ret = 0x8000;
	else if ( val >= 800)
		ret = 0x7FFF;
	else
		ret = (int16_t)((val* 4096)/100);


	return ret;
}

/*---------------------------------------------------------------------------
  isx006_init_flash_ctrl_type
   ---------------------------------------------------------------------------*/
void isx006_init_flash_ctrl(void)
{
	struct isx006_flash_t *ctrl_flash = &isx006_ctrl_flash;

	if(ctrl_flash->test_mode_enable == 0){
		memset(&isx006_ctrl_flash,0x00,sizeof(isx006_ctrl_flash));	
    	isx006_set_led_state(FLASH_STATE_OFF);
	}
}

/*---------------------------------------------------------------------------
  isx006_set_flash_ctrl_type
   ---------------------------------------------------------------------------*/
int32_t isx006_set_flash_ctrl(int8_t type,int8_t value)
{
	struct isx006_flash_t *ctrl_flash = &isx006_ctrl_flash;
	int32_t rc = 0;

	switch(type){
	case ISX006_FLASH_LED_MODE:
		ctrl_flash->led_mode = value;
		break;
	case ISX006_FLASH_PREV_FLASH_STATE:
		ctrl_flash->prev_flash_state = value;
		break;
	case ISX006_FLASH_TEST_MODE_ENABLE:
		ctrl_flash->test_mode_enable = value;
		break;
	case ISX006_FLASH_TEST_MODE_LED_MODE:
		ctrl_flash->test_led_mode = value;
		break;
	default:
		break;
	}

	return rc;
	
}

/*---------------------------------------------------------------------------
  isx006_get_flash_ctrl_type
   ---------------------------------------------------------------------------*/
int8_t isx006_get_flash_ctrl(int8_t type)
{
    struct isx006_flash_t *ctrl_flash = &isx006_ctrl_flash;
	int8_t value = 0;

	switch(type){
	 case ISX006_FLASH_LED_MODE:
	 	 value = ctrl_flash->led_mode;
		 break;
	 case ISX006_FLASH_PREV_FLASH_STATE:
	 	 value = ctrl_flash->prev_flash_state;
		 break;
	 case ISX006_FLASH_TEST_MODE_ENABLE:
	 	 value = ctrl_flash->test_mode_enable;
		 break;
	 case ISX006_FLASH_TEST_MODE_LED_MODE:
	 	 value = ctrl_flash->test_led_mode;
		 break;
	 default:
		 break;
	}

	return value;
}


/*---------------------------------------------------------------------------
   isx006_cal_ae_awb_for_flash_capture
   ---------------------------------------------------------------------------*/
int32_t isx006_cal_ae_awb_for_flash_capture(void)
{
	int32_t   	rc = 0;
	int16_t 	ae_now = 0,ersc_now = 0;
	int16_t 	ratio_r_now= 0,ratio_b_now= 0;
	int16_t 	aediff = 0,aeoffset = 0;
	uint16_t	stb_cont_shift_r = 0x00, stb_cont_shift_b = 0x00;


	rc = isx006_i2c_read_ext(0x028A, &ae_now, WORD_LEN);  // aescl_now
	rc = isx006_i2c_read_ext(0x0286, &ersc_now, WORD_LEN);  // errscl_now
	/*
	rc = isx006_i2c_read_ext(0x026E, &ratio_r_now, WORD_LEN);  // ratio_r
	rc = isx006_i2c_read_ext(0x0270, &ratio_b_now, WORD_LEN);  // ratio_b
	*/

	/*printk("%s:ae_now[0x%x] ersc_now[0x%x] aescl_auto[0x%x] errscl_auto[0x%x]\n",
			__func__,ae_now,ersc_now,aescl_auto,errscl_auto);*/
	
	aediff = (ae_now + ersc_now) - (aescl_auto + errscl_auto);
	if(aediff < 0) aediff = 0;
	
	if ( ersc_now < 0 ) {	  //100729
	    
		if ( aediff >= AE_MAXDIFF )
			aeoffset = -AE_OFSETVAL- ersc_now;		
		else
			aeoffset = -isx006_aeoffset_table[aediff/10] - ersc_now;	
	}
	else {   
		if ( aediff >= AE_MAXDIFF )
			aeoffset = -AE_OFSETVAL;
		else
			aeoffset = -isx006_aeoffset_table[aediff/10];
	}

	//printk("%s: aeoffset[0x%x]\n",__func__,aeoffset);
	
	rc = isx006_i2c_write_ext(ISX006_REG_CAP_GAINOFFSET,aeoffset,WORD_LEN);	
	
	if(aediff < AE_HIGH){		
		stb_cont_shift_r = STB_CONT_SHIFT_R_DATA_H;
		stb_cont_shift_b = STB_CONT_SHIFT_B_DATA_H;
	}
	else if(aediff > AE_LOW){
		stb_cont_shift_r = STB_CONT_SHIFT_R_DATA_L;
		stb_cont_shift_b = STB_CONT_SHIFT_B_DATA_L;
	}
	else{
		stb_cont_shift_r = STB_CONT_SHIFT_R_DATA_M;
		stb_cont_shift_b = STB_CONT_SHIFT_B_DATA_M;
	}

	rc = isx006_i2c_write_ext(ISX006_REG_STB_CONT_SHIFT_R,stb_cont_shift_r,WORD_LEN );
	rc = isx006_i2c_write_ext(ISX006_REG_STB_CONT_SHIFT_B,stb_cont_shift_b,WORD_LEN );
	rc = isx006_i2c_write_ext(ISX006_REG_CONT_SHIFT,2,BYTE_LEN );

	return rc;

}

/*---------------------------------------------------------------------------
   isx006_check_ae_awb_preview_mode -  This code is used to control AE
  ---------------------------------------------------------------------------*/
int32_t isx006_check_ae_awb_preview_mode(void)
{
	int32_t rc = 0;
	unsigned short reg_val1 = 0,reg_val2 = 0;

	rc = isx006_i2c_read_ext(0x028A, &reg_val1, WORD_LEN);
	rc = isx006_i2c_read_ext(0x0286, &reg_val2, WORD_LEN);

	aescl_auto = reg_val1;
	errscl_auto = reg_val2;

	rc = isx006_i2c_read_ext(0x026A, &reg_val1, WORD_LEN);  // ratio R
	rc = isx006_i2c_read_ext(0x026C, &reg_val2, WORD_LEN);  // ratio B

	ratio_r_auto = reg_val1;	
	ratio_b_auto = reg_val2;
				

	return rc;

}

/*---------------------------------------------------------------------------
   isx006_set_led_control - to set LED control & AE/AWB control when turning on LED 
   ---------------------------------------------------------------------------*/
int32_t isx006_set_led_control(int8_t on){
	int32_t rc = 0;
	
	CAM_MSG("%s : led control : onoff[%d]\n",__func__,on);

	if(on){  /* LED ON */
		  rc = isx006_check_ae_awb_preview_mode();
		  rc = isx006_i2c_write_table(&isx006_regs.led_flash_on_reg_settings[0], 
		  				isx006_regs.led_flash_on_reg_settings_size);
	}
	else{ /* LED OFF */
			rc = isx006_i2c_write_table(&isx006_regs.led_flash_off_reg_settings[0], 
						isx006_regs.led_flash_off_reg_settings_size);
	}
	
	if(rc<0){
		CAM_MSG("[isx006.c]%s: fail in writing for focus\n",__func__);
		return rc;
	}

	return rc;
}

/*---------------------------------------------------------------------------
  isx006_set_led_state
   ---------------------------------------------------------------------------*/
int32_t isx006_set_led_state(int8_t flash_state){
	int32_t rc = 0;

#ifdef CONFIG_LM3559_FLASH

	rc =lm3559_flash_set_led_state(flash_state);

#endif

	return rc;
}

/*---------------------------------------------------------------------------
  isx006_flash_led_mode_auto
   ---------------------------------------------------------------------------*/
int32_t isx006_flash_led_mode_auto(int8_t flash_state, int8_t iso)
{
	struct isx006_flash_t *ctrl_flash = &isx006_ctrl_flash;
	uint16_t reg_val = 0;
	int32_t  rc = 0,i;

	switch(flash_state)
	{
	case FLASH_STATE_PREFLASH:
		 printk("[AUTO] FLASH_STATE_PREFLASH\n");
		 rc = isx006_i2c_read_ext(ISX006_REG_AGC_SCL_NOW,&reg_val,WORD_LEN);
		 if((iso == 0 && reg_val > ISX006_AGC_THRESHOLD_ISO_OFF) 
		 	|| (iso > 0 && reg_val >= ISX006_AGC_THRESHOLD_ISO_ON)){	
		 	printk("[AUTO] ISX006_AGC_THRESHOLD\n");
			if(ctrl_flash->prev_flash_state != FLASH_STATE_PREFLASH){
				rc = isx006_set_led_control(ISX006_LED_ON);
				/* [START] [MS910] sungmin.cho@lge.com 2011.03.31 use high current */
				//rc = isx006_set_led_state(FLASH_STATE_TORCH);
				rc = isx006_set_led_state(FLASH_STATE_PREFLASH);
				/* [END] [MS910] sungmin.cho@lge.com 2011.03.31 */
				ctrl_flash->prev_flash_state = FLASH_STATE_PREFLASH;
			}
			else
			  ctrl_flash->prev_flash_state = FLASH_STATE_OFF;	
		 }
		 else
			ctrl_flash->prev_flash_state = FLASH_STATE_OFF;
		 break;
     case FLASH_STATE_CANCEL_PREFLASH:
	 	  printk("[AUTO] FLASH_STATE_CANCEL_PREFLASH\n");
	 	  if(ctrl_flash->prev_flash_state == FLASH_STATE_PREFLASH) 
		   		rc = isx006_set_led_control(ISX006_LED_OFF);
		  
		  rc = isx006_set_led_state(FLASH_STATE_OFF); 
	 	  ctrl_flash->prev_flash_state = FLASH_STATE_OFF;
		 break;
	 case FLASH_STATE_PREFLASH_OFF:
	 	 printk("[AUTO] FLASH_STATE_PREFLASH_OFF\n");
		 if(ctrl_flash->prev_flash_state == FLASH_STATE_PREFLASH){
		 	 rc = isx006_cal_ae_awb_for_flash_capture();
		 	ctrl_flash->prev_flash_state = FLASH_STATE_PREFLASH_OFF;

		 }
		 else 
		    ctrl_flash->prev_flash_state = FLASH_STATE_OFF;
		 
		 rc = isx006_set_led_state(FLASH_STATE_OFF);
		 break;
	 
	 case FLASH_STATE_STROBE:
	 	 if((ctrl_flash->prev_flash_state == FLASH_STATE_PREFLASH_OFF) ||
		 	(ctrl_flash->prev_flash_state == FLASH_STATE_PREFLASH_OFF_MF)){
		 	
	 	 	rc = isx006_set_led_state(FLASH_STATE_STROBE);
		 	ctrl_flash->prev_flash_state = FLASH_STATE_STROBE;

			printk("[AUTO] FLASH_STATE_STROBE\n");
			
	 	 }
		 break;
	 case FLASH_STATE_OFF:
	 	 printk("[AUTO] FLASH_STATE_OFF\n");
		 if(ctrl_flash->prev_flash_state == FLASH_STATE_STROBE)
			rc = isx006_set_led_control(ISX006_LED_OFF);
			
		 rc = isx006_set_led_state(FLASH_STATE_OFF);
		 
		 ctrl_flash->prev_flash_state = FLASH_STATE_OFF;
		 break;
	case FLASH_STATE_PREFLASH_MF:
		 printk("[AUTO] FLASH_STATE_PREFLASH_MF\n");
		 rc = isx006_i2c_read_ext(ISX006_REG_AGC_SCL_NOW,&reg_val,WORD_LEN);
		 if((iso == 0 && reg_val > ISX006_AGC_THRESHOLD_ISO_OFF) 
		 	|| (iso > 0 && reg_val >= ISX006_AGC_THRESHOLD_ISO_ON)){	
			if(ctrl_flash->prev_flash_state != FLASH_STATE_PREFLASH_MF){
		 		rc = isx006_set_led_control(ISX006_LED_ON);
				/* [START] [MS910] sungmin.cho@lge.com 2011.03.31 use high current */
		 		//rc = isx006_set_led_state(FLASH_STATE_TORCH);
				rc = isx006_set_led_state(FLASH_STATE_PREFLASH);
				/* [END] [MS910] sungmin.cho@lge.com 2011.03.31 */
		 		rc = isx006_i2c_write_table(&isx006_regs.manual_focus_normal_reg_settings[0],
                			isx006_regs.manual_focus_normal_reg_settings_size);

				for(i=0 ; i < 4 ; i++)  // 3 frame delay
		 			msleep(50);

				ctrl_flash->prev_flash_state = FLASH_STATE_PREFLASH_MF;	
			}
			else
				ctrl_flash->prev_flash_state = FLASH_STATE_OFF;
			
		 }
		 else
			ctrl_flash->prev_flash_state = FLASH_STATE_OFF;
		 break;
	 case FLASH_STATE_PREFLASH_OFF_MF:
	 	 printk("[AUTO] FLASH_STATE_PREFLASH_OFF_MF\n");
	 	 if(ctrl_flash->prev_flash_state == FLASH_STATE_PREFLASH_MF){
		 	
		 	 for(i= 0 ; i < 50 ; i++){ 
				
				rc = isx006_i2c_read_ext(0x02AA, &reg_val, WORD_LEN);
				if(reg_val == 0){
					printk("[AUTO] FLASH_STATE_PREFLASH_OFF_MF..... i[%d]",i);
					break;
				}
				msleep(10);
		 	}
			
			rc = isx006_set_led_state(FLASH_STATE_OFF); 	
		 	ctrl_flash->prev_flash_state = FLASH_STATE_PREFLASH_OFF_MF;
		 }
		 else
		 	ctrl_flash->prev_flash_state = FLASH_STATE_OFF;
	 	 break;
	 default:
		break;
	 }		
	
	 return rc;
	
}

/*---------------------------------------------------------------------------
  isx006_flash_led_mode_on
   ---------------------------------------------------------------------------*/
int32_t isx006_flash_led_mode_on(int8_t flash_state)
{
	struct isx006_flash_t *ctrl_flash = &isx006_ctrl_flash;
	int32_t rc = 0, reg_val = 0, i;

	switch(flash_state)
	{
	case FLASH_STATE_PREFLASH: // snapshot
		 printk("[ON] FLASH_STATE_PREFLASH\n");
		 rc = isx006_set_led_control(ISX006_LED_ON);
		 /* [START] [MS910] sungmin.cho@lge.com 2011.03.31 */
		 //rc = isx006_set_led_state(FLASH_STATE_TORCH);
		 rc = isx006_set_led_state(FLASH_STATE_PREFLASH);
		 /* [END] sungmin.cho@lge.com 2011.03.31 */
		 ctrl_flash->prev_flash_state = FLASH_STATE_PREFLASH;
		 break;
	case FLASH_STATE_PREFLASH_OFF:
		 printk("[ON] FLASH_STATE_PREFLASH_OFF\n");
		 if(ctrl_flash->prev_flash_state == FLASH_STATE_PREFLASH)
		 	rc = isx006_cal_ae_awb_for_flash_capture();

		 rc = isx006_set_led_state(FLASH_STATE_OFF);
		 ctrl_flash->prev_flash_state = FLASH_STATE_OFF;
		 break;
	case FLASH_STATE_CANCEL_PREFLASH:
		 printk("[ON] FLASH_STATE_CANCEL_PREFLASH\n");
		 if(ctrl_flash->prev_flash_state == FLASH_STATE_PREFLASH){
		 	rc = isx006_set_led_control(ISX006_LED_OFF);
		 }
		 rc = isx006_set_led_state(FLASH_STATE_OFF);  
		 ctrl_flash->prev_flash_state = FLASH_STATE_OFF;
		 break;
	case FLASH_STATE_STROBE:
		 printk("[ON] FLASH_STATE_STROBE\n");
		 rc = isx006_set_led_state(FLASH_STATE_STROBE); 
		 ctrl_flash->prev_flash_state = FLASH_STATE_STROBE;
		 break;
	case FLASH_STATE_TORCH: // video
		 printk("[ON] FLASH_STATE_TORCH\n");
		 rc = isx006_set_led_state(FLASH_STATE_TORCH);
		 ctrl_flash->prev_flash_state = FLASH_STATE_TORCH;
		 break;	
	case FLASH_STATE_OFF:
		 printk("[ON] FLASH_STATE_OFF\n");
		 if(ctrl_flash->prev_flash_state == FLASH_STATE_STROBE)
			rc = isx006_set_led_control(ISX006_LED_OFF);
	
		 rc = isx006_set_led_state(FLASH_STATE_OFF); 	
		 ctrl_flash->prev_flash_state = FLASH_STATE_OFF;
		 break;
	case FLASH_STATE_PREFLASH_MF:
		  printk("[ON] FLASH_STATE_PREFLASH_MF\n");
		 if(ctrl_flash->prev_flash_state != FLASH_STATE_PREFLASH_MF){
		 	rc = isx006_set_led_control(ISX006_LED_ON);
			/* [START] [MS910] sungmin.cho@lge.com 2011.03.31 */
		 	//rc = isx006_set_led_state(FLASH_STATE_TORCH);
		 	rc = isx006_set_led_state(FLASH_STATE_PREFLASH);
			/* [END] sungmin.cho@lge.com 2011.03.31 */
		 	rc = isx006_i2c_write_table(&isx006_regs.manual_focus_normal_reg_settings[0],
                	isx006_regs.manual_focus_normal_reg_settings_size);

		 	for(i=0 ; i < 4 ; i++)  // 3 frame delay
		 		 msleep(50); 

			ctrl_flash->prev_flash_state = FLASH_STATE_PREFLASH_MF;
		 }
		 else 
		 	ctrl_flash->prev_flash_state = FLASH_STATE_OFF;
		 break;
	case FLASH_STATE_PREFLASH_OFF_MF:	 
		 printk("[ON] FLASH_STATE_PREFLASH_OFF_MF\n");
		 if(ctrl_flash->prev_flash_state == FLASH_STATE_PREFLASH_MF){

			 for(i= 0 ; i < 50 ; i++){  
				rc = isx006_i2c_read_ext(0x02AA, &reg_val, BYTE_LEN);
				if(reg_val == 0){
					printk("[ON] FLASH_STATE_PREFLASH_OFF_MF..... i[%d]",i);
					break;
				}
				
				msleep(10);
		 	}
			rc = isx006_set_led_state(FLASH_STATE_OFF); 	
		 	ctrl_flash->prev_flash_state = FLASH_STATE_OFF;
		 }
		 else
		 	ctrl_flash->prev_flash_state = FLASH_STATE_OFF;
		 break;
	default:
		 break;
	}
		
	return rc;
	
}

/*---------------------------------------------------------------------------
  isx006_flash_led_mode_off
   ---------------------------------------------------------------------------*/
int32_t isx006_flash_led_mode_off(void)
{
	struct isx006_flash_t *ctrl_flash = &isx006_ctrl_flash;
	int32_t rc = 0;

	if(ctrl_flash->prev_flash_state != FLASH_STATE_OFF){

		rc = isx006_set_led_state(FLASH_STATE_OFF);	
	
		ctrl_flash->prev_flash_state = FLASH_STATE_OFF;
	}

	return rc;	
}

/*---------------------------------------------------------------------------
  isx006_update_flash_state
   ---------------------------------------------------------------------------*/
int32_t isx006_update_flash_state(int8_t flash_state, int8_t iso)
{
	struct isx006_flash_t *ctrl_flash = &isx006_ctrl_flash;
	int32_t rc = 0;

	if(ctrl_flash->test_mode_enable){
		
		if(ctrl_flash->test_led_mode == LED_MODE_ON){
			isx006_flash_led_mode_on(flash_state);
		}
		else {
			isx006_flash_led_mode_off();
		}
		
	}
	else{
		if(ctrl_flash->led_mode == LED_MODE_AUTO)
			isx006_flash_led_mode_auto(flash_state, iso);
		else if(ctrl_flash->led_mode == LED_MODE_ON)
			isx006_flash_led_mode_on(flash_state);
		else 
			isx006_flash_led_mode_off();
	}

	return rc;
	
}


