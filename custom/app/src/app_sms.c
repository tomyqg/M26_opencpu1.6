/*********************************************************************
 *
 * Filename:
 * ---------
 *   app_sms.c 
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
#include "ql_error.h"
#include "ql_gprs.h"
#include "ql_socket.h"
#include "ril_sms.h"
#include "ril.h"
#include "app_common.h"
#include "app_tokenizer.h"
#include "app_server.h"
#include "app_socket.h"
#include "app_sms.h"

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
extern u32 battery;

/*********************************************************************
 * FUNCTIONS
 */
static void Hdlr_RecvNewSMS(u32 nIndex);
static void Parse_SMS_Data(const ST_RIL_SMS_DeliverParam *pDeliverTextInfo);
 
/**************************************************************
* sms sub task
***************************************************************/
void proc_subtask_sms(s32 TaskId)
{
    ST_MSG subtask_msg;
    
    APP_DEBUG("sms_subtask_entry,subtaskId = %d\n",TaskId);
    
    while(TRUE)
    {    
        Ql_OS_GetMessage(&subtask_msg);
        switch(subtask_msg.message)
        {
			case MSG_ID_RIL_READY:
			{
				APP_DEBUG("proc_subtask_sms revice MSG_ID_RIL_READY\n");
				break;
			}	
            case MSG_ID_NEW_SMS:
            {
                APP_DEBUG("recv msg: MSG_ID_NEW_SMS\n");
                Hdlr_RecvNewSMS(subtask_msg.param1);
                RIL_SMS_DeleteSMS(subtask_msg.param1, RIL_SMS_DEL_INDEXED_MSG);
                break;
            }

            default:
                break;
        }
    }    
}

static void Hdlr_RecvNewSMS(u32 nIndex)
{
    s32 iResult = 0;
    ST_RIL_SMS_TextInfo *pTextInfo = NULL;
    ST_RIL_SMS_DeliverParam *pDeliverTextInfo = NULL;
    
    pTextInfo = Ql_MEM_Alloc(sizeof(ST_RIL_SMS_TextInfo));
    if (NULL == pTextInfo)
    {
        APP_ERROR("%s/%d:Ql_MEM_Alloc FAIL! size:%u\r\n", sizeof(ST_RIL_SMS_TextInfo), __func__, __LINE__);
        return;
    }
    Ql_memset(pTextInfo, 0x00, sizeof(ST_RIL_SMS_TextInfo));
    iResult = RIL_SMS_ReadSMS_Text(nIndex, LIB_SMS_CHARSET_GSM, pTextInfo);
    if (iResult != RIL_AT_SUCCESS)
    {
        Ql_MEM_Free(pTextInfo);
        APP_ERROR("Fail to read text SMS[%d], cause:%d\r\n", nIndex, iResult);
        return;
    }        
    
    if ((LIB_SMS_PDU_TYPE_DELIVER != (pTextInfo->type)) || (RIL_SMS_STATUS_TYPE_INVALID == (pTextInfo->status)))
    {
        Ql_MEM_Free(pTextInfo);
        APP_ERROR("WARNING: NOT a new received SMS.\r\n");    
        return;
    }
    
    pDeliverTextInfo = &((pTextInfo->param).deliverParam);    

    if(TRUE == pDeliverTextInfo->conPres)  //Receive CON-SMS segment
    {
        APP_DEBUG("This is a concatenate SMS\r\n");
        Ql_MEM_Free(pTextInfo);
        return;
    }

    APP_DEBUG("data = %s\r\n",(pDeliverTextInfo->data));

    if(pDeliverTextInfo->length < 7 || pDeliverTextInfo->data[pDeliverTextInfo->length-1] != '#')
    {
		APP_ERROR("This is not a complete SMS\n");
        Ql_MEM_Free(pTextInfo);
        return;
    }
	Parse_SMS_Data(pDeliverTextInfo);
    Ql_MEM_Free(pTextInfo);
    return;
}

