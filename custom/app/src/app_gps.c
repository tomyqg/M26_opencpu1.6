/*********************************************************************
 *
 * Filename:
 * ---------
 *   app_gps.c 
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
#ifdef __CUSTOMER_CODE__
/*********************************************************************
 * INCLUDES
 */
#include <math.h>
#include "ql_stdlib.h"
#include "ql_memory.h"
#include "ql_time.h"
#include "ql_timer.h"
#include "ql_error.h"
#include "app_common.h"
#include "app_socket.h"
#include "app_server.h"
#include "app_gps.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */
#define GPS_SERIAL_RX_BUFFER_LEN  1024
#define PI                      3.1415926  
#define EARTH_RADIUS            6378137

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * VARIABLES
 */
static char RxBuf_GPS[GPS_SERIAL_RX_BUFFER_LEN];
GpsReader  mGpsReader[1];
bool gps_info_out = FALSE;

/*********************************************************************
 * FUNCTIONS
 */
static void CallBack_UART_GPS(Enum_SerialPort port, Enum_UARTEventType msg, bool level, void* customizedPara);
static void gps_reader_init( GpsReader* r );
//static s32 str2int( const s8* p, const s8* end );
static double str2float( const char* p, const char* end );

static void Timer_Handler(u32 timerId, void* param);

static float get_distance(float lat1, float lng1, float lat2, float lng2);
 
/**************************************************************
* gps sub task
***************************************************************/
void proc_subtask_gps(s32 TaskId)
{
    ST_MSG subtask_msg;
	s32 ret;
    
    APP_DEBUG("gps_subtask_entry,subtaskId = %d\n",TaskId);
    
    // Register & open UART port
    ret = Ql_UART_Register(GPS_UART_PORT, CallBack_UART_GPS, NULL);
    if (ret < QL_RET_OK)
    {
        APP_ERROR("Fail to register gps serial port,ret=%d\n",ret);
    }
    ret = Ql_UART_Open(GPS_UART_PORT, GPS_UART_baudrate, FC_NONE);
    if (ret < QL_RET_OK)
    {
        APP_ERROR("Fail to open gps serial port,ret=%d\r\n",ret);
    }

    //timer
    ret = Ql_Timer_Register(GPS_TIMER_ID, Timer_Handler, NULL);
    if(ret <0)
    {
        APP_ERROR("\r\nfailed!!, Timer gps register: timer(%d) fail ,ret = %d\r\n",HB_TIMER_ID,ret);
    }
	//start a timer,repeat=FALSE;
	if(gLocation_Policy.location_policy == TIMER_REPORT_LOCATION || gLocation_Policy.location_policy == DIS_TIM_REPORT_LOCATION)
	{
		ret = Ql_Timer_Start(GPS_TIMER_ID,gLocation_Policy.time_Interval*1000,FALSE);
		if(ret < 0)
		{
			APP_ERROR("\r\nfailed!!, Timer gps start fail ret=%d\r\n",ret);        
		}
	}

    //gps init
    gps_reader_init(mGpsReader);
    
    while(TRUE)
    {    
        Ql_OS_GetMessage(&subtask_msg);
        switch(subtask_msg.message)
        {
			case MSG_ID_RIL_READY:
			{
				APP_DEBUG("proc_subtask_gps revice MSG_ID_RIL_READY\n");
				break;
			}	
            case MSG_ID_GPS_MODE_CONTROL:
            {
                APP_DEBUG("recv msg: gps mode control\n");
                //if (subtask_msg.param1 == NW_STAT_REGISTERED)
				//{
					//GPRS_TCP_Program();
				//}
                break;
            }
            case MSG_ID_GPS_TIMER_REPORT:
            {
                APP_DEBUG("recv msg: start timer report location\n");
                ret = Ql_Timer_Start(GPS_TIMER_ID,gLocation_Policy.time_Interval*1000,FALSE);
				if(ret < 0)
				{
					APP_ERROR("\r\nfailed!!, Timer gps start fail ret=%d\r\n",ret);        
				}
                break;
            }

            default:
                break;
        }
    }    
}

