/*****************************************************************************
 *
 * Filename:
 * ---------
 *   app_socket.c
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
#ifdef __CUSTOMER_CODE__ 
#include "ql_stdlib.h"
#include "ql_error.h"
#include "ql_trace.h"
#include "ql_system.h"
#include "ql_timer.h"
#include "ql_socket.h"
#include "ql_gprs.h"
#include "ril.h"
#include "ril_util.h"
#include "ril_network.h"
#include "ril_alarm.h"
#include "ril_system.h"
#include "app_socket.h"
#include "app_server.h"
#include "app_common.h"
#include "app_gps.h"

#define SOC_RECV_BUFFER_LEN   1460
#define SRVADDR_BUFFER_LEN    100
#define SEND_BUFFER_LEN       2048
#define RECV_BUFFER_LEN       2048

/************************************************************************/
/* Definition for GPRS PDP context                                      */
/************************************************************************/
ST_GprsConfig m_GprsConfig = {
    "CMNET",    // APN name
    "",         // User name for APN
    "",         // Password for APN
    0,
    NULL,
    NULL,
};

/************************************************************************/
/* Definition for Server IP Address and Socket Port Number              */
/************************************************************************/
//pan
//static u8  m_SrvADDR[20] = "54.223.223.222\0";
//static u32 m_SrvPort = 5605;
//IBM
//u8  SrvADDR[MAX_SRV_LEN] = "54.223.54.184\0";
u8  SrvADDR[MAX_SRV_LEN];
u16 SrvPort;
u8  ipAddress[4] = {0, 0, 0, 0};
//static u8  m_SrvADDR[20] = "116.24.214.28\0";
//static u32 m_SrvPort = 6800;

static u8 m_recv_buf[RECV_BUFFER_LEN];
static u64 m_nSentLen  = 0;      // Bytes of number sent data through current socket    
static s32 m_remain_len = 0;     // record the remaining number of bytes in send buffer.
static char *m_pCurrentPos = NULL; 
//static u8 m_tcp_state = STATE_NW_GET_SIMSTATE;

s32 g_PdpCntxtId = -1;
s32 g_SocketId = -1;  // Store socket Id that returned by Ql_SOC_Create()
volatile Enum_TCPSTATE mTcpState = STATE_GPRS_UNKNOWN;
u8 Parameter_Buffer[PAR_BLOCK_LEN] = {0};
volatile u8 alarm_on_off = 1;
volatile bool gsm_wake_sleep = TRUE;
volatile bool keep_wake = FALSE;

static ST_PDPContxt_Callback callback_gprs_func = 
{
    Callback_GPRS_Actived,
    CallBack_GPRS_Deactived
};
static ST_SOC_Callback callback_soc_func=
{
    Callback_Socket_Connect,
    Callback_Socket_Close,
    Callback_Socket_Accept,
    Callback_Socket_Read,    
    Callback_Socket_Write
};

void Timer_Handler_Network_State(u32 timerId, void* param);
void check_network_state(u32 state);

