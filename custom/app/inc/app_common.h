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
#define VERSION "M26-V0.0.2"
#define BUILD   "2016-01-14 00:00"
 
#define SUB_TASK_NUM   4

//timer id
#define NETWOEK_STATE_TIMER_ID             (TIMER_ID_USER_START + 105)
#define NETWOEK_STATE_TIMER_PERIOD         30000   //30s

typedef enum { 
	//gprs 
	MSG_ID_GPRS_STATE = MSG_ID_USER_START+0x100,
	MSG_ID_IMEI_IMSI_OK,               	//MSG_ID_USER_START+0x101
	MSG_ID_NETWORK_STATE,               //MSG_ID_USER_START+0x102
	MSG_ID_ALARM_REP, 	                //MSG_ID_USER_START+0x103

	//gps
	MSG_ID_GPS_TIMER_REPORT,  	        //MSG_ID_USER_START+0x104
	MSG_ID_GPS_MODE_CONTROL,  	        //MSG_ID_USER_START+0x105
	MSG_ID_GPS_REP_LOCATION,  	        //MSG_ID_USER_START+0x106

	MSG_ID_CLK_ALARM,  	                //MSG_ID_USER_START+0x107
	MSG_ID_BLE_PARAMETER_UPDATA,  	    //MSG_ID_USER_START+0x108
	MSG_ID_NEW_SMS,
	MSG_ID_SYSTEM_REBOOT,
} Enum_APP_MSG_ID;

#define MSG_ID_MUTEX_TEST           	    MSG_ID_USER_START+0x151
#define MSG_ID_SEMAPHORE_TEST           	MSG_ID_USER_START+0x152
#define MSG_ID_GET_ALL_TASK_PRIORITY    	MSG_ID_USER_START+0x153
#define MSG_ID_GET_ALL_TASK_REMAINSTACK 	MSG_ID_USER_START+0x154

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

