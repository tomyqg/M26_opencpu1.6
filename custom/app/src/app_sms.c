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
		
        s32 iResult = RIL_SMS_SendSMS_Text(aPhNum, Ql_strlen(aPhNum),LIB_SMS_CHARSET_GSM,rMsg,Ql_strlen(rMsg),NULL);
        if (iResult != RIL_AT_SUCCESS)
        {
        	APP_ERROR("RIL_SMS_SendSMS_Text FAIL! iResult:%u\r\n",iResult);
        }
    } 
    else if ( !Ql_memcmp(tok.p, "SERVER", 6) )
    {
		static u8* rMsg = "SET SERVER OK";
		u8  m_SrvADDR[20];
		u32 m_SrvPort;
		if(tzer[0].count == 5)
		{
			tok = tokenizer_get(tzer, 2);
			if(*(tok.p) == '0')
        	{
				APP_DEBUG("set main server!");
        	}
        	else if(*(tok.p) == '1')
        	{
				APP_DEBUG("set slave server!");
        	}
			tok = tokenizer_get(tzer, 3);
			Ql_memcpy(m_SrvADDR, tok.p, tok.end - tok.p);
			tok = tokenizer_get(tzer, 4);
			m_SrvPort = Ql_atoi(tok.p);
			APP_DEBUG("set server:%s,port:%d\n",m_SrvADDR,m_SrvPort);
        	s32 iResult = RIL_SMS_SendSMS_Text(aPhNum, Ql_strlen(aPhNum),LIB_SMS_CHARSET_GSM,rMsg,Ql_strlen(rMsg),NULL);
        	if (iResult != RIL_AT_SUCCESS)
        	{
        		APP_ERROR("RIL_SMS_SendSMS_Text FAIL! iResult:%u\r\n",iResult);
        	}
        }	
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
            
			tok = tokenizer_get(tzer, 4);
			m_SrvPort = Ql_atoi(tok.p);
			APP_DEBUG("set upgrade,file:%s,server:%s,port:%d\n",m_FileName,m_SrvADDR,m_SrvPort);
        	s32 iResult = RIL_SMS_SendSMS_Text(aPhNum, Ql_strlen(aPhNum),LIB_SMS_CHARSET_GSM,rMsg,Ql_strlen(rMsg),NULL);
        	if (iResult != RIL_AT_SUCCESS)
        	{
        		APP_ERROR("RIL_SMS_SendSMS_Text FAIL! iResult:%u\r\n",iResult);
        	}
        }	
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
    }
    else if ( !Ql_memcmp(tok.p, "VERSION", 7) )
    {
		static u8* rMsg = "SET VERSION OK";
		APP_DEBUG("get VERSION\n");
        s32 iResult = RIL_SMS_SendSMS_Text(aPhNum, Ql_strlen(aPhNum),LIB_SMS_CHARSET_GSM,rMsg,Ql_strlen(rMsg),NULL);
        if (iResult != RIL_AT_SUCCESS)
        {
        	APP_ERROR("RIL_SMS_SendSMS_Text FAIL! iResult:%u\r\n",iResult);
        }	
    }
    else if ( !Ql_memcmp(tok.p, "PARAM", 5) )
    {
		static u8* rMsg = "GET PARAM OK";
		APP_DEBUG("get PARAM\n");
        s32 iResult = RIL_SMS_SendSMS_Text(aPhNum, Ql_strlen(aPhNum),LIB_SMS_CHARSET_GSM,rMsg,Ql_strlen(rMsg),NULL);
        if (iResult != RIL_AT_SUCCESS)
        {
        	APP_ERROR("RIL_SMS_SendSMS_Text FAIL! iResult:%u\r\n",iResult);
        }	
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
    }
    else if ( !Ql_memcmp(tok.p, "WHERE", 5) )
    {
		static u8* rMsg = "SET WHERE OK";
		APP_DEBUG("get WHERE\n");
        s32 iResult = RIL_SMS_SendSMS_Text(aPhNum, Ql_strlen(aPhNum),LIB_SMS_CHARSET_GSM,rMsg,Ql_strlen(rMsg),NULL);
        if (iResult != RIL_AT_SUCCESS)
        {
        	APP_ERROR("RIL_SMS_SendSMS_Text FAIL! iResult:%u\r\n",iResult);
        }	
    }
    else if ( !Ql_memcmp(tok.p, "123", 3) )
    {
		static u8* rMsg = "SET 123 OK";
		APP_DEBUG("get 123\n");
        s32 iResult = RIL_SMS_SendSMS_Text(aPhNum, Ql_strlen(aPhNum),LIB_SMS_CHARSET_GSM,rMsg,Ql_strlen(rMsg),NULL);
        if (iResult != RIL_AT_SUCCESS)
        {
        	APP_ERROR("RIL_SMS_SendSMS_Text FAIL! iResult:%u\r\n",iResult);
        }	
    }
    else if ( !Ql_memcmp(tok.p, "STATUS", 6) )
    {
		static u8* rMsg = "SET STATUS OK";
		APP_DEBUG("get STATUS\n");
        s32 iResult = RIL_SMS_SendSMS_Text(aPhNum, Ql_strlen(aPhNum),LIB_SMS_CHARSET_GSM,rMsg,Ql_strlen(rMsg),NULL);
        if (iResult != RIL_AT_SUCCESS)
        {
        	APP_ERROR("RIL_SMS_SendSMS_Text FAIL! iResult:%u\r\n",iResult);
        }	
    }
    else if ( !Ql_memcmp(tok.p, "SLEEP", 5) )
    {
		static u8* rMsg = "SET SLEEP OK";
		APP_DEBUG("get SLEEP\n");
        s32 iResult = RIL_SMS_SendSMS_Text(aPhNum, Ql_strlen(aPhNum),LIB_SMS_CHARSET_GSM,rMsg,Ql_strlen(rMsg),NULL);
        if (iResult != RIL_AT_SUCCESS)
        {
        	APP_ERROR("RIL_SMS_SendSMS_Text FAIL! iResult:%u\r\n",iResult);
        }	
    }
    else if ( !Ql_memcmp(tok.p, "WAKEUP", 6) )
    {
		static u8* rMsg = "SET WAKEUP OK";
		APP_DEBUG("get WAKEUP\n");
        s32 iResult = RIL_SMS_SendSMS_Text(aPhNum, Ql_strlen(aPhNum),LIB_SMS_CHARSET_GSM,rMsg,Ql_strlen(rMsg),NULL);
        if (iResult != RIL_AT_SUCCESS)
        {
        	APP_ERROR("RIL_SMS_SendSMS_Text FAIL! iResult:%u\r\n",iResult);
        }	
    }
    else if ( !Ql_memcmp(tok.p, "PWD", 3) )
    {
		static u8* rMsg = "SET PASSWORD OK";
		APP_DEBUG("set PASSWORD\n");
        s32 iResult = RIL_SMS_SendSMS_Text(aPhNum, Ql_strlen(aPhNum),LIB_SMS_CHARSET_GSM,rMsg,Ql_strlen(rMsg),NULL);
        if (iResult != RIL_AT_SUCCESS)
        {
        	APP_ERROR("RIL_SMS_SendSMS_Text FAIL! iResult:%u\r\n",iResult);
        }
    }
}
 
