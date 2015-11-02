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

/*****************************************************************
* define process state
******************************************************************/
typedef enum{
    STATE_NW_GET_SIMSTATE,
    STATE_NW_QUERY_STATE,
    STATE_GPRS_REGISTER,
    STATE_GPRS_CONFIG,
    STATE_GPRS_ACTIVATE,
    STATE_GPRS_ACTIVATING,
    STATE_GPRS_GET_DNSADDRESS,
    STATE_GPRS_GET_LOCALIP,
    STATE_CHACK_SRVADDR,
    STATE_SOC_REGISTER,
    STATE_SOC_CREATE,
    STATE_SOC_CONNECT,
    STATE_SOC_CONNECTING,
    STATE_SOC_SEND,
    STATE_SOC_SENDING,
    STATE_SOC_ACK,
    STATE_SOC_CLOSE,
    STATE_GPRS_DEACTIVATE,
    STATE_TOTAL_NUM
}Enum_TCPSTATE;

/************************************************************************/
/* Declarations common variable                                         */
/************************************************************************/
extern s32 g_SocketId;

/************************************************************************/
/* Declarations for GPRS and TCP socket callback                        */
/************************************************************************/
//
// Activate GPRS and program TCP.
extern void GPRS_TCP_Program(void);
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

#endif  //__APP_SOCKET_H__