/**************************************************************
* the gprs sub task
***************************************************************/
void proc_subtask_gprs(s32 TaskId)
{
	s32 ret;
    ST_MSG subtask_msg;
    
    APP_DEBUG("gprs_subtask_entry,subtaskId = %d\n",TaskId);

    //register a timer for heartbeat
    ret = Ql_Timer_Register(HB_TIMER_ID, Timer_Handler_HB, NULL);
    if(ret <0)
    {
        APP_ERROR("\r\nfailed!!, Timer heartbeat register: timer(%d) fail ,ret = %d\r\n",HB_TIMER_ID,ret);
    }

    //register a timer for network state checkout
    ret = Ql_Timer_Register(NETWOEK_STATE_TIMER_ID, Timer_Handler_Network_State, NULL);
    if(ret <0)
    {
        APP_ERROR("\r\nfailed!!, Timer register: timer(%d) fail ,ret = %d\r\n",NETWOEK_STATE_TIMER_ID,ret);
    }

    //register a timer for alarm
    ret = Ql_Timer_Register(ALARM_TIMER_ID, Timer_Handler_Alarm, NULL);
    if(ret <0)
    {
        APP_ERROR("\r\nfailed!!, Timer alarm register: timer(%d) fail ,ret = %d\r\n",HB_TIMER_ID,ret);
    }
	
    while(TRUE)
    {    
        Ql_OS_GetMessage(&subtask_msg);
        switch(subtask_msg.message)
        {
			case MSG_ID_RIL_READY:
			{
				APP_DEBUG("proc_subtask_gprs revice MSG_ID_RIL_READY\n");

				//read par
    			ret = Ql_SecureData_Read(PAR_BLOCK, Parameter_Buffer, PAR_BLOCK_LEN);
    			if(ret >= (s32)(sizeof(gParmeter)) && Parameter_Buffer[0] == PAR_STORED_FLAG)
    			{
					APP_DEBUG("read parameter ok, ret = %d\n",ret);
					Ql_memcpy(&gParmeter, Parameter_Buffer+1, sizeof(gParmeter));
    			}else{
					APP_ERROR("read parameter error or not store! Using default.\r\n");
    			}
				ST_Time datetime;
				Ql_GetLocalTime(&datetime);
				update_clk_alarm(&datetime);

				u8 *Buffer = (u8 *)Ql_MEM_Alloc(SYS_CONFIG_BLOCK_LEN);
    			if (Buffer == NULL)
  				{
  	 				APP_ERROR("%s/%d:Ql_MEM_Alloc FAIL! size:%u\r\n", __func__, __LINE__,SYS_CONFIG_BLOCK_LEN);
     				return;
  				}
  				ret = Ql_SecureData_Read(SYS_CONFIG_BLOCK, Buffer, SYS_CONFIG_BLOCK_LEN);
    			if(ret >= (s32)(sizeof(mSys_config)) && Buffer[0] == SYS_CONFIG_STORED_FLAG)
    			{
					APP_DEBUG("read sys config ok, ret = %d\n",ret);
					Ql_memcpy(&mSys_config, Buffer+1, sizeof(mSys_config));
    			}else{
					APP_ERROR("read sys config error or not store! Using default.\r\n");
    			}
    			
				ret = Ql_SecureData_Read(SRV_CONFIG_BLOCK, Buffer, SRV_CONFIG_BLOCK_LEN);
    			if(ret >= (s32)(sizeof(mSrv_config)) && Buffer[0] == SRV_CONFIG_STORED_FLAG)
    			{
					APP_DEBUG("read srv config ok, ret = %d\n",ret);
					Ql_memcpy(&mSrv_config, Buffer+1, sizeof(mSrv_config));
    			}else{
					APP_ERROR("read srv config error or not store! Using default.\r\n");
    			}

				if(gParmeter.parameter_8[SRV_MASTER_SLAVER_INDEX].data == 1)
				{
					//slave srv
					Ql_strcpy(SrvADDR, mSrv_config.slaveSrvAddress);
					SrvPort = mSrv_config.slaveSrvPort;
				} else {
					//master srv
					Ql_strcpy(SrvADDR, mSrv_config.masterSrvAddress);
					SrvPort = mSrv_config.masterSrvPort;
				}
			}	
			
            case MSG_ID_GPRS_STATE:
            {
                APP_DEBUG("recv MSG: gprs state = %d, %d\r\n",subtask_msg.param1,mTcpState);
                if (subtask_msg.param1 == NW_STAT_REGISTERED ||
                    subtask_msg.param1 == NW_STAT_REGISTERED_ROAMING)
				{
					s32 ret = GPRS_TCP_Program();
					if(ret != SOC_SUCCESS)
					{
						//need to reboot
					}
				}
                break;
            }
            case MSG_ID_GPS_REP_LOCATION:
            {
				#if 0
				APP_DEBUG("latitude = %d,longitude = %d,altitude  = %d,speed  = %d,bearing = %d\n",
    			          mGpsReader[0].fix.latitude,mGpsReader[0].fix.longitude,mGpsReader[0].fix.altitude,mGpsReader[0].fix.speed,mGpsReader[0].fix.bearing);
				#endif
				gGpsLocation.latitude  = TOBIGENDIAN32(mGpsReader[0].fix.latitude);
				gGpsLocation.longitude = TOBIGENDIAN32(mGpsReader[0].fix.longitude);
				gGpsLocation.altitude = TOBIGENDIAN32(mGpsReader[0].fix.altitude);
				gGpsLocation.speed = TOBIGENDIAN32(mGpsReader[0].fix.speed);
				gGpsLocation.bearing = TOBIGENDIAN32(mGpsReader[0].fix.bearing);
				gGpsLocation.starInusing = TOBIGENDIAN32(mGpsReader[0].fix.starInusing);
				App_Report_Location();
                break;
            }
            case MSG_ID_ALARM_REP:
            {
				update_alarm(subtask_msg.param1,subtask_msg.param2);
                break;
            }
            
            case MSG_ID_NETWORK_STATE:
            {
				APP_DEBUG("network state:%d\n",subtask_msg.param1);
				check_network_state(subtask_msg.param1);
            }
            default:
                break;
        }
    }    
}

