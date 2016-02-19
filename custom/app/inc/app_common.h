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
#define VERSION "16021800"
#define BUILD   "2016-02-18 00:00"

extern s8 hw_sw_version[2][4];
 
#define SUB_TASK_NUM   4

//timer id
#define NETWOEK_STATE_TIMER_ID             (TIMER_ID_USER_START + 105)
#define NETWOEK_STATE_TIMER_PERIOD         30000   //30s

#define MOTIONAL_STATIC_TIMER_ID           (TIMER_ID_USER_START + 106)
#define MOTIONAL_STATIC_TIMER_PERIOD       6000   //6s

typedef enum { 
	//gprs 
	MSG_ID_GPRS_STATE = MSG_ID_USER_START+0x100,
	MSG_ID_GSM_STATE,
	MSG_ID_IMEI_IMSI_OK,
	MSG_ID_NETWORK_STATE,
	MSG_ID_ALARM_REP,

	//gps
	MSG_ID_GPS_TIMER_REPORT,
	MSG_ID_GPS_MODE_CONTROL,
	MSG_ID_GPS_REP_LOCATION,
	MSG_ID_GPS_FIX_STATUS,

	MSG_ID_CLK_ALARM,
	MSG_ID_BLE_PARAMETER_UPDATA,
	MSG_ID_NEW_SMS,
	MSG_ID_SYSTEM_REBOOT,
	MSG_ID_SRV_CHANGED,
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

