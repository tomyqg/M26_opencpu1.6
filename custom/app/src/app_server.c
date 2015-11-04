/*********************************************************************
 *
 * Filename:
 * ---------
 *   app_server.c 
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
 *====================================================================
 *             HISTORY
 *--------------------------------------------------------------------
 * 
 *********************************************************************/
/*********************************************************************
 * INCLUDES
 */
 
#include "ql_stdlib.h"
#include "ql_memory.h"
#include "ql_socket.h"
#include "ql_time.h"
#include "ql_timer.h"
#include "app_debug.h"
#include "app_socket.h"
#include "app_server.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * VARIABLES
 */

extern u8 g_imei[8];
extern u8 g_imsi[8];
 
static s16 g_msg_number = 0;

static s8 hw_sw_version[2][4] = {
  {0x15,0x10,0x30,0x01},
  {0x15,0x10,0x30,0x01}
};

static u8 gRegister_Count = 9;
static u8 gServer_State = SERVER_STATE_UNKNOW;

bool greceived_heartbeat_rsp = TRUE;
static u16 gmsg_num_heartbeat;

/*********************************************************************
 * FUNCTIONS
 */

static void Timer_Handler(u32 timerId, void* param);

/*********************************************************************
 * @fn      Server_Msg_Send
 *
 * @brief   Server_Msg_Send 
 *
 * @param   
 *
 * @return  
 *********************************************************************/
s32 Server_Msg_Send(Server_Msg_Head *msg_head, u16 msg_head_lenght,
                    u8 *msg_body, u16 msg_body_len)
{
  u8 count;
  u8 *msg_buff, *msg_send_buff;
  u16 msg_buff_len = 3+msg_head_lenght+msg_body_len;
  u16 msg_send_buff_len;
  
  msg_buff = (u8 *)Ql_MEM_Alloc(msg_buff_len);
  
  //change head to bg_endian
  msg_head->protocol_version = TOBIGENDIAN16(msg_head->protocol_version);
  msg_head->msg_id = TOBIGENDIAN16(msg_head->msg_id);
  msg_head->msg_length = TOBIGENDIAN16(msg_head->msg_length);
  msg_head->msg_number = TOBIGENDIAN16(msg_head->msg_number);
  
  msg_buff[0] = MSG_IDENTIFIER;
  Ql_memcpy(msg_buff+1,msg_head,msg_head_lenght);
  Ql_memcpy(msg_buff+msg_head_lenght+1,msg_body,msg_body_len);
  
  msg_buff[msg_buff_len-2] = server_msg_calculate_checkcode(msg_buff, msg_buff_len);
  msg_buff[msg_buff_len-1] = MSG_IDENTIFIER;
  
  count = msg_need_transform(msg_buff, msg_buff_len);
  if(count > 0)
  {
    msg_send_buff_len = msg_buff_len + count;
    msg_send_buff = (u8 *)Ql_MEM_Alloc(msg_send_buff_len);
  	server_msg_transform(msg_buff, msg_buff_len,msg_send_buff,msg_send_buff_len);
  }
  else
  {
	msg_send_buff = NULL;
  }
  
  if(msg_send_buff == NULL)
  {
	 s32 ret;
	 
	 #if 1
     for(int i = 0; i < msg_buff_len; i++)
     	APP_DEBUG("0x%02x ",msg_buff[i]);
     APP_DEBUG("\r\n");
     #endif
     
	 ret = Ql_SOC_Send(g_SocketId, msg_buff, msg_buff_len);
     if (ret != msg_buff_len)
     {
       	APP_ERROR("App Fail to send socket data.\r\n");
       	Ql_SOC_Close(g_SocketId);
       //return -1;
     }
  }
  else
  {
	 s32 ret;

	 #if 1
     for(int i = 0; i < msg_send_buff_len; i++)
     	APP_DEBUG("0x%02x ",msg_send_buff[i]);
     APP_DEBUG("\r\n");
     #endif
	 
	 ret = Ql_SOC_Send(g_SocketId, msg_send_buff, msg_send_buff_len);
     if (ret != msg_send_buff_len)
     {
       	APP_ERROR("App Fail to send socket data.\r\n");
       	Ql_SOC_Close(g_SocketId);
       //return -1;
     }
     Ql_MEM_Free(msg_send_buff);
  }
  
  Ql_MEM_Free(msg_buff);
  return 0;
}

