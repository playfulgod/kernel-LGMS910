/* drivers/media/video/msm/isx006_flash.h
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


#ifndef _ISX006_FLASH_H_
#define _ISX006_FLASH_H_

#include <linux/types.h>


enum{
  ISX006_FLASH_LED_MODE,
  ISX006_FLASH_PREV_FLASH_STATE,	
  ISX006_FLASH_TEST_MODE_ENABLE,
  ISX006_FLASH_TEST_MODE_LED_MODE,
};



int32_t isx006_update_flash_state(int8_t flash_state, int8_t iso);
int32_t isx006_set_led_state(int8_t flash_state);
int32_t isx006_set_flash_ctrl(int8_t type,int8_t value);
int8_t 	isx006_get_flash_ctrl(int8_t type);
void	isx006_init_flash_ctrl(void);


#endif // _ISX006_FLASH_H_