//
// Activate GPRS and program TCP.
//
s32 GPRS_Program(void)
{
	s32 ret;
    s32 pdpCntxtId;
	
    //1. get PDP context id
    pdpCntxtId = Ql_GPRS_GetPDPContextId();
    if (GPRS_PDP_ERROR == pdpCntxtId)
    {
        APP_ERROR("No PDP context is available\r\n");
        return pdpCntxtId;
    }
    
    //2. Register GPRS callback
    ret = Ql_GPRS_Register(pdpCntxtId, &callback_gprs_func, NULL);
    if (GPRS_PDP_SUCCESS != ret)
    {
        APP_ERROR("<-- Fail to register GPRS, cause=%d. -->\r\n", ret);
        return ret;
    }

    //3. Configure PDP
    ST_GprsConfig GprsConfig;
    Ql_memcpy(&GprsConfig,&m_GprsConfig,sizeof(ST_GprsConfig));
    Ql_strcpy(GprsConfig.apnName,mSys_config.apnName);
    Ql_strcpy(GprsConfig.apnPasswd,mSys_config.apnPasswd);
    Ql_strcpy(GprsConfig.apnUserId,mSys_config.apnUserId);
    ret = Ql_GPRS_Config(pdpCntxtId, &GprsConfig);
    if (GPRS_PDP_SUCCESS == ret)
    {
        APP_DEBUG("<-- Configure PDP context -->\r\n");
    }else{
        APP_DEBUG("<-- Fail to configure GPRS PDP, cause=%d. -->\r\n", ret);
        return;
    }
    
    u8 i = 3;
	do
	{
    	//4. Activate GPRS PDP context
    	ret = Ql_GPRS_ActivateEx(pdpCntxtId, TRUE);
    	if (ret == GPRS_PDP_SUCCESS)
    	{
        	mTcpState = STATE_GPRS_ACTIVATED;
        	APP_DEBUG("<-- Activate GPRS successfully. -->\r\n");
        	break;
    	}
		else if (ret == GPRS_PDP_ALREADY)
		{
			// GPRS has been activating...
			mTcpState = STATE_GPRS_ACTIVATED;
			APP_DEBUG("<-- GPRS has been activating. -->\r\n");
			break;
		}else{
			// Fail to activate GPRS, error code is in "ret".
			APP_ERROR("<-- Fail to activate GPRS cause=%d. -->\r\n",ret);
			Ql_Sleep(100);
		}
	}while(--i);	

	if(i)
		return pdpCntxtId;
	else
	{
		return ret;
	}
}

