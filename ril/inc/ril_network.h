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
 *   ril_network.h 
 *
 * Project:
 * --------
 *   OpenCPU
 *
 * Description:
 * ------------
 *   The file declares some API functions, which are related to network, including
 *   SIM state, GSM/GPRS network state and signal strength, and so forth.
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
#ifndef __RIL_NETWORK_H__
#define __RIL_NETWORK_H__
#include "ql_type.h"


/****************************************************************************
 * Definition for SIM Card State
 ***************************************************************************/
typedef enum {
    SIM_STAT_NOT_INSERTED = 0,
    SIM_STAT_READY,
    SIM_STAT_PIN_REQ,
    SIM_STAT_PUK_REQ,
    SIM_STAT_PH_PIN_REQ,
    SIM_STAT_PH_PUK_REQ,
    SIM_STAT_PIN2_REQ,
    SIM_STAT_PUK2_REQ,
    SIM_STAT_BUSY,
    SIM_STAT_NOT_READY,
    SIM_STAT_UNSPECIFIED
 }Enum_SIMState;

typedef enum {
    NW_STAT_NOT_REGISTERED = 0,    // Not register to network
    NW_STAT_REGISTERED,            // The normal network state
    NW_STAT_SEARCHING,             // Searching network
    NW_STAT_REG_DENIED,            // The register request is denied
    NW_STAT_UNKNOWN,
    NW_STAT_REGISTERED_ROAMING     //Registered and Roaming state
}Enum_NetworkState;

typedef enum {
    IP_INITIAL=0,
    IP_START,
    IP_CONFIG,
    IP_IND,
    IP_GPRSACT,
    IP_STATUS,
    TCP_PROCESSING,
    UDP_PROCESSING=TCP_PROCESSING,
    IP_CLOSE,
    CONNECT_OK,
    GPRS_CONTEXT_DEACT,
    IP_STATUS_END
}Enum_ContextIPState;

typedef struct{
    int rssi;
    int ber;
}ST_CSQ_Reponse;


/******************************************************************************
* Function:     RIL_NW_GetSIMCardState
*  
* Description:
*                This function gets the state of SIM card. 
*
* Parameters:    
*               stat:
*                   [out]SIM card State.
* Return:  
*               SIM card state code, one value of Enum_SIMState.
*               -1 will be returned if fail to get the SIM card state.
******************************************************************************/
s32  RIL_NW_GetSIMCardState(s32 *stat);

/******************************************************************************
* Function:     RIL_NW_GetGSMState
*  
* Description:
*               This function gets the GSM network register state. 
*
* Parameters:    
*               stat:
*                   [out]GSM State.
* Return:  
*               One value of Enum_NetworkState: network register state code.
*               -1 : fail to get the network state.
******************************************************************************/
s32  RIL_NW_GetGSMState(s32 *stat);

/******************************************************************************
* Function:     RIL_NW_GetGPRSState
*  
* Description:
*               This function gets the GPRS network register state. 
*
* Parameters:    
*               stat:
*                   [out]GPRS State.
* Return:  
*               One value of Enum_NetworkState: network register state code.
*               -1 : fail to get the network state.
******************************************************************************/
s32  RIL_NW_GetGPRSState(s32 *stat);

/******************************************************************************
* Function:     RIL_NW_GetSignalQuality
*  
* Description:
*               This function gets the signal quality level and bit error rate. 
*
* Parameters:    
*               rssi:
*                   [out] Signal quality level, 0~31 or 99. 99 indicates module
*                         doesn't register to GSM network.
*                       
*               ber:
*                   [out] The bit error code of signal.
* Return:  
*               QL_RET_OK indicates success.
*               QL_RET_ERR_INVALID_PARAMETER indicates something wrong for input parameters. 
******************************************************************************/
s32  RIL_NW_GetSignalQuality(u32* rssi, u32* ber);

/******************************************************************************
* Function:     RIL_NW_SetGPRSContext
*  
* Description:
*               This function select a context as foreground context
*
* Parameters:    
*               foregroundContext:
*                   [IN] Anumeric indicates which context will be set as foreground context.The range is 0-1. 
* Return:  
*                RIL_AT_SUCCESS,send AT successfully.
*                RIL_AT_FAILED, send AT failed.
*                RIL_AT_TIMEOUT,send AT timeout.
*                RIL_AT_BUSY,   sending AT.
*                RIL_AT_INVALID_PARAM, invalid input parameter.
*                RIL_AT_UNINITIALIZED, RIL is not ready, need to wait for MSG_ID_RIL_READY
*                                      and then call Ql_RIL_Initialize to initialize RIL.
******************************************************************************/
s32  RIL_NW_SetGPRSContext(u8 foregroundContext);

s32  RIL_NW_SetAPN(u8 mode, char* apn, char* userName, char* pw);

s32  RIL_NW_GetIPStatus(void);

s32  RIL_NW_OpenPDPContext(void);

s32  RIL_NW_ClosePDPContext(void);

#endif // __RIL_NETWORK_H__
