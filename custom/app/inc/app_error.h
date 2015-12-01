/*****************************************************************************
 *
 * Filename:
 * ---------
 *   app_error.h 
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


#ifndef __APP_ERROR_H__
#define __APP_ERROR_H__

/****************************************************************************
 * Error Code Definition
 ***************************************************************************/
enum {
    APP_RET_OK         = 0,
    APP_RET_ERR_PARAM  = -1,       //error parameter
    APP_RET_ERR_MEM_OLLOC = -2,         //Ql_MEM_Alloc
    APP_RET_ERR_NOT_FOUND = -3,         //don't found
    APP_RET_ERR_UART_READ = -4,         //uart read error
    APP_RET_ERR_UART_WRITE = -5,        //uart write error
    APP_RET_ERR_SOC_SEND = -6,          //socket send error
    APP_RET_ERR_SEV_NOT_REGISTER = -7,
};

#endif // End-of __APP_ERROR_H__ 

