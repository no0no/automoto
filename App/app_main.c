#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_gpio.h"
#include "main.h"

#define MCP9808_REG_DEVID 0x07
#define MCP9808_REG_MANID 0x06
#define MCP9808_DEVID 0x40000
#define MCP9808_MANID 0x54

#define MCP9808_REG_TEMP 0x05
#define MCP9808_REG_CONF 0x01
#define MCP9808_ADDR	 (0x18 << 1)

void init_button();
void sample_sensors(UART_HandleTypeDef huart2, TIM_HandleTypeDef htim10);
void sample_button(UART_HandleTypeDef huart2);
void delay_us(uint16_t us, TIM_HandleTypeDef htim10);
void set_pin_input(GPIO_TypeDef *gpio, uint16_t gpio_pin);
void set_pin_output(GPIO_TypeDef *gpio, uint16_t gpio_pin);
void temp_loop(UART_HandleTypeDef huart2, I2C_HandleTypeDef hi2c1);

char buffer[64];

void temp_loop(UART_HandleTypeDef huart2, I2C_HandleTypeDef hi2c1) {
	HAL_StatusTypeDef ret;

	uint8_t temp_data[2];
	uint8_t buf[12];
	uint8_t buf2[12];

	float temp;

	uint8_t dev_id[2];
	dev_id[0] = 0;
	dev_id[1] = 0;
	uint8_t man_id[2];
	man_id[0] = 0;
	man_id[1] = 0;

	while (1) {

		// ret = HAL_I2C_Mem_Read(&hi2c1, MCP9808_ADDR | 0x01, MCP9808_REG_TEMP, 1, temp_data, 2, 50);

		ret = HAL_I2C_Mem_Read(&hi2c1, MCP9808_ADDR, MCP9808_REG_DEVID, I2C_MEMADD_SIZE_8BIT, dev_id, 2, HAL_MAX_DELAY);
		ret = HAL_I2C_Mem_Read(&hi2c1, MCP9808_ADDR, MCP9808_REG_MANID, I2C_MEMADD_SIZE_8BIT, man_id, 2, HAL_MAX_DELAY);
		sprintf((char*)buf, "devid: %d\n\r", dev_id);
		sprintf((char*)buf2, "manid: %d\n\r", man_id);

		HAL_UART_Transmit(&huart2, buf2, strlen((char*)buf2), HAL_MAX_DELAY);
		HAL_Delay(1000);
		HAL_UART_Transmit(&huart2, buf, strlen((char*)buf), HAL_MAX_DELAY);

		HAL_Delay(500);
	}
}

void app_main(UART_HandleTypeDef huart2, TIM_HandleTypeDef htim10, I2C_HandleTypeDef hi2c1) {

	// init_button();
	// sample_sensors(huart2, htim10);
	__HAL_RCC_I2C1_FORCE_RESET();
	HAL_Delay(1000);
	__HAL_RCC_I2C1_RELEASE_RESET();
	temp_loop(huart2, hi2c1);
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

void sample_sensors(UART_HandleTypeDef huart2, TIM_HandleTypeDef htim10) {
	while (1) {
		// sample_button(huart2);
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

void delay_us(uint16_t us, TIM_HandleTypeDef htim10) {
	__HAL_TIM_SET_COUNTER(&htim10, 0);  // set the counter value a 0
	while (__HAL_TIM_GET_COUNTER(&htim10) < us);  // wait for the counter to reach the us input in the parameter
}
