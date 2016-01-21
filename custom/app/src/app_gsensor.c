/*********************************************************************
 *
 * Filename:
 * ---------
 *   app_gsensor.c 
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
#include "ql_stdlib.h"
#include "ql_error.h"
#include "ql_eint.h"
#include "app_common.h"
#include "app_gsensor.h"

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
Enum_PinName pinname = PINNAME_CTS; 

/*********************************************************************
 * FUNCTIONS
 */
static void callback_eint_handle(Enum_PinName eintPinName, Enum_PinLevel pinLevel, void* customParam)
{
    s32 ret;
    //mask the specified EINT pin.
    Ql_EINT_Mask(pinname);
    
    APP_DEBUG("<--Eint callback: pin(%d), levle(%d)-->\r\n",eintPinName,pinLevel);
    //ret = Ql_EINT_GetLevel(eintPinName);
    //APP_DEBUG("<--Get Level, pin(%d), levle(%d)-->\r\n",eintPinName,ret);
    
    //unmask the specified EINT pin
    Ql_EINT_Unmask(pinname);
}

static s32 gsensor_read_register(u8 registerAddr, u8 *buffer)
{
    s32 ret;
    ret = Ql_IIC_Write_Read(GSENSOR_I2C_CHANNEL_NO, GSENSOR_I2C_SLAVE_ADDR8BIT, &registerAddr, 1,buffer, 1);
    if(ret < 0)
    {
        APP_ERROR("\r\n<-- IIC controller Ql_IIC_Write_Read channel %d fail ret=%d-->\r\n",
                  GSENSOR_I2C_CHANNEL_NO,ret);
    }
	return ret;    
}

static s32 gsensor_write_register(u8 registerAddr, u8 buffer)
{
    s32 ret;
    u8 write_buffer[2];
    write_buffer[0] = registerAddr;
    write_buffer[1] = buffer;
    ret = Ql_IIC_Write(GSENSOR_I2C_CHANNEL_NO, GSENSOR_I2C_SLAVE_ADDR8BIT, write_buffer, 2);
    if(ret < 0)
    {
    	APP_ERROR("\r\n<-- IIC controller Ql_IIC_Write channel %d fail ret=%d-->\r\n",GSENSOR_I2C_CHANNEL_NO,ret);       
    }
    return ret;
}
 
void gsensor_init(void)
{
	s32 ret;
	
	ret = Ql_IIC_Init(GSENSOR_I2C_CHANNEL_NO,PINNAME_RI,PINNAME_DCD,TRUE);
    if(ret != QL_RET_OK)
    {
    	APP_ERROR("\r\n<-- IIC controller Ql_IIC_Init channel %d fail ret=%d-->\r\n",GSENSOR_I2C_CHANNEL_NO,ret);
    	return;
    }

    ret = Ql_IIC_Config(GSENSOR_I2C_CHANNEL_NO,TRUE, GSENSOR_I2C_SLAVE_ADDR8BIT, GSENSOR_I2C_SPEED);// just for the IIC controller
    if(ret != QL_RET_OK)
    {
    	APP_ERROR("\r\n<-- IIC controller Ql_IIC_Config channel %d fail ret=%d-->\r\n",GSENSOR_I2C_CHANNEL_NO,ret);
    	return;
    }

    ret = Ql_EINT_Register(pinname,callback_eint_handle, NULL);    
    if(ret != QL_RET_OK)
    {
        APP_ERROR("<-- Ql_EINT_Register fail.-->\r\n"); 
        return;
    }

    ret = Ql_EINT_Init(pinname, EINT_LEVEL_TRIGGERED, 0, 5,FALSE);
    if(ret != QL_RET_OK)
    {
        APP_ERROR("<--OpenCPU: Ql_EINT_Init fail.-->\r\n"); 
        return;    
    }
    APP_DEBUG("<--OpenCPU: Ql_EINT_Init OK.-->\r\n");  

    //INT_OUT_CTR, 0x02:int1 open drain and active low
    gsensor_write_register(BMA2x2_INT_SET_REG,0x02);

	//PMU_BW, 0x08:7.81HZ
    gsensor_write_register(BMA2x2_BW_SEL_REG,0x08);

    //INT_MAP_0, 0x04:int1_slope to int1
    gsensor_write_register(BMA2x2_INT1_PAD_SEL_REG,0x04);

	//INT_EN_0, 0x07: slope inttreput x\y\z enable
    gsensor_write_register(BMA2x2_INT_ENABLE1_REG,0x07);
    
}
#endif // __CUSTOMER_CODE__
