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
    APP_RET_ERR_MEM_OLLOC,         //Ql_MEM_Alloc
    APP_RET_ERR_NOT_FOUND,         //don't found
    APP_RET_ERR_UART_READ,         //uart read error
    APP_RET_ERR_UART_WRITE,        //uart write error
};

#endif // End-of __APP_ERROR_H__ 


