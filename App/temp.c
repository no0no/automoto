/*
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_gpio.h"

#define MCP9808_REG_TEMP (0x05)
#define MCP9808_REG_CONF (0x01)
#define MCP9808_ADDR	 (0x30)

void temp_loop(UART_HandleTypeDef huart2, I2C_HandleTypeDef hi2c1, UART_HandleTypeDef huart2);

void temp_loop(UART_HandleTypeDef huart2, I2C_HandleTypeDef hi2c1, UART_HandleTypeDef huart2) {


	HAL_StatusTypeDef ret;

	uint8_t buf[12];
	int16_t val;

	float temp_c;

	while(1) {
		buf[0] = MCP9808_REG_TEMP;
		ret = HAL_I2C_Master_Transmit(&hi2c1, MCP9808_ADDR, buf, 1, HAL_MAX_DELAY);
		if (ret != HAL_OK) {
			strcpy((char*)buf, "Error Tx\r\n");
		} else {
			ret = HAL_I2C_Master_Receive(&hi2c1, MCP9808_ADDR, buf, 2, HAL_MAX_DELAY);
			if (ret != HAL_OK) {
				strcpy((char*)buf, "Error Rx\r\n");
			} else {
				val = ((int16_t)buf[0] << 4) | (buf[1] >> 4);
				if (val > 0x7FF) {
					val |= 0xF000;
				}

				temp_c = val * 0.0625;

				temp_c *= 100;
				sprintf((char*)buf, "%u,%u C\r\n", ((unsigned int)temp_c / 100), ((unsigned int)temp_c  % 100));
			}
		}

		HAL_UART_Transmit(&huart2, buf, strlen((char*)buf), HAL_MAX_DELAY);

		HAL_Delay(500);
	}
}
*/
