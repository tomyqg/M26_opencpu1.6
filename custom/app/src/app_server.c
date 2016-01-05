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
#ifdef __CUSTOMER_CODE__ 
/*********************************************************************
 * INCLUDES
 */
 
#include "ql_stdlib.h"
#include "ql_memory.h"
#include "ql_socket.h"
#include "ql_time.h"
#include "ql_timer.h"
#include "ql_error.h"
#include "ql_gprs.h"
#include "ril_system.h"
#include "fota_main.h"
#include "ril_network.h"
#include "app_common.h"
#include "app_socket.h"
#include "app_server.h"
#include "app_gps.h"

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
 
static u16 g_msg_number = 0;

static s8 hw_sw_version[2][4] = {
  {0x15,0x10,0x30,0x01},
  {0x15,0x10,0x30,0x01}
};

static u8 gRegister_Count = 9;
u8 gServer_State = SERVER_STATE_UNKNOW;

bool greceived_heartbeat_rsp = TRUE;
static u16 gmsg_num_heartbeat;

Parameter gParmeter = {
	{{0x0001,4,30},
	 {0x0002,4,10},
	 {0x0003,4,10},
	 {0x0004,4,5},
	 {0x0019,4,0},
	 {0x0100,4,0xffffffff},
	 {0x0101,4,0xffffffff},
	 {0x0102,4,0xffffffff},
	 {0x0104,3,0x233000},
	 {0x0105,4,0xffffffff},
	 {0x0108,4,0},
	 {0x0109,4,0xffffffff},
	 {0x010A,4,0},
	 {0x0110,4,20},
	 {0x0111,4,20},
	 {0x0112,4,1},
	 {0x0113,4,1},
	 {0x0200,4,120},
	 {0x0201,4,15},
	 {0x0202,4,10},
	 {0x0203,4,0xffffffff},
	 {0x0210,4,30},
	 {0x0300,4,0x0}},
	 
	{{0x0301,8,{0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff}},
	 {0x0302,8,{0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff}},
	 {0x0303,8,{0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0}},
	 {0x0304,8,{0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0}},
	 {0x0305,6,{0x15,0x11,0x05,0x16,0x51,0x53}}}
};	 

Lac_CellID glac_ci;
GpsLocation gGpsLocation;

SYS_CONFIG mSys_config = {
	//mSys_config.gLocation_Policy;
	{
    	1,   //location_policy;    
    	1,   //static_policy;
    	30,  //time_Interval;
    	100, //distance_Interval;
    	45,  //bearing_Interval;
	},
	//mSys_config.password
	{'1','2','3','4'},
    "CMNET",          //APN name
    "",               //User name for APN
    "",               //Password for APN
    {54,223,54,184},  //srv addr
    8300,             //srv port
};

Alarm_Flag gAlarm_Flag = {
	0x0,
	0x0,
};

bool work_mode = FALSE;
static bool alarm_timer_started = FALSE;
extern ST_GprsConfig m_GprsConfig;

/*********************************************************************
 * FUNCTIONS
 */