static void Timer_Handler(u32 timerId, void* param)
{
    if(GPS_TIMER_ID == timerId)
    {
		if (mGpsReader[0].flag == TRUE)
		{
        	#if 0
			APP_DEBUG("latitude = %d,longitude = %d,altitude  = %d,speed  = %d,bearing = %d\n",
    			   	   r->fix.latitude,r->fix.longitude,r->fix.altitude,r->fix.speed,r->fix.bearing);
			#endif
			if(mGpsReader[0].callback)
			{
				mGpsReader[0].callback();
			}
			mGpsReader[0].flag = FALSE;
		}

		//start a timer,repeat=FALSE;
		if(gLocation_Policy.location_policy == TIMER_REPORT_LOCATION || 
		   gLocation_Policy.location_policy == DIS_TIM_REPORT_LOCATION)
		{
			u32 ret;
			ret = Ql_Timer_Start(GPS_TIMER_ID,gLocation_Policy.time_Interval*1000,FALSE);
			if(ret < 0)
			{
				APP_ERROR("\r\nfailed!!, Timer gps start fail ret=%d\r\n",ret);        
			}
		}
    }
}

#if 0
static s32 str2int( const s8* p, const s8* end )
{
    s32   result = 0;
    s32   len    = end - p;

    for ( ; len > 0; len--, p++ )
    {
        s32  c;
        if (p >= end)
            goto Fail;

        c = *p - '0';
        if ((unsigned)c >= 10)
            goto Fail;

        result = result*10 + c;
    }
    return  result;

Fail:
    return -1;
}
#endif

static double str2float( const char* p, const char* end )
{
    s32   len    = end - p;
    char  temp[16];

    if (len >= (s32)sizeof(temp))
        return 0.;

    Ql_memcpy( temp,p,len );
    temp[len] = '\0';
    return Ql_atof( temp );
}

static s32 gps_tokenizer_init( GpsTokenizer*  t, const char*  p, const char*  end )
{
    s32    count = 0;

    // the initial '$' is optional
    if (p < end && p[0] == '$')
        p += 1;
    // remove trailing newline
    if (end > p && end[-1] == '\n') {
        end -= 1;
        if (end > p && end[-1] == '\r')
            end -= 1;
    }
    // get rid of checksum at the end of the sentecne
    if (end >= p+3 && end[-3] == '*') {
        end -= 3;
    }

    while (p < end) {
        const char*  q = p;
        q = memchr(p, ',', end-p);
        if (q == NULL){
            q = end;
		}
        if (q >= p) {
            if (count < MAX_GPS_TOKENS) {
                t->tokens[count].p   = p;
                t->tokens[count].end = q;
                count += 1;
            }
        }
        if (q < end){
            q += 1;
		}
        p = q;
    }

    t->count = count;
    return count;
}

static Token gps_tokenizer_get( GpsTokenizer*  t, s32  index )
{
    Token  tok;
    static const char*  dummy = "";

    if (index < 0 || index >= t->count) {
        tok.p = tok.end = dummy;
    } else {
        tok = t->tokens[index];
    }
    return tok;
}

static double convert_from_hhmm( Token  tok )
{
    double  val     = str2float(tok.p, tok.end);
    s32  degrees = (s32)((val) / 100);
    double  minutes = val - degrees*100.;
    double  dcoord  = degrees + minutes / 60.0;
    return dcoord;
}

static s32 gps_reader_update_latlong( GpsReader* r, Token latitude, s8 latitudeHemi, Token longitude, s8 longitudeHemi )
{
    double   lat, lon;
    Token    tok;

    tok = latitude;
    if (tok.p + 6 > tok.end) {
        //APP_DEBUG("latitude is too short: '%.*s'\n", tok.end-tok.p, tok.p);
        return -1;
    }
    lat = convert_from_hhmm(tok);
    if (latitudeHemi == 'S')
        lat = -lat;

    tok = longitude;
    if (tok.p + 6 > tok.end) {
        //APP_DEBUG("longitude is too short: '%.*s'\n", tok.end-tok.p, tok.p);
        return -1;
    }
    lon = convert_from_hhmm(tok);
    if (longitudeHemi == 'W')
        lon = -lon;

    r->fix.latitude  = lat * 1000000;
    r->fix.longitude = lon * 1000000;
    return 0;
}

static s32 gps_reader_update_starInusing(GpsReader* r, Token starInusing)
{
    Token tok = starInusing;

    if (tok.p >= tok.end)
        return -1;

    r->fix.starInusing = str2float(tok.p, tok.end);
    return 0;
}

static s32 gps_reader_update_altitude( GpsReader*  r, Token altitude, Token units )
{
    Token tok = altitude;

    if (tok.p >= tok.end)
        return -1;

    r->fix.altitude = str2float(tok.p, tok.end);
    return 0;
}

static s32 gps_reader_update_speed( GpsReader* r, Token speed )
{
    Token   tok = speed;

    if (tok.p >= tok.end)
        return -1;

    r->fix.speed = str2float(tok.p, tok.end) * 1.852;
    return 0;
}

