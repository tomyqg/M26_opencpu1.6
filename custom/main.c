
/*****************************************************************************
*  Copyright Statement:
*  --------------------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of Quectel Co., Ltd. 2013
*
*****************************************************************************/
/*****************************************************************************
 *
 * Filename:
 * ---------
 *   main.c
 *
 * Project:
 * --------
 *   OpenCPU
 *
 * Description:
 * ------------
 *   This app demonstrates how to send AT command with RIL API, and transparently
 *   transfer the response through MAIN UART. And how to use UART port.
 *   Developer can program the application based on this example.
 * 
 ****************************************************************************/
#ifdef __CUSTOMER_CODE__
#include "custom_feature_def.h"
#include "ril.h"
#include "ril_util.h"
#include "ril_network.h"
#include "ril_telephony.h"
#include "ril_sms.h"
#include "ql_stdlib.h"
#include "ql_error.h"
#include "ql_trace.h"
#include "ql_system.h"
#include "ql_timer.h"
#include "ql_gprs.h"
#include "ql_socket.h"
#include "ql_uart.h"
#include "app_common.h"
#include "app_socket.h"
#include "app_server.h"
#include "app_tokenizer.h"

#define SERIAL_RX_BUFFER_LEN  2048

// Define the UART port and the receive data buffer
static Enum_SerialPort m_myUartPort  = UART_PORT1;
static u8 m_RxBuf_Uart1[SERIAL_RX_BUFFER_LEN];
static void CallBack_UART_Hdlr(Enum_SerialPort port, Enum_UARTEventType msg, bool level, void* customizedPara);
static s32 ATResponse_Handler(char* line, u32 len, void* userData);
static s32 GetIMEIandIMSI(void);
s32 gGprsState    = 0;   // GPRS state, 0= not REGISTERED, 1=REGISTERED

char userdata[20];
u8 g_imei[8],g_imsi[8];
extern Lac_CellID glac_ci;
extern ST_GprsConfig m_GprsConfig;

extern s32 RIL_SIM_GetIMEI(char* pIMEI, u32 pIMEILen);
extern s32 RIL_SIM_GetIMSI(char* pIMSI, u32 pIMSILen);

s32  s_iMutexId = 0;

