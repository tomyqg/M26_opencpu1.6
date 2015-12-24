/*********************************************************************
 *
 * Filename:
 * ---------
 *   app_ble.c 
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
#include "ql_timer.h"
#include "ql_error.h"
#include "ril_network.h"
#include "app_common.h"
#include "app_ble.h"
#include "app_ble_list.h"
#include "app_server.h"
#include "app_socket.h"

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
extern u8 g_imei[8],g_imsi[8];

#define BLE_SERIAL_RX_BUFFER_LEN  512

static bool waiting_rsp = FALSE;
static bool have_start_timer_rsp = FALSE;
static UART_MSG last_uart_send;
static SList gSList_BLE;
u32 battery = 50;
u32 ble_state;

/*********************************************************************
 * FUNCTIONS
 */
static void CallBack_UART_BLE(Enum_SerialPort port, Enum_UARTEventType msg, bool level, void* customizedPara);
static void Timer_handler_Ble_Uart(u32 timerId, void* param);
static bool Uart_Msg_Restart_Timer( void );
static void Uart_BLE_Msg_Handle(u8 *pBuffer, u16 length);

/**************************************************************
* ble sub task
***************************************************************/
void proc_subtask_ble(s32 TaskId)
{
    ST_MSG subtask_msg;
	s32 ret;
    
    APP_DEBUG("ble_subtask_entry,subtaskId = %d\n",TaskId);

    //list 
  	ret = SL_Creat(&gSList_BLE);
  	if (!ret)
    {
        APP_ERROR("Fail to SL_Creat ble\n");
    }

    //register a timer
    ret = Ql_Timer_Register(BLE_UART_RSP_TIMER_ID, Timer_handler_Ble_Uart, NULL);
    if(ret <0)
    {
        APP_ERROR("failed!!, Ql_Timer_Register: timer(%d) fail ,ret = %d\r\n",BLE_UART_RSP_TIMER_ID,ret);
    }

    //register a timer
    ret = Ql_Timer_Register(BLE_UART_HEARBEAT_TIMER_ID, Timer_handler_Ble_Uart, NULL);
    if(ret <0)
    {
        APP_ERROR("failed!!, Ql_Timer_Register: timer(%d) fail ,ret = %d\r\n",BLE_UART_RSP_TIMER_ID,ret);
    }
    
    // Register & open UART port
    ret = Ql_UART_Register(BLE_UART_PORT, CallBack_UART_BLE, NULL);
    if (ret < QL_RET_OK)
    {
        APP_ERROR("Fail to register uart port:%d,ret=%d\n",BLE_UART_PORT,ret);
    }
    ret = Ql_UART_Open(BLE_UART_PORT, BLE_UART_baudrate, FC_NONE);
    if (ret < QL_RET_OK)
    {
        APP_ERROR("Fail to open uart port:%d,ret=%d\n",BLE_UART_PORT,ret);
    }

    while(TRUE)
    {    
        Ql_OS_GetMessage(&subtask_msg);
        switch(subtask_msg.message)
        {
			case MSG_ID_RIL_READY:
			{
				APP_DEBUG("proc_subtask_ble revice MSG_ID_RIL_READY\n");
				break;
			}

			case MSG_ID_IMEI_IMSI_OK:
			{
				APP_DEBUG("proc_subtask_ble revice MSG_ID_IMEI_IMSI_OK\n");
				s32 ret = BLE_Send_IMEI();
				if(ret != APP_RET_OK)
				{
					APP_ERROR("function BLE_Send_IMEI ret error: \n",ret);
				}
				ret = BLE_Send_IMSI();
				if(ret != APP_RET_OK)
				{
					APP_ERROR("function BLE_Send_IMSI ret error: \n",ret);
				}
				ret = BLE_Send_HEARTBEAT();
				if(ret != APP_RET_OK)
				{
					APP_ERROR("function BLE_Send_HEARTBEAT ret error: \n",ret);
				}
				//start timer
    			ret = Ql_Timer_Start(BLE_UART_HEARBEAT_TIMER_ID,BLE_UART_HEARBEAT_INTERVAL,TRUE);
    			if(ret < 0)
    			{
        			APP_ERROR("failed!! timer(%d) Ql_Timer_Start ret=%d\r\n",ret,BLE_UART_HEARBEAT_TIMER_ID);        
    			}
				break;
			}
			case MSG_ID_GPRS_STATE:
            {
                //APP_DEBUG("recv MSG: gprs state = %d, %d\r\n",subtask_msg.param1,mTcpState);
                if (subtask_msg.param1 == NW_STAT_REGISTERED ||
                    subtask_msg.param1 == NW_STAT_REGISTERED_ROAMING)
				{
					BLE_Send_GSM_State(STATE_GSM_Ok_GPRS_NET);
				} else {
					BLE_Send_GSM_State(STATE_GSM_NO_GPRS_NET);
				}
                break;
            }
            case MSG_ID_CLK_ALARM:
            {
                APP_DEBUG(" receive alarm ring\r\n");
                BLE_Send_PowerUp_Paired_GSM();
                break;
			}
            default:
                break;
        }
    }    
}