s32 TCP_Program(s32 pdpCntxtId)
{	
	s32 ret;
	s32 cgreg = 0;

	if(mTcpState != STATE_GPRS_ACTIVATED)
    	return SOC_ERROR;

    ret = RIL_NW_GetGPRSState(&cgreg);
    //APP_DEBUG("<--Network State:cgreg=%d-->\r\n",cgreg);
    if((cgreg != NW_STAT_REGISTERED) && (cgreg != NW_STAT_REGISTERED_ROAMING))
    {
     	mTcpState = STATE_GPRS_UNKNOWN;
     	Ql_GPRS_DeactivateEx(g_PdpCntxtId, TRUE);
    }	
    	
	//1. Register Socket callback
    ret = Ql_SOC_Register(callback_soc_func, NULL);
    if (SOC_SUCCESS == ret)
    {
		mTcpState = STATE_SOC_REGISTER;
        APP_DEBUG("<-- Register socket callback function -->\r\n");
    }else{
        APP_ERROR("<-- Fail to register socket callback, cause=%d. -->\r\n", ret);
        return ret;
    }

    //2. Create socket
    g_SocketId = Ql_SOC_Create(pdpCntxtId, SOC_TYPE_TCP);
    if (g_SocketId >= 0)
    {
		mTcpState = STATE_SOC_CREATED;
        APP_DEBUG("<-- Create socket successfully, socket id=%d. -->\r\n", g_SocketId);
    }else{
        APP_ERROR("<-- Fail to create socket, cause=%d. -->\r\n", g_SocketId);
        return g_SocketId;
    }

    //3. Connect to server
    {
        //3.1 Convert IP format
        APP_DEBUG("<-- Connect to server: %s port: %d -->\r\n",SrvADDR, SrvPort);
        u32 iplength[1];
        ret = Ql_IpHelper_ConvertIpAddr(SrvADDR, (u32 *)ipAddress);
        if (ret != SOC_SUCCESS)
        {
        	ret = Ql_IpHelper_GetIPByHostNameEx(pdpCntxtId, 0, SrvADDR, iplength,(u32 *)ipAddress);
            if(ret != SOC_SUCCESS)
            {
            	APP_ERROR("<-- Get ip by hostname failure:ret=%d-->\r\n",ret);
            	return ret;
            }     
        }
		
        //3.2 Connect to server
        APP_DEBUG("<-- Connecting to server(IP:%d.%d.%d.%d, port:%d)-->\r\n", 
        		  ipAddress[0],ipAddress[1],ipAddress[2],ipAddress[3], SrvPort);
		for(u8 i = 3; i > 0; i--)
		{
        	ret = Ql_SOC_ConnectEx(g_SocketId,(u32) ipAddress, SrvPort, TRUE);
        	if (SOC_SUCCESS == ret)
        	{
            	APP_DEBUG("<-- Connect to server successfully -->\r\n");
            	mTcpState = STATE_SOC_CONNECTED;
            	break;
        	}else{
            	//APP_ERROR("<-- Fail to connect to server, cause=%d -->\r\n", ret);
            	Ql_Sleep(200);
        	}
        }

        if(mTcpState != STATE_SOC_CONNECTED)
        {
			APP_DEBUG("<-- Connect to server failure,close socket -->\r\n");
			Ql_SOC_Close(g_SocketId);
			mTcpState = STATE_GPRS_ACTIVATED;
			return ret;
        }
    }

	//register to server
    ret = App_Server_Register();
    if(ret == APP_RET_OK)
    	return ret;
    else
    {
		Ql_SOC_Close(g_SocketId);
        return ret;
    }	
}

s32 GPRS_TCP_Program(void)
{
    s32 ret;
    APP_DEBUG("%s:mTcpState = %d\n",__func__,mTcpState);

    if(mTcpState != STATE_GPRS_UNKNOWN)
    	return SOC_SUCCESS;

    g_PdpCntxtId = GPRS_Program();
    if(g_PdpCntxtId < 0)
    {
		APP_ERROR("<-- Fail to get pdpCntxtId. -->\r\n");
		return g_PdpCntxtId;
    }
    APP_DEBUG("g_PdpCntxtId = %d\n",g_PdpCntxtId);

    ret = TCP_Program(g_PdpCntxtId);
	if(ret != SOC_SUCCESS)
	{
		//Ql_GPRS_DeactivateEx(g_PdpCntxtId, TRUE);
		mTcpState = STATE_GPRS_ACTIVATED;
		APP_DEBUG("tcp program error,mTcpState = %d\n",mTcpState);
		Ql_Timer_Start(NETWOEK_STATE_TIMER_ID,NETWOEK_STATE_TIMER_PERIOD,FALSE);
	}
	
	return ret;   
}

//
//becase using *Ex, so this callback will not to being invoked.
//
void Callback_GPRS_Actived(u8 contexId, s32 errCode, void* customParam)
{
	APP_ERROR("<--CallBack: active GPRS,errCode=%d-->\r\n",errCode);
}