/************************************************************************/
/*                                                                      */
/* The entrance to application, which is called by bottom system.       */
/*                                                                      */
/************************************************************************/
void proc_main_task(s32 taskId)
{
    s32 ret;
    ST_MSG msg;

    gGprsState = 0;

    // Register & open UART port
    ret = Ql_UART_Register(m_myUartPort, CallBack_UART_Hdlr, NULL);
    if (ret < QL_RET_OK)
    {
        Ql_Debug_Trace("Fail to register serial port[%d], ret=%d\r\n", m_myUartPort, ret);
    }
    ret = Ql_UART_Open(m_myUartPort, 115200, FC_NONE);
    if (ret < QL_RET_OK)
    {
        Ql_Debug_Trace("Fail to open serial port[%d], ret=%d\r\n", m_myUartPort, ret);
    }

    s_iMutexId = Ql_OS_CreateMutex("MyMutex");

    APP_DEBUG("OpenCPU: Customer Application\r\n");
    do {
    	u8 rMsg[50];
    	Ql_sprintf(rMsg, "VERSION:%s\r\nBUILD:%s\r\n",VERSION,BUILD);
    	Ql_UART_Write((Enum_SerialPort)(UART_PORT1), (u8*)(rMsg), Ql_strlen((const char *)(rMsg)));
    } while(0);

    // START MESSAGE LOOP OF THIS TASK
    while(TRUE)
    {
        // Retrieve a message from the message queue of this task. 
        // This task will pend here if no message in the message queue, till a new message arrives.
        Ql_OS_GetMessage(&msg);
        // Handle the received message
        switch(msg.message)
        {
            // Application will receive this message when OpenCPU RIL starts up.
            // Then application needs to call Ql_RIL_Initialize to launch the initialization of RIL.
        case MSG_ID_RIL_READY:
            APP_DEBUG("<-- RIL is ready -->\r\n");
            Ql_RIL_Initialize();
            //
            // After RIL initialization, developer may call RIL-related APIs in the .h files in the directory of SDK\ril\inc
            // RIL-related APIs may simplify programming, and quicken the development.
            //
            SendEvent2AllSubTask(MSG_ID_RIL_READY,0,0);
            break;

            // Handle URC messages.
            // URC messages include "module init state", "CFUN state", "SIM card state(change)",
            // "GSM network state(change)", "GPRS network state(change)" and other user customized URC.
        case MSG_ID_URC_INDICATION:
            switch (msg.param1)
            {
            case URC_CFUN_STATE_IND:
                APP_DEBUG("<-- CFUN Status:%d -->\r\n", msg.param2);
                break;
                // URC for SIM card state(change)
            case URC_SIM_CARD_STATE_IND:
                if (SIM_STAT_READY == msg.param2)
                {
                    APP_DEBUG("<-- SIM card is ready -->\r\n");
					GetIMEIandIMSI();
					Ql_OS_SendMessage(subtask_ble_id, MSG_ID_IMEI_IMSI_OK, 0, 0);
                }
                else
                {
                    APP_DEBUG("<-- SIM card is not available, cause:%d -->\r\n", msg.param2);
                    /* cause: 0 = SIM card not inserted
                     *        2 = Need to input PIN code
                     *        3 = Need to input PUK code
                     *        9 = SIM card is not recognized
                     */
                }
                break;
                // URC for module initialization state
            case URC_SYS_INIT_STATE_IND:
                if (SYS_STATE_SMSOK == msg.param2)
                {
                    // SMS option has been initialized, and application can program SMS
                    APP_DEBUG("<-- Application can program SMS -->\r\n");
                    RIL_SMS_DeleteSMS(1, RIL_SMS_DEL_ALL_MSG);
                }
                break;    

                // URC for GSM network state(change).
                // Application receives this URC message when GSM network state changes, such as register to 
                // GSM network during booting, GSM drops down.
            case URC_GSM_NW_STATE_IND:
                if (NW_STAT_REGISTERED == msg.param2 || NW_STAT_REGISTERED_ROAMING == msg.param2)
                {
                    APP_DEBUG("<-- Module has registered to GSM network status:%d-->\r\n",msg.param2);
                }else{
                    //APP_DEBUG("<-- GSM network status:%d -->\r\n", msg.param2);
                    /* status: 0 = Not registered, module not currently search a new operator
                     *         2 = Not registered, but module is currently searching a new operator
                     *         3 = Registration denied 
                     */
                   	APP_DEBUG("<-- Module GSM network status:%d-->\r\n",msg.param2);  
                   	gGprsState = 0;
                }
                break;

                // URC for GPRS network state(change).
                // Application receives this URC message when GPRS network state changes, such as register to 
                // GPRS network during booting, GSM drops down.
            case URC_GPRS_NW_STATE_IND:
                if (NW_STAT_REGISTERED == msg.param2 || NW_STAT_REGISTERED_ROAMING == msg.param2)
                {
					if(gGprsState == 0)
					{
                    	APP_DEBUG("<-- Module has registered to GPRS network status:%d-->\r\n",msg.param2);
						APP_DEBUG("lac:0x%x,cell_id:0x%x\n",glac_ci.lac,glac_ci.cell_id);
						gGprsState = 1;
                    	// Module has registered to GPRS network,app can start to activate PDP and program TCP
                    	Ql_OS_SendMessage(subtask_gprs_id, MSG_ID_GPRS_STATE, msg.param2, 0);
                    	Ql_OS_SendMessage(subtask_ble_id, MSG_ID_GPRS_STATE, msg.param2, 0);
                    }	
                }else{
                    /* status: 0 = Not registered, module not currently search a new operator
                     *         2 = Not registered, but module is currently searching a new operator
                     *         3 = Registration denied 
                     */
                    APP_DEBUG("<-- GPRS network status:%d -->\r\n", msg.param2);
                    gGprsState = 0;
                    // If GPRS drops down and currently socket connection is on line, app should close socket
                    // and check signal strength. And try to reset the module.
                    if (NW_STAT_NOT_REGISTERED == msg.param2 /*&& m_GprsActState*/)
                    {// GPRS drops down
                        u32 rssi;
                        u32 ber;
                        s32 nRet = RIL_NW_GetSignalQuality(&rssi, &ber);
                        APP_DEBUG("<-- Signal strength:%d, BER:%d -->\r\n", rssi, ber);
                    }
                }
                break;
            case URC_COMING_CALL_IND:
                {
                    ST_ComingCall* pComingCall = (ST_ComingCall*)msg.param2;
                    APP_DEBUG("<-- Coming call, number:%s, type:%d -->\r\n", pComingCall->phoneNumber, pComingCall->type);
                    break;
                }
            case URC_CALL_STATE_IND:
                APP_DEBUG("<-- Call state:%d\r\n", msg.param2);
                break;
            case URC_NEW_SMS_IND:
                APP_DEBUG("<-- New SMS Arrives: index=%d\r\n", msg.param2);
                Ql_OS_SendMessage(subtask_sms_id, MSG_ID_NEW_SMS, msg.param2, 0);
                break;
            case URC_MODULE_VOLTAGE_IND:
                APP_DEBUG("<-- VBatt Voltage Ind: type=%d\r\n", msg.param2);
                break; 
            case URC_ALARM_RING_IND:
                APP_DEBUG("<-- alarm ring,alarm_on_off = %d\r\n",alarm_on_off);
                if(alarm_on_off == 0)
                {
					Ql_OS_SendMessage(subtask_ble_id, MSG_ID_CLK_ALARM, 0, 0);
                   	if(gParmeter.parameter_8[HWJ_SLEEP_WORKUP_POLICY_INDEX].data != 0)
                	{   
						if(gsm_wake_sleep){
      						alarm_on_off = 1;
						} else {
							alarm_on_off = 2;
						}
						ST_Time datetime;
						Ql_GetLocalTime(&datetime);
						update_clk_alarm(&datetime);
                	}
                }else {
					if(alarm_on_off == 1 &&
					   gParmeter.parameter_8[HWJ_SLEEP_WORKUP_POLICY_INDEX].data != 0)
					{
						//set power up time point,syste will power off after 5s
						alarm_on_off = 2;
						APP_DEBUG("set system power up alarm");
						ST_Time datetime;
						Ql_GetLocalTime(&datetime);
						update_clk_alarm(&datetime);
					}
                }
                break;   
            default:
                APP_DEBUG("<-- Other URC: type=%d\r\n", msg.param1);
                break;
            }
            break;
        default:
            break;
        }
    }
}

