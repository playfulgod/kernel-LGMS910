/*******************************************************************************
									L T E _ D E B U G . H
FILE NAME
lte_debug.h

GENERAL DESCRIPTION
 	This file contains
AUTHOR
	kimbosoo@lge.com

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

#define LTE_DEBUG
#define RACE_LTE_TEST

#ifdef LTE_DEBUG
#define LTE_ERROR(fmt,args...) printk("[LTE_ERROR] [%s] "fmt,__FUNCTION__, ## args)
#define LTE_TRACE(fmt,args...) printk("[LTE_TRACE] [%s] "fmt,__FUNCTION__, ## args)
#define LTE_INFO(fmt,args...) printk("[LTE_INFO] [%s] "fmt,__FUNCTION__, ## args)
#else // LTE_DEBUG
#define LTE_ERROR(fmt,args...)
#define LTE_TRACE(fmt,args...)
#define LTE_INFO(fmt,args...)
#endif
#define FUNC_ENTER() LTE_TRACE("enter\n")
#define FUNC_EXIT() LTE_TRACE("exit\n")

/******************************************************************************
*
* DATA DECLARATIONS
******************************************************************************/

/******************************************************************************
*
* FUNCTION DECLARATIONS
******************************************************************************/

