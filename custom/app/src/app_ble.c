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
#include <math.h>
#include "ql_stdlib.h"
#include "ql_memory.h"
#include "ql_time.h"
#include "ql_timer.h"
#include "ql_error.h"
#include "app_common.h"
#include "app_socket.h"
#include "app_server.h"
#include "app_ble.h"

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
#define BLE_SERIAL_RX_BUFFER_LEN  512
static char RxBuf_BLE[BLE_SERIAL_RX_BUFFER_LEN];

/*********************************************************************
 * FUNCTIONS
 */
static void CallBack_UART_BLE(Enum_SerialPort port, Enum_UARTEventType msg, bool level, void* customizedPara);

 
/**************************************************************
* ble sub task
***************************************************************/
void proc_subtask_ble(s32 TaskId)
{
    ST_MSG subtask_msg;
	s32 ret;
    
    APP_DEBUG("ble_subtask_entry,subtaskId = %d\n",TaskId);
    
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
				BLE_Send_IMEI_IMSI();
				break;
			}	
			
            default:
                break;
        }
    }    
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
                s32 totalBytes = ReadSerialPort(port, RxBuf_BLE, BLE_SERIAL_RX_BUFFER_LEN);
                if (totalBytes <= 0)
                {
                    APP_DEBUG("No data in UART buffer!\n");
                    return;
                }
				APP_DEBUG("uart port%d: %s\n",BLE_UART_PORT,RxBuf_BLE);
				//gps_reader_parse( mGpsReader );
				
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

s32 BLE_Send_IMEI_IMSI(void)
{
	
}

#endif // __CUSTOMER_CODE__

