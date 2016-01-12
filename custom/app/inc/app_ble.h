/*****************************************************************************
 *
 * Filename:
 * ---------
 *   app_ble.h 
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


#ifndef __APP_BLE_H__
#define __APP_BLE_H__

/*********************************************************************
 * INCLUDES
 */

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */
#define BLE_UART_PORT           UART_PORT2
#define BLE_UART_baudrate       115200

#define SERIAl_NUM_LEN          8
#define UART2BLE_MSG_BUFFER_LEN 50

//ble msg identifier
#define BLE_MSG_IDENTIFIER                0x55
#define BLE_MSG_IDENTIFIER_TRANSFORM      0x56

#define FROMBLE_COMMON_RSP_ID            0x01
#define FROMBLE_SW_VERSION_ID            0x02  
#define FROMBLE_ENG_MODE_ID              0x03
#define FROMBLE_BATTARY_ID               0x04
#define FROMBLE_PARTNER_IMEI_ID          0x05
#define FROMBLE_PARTNER_IMSI_ID          0x06
#define FROMBLE_BLE_STATUS_ID            0x07
#define FROMBLE_SW_TRANFRER_ID           0x5F

#define TOBLE_COMMON_RSP_ID              0x81
#define TOBLE_TIME_ID                    0x82
#define TOBLE_REBOOT_ID                  0x83
#define TOBLE_IMEI_ID                    0x84
#define TOBLE_IMSI_ID                    0x85
#define TOBLE_BATTERY_ID                 0x86
#define TOBLE_BOOTUP_QST_GSM_ID          0x87
#define TOBLE_QST_TO_POWEROFF_ID         0x88
#define TOBLE_REBOOT_PARTNER_ID          0x89
#define TOBLE_GSM_STATUS_ID              0x8A
#define TOBLE_QST_BLE_ADV_ID             0x8B
#define TOBLE_SET_PAR_ID                 0x8C
#define TOBLE_REBOOT_GSM_ID              0x8D
#define TOBLE_SW_TRANFRER_ID             0x8F

//respond resulet
#define RSP_OK                           0x00
#define RSP_FAILE                        0x01
#define RSP_MSG_ERR                      0x02
#define RSP_NOT_SUP                      0x03

#define BLE_UART_RSP_TIMER_ID        (TIMER_ID_USER_START + 103)
#define BLE_UART_RSP_INTERVAL        200
#define BLE_UART_HEARBEAT_TIMER_ID   (TIMER_ID_USER_START + 104)
#define BLE_UART_HEARBEAT_INTERVAL   30*1000

/*********************************************************************
 * TYPEDEFS
 */
typedef struct 
{
    u8  msg_buffer[UART2BLE_MSG_BUFFER_LEN];
	u16 msg_len;
	bool  need_rsp;
}UART_MSG;

/*********************************************************************
 * VARIABLES
 */
extern u32 battery;
 
/*********************************************************************
 * FUNCTIONS
 */
 
s32 Uart2BLE_Common_Response(u8 rsp_id,u8 rsp_status);
s32 BLE_Send_HEARTBEAT(void);
s32 BLE_Send_Reboot(void);
s32 BLE_Send_IMEI(void);
s32 BLE_Send_IMSI(void);
s32 BLE_Send_Battery(void);
s32 BLE_Send_PowerUp_Paired_GSM(void);
s32 BLE_Send_PowerOff_GSM(void);
s32 BLE_Send_Reboot_Paired(void);
s32 BLE_Send_GSM_State(u8 state);
s32 BLE_Send_Adv_Start(void);
s32 BLE_Send_Parameter(void);
s32 BLE_Send_Reboot_GSM(void);
s32 Uart2BLE_Msg_Send(u8 *pBuffer, u16 length, bool need_rsp);
s32 Uart2BLE_Msg_Send_Action(u8 *pBuffer, u16 length, bool need_rsp);
u8 Uart2BLE_Msg_Calculate_Checkcode(u8 *pBuffer, u16 length);
u16 Uart2BLE_Msg_Need_Transform(u8 *pBuffer, u16 length);
s32 Uart2BLE_Msg_Transform(u8 *pBuffer, u16 length, u8 *pSend_Buffer, u16 send_length);
 
#endif // End-of __APP_BLE_H__ 



