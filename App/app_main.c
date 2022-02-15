#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_gpio.h"
#include "main.h"

void app_main(UART_HandleTypeDef huart2) {
	__HAL_RCC_GPIOC_CLK_ENABLE();
	GPIO_InitTypeDef Init_Button;

	Init_Button.Pin = B1_Pin;
	Init_Button.Mode = GPIO_MODE_INPUT;
	Init_Button.Pull = GPIO_NOPULL;
	Init_Button.Speed = GPIO_SPEED_LOW;

	HAL_GPIO_Init(B1_GPIO_Port, &Init_Button);

	while(1) {
		uint32_t tickstart = 0;
		GPIO_PinState first_press = HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin);
		if (!first_press) {
			tickstart = HAL_GetTick();
			while(1) {
				GPIO_PinState second_press = HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin);
				if (!second_press) {
					uint32_t elapsed = HAL_GetTick() - tickstart;
					char buffer[32];
					HAL_UART_Transmit(&huart2, (uint32_t*)buffer, sprintf(buffer, "%d\n\r", elapsed), 1000);
					HAL_Delay(1000);
				}
			}
		}
	}
}
