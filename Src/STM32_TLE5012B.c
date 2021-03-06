
#include "STM32_TLE5012B.h"
#include "gpio.h"
#include "main.h"
#include "spi.h"
#include "usart.h"

// keeps track of the values stored in the 8 _registers, for which the crc is calculated
uint16_t _registers[CRC_NUM_REGISTERS];

/**
 * Gets the first byte of a 2 byte word
 */
uint8_t _getFirstByte(uint16_t twoByteWord)
{
    return (uint8_t)(twoByteWord >> 8U);
}

/**
 * Gets the second byte of the 2 byte word
 */
uint8_t _getSecondByte(uint16_t twoByteWord)
{
    return (uint8_t)twoByteWord;
}

/**
 * Function for calculation the CRC.
 */
uint8_t _crc8(uint8_t *data, uint8_t length)
{
    uint32_t crc;
    int16_t i, bit;

    crc = CRC_SEED;

    for (i = 0; i < length; i++)
    {
        crc ^= data[i];

        for (bit = 0; bit < 8; bit++)
        {
            if ((crc & 0x80) != 0)
            {
                crc <<= 1;
                crc ^= CRC_POLYNOMIAL;
            }
            else
            {
                crc <<= 1;
            }
        }
    }

    //    return (~crc) & CRC_SEED;
    return (~crc);
}

/**
 * Function for calculation of the CRC
 */
uint8_t _crcCalc(uint8_t *crcData, uint8_t length)
{
    return _crc8(crcData, length);
}

/**
 * Triggers an update
 */
void triggerUpdate(void)
{
    // SCK LOW
    HAL_GPIO_WritePin(TLE5012_SCK_GPIO_Port, TLE5012_SCK_Pin, GPIO_PIN_RESET);
    // MOSI HIGH
    HAL_GPIO_WritePin(TLE5012_MOSI_GPIO_Port, TLE5012_MOSI_Pin, GPIO_PIN_SET);
    SPI_CS_ENABLE;
    HAL_Delay(DELAYuS);
    SPI_CS_DISABLE;
}

//when an error occurs in the safety word, the error bit remains 0(error), until the status register is read again.
//flushes out safety errors, that might have occured by reading the register without a safety word.
void resetSafety(void)
{
    uint16_t u16RegValue = 0;

    triggerUpdate();

    SPI_CS_ENABLE;

    u16RegValue = READ_STA_CMD;
    HAL_SPI_Transmit(TLE5012_SPI, (uint8_t *)(&u16RegValue), sizeof(u16RegValue) / sizeof(uint16_t), 0xFF);
    u16RegValue = DUMMY;
    HAL_SPI_Transmit(TLE5012_SPI, (uint8_t *)(&u16RegValue), sizeof(u16RegValue) / sizeof(uint16_t), 0xFF);
    HAL_SPI_Transmit(TLE5012_SPI, (uint8_t *)(&u16RegValue), sizeof(u16RegValue) / sizeof(uint16_t), 0xFF);

    SPI_CS_DISABLE;
}

/**
 * After every transaction with the Tle5012b_4wire, a safety word is returned to check the validity of the value received.
 * This is the structure of safety word, in which the numbers represent the bit position in 2 bytes.
 * 15 - indication of chip reset or watchdog overflow: 0 reset occured, 1 no reset
 * 14 - 0 System error, 1 No error
 * 13 - 0 Infterface access error, 1 No error
 * 12 - 0 Invalid angle value, 1 valid angle value
 * 11:8 - Sensor number response indicator, the sensor number bit is pulled low and other bits are high
 * 7:0 - CRC
 *
 * A CRC needs to be calculated using all the data sent and received (i.e. the command and the value return from the register, which is 4 bytes),
 * and needs to be checked with the CRC sent in the safety word.
 */

errorTypes checkSafety(uint16_t safety, uint16_t command, uint16_t *readreg, uint16_t length)
{
    errorTypes errorCheck;

    if (!((safety)&SYSTEM_ERROR_MASK))
    {
        errorCheck = SYSTEM_ERROR;
    }

    else if (!((safety)&INTERFACE_ERROR_MASK))
    {
        errorCheck = INTERFACE_ACCESS_ERROR;
    }

    else if (!((safety)&INV_ANGLE_ERROR_MASK))
    {
        errorCheck = INVALID_ANGLE_ERROR;
    }

    else
    {
        uint16_t         lengthOfTemp = length * 2 + 2;
        volatile uint8_t temp[lengthOfTemp];

        temp[0] = _getFirstByte(command);
        temp[1] = _getSecondByte(command);

        for (uint16_t i = 0; i < length; i++)
        {
            temp[2 + 2 * i] = _getFirstByte(readreg[i]);
            temp[2 + 2 * i + 1] = _getSecondByte(readreg[i]);
        }

        uint8_t crcReceivedFinal = _getSecondByte(safety);

        uint8_t crc = _crcCalc(temp, lengthOfTemp);

        if (crc == crcReceivedFinal)
        {
            errorCheck = NO_ERROR;
        }
        else
        {
            errorCheck = CRC_ERROR;
            resetSafety();
        }
    }

    return errorCheck;
}