static void Parse_SMS_Data(const ST_RIL_SMS_DeliverParam *pDeliverTextInfo)
{
    u8 n = 0;
    Tokenizer tzer[1];
    Token tok;
    char aPhNum[RIL_SMS_PHONE_NUMBER_MAX_LEN] = {0,};

    Ql_strcpy(aPhNum, pDeliverTextInfo->oa);
    tokenizer_init(tzer, pDeliverTextInfo->data, pDeliverTextInfo->data + pDeliverTextInfo->length);

	tok = tokenizer_get(tzer, 1);
	n = tok.end - tok.p;
	if(n != 4 || Ql_memcmp(mSys_config.password, tok.p, 4))
    {
		APP_DEBUG("SMS password error\n");
		return;
    }	

    tok = tokenizer_get(tzer, 0);
    if ( !Ql_memcmp(tok.p, "APN", 3) )
    {
		static u8* rMsg = "SET APN OK";
		switch(tzer[0].count)
		{
			case 2:
				APP_DEBUG("using default APN!\n");
				//Ql_memcpy(&mSys_config.GprsConfig, &m_GprsConfig, sizeof(m_GprsConfig));
				break;
			case 3:
				tok = tokenizer_get(tzer, 2);
				Ql_memcpy(mSys_config.apnName, tok.p, tok.end - tok.p);
				mSys_config.apnName[tok.end - tok.p] = '\0';
				mSys_config.apnUserId[0] = '\0';
				mSys_config.apnPasswd[0] = '\0';
				break;
			case 4:
				tok = tokenizer_get(tzer, 2);
				Ql_memcpy(mSys_config.apnName, tok.p, tok.end - tok.p);
				mSys_config.apnName[tok.end - tok.p] = '\0';
				tok = tokenizer_get(tzer, 3);
				Ql_memcpy(mSys_config.apnUserId, tok.p, tok.end - tok.p);
				mSys_config.apnUserId[tok.end - tok.p] = '\0';
				mSys_config.apnPasswd[0] = '\0';
				break;
			case 5:
				tok = tokenizer_get(tzer, 2);
				Ql_memcpy(mSys_config.apnName, tok.p, tok.end - tok.p);
				mSys_config.apnName[tok.end - tok.p] = '\0';
				tok = tokenizer_get(tzer, 3);
				Ql_memcpy(mSys_config.apnUserId, tok.p, tok.end - tok.p);
				mSys_config.apnUserId[tok.end - tok.p] = '\0';
				tok = tokenizer_get(tzer, 4);
				Ql_memcpy(mSys_config.apnPasswd, tok.p, tok.end - tok.p);
				mSys_config.apnPasswd[tok.end - tok.p] = '\0';
				break;
			default:
				APP_DEBUG("using default APN!\n");
				//Ql_memcpy(&mSys_config.GprsConfig, &m_GprsConfig, sizeof(m_GprsConfig));
				break;
		}
		APP_DEBUG("set APN:%s,UserID:%s,Passwd:%s\n",
				mSys_config.apnName,mSys_config.apnUserId,mSys_config.apnPasswd);
		
        s32 iResult = RIL_SMS_SendSMS_Text(aPhNum, Ql_strlen(aPhNum),LIB_SMS_CHARSET_GSM,rMsg,Ql_strlen(rMsg),NULL);
        if (iResult != RIL_AT_SUCCESS)
        {
        	APP_ERROR("RIL_SMS_SendSMS_Text FAIL! iResult:%u\r\n",iResult);
        }
    } 
    else if ( !Ql_memcmp(tok.p, "SERVER", 6) )
    {
		static u8* rMsg = "SET SERVER OK";
		u8 flag;
		if(tzer[0].count == 5)
		{
			tok = tokenizer_get(tzer, 2);
			if(*(tok.p) == '0')
        	{
				flag = 0;
        	}
        	else if(*(tok.p) == '1')
        	{
				flag = 1;
        	}
        	
			tok = tokenizer_get(tzer, 3);
			if(flag == 0)
			{
				Ql_memcpy(mSrv_config.masterSrvAddress, tok.p, tok.end - tok.p);
				mSrv_config.masterSrvAddress[tok.end - tok.p] = '\0';
			} else if(flag == 1){
				Ql_memcpy(mSrv_config.slaveSrvAddress, tok.p, tok.end - tok.p);
				mSrv_config.slaveSrvAddress[tok.end - tok.p] = '\0';
			} else {
				return;
			}
        	
			tok = tokenizer_get(tzer, 4);
			if(flag == 0)
			{
				mSrv_config.masterSrvPort = Ql_atoi(tok.p);
				APP_DEBUG("set master server:%s port:%d\n",
        		      mSrv_config.masterSrvAddress,mSrv_config.masterSrvPort);
        		if(gParmeter.parameter_8[SRV_MASTER_SLAVER_INDEX].data == 0)
				{
					Ql_strcpy(SrvADDR, mSrv_config.masterSrvAddress);
					SrvPort = mSrv_config.masterSrvPort;
					Ql_OS_SendMessage(subtask_gprs_id, MSG_ID_SRV_CHANGED, 0, 0);
				}
			} else if(flag == 1){
				mSrv_config.slaveSrvPort = Ql_atoi(tok.p);
				APP_DEBUG("set slave server:%s port:%d\n",
        		      mSrv_config.slaveSrvAddress,mSrv_config.slaveSrvPort);
        		if(gParmeter.parameter_8[SRV_MASTER_SLAVER_INDEX].data == 1)
				{
					Ql_strcpy(SrvADDR, mSrv_config.slaveSrvAddress);
					SrvPort = mSrv_config.slaveSrvPort;
					Ql_OS_SendMessage(subtask_gprs_id, MSG_ID_SRV_CHANGED, 0, 0);
				}      
			}
			
        	s32 iResult = RIL_SMS_SendSMS_Text(aPhNum, Ql_strlen(aPhNum),LIB_SMS_CHARSET_GSM,rMsg,Ql_strlen(rMsg),NULL);
        	if (iResult != RIL_AT_SUCCESS)
        	{
        		APP_ERROR("RIL_SMS_SendSMS_Text FAIL! iResult:%u\r\n",iResult);
        	}

        	//save
			u8 *Buffer = (u8 *)Ql_MEM_Alloc(SRV_CONFIG_BLOCK_LEN);
    		if (Buffer == NULL)
  			{
  	 			APP_ERROR("%s/%d:Ql_MEM_Alloc FAIL! size:%u\r\n", __func__, __LINE__,SRV_CONFIG_BLOCK_LEN);
     			return;
  			}
			Ql_memcpy(Buffer+1, &mSrv_config, sizeof(mSrv_config));
			Buffer[0] = SRV_CONFIG_STORED_FLAG;
			s32 ret = Ql_SecureData_Store(SRV_CONFIG_BLOCK, Buffer, SRV_CONFIG_BLOCK_LEN);
			if(ret != QL_RET_OK)
			{
				APP_ERROR("srv Config store fail!! errcode = %d\n",ret);
				Buffer[0] = 0;
				Ql_SecureData_Store(SRV_CONFIG_BLOCK, Buffer, 1);
			}else{
				APP_DEBUG("svr Config store OK!!\n");
			}
        }
        return;
    }
    else if ( !Ql_memcmp(tok.p, "UPGRADE", 7) )
    {
		static u8* rMsg = "SET UPGRADE OK";
		u8  m_FileName[20];
		u8  m_SrvADDR[20];
		u32 m_SrvPort;
		if(tzer[0].count == 5)
		{
			tok = tokenizer_get(tzer, 2);
			n = tok.end - tok.p;
			Ql_memcpy(m_FileName, tok.p, n);
            m_FileName[n] = '\0';

            tok = tokenizer_get(tzer, 3);
			n = tok.end - tok.p;
			Ql_memcpy(m_SrvADDR, tok.p, n);
            m_SrvADDR[n] = '\0';

            //Convert IP format
        	u8 m_ipAddress[4];
        	Ql_memset(m_ipAddress,0,5);
        	s32 ret = Ql_IpHelper_ConvertIpAddr(m_SrvADDR, (u32 *)m_ipAddress);
        	if (SOC_SUCCESS != ret) // ip address is xxx.xxx.xxx.xxx
        	{
            	APP_ERROR("<-- Fail to convert IP Address --> \r\n");
        	}
            
			tok = tokenizer_get(tzer, 4);
			m_SrvPort = Ql_atoi(tok.p);
			
			u8 URL_Buffer[100];
            Ql_sprintf(URL_Buffer, "http://%d.%d.%d.%d:%d/%s",
            				m_ipAddress[0],m_ipAddress[1],m_ipAddress[2],m_ipAddress[3],
            				m_SrvPort,m_FileName);
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
				APP_ERROR("system will reboot after 3s!!");
				Ql_Sleep(3000);
				Ql_Reset(0);
            }
            
			APP_DEBUG("set upgrade,file:%s,server:%s,port:%d\n",
					   m_FileName,m_SrvADDR,m_SrvPort);
        	s32 iResult = RIL_SMS_SendSMS_Text(aPhNum, Ql_strlen(aPhNum),LIB_SMS_CHARSET_GSM,rMsg,Ql_strlen(rMsg),NULL);
        	if (iResult != RIL_AT_SUCCESS)
        	{
        		APP_ERROR("RIL_SMS_SendSMS_Text FAIL! iResult:%u\r\n",iResult);
        	}
        }
        return;
    }
    else if ( !Ql_memcmp(tok.p, "RESET", 5) )
    {
		static u8* rMsg = "SET RESET OK";
		APP_DEBUG("set RESET\n");
        s32 iResult = RIL_SMS_SendSMS_Text(aPhNum, Ql_strlen(aPhNum),LIB_SMS_CHARSET_GSM,rMsg,Ql_strlen(rMsg),NULL);
        if (iResult != RIL_AT_SUCCESS)
        {
        	APP_ERROR("RIL_SMS_SendSMS_Text FAIL! iResult:%u\r\n",iResult);
        }
        Ql_Sleep(5000);
        Ql_Reset(0);
        return;
    }
    else if ( !Ql_memcmp(tok.p, "FACTORY", 7) )
    {
		static u8* rMsg = "SET FACTORY OK";
		APP_DEBUG("set FACTORY\n");
        s32 iResult = RIL_SMS_SendSMS_Text(aPhNum, Ql_strlen(aPhNum),LIB_SMS_CHARSET_GSM,rMsg,Ql_strlen(rMsg),NULL);
        if (iResult != RIL_AT_SUCCESS)
        {
        	APP_ERROR("RIL_SMS_SendSMS_Text FAIL! iResult:%u\r\n",iResult);
        }
        Parameter_Buffer[0] = 0;
		Ql_SecureData_Store(PAR_BLOCK, Parameter_Buffer, 1);

		Ql_SecureData_Store(SYS_CONFIG_BLOCK, Parameter_Buffer, 1);

		Ql_SecureData_Store(SRV_CONFIG_BLOCK, Parameter_Buffer, 1);

        Ql_Sleep(5000);
        Ql_Reset(0);
        return;
    }
    else if ( !Ql_memcmp(tok.p, "VERSION", 7) )
    {
		u8 rMsg[50];
        Ql_sprintf(rMsg, "VERSION:%s\r\nBUILD:%s",VERSION,BUILD);
		APP_DEBUG("get VERSION\n");
        s32 iResult = RIL_SMS_SendSMS_Text(aPhNum, Ql_strlen(aPhNum),LIB_SMS_CHARSET_GSM,rMsg,Ql_strlen(rMsg),NULL);
        if (iResult != RIL_AT_SUCCESS)
        {
        	APP_ERROR("RIL_SMS_SendSMS_Text FAIL! iResult:%u\r\n",iResult);
        }
        return;
    }
    else if ( !Ql_memcmp(tok.p, "PARAM", 5) )
    {
		u8 rMsg[150];
		Ql_sprintf(rMsg,"IMEI:%02x%02x%02x%02x%02x%02x%02x%02x\r\nAPN:%s\r\nIP:%d.%d.%d.%d:%d\r\nHBT:%d\r\nRM:%d\r\nSR:%d\r\nTIMER:%d\r\nDIST:%d\r\nDIR:%d\r\n",
						 g_imei[0],g_imei[1],g_imei[2],g_imei[3],
						 g_imei[4],g_imei[5],g_imei[6],g_imei[7],
						 mSys_config.apnName,
						 ipAddress[0],ipAddress[1],
						 ipAddress[2],ipAddress[3],
						 SrvPort,
						 gParmeter.parameter_8[HEARTBEAT_INTERVAL_INDEX].data,
						 mSys_config.gLocation_Policy.location_policy,
						 mSys_config.gLocation_Policy.static_policy,
						 mSys_config.gLocation_Policy.time_Interval,
						 mSys_config.gLocation_Policy.distance_Interval,
						 mSys_config.gLocation_Policy.bearing_Interval);
		APP_DEBUG("get PARAM\n");
        s32 iResult = RIL_SMS_SendSMS_Text(aPhNum, Ql_strlen(aPhNum),LIB_SMS_CHARSET_GSM,rMsg,Ql_strlen(rMsg),NULL);
        if (iResult != RIL_AT_SUCCESS)
        {
        	APP_ERROR("RIL_SMS_SendSMS_Text FAIL! iResult:%u\r\n",iResult);
        }
        return;
    }
    else if ( !Ql_memcmp(tok.p, "PARAMCONFIG", 11) )
    {
		static u8* rMsg = "SET PARAMCONFIG OK";
		APP_DEBUG("set PARAM\n");
        s32 iResult = RIL_SMS_SendSMS_Text(aPhNum, Ql_strlen(aPhNum),LIB_SMS_CHARSET_GSM,rMsg,Ql_strlen(rMsg),NULL);
        if (iResult != RIL_AT_SUCCESS)
        {
        	APP_ERROR("RIL_SMS_SendSMS_Text FAIL! iResult:%u\r\n",iResult);
        }
        
        for(u8 i = 2; i < tzer[0].count; i++)
        {
			tok = tokenizer_get(tzer, 2);
			if(!Ql_memcmp(tok.p, "RM:", 3))
			{
				mSys_config.gLocation_Policy.location_policy = Ql_atoi(tok.end);
				APP_DEBUG("paramconfig RM=%d\n",mSys_config.gLocation_Policy.location_policy);
			}
			else if(!Ql_memcmp(tok.p, "SR:", 3))
			{
				mSys_config.gLocation_Policy.static_policy = Ql_atoi(tok.end);
				APP_DEBUG("paramconfig SR=%d\n",mSys_config.gLocation_Policy.static_policy);
			}
			else if(!Ql_memcmp(tok.p, "TIMER:", 6))
			{
				mSys_config.gLocation_Policy.time_Interval = Ql_atoi(tok.end);
				APP_DEBUG("paramconfig TIMER=%d\n",mSys_config.gLocation_Policy.time_Interval);
			}
			else if(!Ql_memcmp(tok.p, "DIST:", 5))
			{
				mSys_config.gLocation_Policy.distance_Interval = Ql_atoi(tok.end);
				APP_DEBUG("paramconfig DIST=%d\n",mSys_config.gLocation_Policy.distance_Interval);
			}
			else if(!Ql_memcmp(tok.p, "DIR:", 4))
			{
				mSys_config.gLocation_Policy.bearing_Interval = Ql_atoi(tok.end);
				APP_DEBUG("paramconfig DIR=%d\n",mSys_config.gLocation_Policy.bearing_Interval);
			}
        }
        
        return;
    }
    else if ( !Ql_memcmp(tok.p, "WHERE", 5) )
    {
		APP_DEBUG("get WHERE\n");
		u8 rMsg[150];
		ST_Time datetime;
		Ql_GetLocalTime(&datetime);
		Ql_sprintf(rMsg,"Lat:N%f\r\nLon:E%f\r\nCourse:%d\r\nSpeed:%d\r\nLac:%d\r\nCellID:%d\r\nDateTime:%d-%02d-%02d %02d:%02d:%02d\r\n",
						 ((double)gGpsLocation.latitude)/1000000,
						 ((double)gGpsLocation.longitude)/1000000,
						 gGpsLocation.bearing,
						 gGpsLocation.speed,
						 TOSMALLENDIAN16_32(glac_ci.lac),
						 TOSMALLENDIAN16_32(glac_ci.cell_id),
						 datetime.year,datetime.month,datetime.day,
						 datetime.hour,datetime.minute,datetime.second);
		
        s32 iResult = RIL_SMS_SendSMS_Text(aPhNum, Ql_strlen(aPhNum),LIB_SMS_CHARSET_GSM,rMsg,Ql_strlen(rMsg),NULL);
        if (iResult != RIL_AT_SUCCESS)
        {
        	APP_ERROR("RIL_SMS_SendSMS_Text FAIL! iResult:%u\r\n",iResult);
        }
        return;
    }
    else if ( !Ql_memcmp(tok.p, "123", 3) )
    {
		APP_DEBUG("get 123\n");
		u8 rMsg[150];
		ST_Time datetime;
		Ql_GetLocalTime(&datetime);
		Ql_sprintf(rMsg,"http://iobox.baojia.com/sms/location?lat=%f&lon=%f&t=%d-%02d-%02d %02d:%02d:%02d&s=%d",
						 ((double)gGpsLocation.latitude)/1000000,
						 ((double)gGpsLocation.longitude)/1000000,
						 datetime.year,datetime.month,datetime.day,
						 datetime.hour,datetime.minute,datetime.second,
						 gGpsLocation.speed);
        s32 iResult = RIL_SMS_SendSMS_Text(aPhNum, Ql_strlen(aPhNum),LIB_SMS_CHARSET_GSM,rMsg,Ql_strlen(rMsg),NULL);
        if (iResult != RIL_AT_SUCCESS)
        {
        	APP_ERROR("RIL_SMS_SendSMS_Text FAIL! iResult:%u\r\n",iResult);
        }
        return;
    }
    else if ( !Ql_memcmp(tok.p, "STATUS", 6) )
    {
		u8 rMsg[100];
		Ql_sprintf(rMsg,"BATTERY:%d\%%\r\n\GPRS:%s\r\nGSM SIGNAL:%s\r\nGPS:%s\r\nGPS SIGNAL:%s\r\n",
						 battery,
						 "NORMAL",
						 "MIDDLE",
						 "FIXED",
						 "MIDDLE");
		APP_DEBUG("get STATUS\n");
        s32 iResult = RIL_SMS_SendSMS_Text(aPhNum, Ql_strlen(aPhNum),LIB_SMS_CHARSET_GSM,rMsg,Ql_strlen(rMsg),NULL);
        if (iResult != RIL_AT_SUCCESS)
        {
        	APP_ERROR("RIL_SMS_SendSMS_Text FAIL! iResult:%u\r\n",iResult);
        }
        return;
    }
    else if ( !Ql_memcmp(tok.p, "SLEEP", 5) )
    {
		static u8* rMsg = "SET SLEEP OK";
		APP_DEBUG("set SLEEP\n");

		s32 iResult = RIL_SMS_SendSMS_Text(aPhNum, Ql_strlen(aPhNum),LIB_SMS_CHARSET_GSM,rMsg,Ql_strlen(rMsg),NULL);
        if (iResult != RIL_AT_SUCCESS)
        {
        	APP_ERROR("RIL_SMS_SendSMS_Text FAIL! iResult:%u\r\n",iResult);
        }

        Ql_Sleep(30000);
		alarm_on_off = 2;
		keep_wake = FALSE;
		ST_Time datetime;
        Ql_GetLocalTime(&datetime);
		update_clk_alarm(&datetime);
        return;
    }
    else if ( !Ql_memcmp(tok.p, "WAKEUP", 6) )
    {
		static u8* rMsg = "SET WAKEUP OK";
		APP_DEBUG("set WAKEUP\n");
		keep_wake = TRUE;
		ST_Time datetime;
		Ql_GetLocalTime(&datetime);
		update_clk_alarm(&datetime);
        s32 iResult = RIL_SMS_SendSMS_Text(aPhNum, Ql_strlen(aPhNum),LIB_SMS_CHARSET_GSM,rMsg,Ql_strlen(rMsg),NULL);
        if (iResult != RIL_AT_SUCCESS)
        {
        	APP_ERROR("RIL_SMS_SendSMS_Text FAIL! iResult:%u\r\n",iResult);
        }
        return;
    }
    else if ( !Ql_memcmp(tok.p, "PWD", 3) )
    {
		static u8* rMsg = "SET PASSWORD OK";
		tok = tokenizer_get(tzer, 2);
		mSys_config.password[0] = *(tok.p);
		mSys_config.password[1] = *(tok.p + 1);
		mSys_config.password[2] = *(tok.p + 2);
		mSys_config.password[3] = *(tok.p + 3);
		APP_DEBUG("set PASSWORD:%c%c%c%c\n",mSys_config.password[0],mSys_config.password[1],mSys_config.password[2],mSys_config.password[3]);
        s32 iResult = RIL_SMS_SendSMS_Text(aPhNum, Ql_strlen(aPhNum),LIB_SMS_CHARSET_GSM,rMsg,Ql_strlen(rMsg),NULL);
        if (iResult != RIL_AT_SUCCESS)
        {
        	APP_ERROR("RIL_SMS_SendSMS_Text FAIL! iResult:%u\r\n",iResult);
        }
    }

    //save
	u8 *Sys_Config_Buffer = (u8 *)Ql_MEM_Alloc(SYS_CONFIG_BLOCK_LEN);
    if (Sys_Config_Buffer == NULL)
  	{
  	 	APP_ERROR("%s/%d:Ql_MEM_Alloc FAIL! size:%u\r\n", __func__, __LINE__,SYS_CONFIG_BLOCK_LEN);
     	return;
  	}
  	APP_DEBUG("sizeof(mSys_config) = %d\n", sizeof(mSys_config));
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
 