//timer callback function
void Timer_handler_Ble_Uart(u32 timerId, void* param)
{
    if(BLE_UART_RSP_TIMER_ID == timerId)
    {
        //APP_DEBUG("Timer_handler_Ble_Uart\n");
        static u8 restart_time = 4;
		static u8 resend_time = 2;
    
    	have_start_timer_rsp = FALSE;
    
		// Restart timer
    	if ( Uart_Msg_Restart_Timer() )
    	{
      		if(restart_time > 0)
	  		{
				have_start_timer_rsp = TRUE;
				//start timer
    			s32 ret = Ql_Timer_Start(BLE_UART_RSP_TIMER_ID,BLE_UART_RSP_INTERVAL,FALSE);
    			if(ret < 0)
    			{
        			APP_ERROR("failed!! timer(%d) Ql_Timer_Start ret=%d\r\n",ret,BLE_UART_RSP_TIMER_ID);        
    			}
				restart_time--;
	  		}
	  		else if( resend_time > 0 )
	  		{
	  			restart_time = 4;
				resend_time--;
				//resend
				Uart2BLE_Msg_Send_Action(last_uart_send.msg_buffer,
										 last_uart_send.msg_len,
										 last_uart_send.need_rsp);
	  		}	
	  		else
	  		{
	  			//don't receive respond from E009
	    		restart_time = 4;
	    		resend_time = 2;
				waiting_rsp = FALSE;
	    		if(!SL_Empty(gSList_BLE))
	    		{
	    			UART_MSG uart_send;
	      			SL_GetItem(gSList_BLE,&uart_send);
		  			SL_Delete(gSList_BLE);
		  			Uart2BLE_Msg_Send_Action(uart_send.msg_buffer,
										     uart_send.msg_len,
										     uart_send.need_rsp);

	    		}
	  		}       
      	}
      	else
		{
	  		//have received respond from E009
	  		restart_time = 4;
	  		resend_time = 2;
	  		waiting_rsp = FALSE;
	  		if(!SL_Empty(gSList_BLE))
	  		{
	    		UART_MSG uart_send;
	      		SL_GetItem(gSList_BLE,&uart_send);
		  		SL_Delete(gSList_BLE);
		  		Uart2BLE_Msg_Send_Action(uart_send.msg_buffer,
										 uart_send.msg_len,
										 uart_send.need_rsp);
	  		}
		}
	}
	else if(BLE_UART_HEARBEAT_TIMER_ID == timerId)
	{
		s32 ret = BLE_Send_HEARTBEAT();
		if(ret != APP_RET_OK)
		{
			APP_ERROR("function BLE_Send_HEARTBEAT ret error: \n",ret);
		}
	}
}

bool Uart_Msg_Restart_Timer( void )
{
	if( last_uart_send.msg_buffer[0] == 0x55 )   //this was set when recevice respond
	  return TRUE;
	else
	  return FALSE;
}

/*********************************************************************
 * @fn      ReadSerialPort
 *
 * @brief   ReadSerialPort 
 *
 * @param   
 *
 * @return  
 *********************************************************************/