/**
 * General read function for reading _registers from the Tle5012b_4wire.
 * Command[in]  -- the command for reading
 * data[out]    -- where the data received from the _registers will be stored
 *
 *
 * structure of command word, the numbers represent the bit position of the 2 byte command
 * 15 - 0 write, 1 read
 * 14:11 -  0000 for default operational access for addresses between 0x00 - 0x04, 1010 for configuration access for addresses between 0x05 - 0x11
 * 10 - 0 access to current value, 1 access to value in update buffer
 * 9:4 - access to 6 bit register address
 * 3:0 - 4 bit number of data words.
 */

errorTypes readFromSensor(uint16_t command, uint16_t *data)
{
    uint16_t safety = 0;
    uint16_t readreg;
    uint16_t         u16RegValue     = 0;
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };

    SPI_CS_ENABLE;

    u16RegValue = command;

#ifndef TLE5012_NOT_MODIFY_MOSI_MANUALLY
    TLE5012_SET_MOSI_MODE_AF_PP();
#endif

    HAL_SPI_Transmit(TLE5012_SPI, (uint8_t *)(&u16RegValue), sizeof(u16RegValue) / sizeof(uint16_t), 0xFF);

#ifndef TLE5012_NOT_MODIFY_MOSI_MANUALLY
    TLE5012_SET_MOSI_MODE_INPUT();
#endif

    HAL_SPI_Receive(TLE5012_SPI, (uint8_t *)(&readreg), 1, 0xFF);
    HAL_SPI_Receive(TLE5012_SPI, (uint8_t *)(&safety), 1, 0xFF);

    SPI_CS_DISABLE;

    errorTypes checkError = checkSafety(safety, command, &readreg, 1);

    if (checkError != NO_ERROR)
    {
        *data = 0;
        return checkError;
    }
    else
    {
        *data = readreg;
        return NO_ERROR;
    }
}

/**
 * Reads the block of _registers from addresses 08 - 0F in order to figure out the CRC.
 */
errorTypes readBlockCRC(void)
{
    uint16_t u16RegValue = 0;
    uint16_t         safety          = 0;
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };

    SPI_CS_ENABLE;

#ifndef TLE5012_NOT_MODIFY_MOSI_MANUALLY
    TLE5012_SET_MOSI_MODE_AF_PP();
#endif

    u16RegValue = READ_BLOCK_CRC;
    HAL_SPI_Transmit(TLE5012_SPI, (uint8_t *)(&u16RegValue), sizeof(u16RegValue) / sizeof(uint16_t), 0xFFFF);

#ifndef TLE5012_NOT_MODIFY_MOSI_MANUALLY
    TLE5012_SET_MOSI_MODE_INPUT();
#endif

    HAL_SPI_Receive(TLE5012_SPI, (uint8_t *)(&_registers), CRC_NUM_REGISTERS, 0xFF);
    HAL_SPI_Receive(TLE5012_SPI, (uint8_t *)(&safety), 1, 0xFF);

    errorTypes checkError = checkSafety(safety, READ_BLOCK_CRC, _registers, CRC_NUM_REGISTERS);

    return checkError;
}

errorTypes readAngleValue(int16_t *data)
{
    uint16_t rawData = 0;
    errorTypes status = readFromSensor(READ_ANGLE_VAL_CMD, &rawData);

    if (status != NO_ERROR)
    {
        return status;
    }

    rawData = (rawData & (DELETE_BIT_15));

    //check if the value received is positive or negative
    if (rawData & CHECK_BIT_14)
    {
        rawData = rawData - CHANGE_UINT_TO_INT_15;
    }

    *data = rawData;

    return NO_ERROR;
}

/**
 * The angle speed is a 15 bit signed integer. However, the register returns 16 bits, so we need to do some bit arithmetic.
 */