s32 SendEvent2AllSubTask(u32 msgId,u32 iData1, u32 iData2)
{
    int iTmp = 0;
    int iRet = 0;
    
    for(iTmp=subtask_gprs_id; iTmp<SUB_TASK_NUM+subtask_gprs_id; iTmp++)
    {

        iRet = Ql_OS_SendMessage(iTmp,msgId,iData1, iData2);
     
        if(iRet != OS_SUCCESS)
        {
           APP_ERROR("failed!!,Ql_OS_SendMessage(%d,%d,%d,%d) fail, ret=%d\n", iTmp,msgId,iData1,iData2, iRet);
           break;
        } 
    }
    return iRet;
}

s32 GetIMEIandIMSI(void)
{
	u32 ret;
    ret = RIL_SIM_GetIMEI(userdata, sizeof(userdata));
	if (ret != RIL_AT_SUCCESS)
	{
		APP_ERROR("Fail to get IMSI!\r\n");
	}
	
	#if 1
	g_imei[0] = userdata[0] - '0';
	APP_DEBUG("IMEI: %x",g_imei[0]);
	for(u8 i = 1,j = 1; i < 8; i++)
	{
		g_imei[i] = ((userdata[j] - '0')<<4) + (userdata[j+1] - '0');
		j += 2;
		APP_DEBUG("%02x",g_imei[i]);
	}
	#endif
	
	ret = RIL_SIM_GetIMSI(userdata, sizeof(userdata));
	if (ret != RIL_AT_SUCCESS)
	{
		APP_ERROR("Fail to get IMSI!\r\n");
	}
	
	#if 1
	g_imsi[0] = userdata[0] - '0';
	APP_DEBUG(" IMSI: %x",g_imsi[0]);
	for(u8 i = 1,j = 1; i < 8; i++)
	{
		g_imsi[i] = ((userdata[j] - '0')<<4) + (userdata[j+1] - '0');
		j += 2;
		APP_DEBUG("%02x",g_imsi[i]);
	}
	APP_DEBUG("\r\n");
	#endif
	
	Ql_memcpy(gParmeter.parameter_12[0].data, g_imei, 8);
	Ql_memcpy(gParmeter.parameter_12[1].data, g_imsi, 8);

	return APP_RET_OK;
}