//
// This callback function is invoked when GPRS drops down. The cause is in "errCode".
//
void CallBack_GPRS_Deactived(u8 contextId, s32 errCode, void* customParam )
{
    APP_ERROR("<--CallBack: deactived GPRS (may drops down),errCode=%d-->\r\n",errCode); 
}

//
//becase using *Ex, so this callback will not to being invoked.
//
void Callback_Socket_Connect(s32 socketId, s32 errCode, void* customParam )
{
    if (errCode != SOC_SUCCESS)
    {
        APP_ERROR("<--Callback: socket connect failure,(socketId=%d),errCode=%d-->\r\n",socketId,errCode);
    }
}

//
//This callback function will be invoked when the socket connection is closed by the remote side only.
//
void Callback_Socket_Close(s32 socketId, s32 errCode, void* customParam )
{
	s32 ret;
	APP_ERROR("<--Callback: socket close by remote side,errCode=%d-->\r\n",errCode);
	Ql_SOC_Close(socketId);
    gServer_State = SERVER_STATE_UNKNOW;
    mTcpState = STATE_GPRS_ACTIVATED;
    //Ql_Sleep(100);
    //Ql_GPRS_DeactivateEx(g_PdpCntxtId, TRUE);
	//mTcpState = STATE_GPRS_UNKNOWN;

    //send msg
    //Ql_OS_SendMessage(subtask_gprs_id, MSG_ID_NETWORK_STATE, mTcpState, 0);


    //ret = TCP_Program(g_PdpCntxtId);
	//if(ret != SOC_SUCCESS)
	//{
		//APP_ERROR("<--Callback: TCP_Program again error,errCode=%d-->\r\n",ret);
	//}
	Ql_Timer_Stop(HB_TIMER_ID);
	Ql_Timer_Start(NETWOEK_STATE_TIMER_ID,NETWOEK_STATE_TIMER_PERIOD,FALSE);
}

//
//using for TCP server only.
//
void Callback_Socket_Accept(s32 listenSocketId, s32 errCode, void* customParam )
{  
}

void Callback_Socket_Read(s32 socketId, s32 errCode, void* customParam )
{
    s32 ret;
    //APP_DEBUG("call back socket read...\n");
    if(errCode)
    {
        APP_DEBUG("<--CallBack: socket read failure,(sock=%d,error=%d)-->\r\n",socketId,errCode);
        //APP_DEBUG("<-- Close socket.-->\r\n");
        //Ql_SOC_Close(socketId);
        //g_SocketId = -1;
        //if(errCode == SOC_BEARER_FAIL)  
        //{
            //m_tcp_state = STATE_GPRS_DEACTIVATE;
        //}
        //else
        //{
            //m_tcp_state = STATE_GPRS_GET_DNSADDRESS;
        //}  
        return;
    }

    Ql_memset(m_recv_buf, 0, RECV_BUFFER_LEN);
    do
    {
        ret = Ql_SOC_Recv(socketId, m_recv_buf, RECV_BUFFER_LEN);

        if((ret < 0) && (ret != -2))
        {
            APP_DEBUG("<-- Receive data failure,ret=%d.-->\r\n",ret);
            //APP_DEBUG("<-- Close socket.-->\r\n");
            //Ql_SOC_Close(socketId); //you can close this socket  
            //g_SocketId = -1;
            break;
        }
        else if(ret == -2)
        {
            //wait next CallBack_socket_read
            break;
        }
        else if(ret < RECV_BUFFER_LEN)
        {     	
            Server_Msg_Handle(m_recv_buf,ret);	
            break;
        }else if(ret == RECV_BUFFER_LEN)
        {
            APP_DEBUG("<--Receive data from sock(%d),len(%d):%s\r\n",socketId,ret,m_recv_buf);
        }
    }while(1);
}

