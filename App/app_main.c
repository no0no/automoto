#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_gpio.h"
#include "main.h"

void init_button();
void init_temp_sensor();
void sample_sensors();
void sample_button();
void sample_temp_sensor();

void app_main(UART_HandleTypeDef huart2) {

	init_button();
	init_temp_sensor();

	sample_sensors(huart2);

}

void init_temp_sensor() {
	__HAL_RCC_GPIOA_CLK_ENABLE();
	GPIO_InitTypeDef Init_Temp;

	Init_Temp.Pin = GPIO_PIN_1;
	Init_Temp.Mode = GPIO_MODE_OUTPUT_PP;
	Init_Temp.Pull = GPIO_PULLUP;
	Init_Temp.Speed = GPIO_SPEED_LOW;

	HAL_GPIO_Init(GPIOD, &Init_Temp);

	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);
	HAL_Delay(18);

	Init_Temp.Mode = GPIO_MODE_INPUT;
	HAL_GPIO_Init(GPIOD, &Init_Temp);
}


void init_button() {
	__HAL_RCC_GPIOC_CLK_ENABLE();
	GPIO_InitTypeDef Init_Button;

	Init_Button.Pin = B1_Pin;
	Init_Button.Mode = GPIO_MODE_INPUT;
	Init_Button.Pull = GPIO_NOPULL;
	Init_Button.Speed = GPIO_SPEED_LOW;

	HAL_GPIO_Init(B1_GPIO_Port, &Init_Button);
}

void sample_sensors(UART_HandleTypeDef huart2) {
	while (1) {
		sample_button(huart2);
		sample_temp_sensor(huart2);
	}
}

void sample_button(UART_HandleTypeDef huart2) {
	uint32_t tickstart = 0;
	GPIO_PinState first_press = HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin);
	if (!first_press) {
		tickstart = HAL_GetTick();
		// TODO: Interrupt instead of delay
		HAL_Delay(300);
		while(1) {
			GPIO_PinState second_press = HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin);
			if (!second_press) {
				uint32_t elapsed = HAL_GetTick() - tickstart;
				char ebuffer[32];
				HAL_UART_Transmit(&huart2, (uint32_t*)ebuffer, sprintf(ebuffer, "elapsed: %d\n\r", elapsed), 1000);
				break;
			}
		}
	}
}

void sample_temp_sensor(UART_HandleTypeDef huart2) {
	GPIO_PinState sample = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1);
	char buffer[32];
	HAL_UART_Transmit(&huart2, (uint16_t*)buffer, sprintf(buffer, "temp: %d\n\r", sample), 1000);
	HAL_Delay(2000);
}