/*********************************************************************
 * @fn      server_msg_calculate_checkcode
 *
 * @brief   server_msg_calculate_checkcode
 *
 * @param   pBuffer: msg; length: msg length
 *
 * @return  check code
 *********************************************************************/
u8 server_msg_calculate_checkcode(u8 *pBuffer, u16 length)
{
    u8 i;
    u8 checkCode, checkCodeLocation;
    checkCode = pBuffer[1];
    checkCodeLocation = length - 2;
    for( i = 2; i < checkCodeLocation; i++ )
    {
      checkCode = checkCode ^ pBuffer[i];
    }
    return checkCode;
}

/*********************************************************************
 * @fn      server_msg_transform
 *
 * @brief   Transform msg 
 *
 * @param   pBuffer: msg; length: msg length
 *
 * @return  true: transform ok;false:transform failed
 *********************************************************************/
bool server_msg_transform(u8 *pBuffer, u16 length, u8 *pSend_Buffer, u16 send_length)
{
	u8 i, j;
    for( i = 1, j = 1; i < length - 1; i++)
    {
        if( pBuffer[i] == MSG_IDENTIFIER || pBuffer[i] == MSG_IDENTIFIER_TRANSFORM )
        {
            if( pBuffer[i] == MSG_IDENTIFIER )
            {
                pSend_Buffer[j] = MSG_IDENTIFIER_TRANSFORM;
                j++;
                pSend_Buffer[j] = 0x02;
                j++;
            }
            else if( pBuffer[i] == MSG_IDENTIFIER_TRANSFORM )
            {
                pSend_Buffer[j] = MSG_IDENTIFIER_TRANSFORM;
                j++;
                pSend_Buffer[j] = 0x01;
                j++;
            }
        }
        else
        {
            pSend_Buffer[j] = pBuffer[i];
            j++;
        } 
        
    }
    
    pSend_Buffer[0] = MSG_IDENTIFIER;
	pSend_Buffer[j] = MSG_IDENTIFIER;
	if(send_length == (j + 1))
    	return TRUE;
    else
    	return FALSE;
}

/*********************************************************************
 * @fn      msg_need_transform
 *
 * @brief   msg_need_transform
 *
 * @param   pBuffer: msg; length: msg length
 *
 * @return  
 *********************************************************************/
u8 msg_need_transform(u8 *pBuffer, u16 length)
{
	u16 i;
	u16 m = length - 1;
	u8 count = 0;
	
	for(i = 1; i < m; i++)
	{
		if( pBuffer[i] == MSG_IDENTIFIER ||
			pBuffer[i] == MSG_IDENTIFIER_TRANSFORM )
		{
			count++;
		}	
	}

	return count;
}

/*********************************************************************
 * @fn      Server_Msg_Handle
 *
 * @brief   Server_Msg_Handle
 *
 * @param   
 *
 * @return  
 *********************************************************************/
bool Server_Msg_Handle(u8 *pbuffer,u16 numBytes)
{
	u16 retransform_count;

    #if 1
    for(int i = 0; i < numBytes; i++)
    	APP_DEBUG("0x%02x ",pbuffer[i]);
    APP_DEBUG("\r\n");
    #endif
	
	retransform_count = Server_Msg_Need_Retransform(pbuffer,numBytes);
	if(retransform_count > 0)
	{
		//need to retransform
		numBytes = Server_Msg_Retransform(pbuffer,numBytes);
	}
	
    if ( !Uart_Msg_Verify_Checkcode(pbuffer, numBytes ) )
    {
		APP_ERROR("check code error\n");
      	return FALSE;
    }
    Server_Msg_Parse(pbuffer, numBytes);

    return TRUE;
}

/*********************************************************************
 * @fn      Server_Msg_Need_Retransform
 *
 * @brief   Server_Msg_Need_Retransform
 *
 * @param   pBuffer: msg; length: msg length
 *
 * @return  
 *********************************************************************/
u16 Server_Msg_Need_Retransform(u8 *pBuffer, u16 length)
{
	u16 i;
	u16 m = length - 1;
	u16 count = 0;
	
	for(i = 1; i < m; i++)
	{
		if( pBuffer[i] == MSG_IDENTIFIER_TRANSFORM &&
		  ((pBuffer[i+1] == 0x01) || (pBuffer[i+1] == 0x02)))
		{
			count++;
		}	
	}

	return count;
}