void Timer_Handler(u32 timerId, void* param);

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
  u8 *msg_buff, *msg_send_buff;
  u16 msg_buff_len = 3+msg_head_lenght+msg_body_len;
  u16 msg_send_buff_len;
  u16 count;
  s32 ret;
  
  msg_buff = (u8 *)Ql_MEM_Alloc(msg_buff_len);
  if (msg_buff == NULL)
  {
  	 APP_ERROR("%s/%d:Ql_MEM_Alloc FAIL! size:%u\r\n", __func__, __LINE__,msg_buff_len);
     return APP_RET_ERR_MEM_OLLOC;
  }
  
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
    if (msg_send_buff == NULL)
  	{
  	 	APP_ERROR("%s/%d:Ql_MEM_Alloc FAIL! size:%u\r\n", __func__, __LINE__,msg_send_buff_len);
     	return APP_RET_ERR_MEM_OLLOC;
  	}
  	server_msg_transform(msg_buff, msg_buff_len,msg_send_buff,msg_send_buff_len);
  	Ql_MEM_Free(msg_buff);
  	msg_buff = NULL;
  }
  else
  {
	msg_send_buff_len = msg_buff_len;
	msg_send_buff = msg_buff;
  }
  
  #if 0
  	for(int i = 0; i < msg_send_buff_len; i++)
     	APP_DEBUG("%02x",msg_send_buff[i]);
    APP_DEBUG("\r\n");
  #endif

  if(gServer_State != SERVER_STATE_REGISTER_OK && gServer_State != SERVER_STATE_REGISTERING)
  {
	return APP_RET_ERR_SEV_NOT_REGISTER;
  }	

  ret = Ql_SOC_Send(g_SocketId, msg_send_buff, msg_send_buff_len);
  if (ret != msg_send_buff_len)
  {
  	APP_ERROR("App Fail to send socket data.\r\n");
  	Ql_MEM_Free(msg_send_buff);
    msg_send_buff = NULL;
    return APP_RET_ERR_SOC_SEND;
  }
  
  Ql_MEM_Free(msg_send_buff);
  msg_send_buff = NULL;
  return APP_RET_OK;
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
    u8 checkCode;
    u16 checkCodeLocation;
    checkCode = pBuffer[1];
    checkCodeLocation = length - 2;
    for(u16 i = 2; i < checkCodeLocation; i++ )
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
	u16 i, j;
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
u16 msg_need_transform(u8 *pBuffer, u16 length)
{
	u16 i;
	u16 m = length - 1;
	u16 count = 0;
	
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

    #if 0
    for(int i = 0; i < numBytes; i++)
    	APP_DEBUG("%02x ",pbuffer[i]);
    APP_DEBUG("\r\n");
    #endif
	
	retransform_count = Server_Msg_Need_Retransform(pbuffer,numBytes);
	if(retransform_count > 0)
	{
		//need to retransform
		numBytes = Server_Msg_Retransform(pbuffer,numBytes);
	}
	
    if ( !Server_Msg_Verify_Checkcode(pbuffer, numBytes ) )
    {
		APP_ERROR("check code error:%s\n",__func__);
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
 * @fn      Server_Msg_Verify_Checkcode
 *
 * @brief   
 *
 * @param   pBuffer: msg; length: msg length
 *
 * @return  None
 *********************************************************************/
bool Server_Msg_Verify_Checkcode(u8 *pBuffer, u16 length)
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
    u16 msg_attribute = TOSMALLENDIAN16(pBuffer[5],pBuffer[6]);
    u16 msg_num = TOSMALLENDIAN16(pBuffer[15],pBuffer[16]);
    //APP_DEBUG("Server_Msg_Parse 0x%x\n",command_id);
    switch(command_id)
    {
        case TODEVICE_GENERIC_RSP_ID:
        {
            u16 rsp_msg_id_num = TOSMALLENDIAN16(pBuffer[17],pBuffer[18]);
			u16 rsp_msg_id = TOSMALLENDIAN16(pBuffer[19],pBuffer[20]);
		    if(rsp_msg_id == TOSERVER_HEARTBEAT_ID && rsp_msg_id_num == gmsg_num_heartbeat)
			{
			  	greceived_heartbeat_rsp = TRUE;
			  	APP_DEBUG("heartbeat ok\n");
			}  
        }    
        break;
		
        case TODEVICE_REGISTER_RSP_ID:
        {
		    if(pBuffer[21] == SERVER_STATE_REGISTER_OK)
			{ 
				APP_DEBUG("register ok\n");
			    gServer_State = SERVER_STATE_REGISTER_OK;
			
			  	//set system time
			  	ST_Time datetime;
			  	//time start from 2000-1-1-00:00:00
				datetime.year=BCDTODEC(pBuffer[22]) + 2000;
				datetime.month=BCDTODEC(pBuffer[23]);
				datetime.day=BCDTODEC(pBuffer[24]);
				datetime.hour=BCDTODEC(pBuffer[25]);
				datetime.minute=BCDTODEC(pBuffer[26]);
				datetime.second=BCDTODEC(pBuffer[27]);
				datetime.timezone=TIMEZONE;
  			  	Ql_SetLocalTime(&datetime);
  			  	Ql_Sleep(20);
  			  	update_clk_alarm(&datetime);
  			  	//APP_DEBUG("time: %d-%d-%d %d:%d:%d\n",datetime.year,datetime.month,datetime.day,datetime.hour,datetime.minute,datetime.second);
			}
			else
			{
			  	//register failure
			  	gServer_State = SERVER_STATE_REGISTER_FAILURE;
			  	APP_DEBUG("register failure\n");
			}
        	break;
        }	

        case TODEVICE_SET_PARAMETER_ID:
        {
			App_Set_Parameter(pBuffer,length);
			App_Report_Parameter(TODEVICE_SET_PARAMETER_ID, msg_num);
        	break;
        }	

        case TODEVICE_GET_PARAMETER_ID:
        {
			App_Report_Parameter(TODEVICE_GET_PARAMETER_ID, msg_num);
        	break;
        }

        case TODEVICE_SET_SLEEP_MODE_ID:
        {
			App_CommonRsp_To_Server(TODEVICE_SET_SLEEP_MODE_ID,msg_num,APP_RSP_OK);
			App_Set_Sleep_mode(pBuffer,length);
        	break;
        }

        case TODEVICE_REQUEST_LOCATION_ID:
        {
			if(!msg_attribute)
			{
				App_Report_Location();
			}
        	break;
        }

        case TODEVICE_LOCATION_POLICY_ID:
        {
			App_CommonRsp_To_Server(TODEVICE_LOCATION_POLICY_ID,msg_num,APP_RSP_OK);
			App_Set_Location_policy(pBuffer,length);
        	break;
        }

        case TODEVICE_OTA_ID:
        {
			u8 URL_Buffer[100];
			s32 ret;
			
			App_CommonRsp_To_Server(TODEVICE_OTA_ID,msg_num,APP_RSP_OK);
			
            Ql_sprintf(URL_Buffer, "http://%d.%d.%d.%d:%d/%.*s",
            				pBuffer[17],pBuffer[18],pBuffer[19],pBuffer[20],
            				TOSMALLENDIAN16(pBuffer[21],pBuffer[22]),msg_attribute-6,pBuffer+23);
            APP_DEBUG("\r\n<-- URL:%s-->\r\n",URL_Buffer);
    		ST_GprsConfig GprsConfig;
    		Ql_memcpy(&GprsConfig,&m_GprsConfig,sizeof(ST_GprsConfig));
    		Ql_strcpy(GprsConfig.apnName,mSys_config.apnName);
    		Ql_strcpy(GprsConfig.apnPasswd,mSys_config.apnPasswd);
    		Ql_strcpy(GprsConfig.apnUserId,mSys_config.apnUserId);
    		
            ret = Ql_FOTA_StartUpgrade(URL_Buffer, &m_GprsConfig, NULL);
            if(ret != SOC_SUCCESS)
            {
				APP_ERROR("\r\n<-- ota start upgrade fail ret:%d-->\r\n",ret);
				//App_CommonRsp_To_Server(TODEVICE_OTA_ID,msg_num,APP_RSP_FAILURE);
            }
        	break;
        }
			
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
void Timer_Handler_HB(u32 timerId, void* param)
{
    if(HB_TIMER_ID == timerId)
    {
		if(gServer_State == SERVER_STATE_REGISTER_OK)
		{
			//heartbeat send
			//App_Heartbeat_To_Server();
			Ql_Timer_Start(HB_TIMER_ID,gParmeter.parameter_8[HEARTBEAT_INTERVAL_INDEX].data*1000,FALSE);
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
 * @fn      Timer_Handler_Alarm
 *
 * @brief   timer callback function
 *
 * @param   timer id;
 *
 * @return  
 *********************************************************************/
void Timer_Handler_Alarm(u32 timerId, void* param)
{
    if(ALARM_TIMER_ID == timerId)
    {
		alarm_timer_started = FALSE;
		if(gAlarm_Flag.alarm_flags == 0)
		{
			gAlarm_Flag.alarm_flags_bk = 0;
		}
		else
		{
			App_Ropert_Alarm();
			gAlarm_Flag.alarm_flags_bk = gAlarm_Flag.alarm_flags_bk & gAlarm_Flag.alarm_flags;
		}
    }
}

/*********************************************************************
 * @fn      App_CommonRsp_To_Server
 *
 * @brief   App_CommonRsp_To_Server
 *
 * @param   
 *
 * @return  
 *********************************************************************/
s32 App_CommonRsp_To_Server( u16 msg_id, u16 msg_number, u8 rsp)
{
	APP_DEBUG("App_CommonRsp_To_Server\n");

    //head
	Server_Msg_Head m_Server_Msg_Head;
	m_Server_Msg_Head.protocol_version = PROTOCOL_VERSION;
	m_Server_Msg_Head.msg_id= TOSERVER_COMMON_RSP_ID;
	m_Server_Msg_Head.msg_length = 5;
	Ql_memcpy(m_Server_Msg_Head.device_imei, g_imei, 8);
	m_Server_Msg_Head.msg_number = ++g_msg_number;

	//body
  	u8 msg_body[m_Server_Msg_Head.msg_length];
  	msg_id = TOBIGENDIAN16(msg_id);
	msg_number = TOBIGENDIAN16(msg_number);
    Ql_memcpy(msg_body,&msg_number,2);
    Ql_memcpy(msg_body+2,&msg_id,2);
    msg_body[4] = rsp;
  
  	Server_Msg_Send(&m_Server_Msg_Head, 16, msg_body, m_Server_Msg_Head.msg_length);
	return APP_RET_OK;
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

	gServer_State = SERVER_STATE_UNKNOW;

    //head
	Server_Msg_Head m_Server_Msg_Head;
	m_Server_Msg_Head.protocol_version = PROTOCOL_VERSION;
	m_Server_Msg_Head.msg_id= TOSERVER_REGISTER_ID;
	m_Server_Msg_Head.msg_length = 20;
	Ql_memcpy(m_Server_Msg_Head.device_imei, g_imei, 8);
	m_Server_Msg_Head.msg_number = ++g_msg_number;

	//body
  	u8 msg_body[20];
  	for(int i = 0; i < 4; i++)
		msg_body[i] = 0xFF;
  
  	Ql_memcpy(msg_body+4,g_imsi,8);
  	Ql_memcpy(msg_body+12,hw_sw_version,8);

	//start a timer,repeat=true;
	s32 ret;
	ret = Ql_Timer_Start(HB_TIMER_ID,gParmeter.parameter_8[RSP_TIMEOUT_INDEX].data*1000,FALSE);
	if(ret < 0)
	{
		APP_ERROR("\r\nfailed!!, Timer heartbeat start fail ret=%d\r\n",ret);        
	}
	gServer_State = SERVER_STATE_REGISTERING;
	gRegister_Count--;

	Server_Msg_Send(&m_Server_Msg_Head, 16, msg_body, 20);
	
	return APP_RET_OK;
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
	APP_DEBUG("heartbeat to server\n");
	
    //head
	Server_Msg_Head m_Server_Msg_Head;
	m_Server_Msg_Head.protocol_version = PROTOCOL_VERSION;
	m_Server_Msg_Head.msg_id= TOSERVER_HEARTBEAT_ID;
	m_Server_Msg_Head.msg_length = 20;
	Ql_memcpy(m_Server_Msg_Head.device_imei, g_imei, 8);
	m_Server_Msg_Head.msg_number = ++g_msg_number;
  
  	gmsg_num_heartbeat = m_Server_Msg_Head.msg_number;
  	greceived_heartbeat_rsp = FALSE;
  
	//body
	u8 msg_body[20];
	
	for(int i = 0; i < 4; i++)
		msg_body[i] = 0x0;
	  
	//LAC&CI
	Ql_memcpy(msg_body+4,&glac_ci,8);
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
  		if(lost_hearbeat_rsp_count > gParmeter.parameter_8[LOST_HEARTBEAT_RSP_MAX_INDEX].data)
		{
			APP_DEBUG("lost hearbeat rsp count = %d\n",lost_hearbeat_rsp_count);
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

/*********************************************************************
 * @fn      App_Report_Parameter
 *
 * @brief   App_Report_Parameter
 *
 * @param   
 *
 * @return  
 *********************************************************************/
void App_Report_Parameter(u16 msg_id, u16 msg_number)
{
    u16 j;
    APP_DEBUG("app report parameter\n");
    
    //head
	Server_Msg_Head m_Server_Msg_Head;
	m_Server_Msg_Head.protocol_version = PROTOCOL_VERSION;
	m_Server_Msg_Head.msg_id= TOSERVER_PARAMETER_ID;
	m_Server_Msg_Head.msg_length = sizeof(gParmeter) + 7;
	Ql_memcpy(m_Server_Msg_Head.device_imei, g_imei, 8);
	m_Server_Msg_Head.msg_number = ++g_msg_number;
    
	//body
	u8 *msg_body;
	msg_body = (u8 *)Ql_MEM_Alloc(m_Server_Msg_Head.msg_length);
	Ql_memset(msg_body, 0, m_Server_Msg_Head.msg_length);
	
	msg_id = TOBIGENDIAN16(msg_id);
	msg_number = TOBIGENDIAN16(msg_number);
    Ql_memcpy(msg_body,&msg_number,2);
    Ql_memcpy(msg_body+2,&msg_id,2);
    msg_body[4] = APP_RSP_OK;
    
    msg_body[5] = 0;
    msg_body[6] = parameter_8_num + parameter_12_num;

    j = 6;
	for(u16 i = 0; i < parameter_8_num; i++)
	{
        if(gParmeter.parameter_8[i].length != 4)
        {
			if(gParmeter.parameter_8[i].length == 3)
			{
				msg_body[++j] = gParmeter.parameter_8[i].id >> 8;
				msg_body[++j] = gParmeter.parameter_8[i].id;
				msg_body[++j] = gParmeter.parameter_8[i].length >> 8;
				msg_body[++j] = gParmeter.parameter_8[i].length;
				msg_body[++j] = gParmeter.parameter_8[i].data >> 16;
				msg_body[++j] = gParmeter.parameter_8[i].data >> 8;
				msg_body[++j] = gParmeter.parameter_8[i].data;
				#if 0
				for(s8 i =6; i >= 0; i--)
				{
				 	APP_DEBUG("%02x",msg_body[j-i]);
	            }
	            #endif
			}
			continue;
		}	
		msg_body[++j] = gParmeter.parameter_8[i].id >> 8;
		msg_body[++j] = gParmeter.parameter_8[i].id;
		msg_body[++j] = gParmeter.parameter_8[i].length >> 8;
		msg_body[++j] = gParmeter.parameter_8[i].length;
		msg_body[++j] = gParmeter.parameter_8[i].data >> 24;
		msg_body[++j] = gParmeter.parameter_8[i].data >> 16;
		msg_body[++j] = gParmeter.parameter_8[i].data >> 8;
		msg_body[++j] = gParmeter.parameter_8[i].data;
        #if 0
		for(s8 i = 7; i >= 0; i--)
		{
			APP_DEBUG("%02x",msg_body[j-i]);
        }
        #endif
	}
	
    for(u16 i = 0; i < parameter_12_num; i++)
	{
		if(gParmeter.parameter_12[i].length != 8)
		{
			if(gParmeter.parameter_12[i].length == 6)
			{
				msg_body[++j] = gParmeter.parameter_12[i].id >> 8;
				msg_body[++j] = gParmeter.parameter_12[i].id;
				msg_body[++j] = gParmeter.parameter_12[i].length >> 8;
				msg_body[++j] = gParmeter.parameter_12[i].length;

				Ql_memcpy((msg_body+j+1), gParmeter.parameter_12[i].data, 6);
				j += 6;
	            #if 0
				for(s8 i =9; i >= 0; i--)
					APP_DEBUG("%02x",msg_body[j-i]);
				#endif
			}
			continue;
		}
		
		msg_body[++j] = gParmeter.parameter_12[i].id >> 8;
		msg_body[++j] = gParmeter.parameter_12[i].id;
		msg_body[++j] = gParmeter.parameter_12[i].length >> 8;
		msg_body[++j] = gParmeter.parameter_12[i].length;
		
		Ql_memcpy((msg_body+j+1), gParmeter.parameter_12[i].data, 8);
		j += 8;
        #if 0
		for(s8 i =11; i >= 0; i--)
			APP_DEBUG("%02x",msg_body[j-i]);
		#endif
	}

	m_Server_Msg_Head.msg_length = j + 1;
	//pack msg
	Server_Msg_Send(&m_Server_Msg_Head,16,msg_body,m_Server_Msg_Head.msg_length);

	Ql_MEM_Free(msg_body);
	msg_body = NULL;
  
  	return;  
}

/*********************************************************************
 * @fn      App_Set_Parameter
 *
 * @brief   App_Set_Parameter
 *
 * @param   
 *
 * @return  
 *********************************************************************/
void App_Set_Parameter(u8* pBuffer, u16 length)
{
    u16 num = TOSMALLENDIAN16(pBuffer[17],pBuffer[18]);
	APP_DEBUG("setting paramter number %d\n",num);
	u16 pLocation = 19;
	s32 ret;
	for(u16 i = 0; i < num; i++)
	{
		u16 par_id = TOSMALLENDIAN16(pBuffer[pLocation],pBuffer[pLocation+1]);
		u8  par_len = pBuffer[pLocation+2];

		if(par_id != NETWORK_TIME && par_id != PASSWORD)
		{
			s32 index = binary_search_parameter(gParmeter.parameter_8, parameter_8_num, par_id, par_len);
			if(index < APP_RET_OK)
			{
				APP_ERROR("binary_search error: App_Set_Parameter, ret = %d\n",index);
			}
			//APP_DEBUG("find:0x%04x,index:%d\n",par_id,index);
			if(par_len == 4)
			{
				gParmeter.parameter_8[index].data = TOSMALLENDIAN32(pBuffer[pLocation+3],pBuffer[pLocation+4],pBuffer[pLocation+5],pBuffer[pLocation+6]);
				//APP_DEBUG("find:0x%04x,index:%d,data:%d\n",par_id,index,gParmeter.parameter_8[index].data);
			}
			else if(par_len == 3)
			{
				//gParmeter.parameter_8[index].data = TOSMALLENDIAN32(BCDTODEC(pBuffer[pLocation+3]),BCDTODEC(pBuffer[pLocation+4]),BCDTODEC(pBuffer[pLocation+5]),0);
				gParmeter.parameter_8[index].data = TOSMALLENDIAN32(0,pBuffer[pLocation+3],pBuffer[pLocation+4],pBuffer[pLocation+5]);
				//APP_DEBUG("find:0x%04x,index:%d,data:%x\n",par_id,index,gParmeter.parameter_8[index].data);
			}
		}
		else if(par_id == NETWORK_TIME)
		{
			//set local time
			ST_Time datetime;
			//time start from 2000-1-1-00:00:00
		    datetime.year=BCDTODEC(pBuffer[pLocation+3]) + 2000;
			datetime.month=BCDTODEC(pBuffer[pLocation+4]);
			datetime.day=BCDTODEC(pBuffer[pLocation+5]);
			datetime.hour=BCDTODEC(pBuffer[pLocation+6]);
			datetime.minute=BCDTODEC(pBuffer[pLocation+7]);
			datetime.second=BCDTODEC(pBuffer[pLocation+8]);
			datetime.timezone=TIMEZONE;
  			Ql_SetLocalTime(&datetime);
  			Ql_Sleep(20);
  			update_clk_alarm(&datetime);
  			//APP_DEBUG("time: %d-%d-%d %d:%d:%d\n",datetime.year,datetime.month,datetime.day,datetime.hour,datetime.minute,datetime.second);
		}
		else
		{
			//save password
			mSys_config.password[0] = pBuffer[pLocation+3];
			mSys_config.password[1] = pBuffer[pLocation+4];
			mSys_config.password[2] = pBuffer[pLocation+5];
			mSys_config.password[3] = pBuffer[pLocation+6];
			
			u8 *Sys_Config_Buffer = (u8 *)Ql_MEM_Alloc(SYS_CONFIG_BLOCK_LEN);
    		if (Sys_Config_Buffer == NULL)
  			{
  	 			APP_ERROR("%s/%d:Ql_MEM_Alloc FAIL! size:%u\r\n", __func__, __LINE__,SYS_CONFIG_BLOCK_LEN);
     			return;
  			}
			Ql_memcpy(Sys_Config_Buffer+1, &mSys_config, sizeof(mSys_config));
			Sys_Config_Buffer[0] = SYS_CONFIG_STORED_FLAG;
			s32 ret = Ql_SecureData_Store(SYS_CONFIG_BLOCK, Sys_Config_Buffer, SYS_CONFIG_BLOCK_LEN);
			if(ret != QL_RET_OK)
			{
				APP_ERROR("Sys Config store fail!! errcode = %d\n",ret);
				Sys_Config_Buffer[0] = 0;
				Ql_SecureData_Store(SYS_CONFIG_BLOCK, Sys_Config_Buffer, 1);
			}else{
				APP_DEBUG("Sys Config store OK!!\n");
			}
			APP_DEBUG("save password %.*s\n",4,mSys_config.password);
		}
		
		pLocation = pLocation + par_len + 3;
	}

	//update parameter to ble
	Ql_OS_SendMessage(subtask_ble_id, MSG_ID_BLE_PARAMETER_UPDATA, 0, 0);

	//save parameter
	Ql_memcpy(Parameter_Buffer+1, &gParmeter, sizeof(gParmeter));
	Parameter_Buffer[0] = PAR_STORED_FLAG;
	ret = Ql_SecureData_Store(PAR_BLOCK, Parameter_Buffer, PAR_BLOCK_LEN);
	if(ret != QL_RET_OK)
	{
		APP_ERROR("parameter store fail!! errcode = %d\n",ret);
		Parameter_Buffer[0] = 0;
		Ql_SecureData_Store(PAR_BLOCK, Parameter_Buffer, 1);
	}else{
		APP_DEBUG("parameter store OK!!\n");
	}
	
}

/*********************************************************************
 * @fn      App_Report_Location
 *
 * @brief   App_Report_Location
 *
 * @param   
 *
 * @return  
 *********************************************************************/
void App_Report_Location( void )
{
	APP_DEBUG("App_Report_Location\n");
	//head
	Server_Msg_Head m_Server_Msg_Head;
	m_Server_Msg_Head.protocol_version = PROTOCOL_VERSION;
	m_Server_Msg_Head.msg_id= TOSERVER_LOCATION_INFO_ID;
	m_Server_Msg_Head.msg_length = 50;
	Ql_memcpy(m_Server_Msg_Head.device_imei, g_imei, 8);
	m_Server_Msg_Head.msg_number = ++g_msg_number;
    
	//body
	u8 *msg_body;
	msg_body = (u8 *)Ql_MEM_Alloc(m_Server_Msg_Head.msg_length);
	Ql_memset(msg_body, 0, m_Server_Msg_Head.msg_length);
	
	u32 alarm_flag = TOBIGENDIAN32(gAlarm_Flag.alarm_flags);
	Ql_memcpy(msg_body,&alarm_flag,4);
	//status not use;
	//Ql_memcpy(msg_body+4,&status,4);
	
	Ql_memcpy(msg_body+8,&gGpsLocation,20);
	Ql_memcpy(msg_body+34,&gGpsLocation.starInusing,4);

	//time start from 2000-1-1-00:00:00
	ST_Time datetime;
	Ql_GetLocalTime(&datetime);
	msg_body[28] = DECTOBCD(datetime.year -2000);
	msg_body[29] = DECTOBCD(datetime.month);
	msg_body[30] = DECTOBCD(datetime.day);
	msg_body[31] = DECTOBCD(datetime.hour);
	msg_body[32] = DECTOBCD(datetime.minute);
	msg_body[33] = DECTOBCD(datetime.second);

	//LAC&CI
	Ql_memcpy(msg_body+38,&glac_ci,8);
	//RSSI
	u32 rssi,ber;
    RIL_NW_GetSignalQuality(&rssi, &ber);
    rssi = TOBIGENDIAN32(rssi);
	Ql_memcpy(msg_body+46,&rssi,4);

	//pack msg
	Server_Msg_Send(&m_Server_Msg_Head,16,msg_body,m_Server_Msg_Head.msg_length);
	Ql_MEM_Free(msg_body);
	msg_body = NULL;
}

/*********************************************************************
 * @fn      App_Set_Location_policy
 *
 * @brief   App_Set_Location_policy
 *
 * @param   
 *
 * @return  
 *********************************************************************/
void App_Set_Location_policy( u8* pBuffer, u16 length )
{
	//APP_DEBUG("setting location policy\n");

	Ql_memcpy(&mSys_config.gLocation_Policy, pBuffer+17, 20);

	mSys_config.gLocation_Policy.location_policy = TOBIGENDIAN32(mSys_config.gLocation_Policy.location_policy);	
	//APP_DEBUG("setting location policy %d\n",mSys_config.gLocation_Policy.location_policy);

	mSys_config.gLocation_Policy.static_policy = TOBIGENDIAN32(mSys_config.gLocation_Policy.static_policy);	
	//APP_DEBUG("setting static policy %d\n",mSys_config.gLocation_Policy.static_policy);

	mSys_config.gLocation_Policy.time_Interval = TOBIGENDIAN32(mSys_config.gLocation_Policy.time_Interval);	
	//APP_DEBUG("setting time policy %d\n",mSys_config.gLocation_Policy.time_Interval);

	mSys_config.gLocation_Policy.distance_Interval = TOBIGENDIAN32(mSys_config.gLocation_Policy.distance_Interval);	
	//APP_DEBUG("setting distance policy %d\n",mSys_config.gLocation_Policy.distance_Interval);

	mSys_config.gLocation_Policy.bearing_Interval = TOBIGENDIAN32(mSys_config.gLocation_Policy.bearing_Interval);	
	//APP_DEBUG("setting bearing policy %d\n",mSys_config.gLocation_Policy.bearing_Interval);

    //start a timer,repeat=FALSE;
	if(mSys_config.gLocation_Policy.location_policy == TIMER_REPORT_LOCATION || 
	   mSys_config.gLocation_Policy.location_policy == DIS_TIM_REPORT_LOCATION)
	{
		Ql_OS_SendMessage(subtask_gps_id, MSG_ID_GPS_TIMER_REPORT, 0, 0);
	}

	//save
	u8 *Sys_Config_Buffer = (u8 *)Ql_MEM_Alloc(SYS_CONFIG_BLOCK_LEN);
    if (Sys_Config_Buffer == NULL)
  	{
  	 	APP_ERROR("%s/%d:Ql_MEM_Alloc FAIL! size:%u\r\n", __func__, __LINE__,SYS_CONFIG_BLOCK_LEN);
     	return;
  	}
	Ql_memcpy(Sys_Config_Buffer+1, &mSys_config, sizeof(mSys_config));
	Sys_Config_Buffer[0] = SYS_CONFIG_STORED_FLAG;
	s32 ret = Ql_SecureData_Store(SYS_CONFIG_BLOCK, Sys_Config_Buffer, SYS_CONFIG_BLOCK_LEN);
	if(ret != QL_RET_OK)
	{
		APP_ERROR("Sys Config store fail!! errcode = %d\n",ret);
		Sys_Config_Buffer[0] = 0;
		Ql_SecureData_Store(SYS_CONFIG_BLOCK, Sys_Config_Buffer, 1);
	}else{
		APP_DEBUG("Sys Config store OK!!\n");
	}
}

/*********************************************************************
 * @fn      binary_search_parameter
 *
 * @brief   binary_search_parameter
 *
 * @param   
 *
 * @return  
 *********************************************************************/
s32 binary_search_parameter(Parameter_8 *pParameter_8, u32 len, u16 goal, u8 par_len)
{
	s32 low = 0;
	s32 high = len - 1;
	while(low <= high)
	{
		s32 middle = (low + high)/2;
		if(pParameter_8[middle].id == goal && pParameter_8[middle].length == par_len)
			return middle;
		else if(pParameter_8[middle].id > goal)
			high = middle - 1;
		else
			low = middle + 1;
	}
	return APP_RET_ERR_NOT_FOUND;
}
/*********************************************************************
 * @fn      get_lac_cellid
 *
 * @brief   get_lac_cellid
 *
 * @param   
 *
 * @return  
 *********************************************************************/
void get_lac_cellid(char *s)
{
	char *p1,*p2;
	s32 j;
	
	//LAC
	glac_ci.lac = 0;
	p1 = Ql_strchr(s, '\"');
	p2 = Ql_strchr(p1+1, '\"');
	j = p2-p1;

	for(u32 i = 1; i < j; i++)
	{
		if(Ql_isdigit(*(p1+i)))
		{
			glac_ci.lac += (u32)((*(p1+i) - '0')<<(16-i*4));
		}
		else
		{   char c;
			c = Ql_toupper(*(p1+i));
			glac_ci.lac += (u32)((c - 'A')<<(16-i*4)) + 10;
		}	
	}

	//CELL ID
	glac_ci.cell_id = 0;
    p1 = Ql_strchr(p2+1, '\"');
	p2 = Ql_strchr(p1+1, '\"');
	j = p2-p1;
	for(u32 i = 1; i < j; i++)
	{
		if(Ql_isdigit(*(p1+i)))
		{
			glac_ci.cell_id += (u32)((*(p1+i) - '0')<<(16-i*4));
		}
		else
		{   char c = *(p1+i);
		    if( c >= 'a' && c <= 'f')
				c = Ql_toupper(*(p1+i));
			glac_ci.cell_id += (u32)((c - 'A')<<(16-i*4)) + 10;
		}	
	}

	glac_ci.lac = TOBIGENDIAN32(glac_ci.lac);
	glac_ci.cell_id = TOBIGENDIAN32(glac_ci.cell_id);
}

/*********************************************************************
 * @fn      update_alarm
 *
 * @brief   update_alarm
 *
 * @param   
 *
 * @return  
 *********************************************************************/
void update_alarm(u32 alarm_bit, u32 alarm)
{
	if(alarm == 1)
	{
		if(gParmeter.parameter_8[ALARM_FLAG_INDEX].data & BV(alarm_bit))
		{
			gAlarm_Flag.alarm_flags = SET_BIT(gAlarm_Flag.alarm_flags, alarm_bit);
			if(gAlarm_Flag.alarm_flags_bk & BV(alarm_bit))
			{
				return;
			}	
			App_Ropert_Alarm();
			gAlarm_Flag.alarm_flags_bk = SET_BIT(gAlarm_Flag.alarm_flags_bk, alarm_bit);
		}
	}
	else
	{
		gAlarm_Flag.alarm_flags = CLE_BIT(gAlarm_Flag.alarm_flags, alarm_bit);
	}
}

/*********************************************************************
 * @fn      App_Ropert_Alarm
 *
 * @brief   App_Ropert_Alarm
 *
 * @param   
 *
 * @return  
 *********************************************************************/
void App_Ropert_Alarm(void)
{
	s32 ret;
		
	APP_DEBUG("App_Ropert_Alarm\n");
	//head
	Server_Msg_Head m_Server_Msg_Head;
	m_Server_Msg_Head.protocol_version = PROTOCOL_VERSION;
	m_Server_Msg_Head.msg_id= TOSERVER_ALARM_ID;
	m_Server_Msg_Head.msg_length = 50;
	Ql_memcpy(m_Server_Msg_Head.device_imei, g_imei, 8);
	m_Server_Msg_Head.msg_number = ++g_msg_number;
    
	//body
	u8 *msg_body;
	msg_body = (u8 *)Ql_MEM_Alloc(m_Server_Msg_Head.msg_length);
	Ql_memset(msg_body, 0, m_Server_Msg_Head.msg_length);
	
	u32 alarm_flag = TOBIGENDIAN32(gAlarm_Flag.alarm_flags);
	Ql_memcpy(msg_body,&alarm_flag,4);
	//status not use;
	//Ql_memcpy(msg_body+4,&status,4);
	
	Ql_memcpy(msg_body+8,&gGpsLocation,20);
	Ql_memcpy(msg_body+34,&gGpsLocation.starInusing,4);

	//time start from 2000-1-1-00:00:00
	ST_Time datetime;
	Ql_GetLocalTime(&datetime);
	msg_body[28] = DECTOBCD(datetime.year -2000);
	msg_body[29] = DECTOBCD(datetime.month);
	msg_body[30] = DECTOBCD(datetime.day);
	msg_body[31] = DECTOBCD(datetime.hour);
	msg_body[32] = DECTOBCD(datetime.minute);
	msg_body[33] = DECTOBCD(datetime.second);

	//LAC&CI
	Ql_memcpy(msg_body+38,&glac_ci,8);
	//RSSI
	u32 rssi,ber;
    RIL_NW_GetSignalQuality(&rssi, &ber);
    rssi = TOBIGENDIAN32(rssi);
	Ql_memcpy(msg_body+46,&rssi,4);

	//pack msg
	Server_Msg_Send(&m_Server_Msg_Head,16,msg_body,m_Server_Msg_Head.msg_length);
	Ql_MEM_Free(msg_body);
	msg_body = NULL;

	if(!alarm_timer_started)
	{
		//start a timer
		ret = Ql_Timer_Start(ALARM_TIMER_ID,gParmeter.parameter_8[ALARM_INTERVAL_INDEX].data*1000,FALSE);
		if(ret < 0)
		{
			APP_ERROR("\r\nfailed!!, Timer alarm start fail ret=%d\r\n",ret);
		}else{
			alarm_timer_started = TRUE;
		}
	}
}

void App_Set_Sleep_mode(u8* pBuffer,u16 length)
{
	u32 flag = TOSMALLENDIAN32(pBuffer[17],pBuffer[18],pBuffer[19],pBuffer[20]);
	APP_DEBUG("set sleep mode,flag = 0x%x\n",flag);
	for(u8 i = 0; i < 6; i++)
	{
		if(flag & BV(i))
		{
			switch(i)
			{
				case 0:
					//
					break;

				case 1:
					//
					break;

				case 2:
					//
					break;

				case 3:
					//
					break;

				case 4:
					//
					break;

				case 5:
					//
					break;

				default:
					break;
			}
		}
	}
}

#endif // __CUSTOMER_CODE__