static s32 gps_reader_update_bearing( GpsReader* r,Token bearing )
{
    Token   tok = bearing;

    if (tok.p >= tok.end)
        return -1;

    r->fix.bearing = str2float(tok.p, tok.end);
    return 0;
}

// Calculate radian
static float radian(float d)  
{  
     return d * PI / 180.0;
}  

static float get_distance(float lat1, float lng1, float lat2, float lng2)  
{  
     float radLat1 = radian(lat1);  
     float radLat2 = radian(lat2);  
     float a = radLat1 - radLat2;  
     float b = radian(lng1) - radian(lng2);  
	 
     float dst = 2 * asin((sqrt(pow(sin(a / 2), 2) + cos(radLat1) * cos(radLat2) * pow(sin(b / 2), 2) )));  
     dst = dst * EARTH_RADIUS;
     dst= round(dst * 10000) / 10000;  
     return dst;
} 

static void gps_reader_parse( GpsReader* r )
{
   /* we received a complete sentence, now parse it to generate
    * a new GPS fix...
    */
    GpsTokenizer  tzer[1];
    Token         tok;

    //APP_DEBUG("Received: '%.*s'\n", r->pos, r->in);
	
    if (r->pos < 9) {
        APP_ERROR("Too short. discarded.\n");
        return;
    }

    gps_tokenizer_init(tzer, r->in, r->in + r->pos);
#if 0
    {
        s32  n;
        APP_DEBUG("Found %d tokens:\n", tzer->count);
        for (n = 0; n < tzer->count; n++) {
            Token  tok = gps_tokenizer_get(tzer,n);
            APP_DEBUG("%2d: '%.*s'\n", n, tok.end-tok.p, tok.p);
        }
    }
#endif

    tok = gps_tokenizer_get(tzer, 0);
    if (tok.p + 5 > tok.end) {
        APP_DEBUG("sentence id '%.*s' too long, ignored.\n", tok.end-tok.p, tok.p);
        return;
    }

    // ignore first two characters.
    tok.p += 2;
    if ( !Ql_memcmp(tok.p, "GGA", 3) ) {
    	//APP_DEBUG("GPS found GGA\n");
        // GPS fix
        Token  tok_latitude      = gps_tokenizer_get(tzer,2);
        Token  tok_latitudeHemi  = gps_tokenizer_get(tzer,3);
        Token  tok_longitude     = gps_tokenizer_get(tzer,4);
        Token  tok_longitudeHemi = gps_tokenizer_get(tzer,5);
        Token  tok_starInusing   = gps_tokenizer_get(tzer,7);
        Token  tok_altitude      = gps_tokenizer_get(tzer,9);
        Token  tok_altitudeUnits = gps_tokenizer_get(tzer,10);

        gps_reader_update_latlong(r,  tok_latitude,
                                      tok_latitudeHemi.p[0],
                                      tok_longitude,
                                      tok_longitudeHemi.p[0]);
        gps_reader_update_starInusing(r, tok_starInusing);
        gps_reader_update_altitude(r, tok_altitude, tok_altitudeUnits);
    }   
	else if ( !Ql_memcmp(tok.p, "RMC", 3) ) {
        Token tok_fixStatus = gps_tokenizer_get(tzer,2);  
        Token tok_speed = gps_tokenizer_get(tzer,7);  
        Token tok_bearing = gps_tokenizer_get(tzer,8);  
       
        if (tok_fixStatus.p[0] == 'A'){
        	r->flag = TRUE;
            gps_reader_update_bearing( r, tok_bearing );
            gps_reader_update_speed ( r, tok_speed );
            //r->fix.speed = 130;
            if(r->fix.speed > gParmeter.parameter_8[GPS_SPEED_UP_LIMITES_INDEX].data)
            {
            	//report to gprs task
				Ql_OS_SendMessage(subtask_gprs_id, MSG_ID_ALARM_REP, ALARM_BIT_SPEED_UP, TRUE);
			}
			else if(gAlarm_Flag.alarm_flags & BV(ALARM_BIT_SPEED_UP))
			{
				//clean
				Ql_OS_SendMessage(subtask_gprs_id, MSG_ID_ALARM_REP, ALARM_BIT_SPEED_UP, FALSE);
			}
		}
    }else {
        tok.p -= 2;
        //APP_DEBUG("unknown sentence '%.*s\n", tok.end-tok.p, tok.p);
    }

    if (r->flag == TRUE) {
        #if 0
		APP_DEBUG("latitude = %d,longitude = %d,altitude  = %d,speed  = %d,bearing = %d\n",
    			   r->fix.latitude,r->fix.longitude,r->fix.altitude,r->fix.speed,r->fix.bearing);
		#endif
		if(gLocation_Policy.location_policy == DISTANCE_REPORT_LOCATION || 
		   gLocation_Policy.location_policy == DIS_TIM_REPORT_LOCATION)
		{
            s32 mbearing = gGpsLocation.bearing - r->fix.bearing;
			if(ABS(mbearing) > gLocation_Policy.bearing_Interval)
			{
				r->callback();
				r->flag = FALSE;
				return;
			}

			float distance = get_distance(r->fix.latitude, r->fix.longitude, gGpsLocation.latitude, gGpsLocation.longitude);
			if(distance >= gLocation_Policy.distance_Interval && r->callback)
			{
				r->callback();
				r->flag = FALSE;
			}
		}
	}
}

