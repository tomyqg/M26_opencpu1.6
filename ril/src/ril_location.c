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
 *   ril_location.c 
 *
 * Project:
 * --------
 *   OpenCPU
 *
 * Description:
 * ------------
 *   The module defines the information, and APIs related to RIL.
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
#include "ril_location.h"
#include "ril.h"
#include "ril_util.h"
#include "ql_common.h"
#include "ql_uart.h"
#include "ql_stdlib.h"
#include "ql_error.h"


#define RIL_LOC_DEBUG_ENABLE 0
#if RIL_LOC_DEBUG_ENABLE > 0
#define RIL_LOC_DEBUG_PORT  UART_PORT2
static char DBG_Buffer[100];
#define RIL_LOC_DEBUG(BUF,...)  QL_TRACE_LOG(RIL_LOC_DEBUG_PORT,BUF,100,__VA_ARGS__)
#else
#define RIL_LOC_DEBUG(BUF,...) 
#endif

static CB_LocInfo callback_loc = NULL;


static s32 ATResponse_Location_handler(char* line, u32 len, void* userdata)
{
    char* p1 = NULL;
    char* p2 = NULL;
    ST_LocInfo locInfo;
    
   //+QCELLLOC:xxx.xxx,xxx.xxx
    char* head = Ql_RIL_FindString(line, len, "+QCELLLOC:");
    if(head)
    {        
        head += 10;             
        p1 = Ql_strstr(head,",");
        if(p1)
        {
            *p1 = 0;
            p1++;
            locInfo.longitude= Ql_atof(head);
        }
         p2 = Ql_strstr(p1,"\r\n");
         if(p2)
         {
            *p2 = 0;
            locInfo.latitude= Ql_atof(p1);
         }         
        callback_loc(RIL_ATRSP_SUCCESS,&locInfo);
        return  RIL_ATRSP_SUCCESS;        
    }
    head = Ql_RIL_FindLine(line, len, "OK");
    if(head)
    {  
        return  RIL_ATRSP_CONTINUE;  
    }
    head = Ql_RIL_FindLine(line, len, "ERROR");
    if(head)
    {  
        return  RIL_ATRSP_FAILED;
    }
    head = Ql_RIL_FindString(line, len, "+CME ERROR:");//fail
    if(head)
    {
        return  RIL_ATRSP_FAILED;
    }
    head = Ql_RIL_FindString(line, len, "+CMS ERROR:");//fail
    if(head)
    {
        return  RIL_ATRSP_FAILED;
    }

    return RIL_ATRSP_CONTINUE; //continue wait
}


s32 RIL_GetLocation(CB_LocInfo cb_loc)
{
    s32 ret = RIL_AT_FAILED;
    char strAT[200];
    
    if (NULL != cb_loc)
    {
        callback_loc = cb_loc;
        Ql_memset(strAT, 0, sizeof(strAT));
        Ql_sprintf(strAT, "AT+QCELLLOC=1\n");                
        RIL_LOC_DEBUG(DBG_Buffer,"<-- Send AT:%s, ret = %d -->\r\n",strAT, ret);        
        ret = Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT),ATResponse_Location_handler,NULL,0);
        if (RIL_AT_SUCCESS != ret)
        {
            return ret;
        }
        return ret;
    }else{
        return QL_RET_ERR_INVALID_PARAMETER;
    }
}

s32 RIL_GetLocationByCell(ST_CellInfo* cell, CB_LocInfo cb_loc)
{
    s32 ret = RIL_AT_SUCCESS;
    char strAT[200];
    
    if(NULL != cb_loc )
    {        
        callback_loc = cb_loc;        
        Ql_memset(strAT, 0, sizeof(strAT));
        Ql_sprintf(strAT, "AT+QCELLLOC=3,%d,%d,%d,%d,%d,%d\n",cell->cellId,cell->lac,cell->mnc,cell->mcc,cell->rssi,cell->timeAd);
        ret = Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT),ATResponse_Location_handler,NULL,0);
        RIL_LOC_DEBUG(DBG_Buffer,"<-- Send AT:%s, ret = %d -->\r\n",strAT, ret);
        
        if (RIL_AT_SUCCESS != ret)
        {
            return ret;
        }
    }
    return ret;
}