/*********************************************************************
 * @fn      Server_Msg_Retransform
 *
 * @brief   Server_Msg_Retransform
 *
 * @param   
 *
 * @return  
 *********************************************************************/
u16 Server_Msg_Retransform(u8 *pBuffer,u16 numBytes)
{
	u16 i,j;
	for(i = 1, j = 0; i < numBytes; i++)
	{
		pBuffer[i-j] = pBuffer[i];
		if( pBuffer[i] == MSG_IDENTIFIER_TRANSFORM && (pBuffer[i+1] == 0x01))
		{
			i++;
			j++;
			continue;
			
		}
		else if( pBuffer[i] == MSG_IDENTIFIER_TRANSFORM && (pBuffer[i+1] == 0x02))
		{
			pBuffer[i-j] = MSG_IDENTIFIER;
			i++;
			j++;
			continue;
		}	
	}
	
	//for(int i = 0; i < numBytes - j; i++)
    	//APP_DEBUG("0x%x ",pBuffer[i]);

    return (numBytes - j);

}

/*********************************************************************
 * @fn      Uart_Msg_Verify_Checkcode
 *
 * @brief   
 *
 * @param   pBuffer: msg; length: msg length
 *
 * @return  None
 *********************************************************************/
bool Uart_Msg_Verify_Checkcode(u8 *pBuffer, u16 length )
{
    u16 checkCode_Location = length - 2;
    u8 checkCode;
    
    checkCode = server_msg_calculate_checkcode(pBuffer,length );
    
    if (checkCode != pBuffer[checkCode_Location])
    {
        return FALSE;
    }
    return TRUE;
}

/*********************************************************************
 * @fn      Server_Msg_Parse
 *
 * @brief   Server_Msg_Parse
 *
 * @param   
 *
 * @return  
 *********************************************************************/
void Server_Msg_Parse(u8* pBuffer, u16 length)
{
    u16 command_id = TOSMALLENDIAN16(pBuffer[3],pBuffer[4]);

    switch(command_id)
    {
        case TODEVICE_GENERIC_RSP_ID:
        {
            u16 rsp_msg_id_num = TOSMALLENDIAN16(pBuffer[17],pBuffer[18]);
			u16 rsp_msg_id = TOSMALLENDIAN16(pBuffer[19],pBuffer[20]);
		    if(rsp_msg_id == TOSERVER_HEARTBEAT_ID && rsp_msg_id_num == gmsg_num_heartbeat)
			{
			  	greceived_heartbeat_rsp = TRUE;
			}  
        }    
        break;
		
        case TODEVICE_REGISTER_RSP_ID:
		    if(pBuffer[21] == SERVER_STATE_REGISTER_OK)
			{ 
				APP_DEBUG("register ok\n");
			    gServer_State = SERVER_STATE_REGISTER_OK;
			
			  	//set system time
			  	ST_Time datetime;
			  	for(u8 i = 0; i < 6; i++)
			  	{	
					BCDTODEC(pBuffer[22+i]);
					//APP_DEBUG("time: %d\r\n",pBuffer[22+i]);
			  	}
			  	//time start from 2000-1-1-00:00:00
				datetime.year=pBuffer[22] + 2000;
				datetime.month=pBuffer[23];
				datetime.day=pBuffer[24];
				datetime.hour=pBuffer[25];
				datetime.minute=pBuffer[26];
				datetime.second=pBuffer[27];
				datetime.timezone=TIMEZONE;
  			  	Ql_SetLocalTime(&datetime);
			}
			else
			{
			  	//register failure
			  	gServer_State = SERVER_STATE_REGISTER_FAILURE;
			  	APP_DEBUG("register failure\n");
			}
        	break;
			
        default:                
            break;
    }                  
}

/*********************************************************************
 * @fn      Timer_Handler
 *
 * @brief   timer callback function
 *
 * @param   timer id;
 *
 * @return  
 *********************************************************************/
void Timer_Handler(u32 timerId, void* param)
{
    if(HB_TIMER_ID == timerId)
    {
		if(gServer_State == SERVER_STATE_REGISTER_OK)
		{
			//heartbeat send
			//App_Heartbeat_To_Server();
			App_Heartbeat_Check();
			gRegister_Count = 9;
		}
		else if(gServer_State == SERVER_STATE_REGISTERING && gRegister_Count > 0)
		{
			App_Server_Register();
		}
		else
		{
			gRegister_Count = 9;
			Ql_Timer_Stop(HB_TIMER_ID);
			APP_ERROR("register server timeout for serveral times\r\n");
		}
    }
}

