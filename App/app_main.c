#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_gpio.h"
#include "main.h"

#define DHT11_PIN GPIO_PIN_7
#define DHT11_PORT GPIOD

void init_button();
void init_temp_sensor(TIM_HandleTypeDef htim10);
void sample_sensors(UART_HandleTypeDef huart2, TIM_HandleTypeDef htim10);
void sample_button(UART_HandleTypeDef huart2);
uint8_t sample_temp_sensor(TIM_HandleTypeDef htim10);
uint8_t temp_check_response(TIM_HandleTypeDef htim10);
void delay_us(uint16_t us, TIM_HandleTypeDef htim10);
void set_pin_input(GPIO_TypeDef *gpio, uint16_t gpio_pin);
void set_pin_output(GPIO_TypeDef *gpio, uint16_t gpio_pin);
void blink_led(TIM_HandleTypeDef htim10);

char buffer[64];

void app_main(UART_HandleTypeDef huart2, TIM_HandleTypeDef htim10) {

	// init_button();
	sample_sensors(huart2, htim10);

}

void blink_led(TIM_HandleTypeDef htim10) {
	HAL_GPIO_TogglePin(DHT11_PORT, DHT11_PIN);
	for (int i = 0; i < 20; i++) {
		delay_us(50000, htim10);
	}
}

void init_temp_sensor(TIM_HandleTypeDef htim10) {
	set_pin_output(DHT11_PORT, DHT11_PIN);
	HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, 0);
	delay_us(18000, htim10);
	HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, 1);
	delay_us(20, htim10);
	set_pin_input(DHT11_PORT, DHT11_PIN);
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
	GPIO_InitTypeDef gpio_init = {0};
	gpio_init.Pin = gpio_pin;
	gpio_init.Mode = GPIO_MODE_INPUT;
	gpio_init.Pull = GPIO_PULLUP;

	HAL_GPIO_Init(gpio, &gpio_init);
}

void set_pin_output(GPIO_TypeDef *gpio, uint16_t gpio_pin) {
	GPIO_InitTypeDef gpio_init = {0};
	gpio_init.Pin = gpio_pin;
	gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
	gpio_init.Speed = GPIO_SPEED_FREQ_LOW;

	HAL_GPIO_Init(gpio, &gpio_init);
}


uint8_t response;
uint8_t rh_byte1;
uint8_t rh_byte2;
uint8_t temp_byte1;
uint8_t temp_byte2;
uint8_t sum;
void sample_sensors(UART_HandleTypeDef huart2, TIM_HandleTypeDef htim10) {
	while (1) {
		// sample_button(huart2);
		init_temp_sensor(htim10);
		response = temp_check_response(htim10);
		rh_byte1 = sample_temp_sensor(htim10);
		rh_byte2 = sample_temp_sensor(htim10);
		temp_byte1 = sample_temp_sensor(htim10);
		temp_byte2 = sample_temp_sensor(htim10);
		sum = sample_temp_sensor(htim10);

		uint8_t data_intact = 0;
		uint16_t data_check = rh_byte1 + rh_byte2 + temp_byte1 + temp_byte2;

		if (sum == data_check) {
			data_intact = 1;
		}

		sprintf(buffer, "temp=%d.%d \t hum=%d.%d\tgood:%d\tresponse:%d\n\r", temp_byte1, temp_byte2, rh_byte1, rh_byte2, data_intact, response);

		HAL_UART_Transmit(&huart2, (uint8_t*)buffer, strlen(buffer), HAL_MAX_DELAY);
		HAL_Delay(1000);
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
	uint8_t sample;
	for (uint8_t i = 0; i < 8; i++) {
		while (!(HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN))) {
			delay_us(40, htim10);
			if (!(HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN))) {
				sample &= ~(1<<(7-i));
			}
			else {
				sample |= (1<<(7-i));
			}
			while ((HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN)));
		}
	}
	return sample;
}

uint8_t temp_check_response(TIM_HandleTypeDef htim10) {
	uint8_t response = 0;
	delay_us(40, htim10);
	if (!(HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN))) {
		delay_us(80, htim10);
		if (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN)) {
			response = 1;
		}
		else {
			response = -1;
		}
	}
	while ((HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN)));

	return response;
}



void delay_us(uint16_t us, TIM_HandleTypeDef htim10) {
	__HAL_TIM_SET_COUNTER(&htim10, 0);  // set the counter value a 0
	while (__HAL_TIM_GET_COUNTER(&htim10) < us);  // wait for the counter to reach the us input in the parameter
}
