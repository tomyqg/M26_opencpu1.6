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
#include "ql_gprs.h"
#include "app_gps.h"

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
#define TOSMALLENDIAN16_32(X)       ((((X >> 16) & 0xff00) >> 8) | (((X >> 16) & 0x00ff) << 8))
#define TOSMALLENDIAN16(X,Y)       (((X) << 8) | (Y))
#define TOSMALLENDIAN32(A,B,C,D)   (((A) << 24) | \
                             	    ((B) << 16) | \
							        ((C) << 8) | \
						             (D)) 

#define BCDTODEC(bcd) (((bcd) % 16) + ((bcd)>>4) * 10)
#define DECTOBCD(bcd) ((((bcd) / 10)*16) + ((bcd)%10))  						   
  
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
extern u8 gServer_State;

//device to server msg id
#define TOSERVER_COMMON_RSP_ID            0x0001
#define TOSERVER_HEARTBEAT_ID             0x0100  
#define TOSERVER_REGISTER_ID              0x0102
#define TOSERVER_PARAMETER_ID             0x0104

#define TOSERVER_LOCATION_INFO_ID         0x0200
#define TOSERVER_ALARM_ID                 0x0201

//server to device msg id  
#define TODEVICE_GENERIC_RSP_ID           0x8001
#define TODEVICE_REGISTER_RSP_ID          0x8002
#define TODEVICE_SET_PARAMETER_ID         0x8103
#define TODEVICE_GET_PARAMETER_ID         0x8104
#define TODEVICE_SET_SLEEP_MODE_ID        0x8105
#define TODEVICE_REQUEST_LOCATION_ID      0x8200
#define TODEVICE_LOCATION_POLICY_ID       0x8202

#define TODEVICE_OTA_ID                   0x8300

//heartbeat
#define HB_TIMER_ID         (TIMER_ID_USER_START + 100)
#define HB_TIMER_PERIOD     20000   //20s

//alarm
#define ALARM_TIMER_ID         (TIMER_ID_USER_START + 102)

//beijin timezone
#define TIMEZONE            8

#define parameter_8_num     23
#define parameter_12_num    5
#define MASTER_SLAVE_SERVER 0x0019
#define QST_NORMAL_ALARM    0x0104
#define HWJ_SLEEP_TIME      0x0108
#define HWJ_WAKE_TIME       0x0109
#define HWJ_POWER_POLICY    0x010A
#define BLE_DOWN_HEART      0x0110
#define BLE_UP_HEART        0x0111
#define QST_ADV_POLICY      0x0112
#define QST_ADV_LAST_TIME   0x0113
#define NETWORK_TIME        0x0305
#define BATTERY_LOW         0x0202
#define PASSWORD            0x0400

#define SRV_CONFIG_BLOCK       11
#define SRV_CONFIG_BLOCK_LEN   100
#define SRV_CONFIG_STORED_FLAG 0xF5

#define SYS_CONFIG_BLOCK       12
#define SYS_CONFIG_BLOCK_LEN   100
#define SYS_CONFIG_STORED_FLAG 0xF5

typedef enum {
	STOP_REPORT_LOCATION = 0,
    TIMER_REPORT_LOCATION,
	DISTANCE_REPORT_LOCATION,
	DIS_TIM_REPORT_LOCATION
}Enum_LocationReportPolicy;

typedef enum {
	HEARTBEAT_INTERVAL_INDEX     = 0,
    LOST_HEARTBEAT_RSP_MAX_INDEX = 1,
	RSP_TIMEOUT_INDEX            = 2,
	TCP_RETRY_TIMES_INDEX        = 3,
	SRV_MASTER_SLAVER_INDEX      = 4,
	QST_UNNORMAL_SLEEP_TIM_INDEX     = 5,
	QST_UNNORMAL_WACKUP_TIM_INDEX    = 6,
	QST_UNNORMAL_WACKUP_POLICY_INDEX = 7,
	QST_WORKUP_TIME_INDEX        = 8,
	HWJ_SLEEP_TIME_INDEX         = 10,
	HWJ_WORKUP_TIME_INDEX        = 11,
	HWJ_SLEEP_WORKUP_POLICY_INDEX    = 12,
	BLE_DOWN_HEARTBEAT_INDEX     = 13,
	BLE_UP_HEARTBEAT_INDEX       = 14,
	QST_UNNORMAL_ADV_POLICY_INDEX    = 15,
	QST_ADV_LAST_TIME_INDEX      = 16,
	GPS_SPEED_UP_LIMITES_INDEX   = 17,
	GPRS_STATE_CHECK_INDEX       = 18,
	BATTERY_ALARM_INDEX          = 19,
	ALARM_FLAG_INDEX             = 20,
	ALARM_INTERVAL_INDEX         = 21,
}Enum_ParameterIndex;

