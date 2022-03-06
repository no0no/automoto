#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_gpio.h"
#include "main.h"

void init_button();
void init_temp_sensor();
void sample_sensors(UART_HandleTypeDef huart2, TIM_HandleTypeDef htim10);
void sample_button(UART_HandleTypeDef huart2);
uint8_t sample_temp_sensor(TIM_HandleTypeDef htim10);
uint8_t temp_check_response(TIM_HandleTypeDef htim10);
void delay_us(uint16_t us, TIM_HandleTypeDef htim10);
void set_pin_input(GPIO_TypeDef *gpio, uint16_t gpio_pin);
void set_pin_output(GPIO_TypeDef *gpio, uint16_t gpio_pin);


void app_main(UART_HandleTypeDef huart2, TIM_HandleTypeDef htim10) {

	// init_button();
	init_temp_sensor();
	sample_sensors(huart2, htim10);

}

void init_temp_sensor() {
	__HAL_RCC_GPIOA_CLK_ENABLE();
	set_pin_output(GPIOA, GPIO_PIN_1);
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, 0);
	HAL_Delay(18);
	set_pin_input(GPIOA, GPIO_PIN_1);
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

void set_pin_input(GPIO_TypeDef *gpio, uint16_t gpio_pin) {
	GPIO_InitTypeDef gpio_init;
	gpio_init.Pin = gpio_pin;
	gpio_init.Mode = GPIO_MODE_INPUT;
	gpio_init.Pull = GPIO_PULLUP;

	HAL_GPIO_Init(gpio, &gpio_init);
}

void set_pin_output(GPIO_TypeDef *gpio, uint16_t gpio_pin) {
	GPIO_InitTypeDef gpio_init;
	gpio_init.Pin = gpio_pin;
	gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
	gpio_init.Speed = GPIO_SPEED_FREQ_LOW;

	HAL_GPIO_Init(gpio, &gpio_init);
}

void sample_sensors(UART_HandleTypeDef huart2, TIM_HandleTypeDef htim10) {
	while (1) {
		// sample_button(huart2);
		uint8_t response = temp_check_response(htim10);
		uint8_t rh_byte1 = sample_temp_sensor(htim10);
		uint8_t rh_byte2 = sample_temp_sensor(htim10);
		uint8_t temp_byte1 = sample_temp_sensor(htim10);
		uint8_t temp_byte2 = sample_temp_sensor(htim10);
		uint8_t sum = sample_temp_sensor(htim10);

		char ebuffer[32];
		HAL_UART_Transmit(&huart2, (uint32_t*)ebuffer, sprintf(ebuffer, "temp: %d\n\r", temp_byte1), 1000);
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

uint8_t sample_temp_sensor(TIM_HandleTypeDef htim10) {
	uint8_t sample = 0;
	for (uint8_t i = 0; i < 8; i++) {
		while (!(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1))) {
			delay_us(40, htim10);
			if (!(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1))) {
				sample &= ~(1<<(7-i));
			}
			else {
				sample |= (1<<(7-i));
			}
			while ((HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1)));
		}
	}
	return sample;
}

uint8_t temp_check_response(TIM_HandleTypeDef htim10) {
	uint8_t response = 0;
	delay_us(40, htim10);
	if (!(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1))) {
		delay_us(80, htim10);
		if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1)) {
			response = 1;
		}
		else {
			response = -1;
		}
	}
	while ((HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1)));

	return response;
}



void delay_us(uint16_t us, TIM_HandleTypeDef htim10) {
	__HAL_TIM_SET_COUNTER(&htim10, 0);  // set the counter value a 0
	while (__HAL_TIM_GET_COUNTER(&htim10) < us);  // wait for the counter to reach the us input in the parameter
}