static s32 ReadSerialPort(Enum_SerialPort port, u8* pBuffer, u32 bufLen)
{
    s32 rdLen = 0;
    s32 rdTotalLen = 0;
    if (NULL == pBuffer || 0 == bufLen)
    {
        return APP_RET_ERR_PARAM;
    }
    Ql_memset(pBuffer, 0x0, bufLen);
    while (1)
    {
        rdLen = Ql_UART_Read(port, pBuffer + rdTotalLen, bufLen - rdTotalLen);
        if (rdLen <= 0)  // All data is read out, or Serial Port Error!
        {
            break;
        }
        rdTotalLen += rdLen;
        // Continue to read...
    }
    if (rdLen < 0) // Serial Port Error!
    {
        APP_ERROR("Fail to read from port[%d]\r\n", port);
        return APP_RET_ERR_UART_READ;
    }
    return rdTotalLen;
}

/*********************************************************************
 * @fn      CallBack_UART_BLE
 *
 * @brief   CallBack_UART_BLE 
 *
 * @param   
 *
 * @return  
 *********************************************************************/
static void CallBack_UART_BLE(Enum_SerialPort port, Enum_UARTEventType msg, bool level, void* customizedPara)
{
    //APP_DEBUG("CallBack_UART_BLE: port=%d, event=%d, level=%d, p=%x\r\n", port, msg, level, customizedPara);
    switch (msg)
    {
    	case EVENT_UART_READY_TO_READ:
        {
            if (BLE_UART_PORT == port)
            {
				u8 RxBuf_BLE[BLE_SERIAL_RX_BUFFER_LEN];
                s32 totalBytes = ReadSerialPort(port, RxBuf_BLE, BLE_SERIAL_RX_BUFFER_LEN);
                if (totalBytes <= 0)
                {
                    APP_DEBUG("No data in UART buffer!\n");
                    return;
                }
                
				#if 1
    			{
    				u8 strTemp[100] = {0};
    				u16 i = 0;
    				for(i = 0; i < totalBytes; i++)
    				{
          				Ql_sprintf(strTemp+i*3, "%02x ",RxBuf_BLE[i]);
    				}
    				APP_DEBUG("UartFromBle: %s\n",strTemp);
    			}
				#endif

				Uart_BLE_Msg_Handle(RxBuf_BLE,totalBytes);
				
            }
            break;
        }
    	case EVENT_UART_READY_TO_WRITE:
    	{
        	break;
        }	
    	default:
        	break;
    }
}
s32 Uart2BLE_Common_Response(u8 rsp_id,u8 rsp_status)
{
	u8 msg_buf[4];
    u8 length = 4;
    s32 ret;
 
    msg_buf[0] = TOBLE_COMMON_RSP_ID;
    msg_buf[1] = 0x02;
    msg_buf[2] = rsp_id;
    msg_buf[3] = rsp_status;
    
    //send msg
    ret = Uart2BLE_Msg_Send(msg_buf,length,FALSE);
    
    return ret;
}

s32 BLE_Send_HEARTBEAT(void)
{
	u8 msg_buf[8];
    u16 length = 8;
    s32 ret;
 
    msg_buf[0] = TOBLE_TIME_ID;
    msg_buf[1] = 6;
    
    //time start from 2000-1-1-00:00:00
	ST_Time datetime;
	Ql_GetLocalTime(&datetime);
	msg_buf[2] = DECTOBCD(datetime.year -2000);
	msg_buf[3] = DECTOBCD(datetime.month);
	msg_buf[4] = DECTOBCD(datetime.day);
	msg_buf[5] = DECTOBCD(datetime.hour);
	msg_buf[6] = DECTOBCD(datetime.minute);
	msg_buf[7] = DECTOBCD(datetime.second);
    
    //Send msg
    ret = Uart2BLE_Msg_Send(msg_buf,length,FALSE);
    return ret;
}

s32 BLE_Send_Reboot(void)
{
	u8 msg_buf[3];
    u16 length = 3;
    s32 ret;
 
    msg_buf[0] = TOBLE_REBOOT_ID;
    msg_buf[1] = 0;
	msg_buf[2] = 0;
    
    //Send msg
    ret = Uart2BLE_Msg_Send(msg_buf,length,TRUE);
    
    return ret;
}

