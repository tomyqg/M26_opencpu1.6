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
#define SUB_TASK_NUM   2
 
//gprs 
#define MSG_ID_GPRS_STATE               	MSG_ID_USER_START+0x100
#define MSG_ID_IMEI_IMSI_OK               	MSG_ID_USER_START+0x101
#define MSG_ID_SEMAPHORE_TEST           	MSG_ID_USER_START+0x102
#define MSG_ID_GET_ALL_TASK_PRIORITY    	MSG_ID_USER_START+0x103
#define MSG_ID_GET_ALL_TASK_REMAINSTACK 	MSG_ID_USER_START+0x104

//gps
#define MSG_ID_GPS_MODE_CONTROL  	        MSG_ID_USER_START+0x105
#define MSG_ID_GPS_REP_LOCATION  	        MSG_ID_USER_START+0x106
#define MSG_ID_GPS_SPEED_UP  	            MSG_ID_USER_START+0x107

#ifndef BV
#define BV(n)      (1 << (n))
#endif

#ifndef BF
#define BF(x,b,s)  (((x) & (b)) >> (s))
#endif

#ifndef MIN
#define MIN(n,m)   (((n) < (m)) ? (n) : (m))
#endif

#ifndef MAX
#define MAX(n,m)   (((n) < (m)) ? (m) : (n))
#endif

#ifndef ABS
#define ABS(n)     (((n) < 0) ? -(n) : (n))
#endif

#define SET_BIT(x,n)   ((x) | BV(n))
#define CLE_BIT(x,n)   ((x) & (~(BV(n))))

#endif // End-of __APP_COMMON_H__ 

