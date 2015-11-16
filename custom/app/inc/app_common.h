/*****************************************************************************
 *
 * Filename:
 * ---------
 *   app_common.h 
 *
 * Project:
 * --------
 * --------
 *
 * Description:
 * ------------
 * ------------
 *
 * Author:
 * -------
 * -------
 *
 *============================================================================
 *             HISTORY
 *----------------------------------------------------------------------------
 * 
 ****************************************************************************/


#ifndef __APP_COMMON_H__
#define __APP_COMMON_H__
#include "Ql_system.h"
#include "app_error.h"
#include "app_debug.h"

/****************************************************************************
 * Task msg id Definition
 ***************************************************************************/
//gprs 
#define MSG_ID_GPRS_STATE               	MSG_ID_USER_START+0x100
#define MSG_ID_MUTEX_TEST               	MSG_ID_USER_START+0x101
#define MSG_ID_SEMAPHORE_TEST           	MSG_ID_USER_START+0x102
#define MSG_ID_GET_ALL_TASK_PRIORITY    	MSG_ID_USER_START+0x103
#define MSG_ID_GET_ALL_TASK_REMAINSTACK 	MSG_ID_USER_START+0x104

//gps
#define MSG_ID_GPS_MODE_CONTROL  	        MSG_ID_USER_START+0x105
#define MSG_ID_GPS_REP_LOCATION  	        MSG_ID_USER_START+0x106

#endif // End-of __APP_COMMON_H__ 