s32 BLE_Send_IMEI(void)
{
	u8 msg_buf[10];
    u16 length = 10;
    s32 ret;
 
    msg_buf[0] = TOBLE_IMEI_ID;
    msg_buf[1] = SERIAl_NUM_LEN;
	
	Ql_memcpy(msg_buf+2,g_imei,SERIAl_NUM_LEN);
    
    //Send msg
    ret = Uart2BLE_Msg_Send(msg_buf,length,TRUE);
    
    return ret;
}

s32 BLE_Send_IMSI(void)
{
	u8 msg_buf[10];
    u16 length = 10;
    s32 ret;
 
    msg_buf[0] = TOBLE_IMSI_ID;
    msg_buf[1] = SERIAl_NUM_LEN;
	
	Ql_memcpy(msg_buf+2,g_imsi,SERIAl_NUM_LEN);
    
    //Send msg
    ret = Uart2BLE_Msg_Send(msg_buf,length,TRUE);
    
    return ret;
}

s32 BLE_Send_Battery(void)
{
	u8 msg_buf[3];
    u16 length = 3;
    s32 ret;
 
    msg_buf[0] = TOBLE_BATTERY_ID;
    msg_buf[1] = 1;
	msg_buf[2] = battery;
    
    //Send msg
    ret = Uart2BLE_Msg_Send(msg_buf,length,TRUE);
    
    return ret;
}

s32 BLE_Send_PowerUp_Paired_GSM(void)
{
	u8 msg_buf[3];
    u16 length = 3;
    s32 ret;
 
    msg_buf[0] = TOBLE_BOOTUP_QST_GSM_ID;
    msg_buf[1] = 0;
	msg_buf[2] = 0;
    
    //Send msg
    ret = Uart2BLE_Msg_Send(msg_buf,length,TRUE);
    
    return ret;
}

s32 BLE_Send_PowerOff_GSM(void)
{
	u8 msg_buf[3];
    u16 length = 3;
    s32 ret;
 
    msg_buf[0] = TOBLE_QST_TO_POWEROFF_ID;
    msg_buf[1] = 0;
	msg_buf[2] = 0;
    
    //Send msg
    ret = Uart2BLE_Msg_Send(msg_buf,length,TRUE);
    
    return ret;
}

s32 BLE_Send_Reboot_Paired(void)
{
	u8 msg_buf[3];
    u16 length = 3;
    s32 ret;
 
    msg_buf[0] = TOBLE_REBOOT_PARTNER_ID;
    msg_buf[1] = 0;
	msg_buf[2] = 0;
    
    //Send msg
    ret = Uart2BLE_Msg_Send(msg_buf,length,TRUE);
    
    return ret;
}

s32 BLE_Send_GSM_State(u8 state)
{
	u8 msg_buf[10];
    u16 length = 10;
    s32 ret;
 
    msg_buf[0] = TOBLE_GSM_STATUS_ID;
    msg_buf[1] = 7;
    //state
	msg_buf[2] = state;
	//time
	msg_buf[3] = 1;
	msg_buf[4] = 2;
	
    //Send msg
    ret = Uart2BLE_Msg_Send(msg_buf,length,TRUE);
    return ret;
}

s32 BLE_Send_Adv_Start(void)
{
	u8 msg_buf[3];
    u16 length = 3;
    s32 ret;
 
    msg_buf[0] = TOBLE_QST_BLE_ADV_ID;
    msg_buf[1] = 0;
	msg_buf[2] = 0;
	 
    //Send msg
    ret = Uart2BLE_Msg_Send(msg_buf,length,TRUE);
    
    return ret;
}

s32 BLE_Send_Parameter(void)
{
	u8 msg_buf[3];
    u16 length = 3;
    s32 ret;
 
    msg_buf[0] = TOBLE_SET_PAR_ID;
    msg_buf[1] = 0;
	msg_buf[2] = 0;
	 
    //Send msg
    ret = Uart2BLE_Msg_Send(msg_buf,length,TRUE);
    
    return ret;
}