enum {
    APP_RSP_OK           = 0,
    APP_RSP_FAILURE      = 1,
    APP_RSP_MSG_ERROR    = 2,
    APP_RSP_NONSUPPORT   = 3,
    APP_RSP_CHECKERR     = 4,
};
/*********************************************************************
 * TYPEDEFS
 */
 
typedef struct {
    u16 protocol_version;    
    u16 msg_id;
    u16 msg_length;
    u8  device_imei[8];
    u16 msg_number;  
}Server_Msg_Head;

typedef struct {
    u32 alarm_flags;    
    u32 alarm_flags_bk;
}Alarm_Flag;
extern Alarm_Flag gAlarm_Flag;

typedef struct {
	u16 id;    
    u16 length;
    u32 data;
}Parameter_8;

typedef struct {
	u16 id;    
    u16 length;
    u8 data[8];
}Parameter_12;

typedef struct {
	Parameter_8 parameter_8[parameter_8_num];
	Parameter_12 parameter_12[parameter_12_num];
}Parameter;
extern Parameter gParmeter;

typedef struct {
	u32 lac;    
    u32 cell_id;
}Lac_CellID;
extern Lac_CellID glac_ci;

typedef struct {
    u32 location_policy;    
    u32 static_policy;
    u32 time_Interval;
    u32 distance_Interval;
    u32 bearing_Interval;
}Location_Policy;

typedef struct {
    Location_Policy gLocation_Policy;
    u8 password[4];
    //ST_GprsConfig GprsConfig;
    u8 apnName[MAX_GPRS_APN_LEN];
    u8 apnUserId[MAX_GPRS_USER_NAME_LEN]; 
    u8 apnPasswd[MAX_GPRS_PASSWORD_LEN];
}SYS_CONFIG;
extern SYS_CONFIG mSys_config;

#define   MAX_SRV_LEN     40
typedef struct {
    u8 masterSrvAddress[MAX_SRV_LEN];
	u16 masterSrvPort;
	u8 slaveSrvAddress[MAX_SRV_LEN];
	u16 slaveSrvPort;
}SRV_CONFIG;
extern SRV_CONFIG mSrv_config;

extern GpsLocation gGpsLocation;

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
u16 msg_need_transform(u8 *pBuffer, u16 length);
s32 App_Server_Register(void);
void App_Heartbeat_To_Server(void);
void App_Heartbeat_Check(void);

bool Server_Msg_Handle(u8 *pbuffer,u16 numBytes);
u16 Server_Msg_Need_Retransform(u8 *pBuffer, u16 length);
u16 Server_Msg_Retransform(u8 *pbuffer,u16 numBytes);
bool Server_Msg_Verify_Checkcode(u8 *pBuffer, u16 length );
void Server_Msg_Parse(u8* pBuffer, u16 length);
void App_Report_Parameter(u16 msg_id, u16 msg_number);
void App_Set_Parameter(u8* pBuffer, u16 length);
s32 binary_search_parameter(Parameter_8 *pParameter_8, u32 len, u16 goal, u8 par_len);
void get_lac_cellid(char *s);
void App_Report_Location( void );
void App_Set_Location_policy( u8* pBuffer, u16 length );
s32 App_CommonRsp_To_Server( u16 msg_id, u16 msg_number, u8 rsp);
void update_alarm(u32 alarm_bit, u32 alarm);
void App_Ropert_Alarm(u32 alarm_flag);
void Timer_Handler_Alarm(u32 timerId, void* param);
void Timer_Handler_HB(u32 timerId, void* param);
void App_Set_Sleep_mode(u8* pBuffer,u16 length);
#endif  //__APP_SERVER_H__
