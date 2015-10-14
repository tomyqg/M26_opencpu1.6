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
 *   ril_atResponse.c 
 *
 * Project:
 * --------
 *   OpenCPU
 *
 * Description:
 * ------------
 *   The module handles the AT response in RIL.
 *   Developer may reprogram to cover some special AT commands according to the requirements.
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
#include "custom_feature_def.h"
#include "ril.h "
#include "ril_util.h"
#include "ql_stdlib.h"
#include "ql_trace.h"

#ifdef __OCPU_RIL_SUPPORT__

/******************************************************************************
* Function:     Default_atRsp_callback
*  
* Description:
*               If you set the callback parameter of Ql_RIL_SendATCmd to NULL,
*               we will provide you with a default callback function.But we need
*               to clarify that this callback function can only handle simple 
*               response of the AT command.The response of AT command only is 
*               OK or error.
*
* Parameters:    
*               line:  
*                   [in]The address of the string.
*               len:   
*                   [in]The length of the string.
*               userdata: 
*                   [out]Used to transfer the customer's parameter.
* Return:  
*               RIL_ATRSP_SUCCESS : AT command executed successfully.
*               RIL_ATRSP_FAILED : AT command executed failed.
******************************************************************************/
s32 Default_atRsp_callback(char* line, u32 len, void* userdata)
{
    if (Ql_RIL_FindLine(line, len, "OK"))// find <CR><LF>OK<CR><LF>,OK<CR><LF>, <CR>OK<CR>£¬<LF>OK<LF>
    {  
        return  RIL_ATRSP_SUCCESS;
    }
    else if (Ql_RIL_FindLine(line, len, "ERROR") // find <CR><LF>ERROR<CR><LF>, <CR>ERROR<CR>£¬<LF>ERROR<LF>
            || Ql_RIL_FindString(line, len, "+CME ERROR:")//fail
            || Ql_RIL_FindString(line, len, "+CMS ERROR:"))//fail
    {  
        return  RIL_ATRSP_FAILED;
    }
    return RIL_ATRSP_CONTINUE; //continue wait
}

#endif  //__OCPU_RIL_SUPPORT__