s32 BLE_Send_Reboot_GSM(void)
{
	u8 msg_buf[3];
    u16 length = 3;
    s32 ret;
 
    msg_buf[0] = TOBLE_REBOOT_GSM_ID;
    msg_buf[1] = 0;
	msg_buf[2] = 0;
	 
    //Send msg
    ret = Uart2BLE_Msg_Send(msg_buf,length,TRUE);
    
    return ret;
}

s32 Uart2BLE_Msg_Send(u8 *pBuffer, u16 length, bool need_rsp)
{
	u8 *buffer,*msg_send_buff;
	u16 count;
    u16 msg_len = length + 3;
    u16 msg_send_buff_len;
  	
    if( pBuffer == NULL || length < 2 )
	{  
    	//error accured
        return APP_RET_ERR_PARAM;
	}
  
  	//identification + checkcode + identification = 3 bytes 
  	buffer = Ql_MEM_Alloc(msg_len);
  	if (buffer == NULL)
    {
        APP_ERROR("%s/%d:Ql_MEM_Alloc FAIL! size:%u\r\n", __func__, __LINE__,msg_len);
        return APP_RET_ERR_MEM_OLLOC;
    }
  
  	//add identification
  	buffer[0] = BLE_MSG_IDENTIFIER;
  
  	//add msg   
  	Ql_memcpy(buffer+1, pBuffer, msg_len);

  	//checkcode
  	buffer[msg_len-2] = Uart2BLE_Msg_Calculate_Checkcode(buffer,msg_len);
  
  	//add identification
  	buffer[msg_len-1] = BLE_MSG_IDENTIFIER;

  	count = Uart2BLE_Msg_Need_Transform(buffer, msg_len);
  	if(count > 0)
  	{
    	msg_send_buff_len = msg_len + count;
    	msg_send_buff = (u8 *)Ql_MEM_Alloc(msg_send_buff_len);
    	if (buffer == NULL)
    	{
        	APP_ERROR("%s/%d:Ql_MEM_Alloc FAIL! size:%u\r\n", __func__, __LINE__,msg_len);
			Ql_MEM_Free(buffer);
  			buffer = NULL;
        	return APP_RET_ERR_MEM_OLLOC;
    	}
    	
  		Uart2BLE_Msg_Transform(buffer,msg_len,msg_send_buff,msg_send_buff_len);
  		Ql_MEM_Free(buffer);
  		buffer = NULL;
  	}
  	else
  	{
		msg_send_buff_len = msg_len;
		msg_send_buff = buffer;
  	}
  	
	#if 0
    for(int i = 0; i < msg_send_buff_len; i++)
    	APP_DEBUG("%02x",msg_send_buff[i]);
    APP_DEBUG("\r\n");
    #endif

	if( waiting_rsp && need_rsp )
	{
	  	//waiting for rsp, can't send
	  	UART_MSG uart_send;
	  	Ql_memcpy(uart_send.msg_buffer, msg_send_buff, msg_send_buff_len);
		uart_send.msg_len = msg_send_buff_len;
		uart_send.need_rsp = need_rsp;
      	SL_Insert(gSList_BLE,uart_send);
      	if( have_start_timer_rsp != TRUE)
      	{
        	have_start_timer_rsp = TRUE;
        	s32 ret = Ql_Timer_Start(BLE_UART_RSP_TIMER_ID,BLE_UART_RSP_INTERVAL,FALSE);
    		if(ret < 0)
    		{
        		APP_ERROR("failed!! timer(%d) Ql_Timer_Start ret=%d\r\n",ret,BLE_UART_RSP_TIMER_ID);        
    		}
      	}
	}
	else
	{  
	  	//not waiting for rsp, can send
	    Uart2BLE_Msg_Send_Action(msg_send_buff,msg_send_buff_len,need_rsp);
	}
	
	Ql_MEM_Free(msg_send_buff);
  	msg_send_buff = NULL;
  	return APP_RET_OK;   
}

