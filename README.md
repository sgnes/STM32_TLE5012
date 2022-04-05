# STM32_TLE5012

A library for TLE5012 based on https://github.com/SummerFalls/STM32F103RCTx-TLE5012B-Magnetic-Angle-Sensor;
and make it more independent of hardware.

# Hardware used for testing

1. https://item.taobao.com/item.htm?spm=a1z09.2.0.0.44fc2e8djzArrm&id=642729169233&_u=5die6ba45f7
2. https://detail.tmall.com/item.htm?spm=a230r.1.14.52.2b7a684cluk2DX&id=609531534888&ns=1&abbucket=17

# How to use it

1. clone from git;
2. modify the TLE5012_SPI and TLE5012_MOSI_GPIO_ALTERNATE in STM32_TLE5012_Config.h
3. Compile

# Demo Code

```cpp
void TLE5012TaskEntry(void *argument) 
 {
  /* USER CODE BEGIN TLE5012TaskEntry */
  errorTypes checkError = NO_ERROR;
  float      d          = 0.0;
  int16_t    numRev     = 0;
  SPI_CS_DISABLE;
  // To make sure that all the _registers are storing values that are not corrupted,
  // the function calculates the CRC based on the initial values of the _registers and stores the values for future use
  checkError = readBlockCRC();
  printf("Init done!!! ERROR CODE: 0x%02X\r\n", checkError);
  checkError = NO_ERROR;

  /* Infinite loop */
  for(;;)
  {
      if (checkError == NO_ERROR)
      {
          checkError = getAngleSpeed(&AngleSpeed);
          printf("getAngleSpeed: %.04f\r\n", AngleSpeed);

          checkError = getAngleValue(&angle);
          printf("getAngleValue: %.04f\r\n", angle);

          checkError = getNumRevolutions(&NumRevolutions);
          printf("getNumRevolutions: %d\r\n", NumRevolutions);

          checkError = getUpdAngleSpeed(&UpdAngleSpeed);
          printf("getUpdAngleSpeed: %.04f\r\n", UpdAngleSpeed);

          checkError = getUpdAngleValue(&UpdAngleValue);
          printf("getUpdAngleValue: %.04f\r\n", UpdAngleValue);

          checkError = getUpdNumRevolutions(&UpdNumRevolutions);
          printf("getUpdNumRevolutions: %d\r\n", UpdNumRevolutions);

          checkError = getTemperature(&temperature);
          printf("getTemperature: %.04f\r\n", temperature);

          checkError = getAngleRange(&anglerange);
          printf("getAngleRange: %.04f\r\n", anglerange);

          printf("\r\n\r\n\r\n");
      }
      else
      {
          printf("**************************************ERROR CODE: 0x%02X\r\n", checkError);
          checkError = NO_ERROR;
      }
      osDelay(1000);
  }
  /* USER CODE END TLE5012TaskEntry */
}
```