static s32 ReadSerialPort(Enum_SerialPort port, /*[out]*/u8* pBuffer, /*[in]*/u32 bufLen)
{
    s32 rdLen = 0;
    s32 rdTotalLen = 0;
    if (NULL == pBuffer || 0 == bufLen)
    {
        return -1;
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
        APP_DEBUG("Fail to read from port[%d]\r\n", port);
        return -99;
    }
    return rdTotalLen;
}

static void CallBack_UART_Hdlr(Enum_SerialPort port, Enum_UARTEventType msg, bool level, void* customizedPara)
{
    //APP_DEBUG("CallBack_UART_Hdlr: port=%d, event=%d, level=%d, p=%x\r\n", port, msg, level, customizedPara);
    switch (msg)
    {
    case EVENT_UART_READY_TO_READ:
        {
            if (m_myUartPort == port)
            {
                s32 totalBytes = ReadSerialPort(port, m_RxBuf_Uart1, sizeof(m_RxBuf_Uart1));
                if (totalBytes <= 0)
                {
                    APP_DEBUG("<-- No data in UART buffer! -->\r\n");
                    return;
                }
                {// Read data from UART
                    s32 ret;
                    char* pCh = NULL;
                    
                    // Echo
                    Ql_UART_Write(m_myUartPort, m_RxBuf_Uart1, totalBytes);

                    pCh = Ql_strstr((char*)m_RxBuf_Uart1, "\r\n");
                    if (pCh)
                    {
                        *(pCh + 0) = '\0';
                        *(pCh + 1) = '\0';
                    }

                    // No permission for single <cr><lf>
                    if (Ql_strlen((char*)m_RxBuf_Uart1) == 0)
                    {
                        return;
                    }

					//gps infor
                    if (Ql_strstr((char*)m_RxBuf_Uart1, "AT+GPSINFO="))
                    {
						pCh = m_RxBuf_Uart1 + 11;
						if(Ql_isdigit(*pCh))
						{
							gps_info_out = (*pCh - '0')?TRUE:FALSE;
                        	return;
                        }
                        return;
                    }
                    
                    ret = Ql_RIL_SendATCmd((char*)m_RxBuf_Uart1, totalBytes, ATResponse_Handler, NULL, 0);
                }
            }
            break;
        }
    case EVENT_UART_READY_TO_WRITE:
        break;
    default:
        break;
    }
}

static s32 ATResponse_Handler(char* line, u32 len, void* userData)
{
    Ql_UART_Write(m_myUartPort, (u8*)line, len);
    
    if (Ql_RIL_FindLine(line, len, "OK"))
    {  
        return  RIL_ATRSP_SUCCESS;
    }
    else if (Ql_RIL_FindLine(line, len, "ERROR"))
    {  
        return  RIL_ATRSP_FAILED;
    }
    else if (Ql_RIL_FindString(line, len, "+CME ERROR"))
    {
        return  RIL_ATRSP_FAILED;
    }
    else if (Ql_RIL_FindString(line, len, "+CMS ERROR:"))
    {
        return  RIL_ATRSP_FAILED;
    }
    return RIL_ATRSP_CONTINUE; //continue wait
}

void MutextTest(int iTaskId)  //Two task Run this function at the same time
{

    APP_DEBUG("<---I am Task %d--->\r\n", iTaskId);
    Ql_OS_TakeMutex(s_iMutexId);
    APP_DEBUG("<-----I am Task %d----->\r\n", iTaskId);   //Another Caller prints this sentence after 5 seconds
    Ql_Sleep(5000);                                                             
    APP_DEBUG("<--(TaskId=%d)Do not reboot with calling Ql_sleep-->\r\n", iTaskId);
    Ql_OS_GiveMutex(s_iMutexId);
}

#endif // __CUSTOMER_CODE__