s32 Uart2BLE_Msg_Send_Action(u8 *pBuffer, u16 length, bool need_rsp)
{
	s32 ret;
	ret = Ql_UART_Write(BLE_UART_PORT, pBuffer, length);
	if (ret != length)
    {
       	APP_ERROR("App Fail to send uart%d data,ret:%d\n",BLE_UART_PORT,ret);
    }
    
    #if 1
    {
    	u8 strTemp[100] = {0};
    	u16 i = 0;
    	for(i = 0; i < length; i++)
    	{
          	Ql_sprintf(strTemp+i*3, "%02x ",pBuffer[i]);
    	}
    	APP_DEBUG("UartToBle: %s\n",strTemp);
    }
	#endif
	
	if(need_rsp)
	{  
		Ql_memcpy(last_uart_send.msg_buffer, pBuffer, length);
		last_uart_send.msg_len = length;
		last_uart_send.need_rsp = need_rsp;
	
		//start a timer to check respond
		waiting_rsp = TRUE;
        have_start_timer_rsp = TRUE;
		s32 ret = Ql_Timer_Start(BLE_UART_RSP_TIMER_ID,BLE_UART_RSP_INTERVAL,FALSE);
    	if(ret < 0)
    	{
        	APP_ERROR("failed!! timer(%d) Ql_Timer_Start ret=%d\r\n",ret,BLE_UART_RSP_TIMER_ID);        
    	}
	}
	return APP_RET_OK;
}

u8 Uart2BLE_Msg_Calculate_Checkcode(u8 *pBuffer, u16 length)
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

u16 Uart2BLE_Msg_Need_Transform(u8 *pBuffer, u16 length)
{
	u16 i;
	u16 m = length - 1;
	u16 count = 0;
	
	for(i = 1; i < m; i++)
	{
		if( pBuffer[i] == BLE_MSG_IDENTIFIER ||
			pBuffer[i] == BLE_MSG_IDENTIFIER_TRANSFORM )
		{
			count++;
		}	
	}

	return count;
}

