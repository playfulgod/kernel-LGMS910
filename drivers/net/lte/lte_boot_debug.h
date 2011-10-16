#ifndef _LTE_BOOT_DEBUG_H
#define _LTE_BOOT_DEBUG_H
/*******************************************************************************
						L T E _ B O O T_D E B U G . H
FILE NAME
lte_debug.h

GENERAL DESCRIPTION
 	This file contains
AUTHOR
	daeok.kim@lge.com

Copyright (c) 2008 LG Electronics Inc. All Rights Reserved.
*******************************************************************************/

/******************************************************************************
*
* INCLUDES
******************************************************************************/

/******************************************************************************
*
* DEFINES
******************************************************************************/
//#define FUNC_ENTER() printk("[%s] %d line (%s) enter\n",__FILE__,__LINE__,__FUNCTION__)
//#define FUNC_EXIT() printk("[%s] %d line (%s) exit\n",__FILE__,__LINE__,__FUNCTION__)

#define LBOOT_DEBUG

#ifdef LBOOT_DEBUG
#define LBOOT_ERROR(fmt,args...) printk("[LBOOT_ERROR] [%s] "fmt,__FUNCTION__, ## args)
#define LBOOT_TRACE(fmt,args...) printk("[LBOOT_TRACE] [%s] "fmt,__FUNCTION__, ## args)
#define LBOOT_INFO(fmt,args...) printk("[LBOOT_INFO] [%s] "fmt,__FUNCTION__, ## args)
#else 
#define LBOOT_ERROR(fmt,args...)
#define LBOOT_TRACE(fmt,args...)
#define LBOOT_INFO(fmt,args...)
#endif
#define FUNC_ENTER() LBOOT_TRACE("enter +\n")
#define FUNC_EXIT() LBOOT_TRACE("exit - \n")

#endif /* _LTE_BOOT_DEBUG_H */

/******************************************************************************
*
* DATA DECLARATIONS
******************************************************************************/

/******************************************************************************
*
* FUNCTION DECLARATIONS
******************************************************************************/