errorTypes readAngleSpeed(int16_t *data)
{
    uint16_t rawData = 0;
    errorTypes status = readFromSensor(READ_ANGLE_SPD_CMD, &rawData);

    if (status != NO_ERROR)
    {
        return status;
    }

    rawData = (rawData & (DELETE_BIT_15));

    //check if the value received is positive or negative
    if (rawData & CHECK_BIT_14)
    {
        rawData = rawData - CHANGE_UINT_TO_INT_15;
    }

    *data = rawData;

    return NO_ERROR;
}
errorTypes readUpdAngleValue(int16_t *data)
{
    uint16_t rawData = 0;
    errorTypes status = readFromSensor(READ_UPD_ANGLE_VAL_CMD, &rawData);

    if (status != NO_ERROR)
    {
        data = 0;
        return status;
    }

    rawData = (rawData & (DELETE_BIT_15));

    //check if the value received is positive or negative
    if (rawData & CHECK_BIT_14)
    {
        rawData = rawData - CHANGE_UINT_TO_INT_15;
    }

    *data = rawData;

    return NO_ERROR;
}
errorTypes readUpdAngleSpeed(int16_t *data)
{
    uint16_t rawData = 0;
    errorTypes status = readFromSensor(READ_UPD_ANGLE_SPD_CMD, &rawData);

    if (status != NO_ERROR)
    {
        *data = 0;
        return status;
    }

    rawData = (rawData & (DELETE_BIT_15));

    //check if the value received is positive or negative
    if (rawData & CHECK_BIT_14)
    {
        rawData = rawData - CHANGE_UINT_TO_INT_15;
    }

    *data = rawData;

    return NO_ERROR;
}

errorTypes readUpdAngleRevolution(int16_t *data)
{
    uint16_t rawData = 0;

    errorTypes status = readFromSensor(READ_UPD_ANGLE_REV_CMD, &rawData);

    if (status != NO_ERROR)
    {
        data = 0;
        return status;
    }

    rawData = (rawData & (DELETE_7BITS));

    //check if the value received is positive or negative
    if (rawData & CHECK_BIT_9)
    {
        rawData = rawData - CHANGE_UNIT_TO_INT_9;
    }

    *data = rawData;

    return NO_ERROR;
}

/**
 * The angle value is a 9 bit signed integer. However, the register returns 16 bits, so we need to do some bit arithmetic.
 */
errorTypes readAngleRevolution(int16_t *data)
{
    uint16_t rawData = 0;
    errorTypes status = readFromSensor(READ_ANGLE_REV_CMD, &rawData);

    if (status != NO_ERROR)
    {
        return status;
    }

    rawData = (rawData & (DELETE_7BITS));

    //check if the value received is positive or negative
    if (rawData & CHECK_BIT_9)
    {
        rawData = rawData - CHANGE_UNIT_TO_INT_9;
    }

    *data = rawData;

    return NO_ERROR;
}

/**
 * The angle value is a 9 bit signed integer. However, the register returns 16 bits, so we need to do some bit arithmetic.
 */
errorTypes readTemp(int16_t *data)
{
    uint16_t rawData = 0;
    errorTypes status = readFromSensor(READ_TEMP_CMD, &rawData);

    if (status != NO_ERROR)
    {
        *data = 0;
        return status;
    }

    rawData = (rawData & (DELETE_7BITS));

    //check if the value received is positive or negative
    if (rawData & CHECK_BIT_9)
    {
        rawData = rawData - CHANGE_UNIT_TO_INT_9;
    }

    *data = rawData;

    return NO_ERROR;
}

errorTypes readIntMode1(uint16_t *data)
{
    return readFromSensor(READ_INTMODE_1, data);
}

/**
 * The next eight functions are used primarily for storing the parameters and control of how the sensor works.
 * The values stored in them are used to calculate the CRC, and their values are stored in the private component of the class, _registers.
 */

errorTypes readIntMode2(uint16_t *data)
{
    return readFromSensor(READ_INTMODE_2, data);
}

/**
 * The formula to calculate the Angle Speed as per the data sheet.
 */

float32 _calculateAngleSpeed(float32 angRange, int16_t rawAngleSpeed, uint16_t firMD, uint16_t predictionVal)
{
    float32 finalAngleSpeed;
    float32 microsecToSec = 0.000001;
    float32 firMDVal;

    if (firMD == 1)
    {
        firMDVal = 42.7;
    }

    else if (firMD == 0)
    {
        firMDVal = 21.3;
    }

    else if (firMD == 2)
    {
        firMDVal = 85.3;
    }

    else if (firMD == 3)
    {
        firMDVal = 170.6;
    }

    else
    {
        firMDVal = 0;
    }

    finalAngleSpeed = ((angRange / POW_2_15) * ((float32)rawAngleSpeed)) / (((float32)predictionVal) * firMDVal * microsecToSec);

    return finalAngleSpeed;
}