void Callback_Socket_Write(s32 socketId, s32 errCode, void* customParam )
{
#if 0
    s32 ret;

    if(errCode)
    {
        APP_DEBUG("<--CallBack: socket write failure,(sock=%d,error=%d)-->\r\n",socketId,errCode);
        APP_DEBUG("<-- Close socket.-->\r\n");
        Ql_SOC_Close(socketId);
        g_SocketId = -1;
        
        if(ret == SOC_BEARER_FAIL)  
        {
            //m_tcp_state = STATE_GPRS_DEACTIVATE;
        }
        else
        {
            //m_tcp_state = STATE_GPRS_GET_DNSADDRESS;
        }  
        return;
    }


    //m_tcp_state = STATE_SOC_SENDING;

    do
    {
        ret = Ql_SOC_Send(g_SocketId, m_pCurrentPos, m_remain_len);
        APP_DEBUG("<--CallBack: Send data,socketid=%d,number of bytes sent=%d-->\r\n",g_SocketId,ret);

        if(ret == m_remain_len)//send compelete
        {
            m_remain_len = 0;
            m_pCurrentPos = NULL;
            m_nSentLen += ret;
            //m_tcp_state = STATE_SOC_ACK;
            break;
         }
         else if((ret < 0) && (ret == SOC_WOULDBLOCK)) 
         {
            //you must wait CallBack_socket_write, then send data;     
            break;
         }
         else if(ret < 0)
         {
              APP_DEBUG("<--Send data failure,ret=%d.-->\r\n",ret);
              APP_DEBUG("<-- Close socket.-->\r\n");
              Ql_SOC_Close(socketId);//error , Ql_SOC_Close
              g_SocketId = -1;

              m_remain_len = 0;
              m_pCurrentPos = NULL; 
              if(ret == SOC_BEARER_FAIL)  
              {
                  //m_tcp_state = STATE_GPRS_DEACTIVATE;
              }
              else
              {
                  //m_tcp_state = STATE_GPRS_GET_DNSADDRESS;
              }       
              break;
        }
        else if(ret < m_remain_len)//continue send, do not send all data
        {
            m_remain_len -= ret;
            m_pCurrentPos += ret; 
            m_nSentLen += ret;
        }
     }while(1);
#endif     
}

void Timer_Handler_Network_State(u32 timerId, void* param)
{
    if(NETWOEK_STATE_TIMER_ID == timerId)
    {
		s32 ret;
		volatile static u8 network_state_count = 10;
		if(network_state_count > 0)
		{
			APP_DEBUG("%s count = %d\n",__func__,network_state_count);
			network_state_count--;
			ret = TCP_Program(g_PdpCntxtId);
			if(ret != SOC_SUCCESS)
			{
				//APP_ERROR("<--Callback: TCP_Program again error,errCode=%d-->\r\n",ret);
				Ql_Timer_Start(NETWOEK_STATE_TIMER_ID,NETWOEK_STATE_TIMER_PERIOD,FALSE);
				return;
			}
	    } else {
			//reboot
			network_state_count = 10;
			APP_DEBUG("%s:reboot\n",__func__);
			Ql_Sleep(50);
			Ql_Reset(0);
	    }
	    network_state_count = 10;
	    APP_DEBUG("%s 2 count = %d\n",__func__,network_state_count);
	}
}

void check_network_state(u32 state)
{
	static s32 count = 3;
	s32 ret;
	switch(state)
	{
		case STATE_GPRS_UNKNOWN:
		{
			APP_DEBUG("socket close by remote side %s\n",__func__);
			mTcpState = STATE_GPRS_UNKNOWN;
			APP_DEBUG("1 start newwork state timer\n");
			//TCP_Program(g_PdpCntxtId);
			ret = GPRS_TCP_Program();
			if(ret != SOC_SUCCESS)
				mTcpState = STATE_GPRS_UNKNOWN;
			APP_DEBUG("2 start newwork state timer\n");
			count--;
			if(count > 0)
			{
				APP_DEBUG("start newwork state timer\n");
				Ql_Timer_Start(NETWOEK_STATE_TIMER_ID,NETWOEK_STATE_TIMER_PERIOD,FALSE);
			}
			else
			{
				//soc can't work
				APP_DEBUG("false start newwork state timer\n");
			}
			break;
		}
		default:
		{
			count = 3;
			break;
		}	
	}
}

