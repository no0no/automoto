#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "stm32f4xx_hal.h"

void app_main(UART_HandleTypeDef huart2) {
	uint32_t time = HAL_GetTick();
	char buffer[32];
	HAL_UART_Transmit(&huart2, (uint32_t*)buffer, sprintf(buffer, "%d\n\r", time), 500);
	HAL_Delay(100);
}
