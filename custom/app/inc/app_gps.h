/*****************************************************************************
 *
 * Filename:
 * ---------
 *   app_gps.h 
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


#ifndef __APP_GPS_H__
#define __APP_GPS_H__

/*********************************************************************
 * INCLUDES
 */
#include "Ql_uart.h"
#include "Ql_time.h"
#include "Ql_timer.h"
#include "app_tokenizer.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */
#define GPS_UART_PORT           UART_PORT3
#define GPS_UART_baudrate       9600

#define GPS_TIMER_ID            (TIMER_ID_USER_START + 101)

/*********************************************************************
 * TYPEDEFS
 */
typedef struct {
    s32  latitude;
    s32  longitude;
    s32  altitude;
    s32  speed;
    s32  bearing;
    s32  starInusing;
} GpsLocation;
 
#define  GPS_MAX_SIZE  83
typedef void (*GpsLocation_cb)(void);
typedef struct {
    int     pos;
    GpsLocation  fix;
    GpsLocation_cb callback;
	char    in[ GPS_MAX_SIZE+1 ];
	bool flag;
} GpsReader; 
extern GpsReader mGpsReader[1];

#define  MAX_GPS_TOKENS  21
typedef struct {
    int     count;
    Token   tokens[ MAX_GPS_TOKENS ];
} GpsTokenizer;

/*********************************************************************
 * VARIABLES
 */
extern GpsLocation gValid_GpsLocation;

/*********************************************************************
 * FUNCTIONS
 */
 
#endif // End-of __APP_GPS_H__ 


