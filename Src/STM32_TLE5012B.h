#ifndef _STM32_TLE5012_
#define _STM32_TLE5012_

#include <stdint.h>

#include "STM32_TLE5012_Config.h"

#define SPI_CS_ENABLE  HAL_GPIO_WritePin(TLE5012_CS_GPIO_Port, TLE5012_CS_Pin, GPIO_PIN_RESET)
#define SPI_CS_DISABLE HAL_GPIO_WritePin(TLE5012_CS_GPIO_Port, TLE5012_CS_Pin, GPIO_PIN_SET)

// Error masks for safety words
#define SYSTEM_ERROR_MASK           0x4000
#define INTERFACE_ERROR_MASK        0x2000
#define INV_ANGLE_ERROR_MASK        0x1000

// Commands for read
#define READ_STA_CMD_NOSAFETY       0x8000
#define READ_STA_CMD                0x8001
#define READ_ACTIV_STA_CMD          0x8011
#define READ_ANGLE_VAL_CMD          0x8021
#define READ_ANGLE_SPD_CMD          0x8031
#define READ_ANGLE_REV_CMD          0x8041
#define READ_TEMP_CMD               0x8051
#define READ_INTMODE_1              0x8061
#define READ_SIL                    0x8071
#define READ_INTMODE_2              0x8081
#define READ_INTMODE_3              0x8091
#define READ_OFFSET_X               0x80A1
#define READ_OFFSET_Y               0x80B1
#define READ_SYNCH                  0x80C1
#define READ_IFAB                   0x80D1
#define READ_INTMODE_4              0x80E1
#define READ_TEMP_COEFF             0x80F1
#define READ_RAW_X_CMD              0x8101
#define READ_RAW_Y_CMD              0x8111

// Commands for updated read
#define READ_UPD_STA_CMD            0x8401
#define READ_UPD_ANGLE_VAL_CMD      0x8421
#define READ_UPD_ANGLE_SPD_CMD      0x8431
#define READ_UPD_ANGLE_REV_CMD      0x8441

#define READ_BLOCK_CRC              0x8088

// Commands for write
#define WRITE_ACTIV_STA             0x0011
#define WRITE_INTMODE_1             0x5061
#define WRITE_SIL                   0x5071
#define WRITE_INTMODE_2             0x5081
#define WRITE_INTMODE_3             0x5091
#define WRITE_OFFSET_X              0x50A1
#define WRITE_OFFSET_Y              0x50B1
#define WRITE_SYNCH                 0x50C1
#define WRITE_IFAB                  0x50D1
#define WRITE_INTMODE_4             0x50E1
#define WRITE_TEMP_COEFF            0x50F1

// mask to check if the command want the value in the register or the value in the update buffer
#define CHECK_CMD_UPDATE            0x0400

// values used for calculating the CRC
#define CRC_POLYNOMIAL              0x1D
#define CRC_SEED                    0xFF
#define CRC_NUM_REGISTERS           8

// Values used to calculate 15 bit signed int sent by the sensor
#define DELETE_BIT_15               0x7FFF
#define CHANGE_UINT_TO_INT_15       32768
#define CHECK_BIT_14                0x4000

// values used to calculate 9 bit signed int sent by the sensor
#define DELETE_7BITS                0x01FF
#define CHANGE_UNIT_TO_INT_9        512
#define CHECK_BIT_9                 0x0100

// values used to for final calculations of angle speed, revolutions, range and value
#define POW_2_15                    32768.0
#define POW_2_7                     128.0
#define ANGLE_360_VAL               360.0

// values used to calculate the temperature
#define TEMP_OFFSET                 152.0
#define TEMP_DIV                    2.776

#define GET_BIT_14_4                0x7FF0

// default speed of SPI transfer
#define SPEED                   500000

// delay for the update
#define DELAYuS                 1

// dummy variable used for receive. Each time this is sent, it is for the purposes of receiving using SPI transfer.
#define DUMMY                   0xFFFF

#ifndef TLE5012_NOT_MODIFY_MOSI_MANUALLY

/* */
#define TLE5012_SET_MOSI_MODE_AF_PP()                            \
    do                                                           \
    {                                                            \
        GPIO_InitStruct.Pin       = TLE5012_MOSI_Pin;            \
        GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;             \
        GPIO_InitStruct.Pull      = GPIO_NOPULL;                 \
        GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_HIGH;        \
        GPIO_InitStruct.Alternate = TLE5012_MOSI_GPIO_ALTERNATE; \
        HAL_GPIO_Init(TLE5012_MOSI_GPIO_Port, &GPIO_InitStruct); \
    } while (0)

#define TLE5012_SET_MOSI_MODE_INPUT()                            \
    do                                                           \
    {                                                            \
        GPIO_InitStruct.Pin   = TLE5012_MOSI_Pin;                \
        GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;                 \
        GPIO_InitStruct.Pull  = GPIO_NOPULL;                     \
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;            \
        HAL_GPIO_Init(TLE5012_MOSI_GPIO_Port, &GPIO_InitStruct); \
    } while (0)

#endif
/**
 * This is used for keeping track of which register need to have its value changed, so that you don't need to read all the _registers each time the CRC needs to be updated
 */
//typedef enum registerIndex
//{
//  INT_MODE2_INDEX = 0x00,
//  INT_MODE3_INDEX = 0x01,
//  OFFSET_X_INDEX = 0x02,
//  OFFSET_Y_INDEX = 0x03,
//  SYNCH_INDEX = 0x04,
//  IFAB_INDEX = 0x05,
//  INT_MODE4_INDEX = 0x06,
//  TEMP_COEFF_INDEX = 0x07,
//  NO_INDEX = 0x08,
//}registerIndex;

/**
 * Error types from safety word
 */
typedef enum errorTypes {
    NO_ERROR = 0x00,
    SYSTEM_ERROR = 0x01,
    INTERFACE_ACCESS_ERROR = 0x02,
    INVALID_ANGLE_ERROR = 0x03,
    CRC_ERROR = 0xFF
} errorTypes;

errorTypes readBlockCRC(void);

//returns the angle speed
errorTypes getAngleSpeed(float32 *angleSpeed);
//returns the angleValue
errorTypes getAngleValue(float32 *angleValue);
//returns the number of revolutions done
errorTypes getNumRevolutions(int16_t *numRev);
//returns the updated angle speed
errorTypes getUpdAngleSpeed(float32 *angleSpeed);
//returns the updated angle value
errorTypes getUpdAngleValue(float32 *angleValue);
//returns the updated number of revolutions
errorTypes getUpdNumRevolutions(int16_t *numRev);
//return the temperature
errorTypes getTemperature(float32 *temp);
//returns the Angle Range
errorTypes getAngleRange(float32 *angleRange);
//triggers an update in the register
void triggerUpdate(void);


#endif
