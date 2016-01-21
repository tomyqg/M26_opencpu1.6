/*****************************************************************************
 *
 * Filename:
 * ---------
 *   app_gsensor.h 
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


#ifndef __APP_GSENSOR_H__
#define __APP_GSENSOR_H__

/*********************************************************************
 * INCLUDES
 */

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */
#define GSENSOR_I2C_CHANNEL_NO       0
#define GSENSOR_I2C_SLAVE_ADDR8BIT   0x30
#define GSENSOR_I2C_SPEED            300

#define BMA2x2_CHIP_ID_REG                      0x00
#define BMA2x2_X_AXIS_LSB_REG                   0x02
#define BMA2x2_X_AXIS_MSB_REG                   0x03
#define BMA2x2_Y_AXIS_LSB_REG                   0x04
#define BMA2x2_Y_AXIS_MSB_REG                   0x05
#define BMA2x2_Z_AXIS_LSB_REG                   0x06
#define BMA2x2_Z_AXIS_MSB_REG                   0x07
#define BMA2x2_STATUS1_REG                      0x09
#define BMA2x2_STATUS2_REG                      0x0A
#define BMA2x2_STATUS_TAP_SLOPE_REG             0x0B
#define BMA2x2_STATUS_ORIENT_HIGH_REG           0x0C
#define BMA2x2_STATUS_FIFO_REG                  0x0E       //ADDED
#define BMA2x2_RANGE_SEL_REG                    0x0F
#define BMA2x2_BW_SEL_REG                       0x10
#define BMA2x2_MODE_CTRL_REG                    0x11
#define BMA2x2_LOW_NOISE_CTRL_REG               0x12
#define BMA2x2_DATA_CTRL_REG                    0x13
#define BMA2x2_RESET_REG                        0x14
#define BMA2x2_INT_ENABLE1_REG                  0x16
#define BMA2x2_INT_ENABLE2_REG                  0x17
#define BMA2x2_INT_SLO_NO_MOT_REG               0x18      //ADDED
#define BMA2x2_INT1_PAD_SEL_REG                 0x19
#define BMA2x2_INT_DATA_SEL_REG                 0x1A
#define BMA2x2_INT2_PAD_SEL_REG                 0x1B
#define BMA2x2_INT_SRC_REG                      0x1E
#define BMA2x2_INT_SET_REG                      0x20
#define BMA2x2_INT_CTRL_REG                     0x21
#define BMA2x2_LOW_DURN_REG                     0x22
#define BMA2x2_LOW_THRES_REG                    0x23
#define BMA2x2_LOW_HIGH_HYST_REG                0x24
#define BMA2x2_HIGH_DURN_REG                    0x25
#define BMA2x2_HIGH_THRES_REG                   0x26
#define BMA2x2_SLOPE_DURN_REG                   0x27
#define BMA2x2_SLOPE_THRES_REG                  0x28
#define BMA2x2_SLO_NO_MOT_THRES_REG             0x29    //ADDED
#define BMA2x2_TAP_PARAM_REG                    0x2A
#define BMA2x2_TAP_THRES_REG                    0x2B
#define BMA2x2_ORIENT_PARAM_REG                 0x2C
#define BMA2x2_THETA_BLOCK_REG                  0x2D
#define BMA2x2_THETA_FLAT_REG                   0x2E
#define BMA2x2_FLAT_HOLD_TIME_REG               0x2F
#define BMA2x2_FIFO_WML_TRIG                    0x30   //ADDED
#define BMA2x2_SELF_TEST_REG                    0x32
#define BMA2x2_EEPROM_CTRL_REG                  0x33
#define BMA2x2_SERIAL_CTRL_REG                  0x34
#define BMA2x2_OFFSET_CTRL_REG                  0x36
#define BMA2x2_OFFSET_PARAMS_REG                0x37
#define BMA2x2_OFFSET_X_AXIS_REG                0x38
#define BMA2x2_OFFSET_Y_AXIS_REG                0x39
#define BMA2x2_OFFSET_Z_AXIS_REG                0x3A
#define BMA2x2_GP0_REG                          0x3B    //ADDED
#define BMA2x2_GP1_REG                          0x3C    //ADDED
#define BMA2x2_FIFO_MODE_REG                    0x3E    //ADDED
#define BMA2x2_FIFO_DATA_OUTPUT_REG             0x3F    //ADDED

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * VARIABLES
 */
 
/*********************************************************************
 * FUNCTIONS
 */
void gsensor_init(void); 

#endif // End-of __APP_GSENSOR_H__ 