/*********************************************************************
 * @fn      App_Server_Register
 *
 * @brief   App_Server_Register
 *
 * @param   
 *
 * @return  
 *********************************************************************/
s32 App_Server_Register( void )
{
	APP_DEBUG("register to server\n");

    //head
	Server_Msg_Head m_Server_Msg_Head;
	m_Server_Msg_Head.protocol_version = PROTOCOL_VERSION;
	m_Server_Msg_Head.msg_id= TOSERVER_REGISTER_ID;
	m_Server_Msg_Head.msg_length = 20;
	Ql_memcpy(m_Server_Msg_Head.device_imei, g_imei, 8);
	m_Server_Msg_Head.msg_number = g_msg_number++;

	//body
  	u8 msg_body[20];
  	for(int i = 0; i < 4; i++)
		msg_body[i] = 0xFF;
  
  	Ql_memcpy(msg_body+4,g_imsi,8);
  	Ql_memcpy(msg_body+12,hw_sw_version,8);

    //register a timer to start heartbeat
    u32 ret;
    ret = Ql_Timer_Register(HB_TIMER_ID, Timer_Handler, NULL);
    if(ret <0)
    {
        APP_ERROR("\r\nfailed!!, Timer heartbeat register: timer(%d) fail ,ret = %d\r\n",HB_TIMER_ID,ret);
    }
	//start a timer,repeat=true;
	ret = Ql_Timer_Start(HB_TIMER_ID,HB_TIMER_PERIOD,TRUE);
	if(ret < 0)
	{
		APP_ERROR("\r\nfailed!!, Timer heartbeat start fail ret=%d\r\n",ret);        
	}
	gServer_State = SERVER_STATE_REGISTERING;
	gRegister_Count--;

	Server_Msg_Send(&m_Server_Msg_Head, 16, msg_body, 20);
	
	return TRUE;
}

/*********************************************************************
 * @fn      App_Heartbeat_To_Server
 *
 * @brief   App_Heartbeat_To_Server
 *
 * @param   
 *
 * @return  
 *********************************************************************/
void App_Heartbeat_To_Server( void )
{
	//APP_DEBUG("heartbeat to server\n");
    //head
	Server_Msg_Head m_Server_Msg_Head;
	m_Server_Msg_Head.protocol_version = PROTOCOL_VERSION;
	m_Server_Msg_Head.msg_id= TOSERVER_HEARTBEAT_ID;
	m_Server_Msg_Head.msg_length = 20;
	Ql_memcpy(m_Server_Msg_Head.device_imei, g_imei, 8);
	m_Server_Msg_Head.msg_number = g_msg_number++;
  
  	gmsg_num_heartbeat = m_Server_Msg_Head.msg_number;
  	greceived_heartbeat_rsp = FALSE;
  
	//body
	u8 msg_body[20];
	
	for(int i = 0; i < 4; i++)
		msg_body[i] = 0x0;
	  
	//LAC&CI
	Ql_memset(msg_body+4,0x55,8);
	//IMEI  
	Ql_memset(msg_body+12,0xff,8);
	  
	//pack msg
	Server_Msg_Send(&m_Server_Msg_Head,16,msg_body,20);
  
  	return;
}

/*********************************************************************
 * @fn      App_Heartbeat_Check
 *
 * @brief   App_Heartbeat_Check
 *
 * @param   
 *
 * @return  
 *********************************************************************/
void App_Heartbeat_Check(void)
{
  	static u8 lost_hearbeat_rsp_count = 1;
  	if(greceived_heartbeat_rsp)
  	{
		App_Heartbeat_To_Server();	
  	}
  	else
  	{
  		if(lost_hearbeat_rsp_count > 5)
		{
	  		//hearbeat timeout
	  		lost_hearbeat_rsp_count = 1;
	  		Ql_Timer_Stop(HB_TIMER_ID);
	  		//reconnect to server again
	  		gServer_State = SERVER_STATE_UNKNOW;
	  		App_Server_Register();
		}
		else
		{
	  		lost_hearbeat_rsp_count++;
	  		App_Heartbeat_To_Server();
		}
  	}
}
