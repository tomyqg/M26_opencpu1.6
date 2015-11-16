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
    APP_RET_ERR_NOT_FOUND  = -2,   //don't found
    APP_RET_ERR_UART_READ  = -3,   //uart read error
};

#endif // End-of __APP_ERROR_H__ 


