/*
 * STM32_TLE5012_Config.h
 *
 *  Created on: Mar 20, 2022
 *      Author: sgnes
 */

#ifndef INC_STM32_TLE5012_CONFIG_H_
#define INC_STM32_TLE5012_CONFIG_H_

typedef float float32;

#define TLE5012_SPI                 (&hspi2)
#define TLE5012_MOSI_GPIO_ALTERNATE (GPIO_AF5_SPI2)

#endif /* INC_STM32_TLE5012_CONFIG_H_ */
