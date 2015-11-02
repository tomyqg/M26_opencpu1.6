/*****************************************************************************
 *
 * Filename:
 * ---------
 *   app_server.h 
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

#ifndef __APP_SERVER_H__
#define __APP_SERVER_H__

/*********************************************************************
 * INCLUDES
 */

/*********************************************************************
 * MACROS
 */
 
//to big_endian
#define TOBIGENDIAN16(X)     ((((X) & 0xff00) >> 8) |(((X) & 0x00ff) << 8))
#define TOBIGENDIAN32(X)     ((((X) & 0xff000000) >> 24) | \
                              (((X) & 0x00ff0000) >> 8) | \
							  (((X) & 0x0000ff00) << 8) | \
						      (((X) & 0x000000ff) << 24))

//to small_endian
#define TOSMALLENDIAN16(X,Y)       (((X) << 8) | (Y))
#define TOSMALLENDIAN32(A,B,C,D)   (((A) << 24) | \
                             	    ((B) << 16) | \
							        ((C) << 8) | \
						             (D)) 

#define BCDTODEC(bcd) ((bcd) = ((bcd) % 16) + ((bcd)>>4) * 10)
#define DECTOBCD(bcd) ((bcd) = (((bcd) / 10)*16) + ((bcd)%10))  						   
  
/*********************************************************************
 * CONSTANTS
 */

//protocol version
#define PROTOCOL_VERSION              0x0001

//msg identifier
#define MSG_IDENTIFIER                0x74
#define MSG_IDENTIFIER_TRANSFORM      0x75  

//device id
#define HWJ_ID                        0x00000000
#define QST_ID                        0xFFFFFFFF

//server state
#define SERVER_STATE_UNKNOW               0xFF
#define SERVER_STATE_REGISTER_OK          0
#define SERVER_STATE_REGISTER_FAILURE     1
#define SERVER_STATE_REGISTERING          2

//device to server msg id
#define TOSERVER_HEARTBEAT_ID             0x0100  
#define TOSERVER_REGISTER_ID              0x0102

//server to device msg id  
#define TODEVICE_GENERIC_RSP_ID           0x8001
#define TODEVICE_REGISTER_RSP_ID          0x8002 

//heartbeat
#define HB_TIMER_ID         (TIMER_ID_USER_START + 100)
#define HB_TIMER_PERIOD     30000   //30s

//beijin timezone
#define TIMEZONE            8

/*********************************************************************
 * TYPEDEFS
 */
 
typedef struct {
    s16 protocol_version;    
    s16 msg_id;
    s16 msg_length;
    u8  device_imei[8];
    s16 msg_number;  
}Server_Msg_Head;

/*********************************************************************
 * VARIABLES
 */
 
/*********************************************************************
 * FUNCTIONS
 */
s32 Server_Msg_Send(Server_Msg_Head *msg_head, u16 msg_head_lenght,
                    u8 *msg_body, u16 msg_body_len);
u8 server_msg_calculate_checkcode(u8 *pBuffer, u16 length);
bool server_msg_transform(u8 *pBuffer, u16 length, u8 *pSend_Buffer, u16 send_length);
u8 msg_need_transform(u8 *pBuffer, u16 length);
s32 App_Server_Register(void);
void App_Heartbeat_To_Server(void);
void App_Heartbeat_Check(void);

bool Server_Msg_Handle(u8 *pbuffer,u16 numBytes);
u16 Server_Msg_Need_Retransform(u8 *pBuffer, u16 length);
u16 Server_Msg_Retransform(u8 *pbuffer,u16 numBytes);
bool Uart_Msg_Verify_Checkcode(u8 *pBuffer, u16 length );
void Server_Msg_Parse(u8* pBuffer, u16 length);

#endif  //__APP_SERVER_H__
