/* drivers/media/video/msm/isx006_tun.h
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


#ifndef ISX006_TUN_H
#define ISX006_TUN_H

/* Register value saved in SDCARD for tunning */
#define IMAGE_TUNING_SET			0  //(0 : OFF 1 : ON)
#define IMAGE_TUNING_LOG			0  //(0 : OFF 1 : ON) Precondition is IMAGE_TUNING_SET 1


#if IMAGE_TUNING_SET
long isx006_reg_pll_ext(void);
long isx006_reg_init_ext(void);
#endif

#endif //ISX006_TUN_H