void GpsLocation_CallBack(void)
{
	if(mGpsReader[0].fix.speed == 0 && gLocation_Policy.static_policy == 1)
		return;
		
	//report to gprs task
	Ql_OS_SendMessage(subtask_gprs_id, MSG_ID_GPS_REP_LOCATION, 0, 0);
}

static void gps_reader_init( GpsReader* r )
{
    Ql_memset( r, 0, sizeof(*r) );
    r->callback = GpsLocation_CallBack;
}

/*********************************************************************
 * @fn      ReadSerialPort
 *
 * @brief   ReadSerialPort 
 *
 * @param   
 *
 * @return  
 *********************************************************************/
static s32 ReadSerialPort(Enum_SerialPort port, u8* pBuffer, u32 bufLen)
{
    s32 rdLen = 0;
    s32 rdTotalLen = 0;
    if (NULL == pBuffer || 0 == bufLen)
    {
        return APP_RET_ERR_PARAM;
    }
    Ql_memset(pBuffer, 0x0, bufLen);
    while (1)
    {
        rdLen = Ql_UART_Read(port, pBuffer + rdTotalLen, bufLen - rdTotalLen);
        if (rdLen <= 0)  // All data is read out, or Serial Port Error!
        {
            break;
        }
        rdTotalLen += rdLen;
        // Continue to read...
    }
    if (rdLen < 0) // Serial Port Error!
    {
        APP_ERROR("Fail to read from port[%d]\r\n", port);
        return APP_RET_ERR_UART_READ;
    }
    return rdTotalLen;
}

/*********************************************************************
 * @fn      CallBack_UART_GPS
 *
 * @brief   CallBack_UART_GPS 
 *
 * @param   
 *
 * @return  
 *********************************************************************/
static void CallBack_UART_GPS(Enum_SerialPort port, Enum_UARTEventType msg, bool level, void* customizedPara)
{
    //APP_DEBUG("CallBack_UART_GPS: port=%d, event=%d, level=%d, p=%x\r\n", port, msg, level, customizedPara);
    switch (msg)
    {
    	case EVENT_UART_READY_TO_READ:
        {
            if (GPS_UART_PORT == port)
            {
                s32 totalBytes = ReadSerialPort(port, RxBuf_GPS, GPS_SERIAL_RX_BUFFER_LEN);
                if (totalBytes <= 0)
                {
                    APP_DEBUG("No data in UART buffer!\n");
                    return;
                }
                
				if(gps_info_out)
				{
					APP_DEBUG("%.*s",totalBytes,RxBuf_GPS);
				}
				
                char *p1 = Ql_strstr(RxBuf_GPS,"$GPGGA");
                if(!p1)
                {
					APP_DEBUG("can't find $GPGGA in received data\n");
					return;
                }
                char *p2 = Ql_strchr(p1+6,'\n');
                if(!p2)
                {
					APP_DEBUG("can't find $GPGGA's '\n' in received data\n");
					return;
                }
                //APP_DEBUG("%.*s",p2-p1,p1);
                mGpsReader[0].pos = p2-p1;
				Ql_memcpy(mGpsReader[0].in, p1, mGpsReader[0].pos);
				gps_reader_parse( mGpsReader );
                
                p1= Ql_strstr(p2,"$GPRMC");
                if(!p1)
                {
					APP_DEBUG("can't find $GPRMC in received data\n");
					return;
                }
                p2 = Ql_strchr(p1+6,'\n');
                if(!p2)
                {
					APP_DEBUG("can't find $GPRMC's '\n' in received data\n");
					return;
                }
                mGpsReader[0].pos = p2-p1;
				Ql_memcpy(mGpsReader[0].in, p1, mGpsReader[0].pos);
				gps_reader_parse( mGpsReader );
            }
            break;
        }
    	case EVENT_UART_READY_TO_WRITE:
        	break;
    	default:
        	break;
    }
}

#endif // __CUSTOMER_CODE__