/**
 * returns the angle speed
 */
errorTypes getAngleSpeed(float32 *finalAngleSpeed)
{
    int16_t  rawAngleSpeed      = 0;
    float32  angleRange         = 0.0;
    uint16_t firMDVal = 0;
    uint16_t intMode2Prediction = 0;

    errorTypes checkError = readAngleSpeed(&rawAngleSpeed);

    if (checkError != NO_ERROR)
    {
        return checkError;
    }

    checkError = getAngleRange(&angleRange);

    if (checkError != NO_ERROR)
    {
        return checkError;
    }

    //checks the value of fir_MD according to which the value in the calculation of the speed will be determined
    checkError = readIntMode1(&firMDVal);

    if (checkError != NO_ERROR)
    {
        return checkError;
    }

    firMDVal >>= 14;

    //according to if prediction is enabled then, the formula for speed changes
    checkError = readIntMode2(&intMode2Prediction);

    if (checkError != NO_ERROR)
    {
        return checkError;
    }

    if (intMode2Prediction & 0x0004)
    {
        intMode2Prediction = 3;
    }

    else
    {
        intMode2Prediction = 2;
    }

    *finalAngleSpeed = _calculateAngleSpeed(angleRange, rawAngleSpeed, firMDVal, intMode2Prediction);

    return NO_ERROR;
}
errorTypes getAngleValue(float32 *angleValue)
{
    int16_t rawAnglevalue = 0;
    errorTypes checkError = readAngleValue(&rawAnglevalue);

    if (checkError != NO_ERROR)
    {
        return checkError;
    }

    *angleValue = (ANGLE_360_VAL / POW_2_15) * ((float32)rawAnglevalue);

    return NO_ERROR;
}

errorTypes getNumRevolutions(int16_t *numRev)
{
    return readAngleRevolution(numRev);
}

// returns the updated angle speed
errorTypes getUpdAngleSpeed(float32 *angleSpeed)
{
    int16_t  rawAngleSpeed      = 0;
    float32  angleRange         = 0.0;
    uint16_t firMDVal = 0;
    uint16_t intMode2Prediction = 0;

    errorTypes checkError = readUpdAngleSpeed(&rawAngleSpeed);

    if (checkError != NO_ERROR)
    {
        return checkError;
    }

    checkError = getAngleRange(&angleRange);

    if (checkError != NO_ERROR)
    {
        return checkError;
    }

    checkError = readIntMode1(&firMDVal);

    if (checkError != NO_ERROR)
    {
        return checkError;
    }

    checkError = readIntMode2(&intMode2Prediction);

    if (checkError != NO_ERROR)
    {
        return checkError;
    }

    if (intMode2Prediction & 0x0004)
    {
        intMode2Prediction = 3;
    }

    else
    {
        intMode2Prediction = 2;
    }

    *angleSpeed = _calculateAngleSpeed(angleRange, rawAngleSpeed, firMDVal, intMode2Prediction);

    return NO_ERROR;
}

errorTypes getUpdAngleValue(float32 *angleValue)
{
    int16_t rawAnglevalue = 0;
    errorTypes checkError = readUpdAngleValue(&rawAnglevalue);

    if (checkError != NO_ERROR)
    {
        return checkError;
    }

    *angleValue = (ANGLE_360_VAL / POW_2_15) * ((float32)rawAnglevalue);

    return NO_ERROR;
}

errorTypes getUpdNumRevolutions(int16_t *numRev)
{
    return readUpdAngleRevolution(numRev);
}

errorTypes getTemperature(float32 *temperature)
{
    int16_t rawTemp = 0;
    errorTypes checkError = readTemp(&rawTemp);

    if (checkError != NO_ERROR)
    {
        return checkError;
    }

    *temperature = (rawTemp + TEMP_OFFSET) / (TEMP_DIV);

    return NO_ERROR;
}

errorTypes getAngleRange(float32 *angleRange)
{
    uint16_t rawData = 0;
    errorTypes checkError = readIntMode2(&rawData);

    if (checkError != NO_ERROR)
    {
        return checkError;
    }

    //Angle Range is stored in bytes 14 - 4, so you have to do this bit shifting to get the right value
    rawData &= GET_BIT_14_4;
    rawData >>= 4;

    *angleRange = ANGLE_360_VAL * (POW_2_7 / (float32)(rawData));

    return NO_ERROR;
}
