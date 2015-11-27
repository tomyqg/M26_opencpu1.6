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
static ST_GprsConfig m_GprsConfig = {
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
static u8  m_SrvADDR[20] = "54.223.54.184\0";
static u32 m_SrvPort = 8300;
//static u8  m_SrvADDR[20] = "113.92.228.197\0";
//static u32 m_SrvPort = 6800;

static s32 m_GprsActState    = 0;   // GPRS PDP activation state, 0= not activated, 1=activated
static s32 m_SocketConnState = 0;   // Socket connection state, 0= disconnected, 1=connected
static u8  m_SocketRcvBuf[SOC_RECV_BUFFER_LEN];
static u8 m_send_buf[SEND_BUFFER_LEN];
static u8 m_recv_buf[RECV_BUFFER_LEN];
static u64 m_nSentLen  = 0;      // Bytes of number sent data through current socket    
static u8  m_ipaddress[5];  //only save the number of server ip, remove the comma
static s32 m_remain_len = 0;     // record the remaining number of bytes in send buffer.
static char *m_pCurrentPos = NULL; 
static u8 m_tcp_state = STATE_NW_GET_SIMSTATE;

s32 g_SocketId = -1;  // Store socket Id that returned by Ql_SOC_Create()

static ST_PDPContxt_Callback     callback_gprs_func = 
{
    Callback_GPRS_Actived,
    CallBack_GPRS_Deactived
};
static ST_SOC_Callback      callback_soc_func=
{
    Callback_Socket_Connect,
    Callback_Socket_Close,
    Callback_Socket_Accept,
    Callback_Socket_Read,    
    Callback_Socket_Write
};

/**************************************************************
* the gprs sub task
***************************************************************/
void proc_subtask_gprs(s32 TaskId)
{
    ST_MSG subtask_msg;
    
    APP_DEBUG("gprs_subtask_entry,subtaskId = %d\n",TaskId);
	
    while(TRUE)
    {    
        Ql_OS_GetMessage(&subtask_msg);
        switch(subtask_msg.message)
        {
			case MSG_ID_RIL_READY:
			{
				APP_DEBUG("proc_subtask_gprs revice MSG_ID_RIL_READY\n");
				ST_Time datetime;
				u8 pBuffer[3];
				Ql_memcpy(pBuffer, &gParmeter.parameter_8[QST_WORKUP_TIME_LOC].data, 3);
				//time start from 2000-1-1-00:00:00
		    	datetime.year=16;
				datetime.month=11;
				datetime.day=25;
				datetime.hour=BCDTODEC(pBuffer[2]);
				datetime.minute=BCDTODEC(pBuffer[1]);
				datetime.second=BCDTODEC(pBuffer[0]);
				APP_DEBUG("hour=%d,min=%d,sec=%d\n",datetime.hour,datetime.minute,datetime.second);
				datetime.timezone=TIMEZONE;
				s32 iRet = RIL_Alarm_Create(&datetime, 1);
				if(iRet != RIL_AT_SUCCESS)
				{
					APP_ERROR("Create alarm error ret:%d\n",iRet);
				}
				break;
			}	
			
            case MSG_ID_GPRS_STATE:
            {
                APP_DEBUG("recv MSG: gprs state = %d\r\n",subtask_msg.param1);
                if (subtask_msg.param1 == NW_STAT_REGISTERED ||
                    subtask_msg.param1 == NW_STAT_REGISTERED_ROAMING)
				{
					GPRS_TCP_Program();
				}
                break;
            }
            case MSG_ID_MUTEX_TEST:
            {
                MutextTest(TaskId);
                break;
            }
            case MSG_ID_SEMAPHORE_TEST:
            {
                //SemaphoreTest1(TaskId);  
                break;
            }
            case MSG_ID_GET_ALL_TASK_PRIORITY:
            {
                //GetCurrentTaskPriority(TaskId);
                break;
            }
            case MSG_ID_GET_ALL_TASK_REMAINSTACK:
            {
                //GetCurrenTaskRemainStackSize(TaskId);
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
            case MSG_ID_GPS_SPEED_UP:
            {
                //APP_DEBUG("speed up event:%d\n",subtask_msg.param1);
                update_alarm(ALARM_BIT_SPEED_UP,subtask_msg.param1);
                break;
            }
            default:
                break;
        }
    }    
}

//
// Activate GPRS and program TCP.
//
void GPRS_TCP_Program(void)
{
    s32 ret;
    s32 pdpCntxtId;

    //ret = Ql_Timer_Register(ALARM_TIMER_ID, Timer_Handler_Alarm, NULL);
    //if(ret <0)
    //{
    //    APP_ERROR("\r\nfailed!!, Timer alarm register: timer(%d) fail ,ret = %d\r\n",HB_TIMER_ID,ret);
    //}
	
    //1. Register GPRS callback
    pdpCntxtId = Ql_GPRS_GetPDPContextId();
    if (GPRS_PDP_ERROR == pdpCntxtId)
    {
        APP_DEBUG("No PDP context is available\r\n");
        return;
    }
    ret = Ql_GPRS_Register(pdpCntxtId, &callback_gprs_func, NULL);
    if (GPRS_PDP_SUCCESS == ret)
    {
        APP_DEBUG("<-- Register GPRS callback function -->\r\n");
    }else{
        APP_DEBUG("<-- Fail to register GPRS, cause=%d. -->\r\n", ret);
        return;
    }

    //2. Configure PDP
    ret = Ql_GPRS_Config(pdpCntxtId, &m_GprsConfig);
    if (GPRS_PDP_SUCCESS == ret)
    {
        APP_DEBUG("<-- Configure PDP context -->\r\n");
    }else{
        APP_DEBUG("<-- Fail to configure GPRS PDP, cause=%d. -->\r\n", ret);
        return;
    }

    //3. Activate GPRS PDP context
    APP_DEBUG("<-- Activating GPRS... -->\r\n");
    ret = Ql_GPRS_ActivateEx(pdpCntxtId, TRUE);
    if (ret == GPRS_PDP_SUCCESS)
    {
        m_GprsActState = 1;
        APP_DEBUG("<-- Activate GPRS successfully. -->\r\n\r\n");
    }else{
        APP_DEBUG("<-- Fail to activate GPRS, cause=%d. -->\r\n\r\n", ret);
        return;
    }

    //4. Register Socket callback
    ret = Ql_SOC_Register(callback_soc_func, NULL);
    if (SOC_SUCCESS == ret)
    {
        APP_DEBUG("<-- Register socket callback function -->\r\n");
    }else{
        APP_DEBUG("<-- Fail to register socket callback, cause=%d. -->\r\n", ret);
        return;
    }

    //5. Create socket
    g_SocketId = Ql_SOC_Create(pdpCntxtId, SOC_TYPE_TCP);
    if (g_SocketId >= 0)
    {
        APP_DEBUG("<-- Create socket successfully, socket id=%d. -->\r\n", g_SocketId);
    }else{
        APP_DEBUG("<-- Fail to create socket, cause=%d. -->\r\n", g_SocketId);
        return;
    }		

    //6. Connect to server
    {
        //6.1 Convert IP format
        u8 m_ipAddress[4]; 
        Ql_memset(m_ipAddress,0,5);
        ret = Ql_IpHelper_ConvertIpAddr(m_SrvADDR, (u32 *)m_ipAddress);
        if (SOC_SUCCESS == ret) // ip address is xxx.xxx.xxx.xxx
        {
            APP_DEBUG("<-- Convert Ip Address successfully,m_ipaddress=%d,%d,%d,%d -->\r\n",m_ipAddress[0],m_ipAddress[1],m_ipAddress[2],m_ipAddress[3]);
        }else{
            APP_DEBUG("<-- Fail to convert IP Address --> \r\n");
            return;
        }

        //6.2 Connect to server
        APP_DEBUG("<-- Connecting to server(IP:%d.%d.%d.%d, port:%d)... -->\r\n", m_ipAddress[0],m_ipAddress[1],m_ipAddress[2],m_ipAddress[3], m_SrvPort);
        ret = Ql_SOC_ConnectEx(g_SocketId,(u32) m_ipAddress, m_SrvPort, TRUE);
        if (SOC_SUCCESS == ret)
        {
            m_SocketConnState = 1;
            APP_DEBUG("<-- Connect to server successfully -->\r\n");
        }else{
            APP_DEBUG("<-- Fail to connect to server, cause=%d -->\r\n", ret);
            APP_DEBUG("<-- Close socket.-->\r\n");
            Ql_SOC_Close(g_SocketId);
            g_SocketId = -1;
            return;
        }
    }
#if 0
    //7. Send data to socket
    {
        //7.1 Send data
        char pchData[200];
        s32  dataLen = 0;
        u64  ackNum = 0;
        Ql_memset(pchData, 0x0, sizeof(pchData));
        dataLen = Ql_sprintf(pchData + dataLen, "%s", "A B C D E F G");
        APP_DEBUG("<-- Sending data(len=%d): %s-->\r\n", dataLen, pchData);
        ret = Ql_SOC_Send(g_SocketId, (u8*)pchData, dataLen);
        if (ret == dataLen)
        {
            APP_DEBUG("<-- Send socket data successfully. --> \r\n");
        }else{
            APP_DEBUG("<-- Fail to send socket data. --> \r\n");
            Ql_SOC_Close(g_SocketId);
            return;
        }

        //7.2 Check ACK number
        do 
        {
            ret = Ql_SOC_GetAckNumber(g_SocketId, &ackNum);
            APP_DEBUG("<-- Current ACK Number:%llu/%d --> \r\n", ackNum, dataLen);
            Ql_Sleep(500);
        } while (ackNum != dataLen);
        APP_DEBUG("<-- Server has received all data --> \r\n");

    }
#endif
    App_Server_Register();
/*
    //8. Close socket
    ret = Ql_SOC_Close(g_SocketId);
    APP_DEBUG("<-- Close socket[%d], cause=%d --> \r\n", g_SocketId, ret);

    //9. Deactivate GPRS
    APP_DEBUG("<-- Deactivating GPRS... -->\r\n");
    ret = Ql_GPRS_DeactivateEx(pdpCntxtId, TRUE);
    APP_DEBUG("<-- Deactivated GPRS, cause=%d -->\r\n\r\n", ret);
*/
}

void Callback_GPRS_Actived(u8 contexId, s32 errCode, void* customParam)
{
    if(errCode == SOC_SUCCESS)
    {
        APP_DEBUG("<--CallBack: active GPRS successfully.-->\r\n");
        m_tcp_state = STATE_GPRS_GET_DNSADDRESS;
    }else
    {
        APP_DEBUG("<--CallBack: active GPRS successfully,errCode=%d-->\r\n",errCode);
        m_tcp_state = STATE_GPRS_ACTIVATE;
    }      
}

//
// This callback function is invoked when GPRS drops down. The cause is in "errCode".
//
void CallBack_GPRS_Deactived(u8 contextId, s32 errCode, void* customParam )
{
    if (errCode == SOC_SUCCESS)
    {
        APP_DEBUG("<--CallBack: deactived GPRS successfully.-->\r\n"); 
        m_tcp_state = STATE_NW_GET_SIMSTATE;
    }else
    {
        APP_DEBUG("<--CallBack: deactived GPRS failure,(contexid=%d,error_cause=%d)-->\r\n",contextId,errCode); 
    }
}

void Callback_Socket_Connect(s32 socketId, s32 errCode, void* customParam )
{
    if (errCode == SOC_SUCCESS)
    {
        //if (timeout_90S_monitor) //stop timeout monitor
        //{
           //Ql_Timer_Stop(TIMEOUT_90S_TIMER_ID);
           //timeout_90S_monitor = FALSE;
        //}

        APP_DEBUG("<--Callback: socket connect successfully.-->\r\n");
        m_tcp_state = STATE_SOC_SEND;
    }else
    {
        APP_DEBUG("<--Callback: socket connect failure,(socketId=%d),errCode=%d-->\r\n",socketId,errCode);
        Ql_SOC_Close(socketId);
        m_tcp_state = STATE_SOC_CREATE;
    }
}

void Callback_Socket_Close(s32 socketId, s32 errCode, void* customParam )
{
    if (errCode == SOC_SUCCESS)
    {
        APP_DEBUG("<--CallBack: close socket successfully.-->\r\n"); 
    }else if(errCode == SOC_BEARER_FAIL)
    {   
        m_tcp_state = STATE_GPRS_DEACTIVATE;
        APP_DEBUG("<--CallBack: close socket failure,(socketId=%d,error_cause=%d)-->\r\n",socketId,errCode); 
    }else
    {
        m_tcp_state = STATE_GPRS_GET_DNSADDRESS;
        APP_DEBUG("<--CallBack: close socket failure,(socketId=%d,error_cause=%d)-->\r\n",socketId,errCode); 
    }
}

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
        APP_DEBUG("<-- Close socket.-->\r\n");
        Ql_SOC_Close(socketId);
        g_SocketId = -1;
        if(errCode == SOC_BEARER_FAIL)  
        {
            m_tcp_state = STATE_GPRS_DEACTIVATE;
        }
        else
        {
            m_tcp_state = STATE_GPRS_GET_DNSADDRESS;
        }  
        return;
    }


    Ql_memset(m_recv_buf, 0, RECV_BUFFER_LEN);
    do
    {
        ret = Ql_SOC_Recv(socketId, m_recv_buf, RECV_BUFFER_LEN);

        if((ret < 0) && (ret != -2))
        {
            APP_DEBUG("<-- Receive data failure,ret=%d.-->\r\n",ret);
            APP_DEBUG("<-- Close socket.-->\r\n");
            Ql_SOC_Close(socketId); //you can close this socket  
            g_SocketId = -1;
            m_tcp_state = STATE_SOC_CREATE;
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
    s32 ret;

    if(errCode)
    {
        APP_DEBUG("<--CallBack: socket write failure,(sock=%d,error=%d)-->\r\n",socketId,errCode);
        APP_DEBUG("<-- Close socket.-->\r\n");
        Ql_SOC_Close(socketId);
        g_SocketId = -1;
        
        if(ret == SOC_BEARER_FAIL)  
        {
            m_tcp_state = STATE_GPRS_DEACTIVATE;
        }
        else
        {
            m_tcp_state = STATE_GPRS_GET_DNSADDRESS;
        }  
        return;
    }


    m_tcp_state = STATE_SOC_SENDING;

    do
    {
        ret = Ql_SOC_Send(g_SocketId, m_pCurrentPos, m_remain_len);
        APP_DEBUG("<--CallBack: Send data,socketid=%d,number of bytes sent=%d-->\r\n",g_SocketId,ret);

        if(ret == m_remain_len)//send compelete
        {
            m_remain_len = 0;
            m_pCurrentPos = NULL;
            m_nSentLen += ret;
            m_tcp_state = STATE_SOC_ACK;
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
                  m_tcp_state = STATE_GPRS_DEACTIVATE;
              }
              else
              {
                  m_tcp_state = STATE_GPRS_GET_DNSADDRESS;
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
}

#endif // __CUSTOMER_CODE__
