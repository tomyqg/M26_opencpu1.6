/*****************************************************************************
 *
 * Filename:
 * ---------
 *   app_socket.h 
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

#ifndef __APP_SOCKET_H__
#define __APP_SOCKET_H__

#include "ql_time.h"
#include "ql_gprs.h"

/*****************************************************************
* define process state
******************************************************************/
typedef enum{
    STATE_NW_GET_SIMSTATE,
    STATE_NW_QUERY_STATE,
    STATE_GPRS_UNKNOWN,
    STATE_GPRS_REGISTER,
    STATE_GPRS_CONFIG,
    STATE_GPRS_ACTIVATING,
    STATE_GPRS_ACTIVATED,
    STATE_GPRS_GET_DNSADDRESS,
    STATE_GPRS_GET_LOCALIP,
    STATE_GPRS_DEACTIVATE,
    STATE_CHACK_SRVADDR,
    STATE_SOC_UNKNOWN,
    STATE_SOC_REGISTER,
    STATE_SOC_CREATED,
    STATE_SOC_CONNECTED,
    STATE_SOC_CONNECTING,
    STATE_SOC_SEND,
    STATE_SOC_SENDING,
    STATE_SOC_ACK,
    STATE_SOC_CLOSE,
    STATE_TOTAL_NUM
}Enum_TCPSTATE;
extern volatile Enum_TCPSTATE mTcpState;

typedef enum {
    ALARM_BEGIN = 0,
    ALARM_BIT_LOW_POWER = 9,
    ALARM_BIT_LOST_DOWN,
    ALARM_BIT_LOST_UP,
    ALARM_BIT_SPEED_UP,
    ALARM_END
}Enum_AlarmType;

typedef enum {
	STATE_GSM_NORMAL = 0,
	STATE_GSM_POWEROFF_SLEEP,
	STATE_GSM_NOT_CON_SERVER,
	STATE_GSM_NO_GPRS_NET,
	STATE_GSM_Ok_GPRS_NET,
	STATE_GSM_NO_GPS_FIX,
	STATE_GSM_OK_GPS_FIX,
}Enum_GSM_STATU;

#define PAR_BLOCK              13
#define PAR_BLOCK_LEN          500
#define PAR_STORED_FLAG        0xF5

/************************************************************************/
/* Declarations common variable                                         */
/************************************************************************/
extern s32 g_SocketId;
extern u8 Parameter_Buffer[PAR_BLOCK_LEN];
extern ST_GprsConfig m_GprsConfig;
extern u8 alarm_on_off;
extern bool gsm_wake_sleep;

/************************************************************************/
/* Declarations for GPRS and TCP socket callback                        */
/************************************************************************/
//
// Activate GPRS and program TCP.
extern s32 GPRS_TCP_Program(void);
//
// This callback function is invoked when GPRS actived.
extern void Callback_GPRS_Actived(u8 contexId, s32 errCode, void* customParam);
//
// This callback function is invoked when GPRS drops down.
extern void CallBack_GPRS_Deactived(u8 contextId, s32 errCode, void* customParam );
//
//
extern void Callback_Socket_Connect(s32 socketId, s32 errCode, void* customParam );
//
// This callback function is invoked when the socket connection is disconnected by server or network.
extern void Callback_Socket_Close(s32 socketId, s32 errCode, void* customParam );
//
//
extern void Callback_Socket_Accept(s32 listenSocketId, s32 errCode, void* customParam );
//
// This callback function is invoked when socket data arrives.
extern void Callback_Socket_Read(s32 socketId, s32 errCode, void* customParam );
//
// This callback function is invoked in the following case:
// The return value is less than the data length to send when calling Ql_SOC_Send(), which indicates
// the socket buffer is full. Application should stop sending socket data till this callback function
// is invoked, which indicates application can continue to send data to socket.
extern void Callback_Socket_Write(s32 socketId, s32 errCode, void* customParam );

extern void update_clk_alarm(ST_Time* dateTime);

#endif  //__APP_SOCKET_H__