s32 Uart2BLE_Msg_Transform(u8 *pBuffer, u16 length, u8 *pSend_Buffer, u16 send_length)
{
	u16 i, j;
    for( i = 1, j = 1; i < length - 1; i++)
    {
        if( pBuffer[i] == BLE_MSG_IDENTIFIER || pBuffer[i] == BLE_MSG_IDENTIFIER_TRANSFORM )
        {
            if( pBuffer[i] == BLE_MSG_IDENTIFIER )
            {
                pSend_Buffer[j] = BLE_MSG_IDENTIFIER_TRANSFORM;
                j++;
                pSend_Buffer[j] = 0x02;
                j++;
            }
            else if( pBuffer[i] == BLE_MSG_IDENTIFIER_TRANSFORM )
            {
                pSend_Buffer[j] = BLE_MSG_IDENTIFIER_TRANSFORM;
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
    
    pSend_Buffer[0] = BLE_MSG_IDENTIFIER;
	pSend_Buffer[j] = BLE_MSG_IDENTIFIER;
	if(send_length == (j + 1))
    	return APP_RET_OK;
    else
    	return APP_RET_ERR_PARAM;
}

u16 Uart_BLE_Msg_Need_Retransform(u8 *pBuffer, u16 length)
{
	u16 i;
	u16 m = length - 1;
	u16 count = 0;
	
	for(i = 1; i < m; i++)
	{
		if( pBuffer[i] == BLE_MSG_IDENTIFIER_TRANSFORM &&
		  ((pBuffer[i+1] == 0x01) || (pBuffer[i+1] == 0x02)))
		{
			count++;
		}	
	}

	return count;
}

u16 Uart_BLE_Msg_Retransform(u8 *pBuffer,u16 numBytes)
{
	u16 i,j;
	for(i = 1, j = 0; i < numBytes; i++)
	{
		pBuffer[i-j] = pBuffer[i];
		if( pBuffer[i] == BLE_MSG_IDENTIFIER_TRANSFORM && (pBuffer[i+1] == 0x01))
		{
			i++;
			j++;
			continue;
			
		}
		else if( pBuffer[i] == BLE_MSG_IDENTIFIER_TRANSFORM && (pBuffer[i+1] == 0x02))
		{
			pBuffer[i-j] = BLE_MSG_IDENTIFIER;
			i++;
			j++;
			continue;
		}	
	}
	
	//for(int i = 0; i < numBytes - j; i++)
    	//APP_DEBUG("0x%x ",pBuffer[i]);

    return (numBytes - j);

}

void Uart_BLE_Msg_Parse(u8* pBuffer, u16 length)
{
    //pBuffer[0] = BLE_MSG_IDENTIFIER;pBuffer[1]=msd id
    switch(pBuffer[1])
    {
        case FROMBLE_COMMON_RSP_ID:
        	if(last_uart_send.msg_buffer[1] == pBuffer[3])
        	{
		    	last_uart_send.msg_buffer[0] = 0;
		    }	
            break;
			
        case FROMBLE_SW_VERSION_ID: 
            Uart2BLE_Common_Response(FROMBLE_SW_VERSION_ID,RSP_OK);
            break;
			
        case FROMBLE_ENG_MODE_ID:
            Uart2BLE_Common_Response(FROMBLE_ENG_MODE_ID,RSP_OK);
            break;
			
        case FROMBLE_BATTARY_ID:
            Uart2BLE_Common_Response(FROMBLE_BATTARY_ID,RSP_OK);
            battery = pBuffer[3];
            if(battery < gParmeter.parameter_8[BATTERY_ALARM_INDEX].data)
            {
				//report to gprs task
				Ql_OS_SendMessage(subtask_gprs_id, MSG_ID_ALARM_REP, ALARM_BIT_LOW_POWER, TRUE);
			}
			else if(gAlarm_Flag.alarm_flags & BV(ALARM_BIT_LOW_POWER))
			{
				//clean
				Ql_OS_SendMessage(subtask_gprs_id, MSG_ID_ALARM_REP, ALARM_BIT_LOW_POWER, FALSE);
			}
            break;         
           
         case FROMBLE_PARTNER_IMEI_ID:
            Uart2BLE_Common_Response(FROMBLE_PARTNER_IMEI_ID,RSP_OK);
            Ql_memcpy(gParmeter.parameter_12[2].data, pBuffer+3, 8);
            break;

        case FROMBLE_PARTNER_IMSI_ID:
            Uart2BLE_Common_Response(FROMBLE_PARTNER_IMSI_ID,RSP_OK);
            Ql_memcpy(gParmeter.parameter_12[3].data, pBuffer+3, 8);
            break;  
			
        case FROMBLE_BLE_STATUS_ID:
            Uart2BLE_Common_Response(FROMBLE_BLE_STATUS_ID,RSP_OK);
            ble_state = pBuffer[3];
            if(ble_state == 1)
            {
				//report to gprs task
				Ql_OS_SendMessage(subtask_gprs_id, MSG_ID_ALARM_REP, ALARM_BIT_LOST_DOWN, TRUE);
			}
			else if((gAlarm_Flag.alarm_flags & BV(ALARM_BIT_LOW_POWER)) && (ble_state == 0))
			{
				//clean
				Ql_OS_SendMessage(subtask_gprs_id, MSG_ID_ALARM_REP, ALARM_BIT_LOST_DOWN, FALSE);
			}
            break;  
			
        case FROMBLE_SW_TRANFRER_ID:
            Uart2BLE_Common_Response(FROMBLE_SW_TRANFRER_ID,RSP_OK);
            break;   
        default:                
            break;
    }                 
}

void Uart_BLE_Msg_Handle(u8 *pBuffer, u16 length)
{
    u16 retransform_count;

	retransform_count = Uart_BLE_Msg_Need_Retransform(pBuffer,length);
	if(retransform_count > 0)
	{
		//need to retransform
		length = Uart_BLE_Msg_Retransform(pBuffer,length);
	}
	
    if ( !Server_Msg_Verify_Checkcode(pBuffer, length))
    {
		APP_ERROR("check code error:%s\n",__func__);
      	return;
    }
    Uart_BLE_Msg_Parse(pBuffer, length);

    return;
}

#endif // __CUSTOMER_CODE__