void update_clk_alarm(ST_Time* dateTime)
{
	u8 pBuffer[3];
	s32 iRet;
	u64 current_seconds,qst_seconds;
	//delete alarm
	iRet = RIL_Alarm_Del();
	if(iRet != RIL_AT_SUCCESS)
	{
		APP_ERROR("del alarm error ret:%d\n",iRet);
	}

	Ql_memcpy(pBuffer, &gParmeter.parameter_8[QST_WORKUP_TIME_INDEX].data, 3);
	current_seconds = Ql_Mktime(dateTime);
	dateTime->hour=BCDTODEC(pBuffer[2]);
	dateTime->minute=BCDTODEC(pBuffer[1]);
	dateTime->second=BCDTODEC(pBuffer[0]);
	qst_seconds = Ql_Mktime(dateTime);
	
	if(gParmeter.parameter_8[HWJ_SLEEP_WORKUP_POLICY_INDEX].data == 0 || keep_wake)
	{
		if(qst_seconds < current_seconds + 15)
			dateTime->day++;
		alarm_on_off = 0;
		APP_DEBUG("alarm_on_off =%d,hwj_power_policy = 0\n",alarm_on_off);
	} else {
		if(alarm_on_off == 1)
		{
			//set power off alarm
			u64 off_time_seconds = current_seconds + gParmeter.parameter_8[HWJ_WORKUP_TIME_INDEX].data*60;
			if(off_time_seconds >= qst_seconds && qst_seconds > current_seconds)
			{
				if(qst_seconds < current_seconds + 15)
					dateTime->second += 15;
				alarm_on_off = 0;
				gsm_wake_sleep = TRUE;
				APP_DEBUG("power off:alarm_on_off =%d,hwj_power_policy = 1\n",alarm_on_off);
			} else {
				Ql_MKTime2CalendarTime(off_time_seconds, dateTime);
				APP_DEBUG("power off:alarm_on_off =%d,hwj_power_policy = 1\n",alarm_on_off);
			}
		} else if(alarm_on_off == 2) {
			//set power up alarm, receive here,system will power off after 5s!!
			u64 up_time_seconds = current_seconds + gParmeter.parameter_8[HWJ_SLEEP_TIME_INDEX].data*60;
			if(up_time_seconds >= qst_seconds && qst_seconds > current_seconds)
			{
				if(qst_seconds < current_seconds + 15)
					dateTime->second += 15;
				alarm_on_off = 0;
				gsm_wake_sleep = FALSE;
				APP_DEBUG("power up:alarm_on_off =%d,hwj_power_policy = 1\n",alarm_on_off);
			} else {
				Ql_MKTime2CalendarTime(up_time_seconds, dateTime);
				APP_DEBUG("power up:alarm_on_off =%d,hwj_power_policy = 1\n",alarm_on_off);
			}
		}
	}

	APP_DEBUG("alarm:year=%d,month=%d,day=%d,hour=%d,min=%d,sec=%d,power=%d\n",
			   dateTime->year,dateTime->month,dateTime->day,dateTime->hour,dateTime->minute,dateTime->second,alarm_on_off);
	dateTime->year = dateTime->year - 2000;
	if(alarm_on_off == 0){
		if(gsm_wake_sleep)
			iRet = RIL_Alarm_Add(dateTime, 1, 0);
		else
		{
			iRet = RIL_Alarm_Add(dateTime, 0, 2);
			if(iRet != RIL_AT_SUCCESS)
			{
				APP_ERROR("add alarm error ret:%d\n",iRet);
				return;
			}
			APP_DEBUG("system will power down after 1s\n");
			Ql_Sleep(1000);
			Ql_PowerDown(1);
		}	
	}else if(alarm_on_off == 1){
		iRet = RIL_Alarm_Add(dateTime, 0, 0);
	}else{
		iRet = RIL_Alarm_Add(dateTime, 0, 2);
		if(iRet != RIL_AT_SUCCESS)
		{
			APP_ERROR("add alarm error ret:%d\n",iRet);
			return;
		}
		APP_DEBUG("system will power down after 1s\n");
		Ql_Sleep(1000);
		Ql_PowerDown(1);
	}
	
	if(iRet != RIL_AT_SUCCESS)
	{
		APP_ERROR("add alarm error ret:%d\n",iRet);
	}
}

#endif // __CUSTOMER_CODE__
