/*********************************************************************
 *
 * Filename:
 * ---------
 *   ril_sim.c 
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
#include "ril.h"
#include "ql_error.h"
#include "app_debug.h"

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

/*********************************************************************
 * @fn      ATResponse_IMEI_Handler
 *
 * @brief   ATResponse_IMEI_Handler 
 *
 * @param   
 *
 * @return  
 *********************************************************************/
static s32 ATResponse_IMEI_Handler(char* line, u32 len, void* param)
{
	char* p1 = NULL;
	char* p2 = NULL;
	char* strImei = (char*)param;
	p1 = Ql_RIL_FindString(line, len, "OK");
	if (p1)
	{
#if 0	
		for(u8 i = 0; i < len; i++)
			APP_DEBUG("%c",line[i]);
		APP_DEBUG("\n");
#endif		
		return RIL_ATRSP_SUCCESS;
	}
	p1 = Ql_RIL_FindLine(line, len, "+CME ERROR:");
	if (p1)
	{
		return RIL_ATRSP_FAILED;
	}
	p1 = Ql_RIL_FindString(line, len, "\r\n");
	if (p1)
	{
		p2 = Ql_strstr(p1 + 2, "\r\n");
		if (p2)
		{
			if( (p2 - p1 -2) == 15)
				Ql_memcpy(strImei, p1 + 2, p2 - p1 -2);
				
			return RIL_ATRSP_CONTINUE;
		}
		else
		{
			return RIL_ATRSP_FAILED;
		}
	}
	return RIL_ATRSP_CONTINUE;
}

/*********************************************************************
 * @fn      RIL_SIM_GetIMEI
 *
 * @brief   RIL_SIM_GetIMEI 
 *
 * @param   
 *
 * @return  
 *********************************************************************/
s32 RIL_SIM_GetIMEI(char* pIMEI, u32 pIMEILen)
{
	if (!pIMEI || pIMEILen < 15)
	{
		return QL_RET_ERR_INVALID_PARAMETER;
	}
	Ql_memset(pIMEI, 0x0, pIMEILen);
	return Ql_RIL_SendATCmd("AT+GSN", Ql_strlen("AT+GSN"),
	                         ATResponse_IMEI_Handler, pIMEI,0);
}

/*********************************************************************
 * @fn      ATResponse_IMSI_Handler
 *
 * @brief   ATResponse_IMSI_Handler 
 *
 * @param   
 *
 * @return  
 *********************************************************************/
static s32 ATResponse_IMSI_Handler(char* line, u32 len, void* param)
{
	char* p1 = NULL;
	char* p2 = NULL;
	char* strImsi = (char*)param;
	p1 = Ql_RIL_FindString(line, len, "OK");
	if (p1)
	{
		return RIL_ATRSP_SUCCESS;
	}
	p1 = Ql_RIL_FindLine(line, len, "+CME ERROR:");
	if (p1)
	{
		return RIL_ATRSP_FAILED;
	}
	p1 = Ql_RIL_FindString(line, len, "\r\n");
	if (p1)
	{
		p2 = Ql_strstr(p1 + 2, "\r\n");
		if (p2)
		{
			if( (p2 - p1 -2) == 15)
				Ql_memcpy(strImsi, p1 + 2, p2 - p1 -2);
				
			return RIL_ATRSP_CONTINUE;
		}
		else
		{
			return RIL_ATRSP_FAILED;
		}
	}
	return RIL_ATRSP_CONTINUE;
}

/*********************************************************************
 * @fn      RIL_SIM_GetIMSI
 *
 * @brief   RIL_SIM_GetIMSI 
 *
 * @param   
 *
 * @return  
 *********************************************************************/
s32 RIL_SIM_GetIMSI(char* pIMSI, u32 pIMSILen)
{
	if (!pIMSI || pIMSILen < 15)
	{
		return QL_RET_ERR_INVALID_PARAMETER;
	}
	Ql_memset(pIMSI, 0x0, pIMSILen);
	return Ql_RIL_SendATCmd("AT+CIMI", Ql_strlen("AT+CIMI"),
	                         ATResponse_IMSI_Handler, pIMSI,0);
}

static s32 ATResponse_LACCI_Handler(char* line, u32 len, void* param)
{
	char* p1 = NULL;
	char* p2 = NULL;
	char* strImsi = (char*)param;
	p1 = Ql_RIL_FindString(line, len, "OK");
	if (p1)
	{
		return RIL_ATRSP_SUCCESS;
	}
	p1 = Ql_RIL_FindLine(line, len, "+CME ERROR:");
	if (p1)
	{
		return RIL_ATRSP_FAILED;
	}
	p1 = Ql_RIL_FindString(line, len, "\r\n");
	if (p1)
	{
		p2 = Ql_strstr(p1 + 2, "\r\n");
		if (p2)
		{
			if( (p2 - p1 -2) == 25)
			{
				Ql_memcpy(strImsi, p1 + 2, p2 - p1 -2);
				//APP_DEBUG("%.*s\n",p2 - p1 -2,strImsi);
			}	
			return RIL_ATRSP_CONTINUE;
		}
		else
		{
			return RIL_ATRSP_FAILED;
		}
	}
	return RIL_ATRSP_CONTINUE;
}

s32 RIL_SIM_LACCI(char* p, u32 pLen)
{
	if (!p || pLen < 25)
	{
		return QL_RET_ERR_INVALID_PARAMETER;
	}
	Ql_memset(p, 0x0, pLen);

	//AT+CGREG=2
	Ql_RIL_SendATCmd("AT+CGREG=2", Ql_strlen("AT+CGREG=2"),NULL, NULL,0);
	Ql_Sleep(100);

	//AT+CGREG?
	Ql_RIL_SendATCmd("AT+CGREG?", Ql_strlen("AT+CGREG?"),ATResponse_LACCI_Handler, p,0);
	Ql_Sleep(100);

	//AT+CGREG=1
	Ql_RIL_SendATCmd("AT+CGREG=1", Ql_strlen("AT+CGREG=1"),NULL, NULL,0);
	
	return QL_RET_OK;
}

