#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_gpio.h"
#include "main.h"
#include "temp.h"

void temp_loop(UART_HandleTypeDef huart2, I2C_HandleTypeDef hi2c1);
void gpio_i2c1_init();

void gpio_i2c1_init() {
	GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FAST;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
    __GPIOB_CLK_ENABLE();
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

void temp_loop(UART_HandleTypeDef huart2, I2C_HandleTypeDef hi2c1) {
	uint16_t mcp9808_temp;

	float sht30_temperature, sht30_humidity;

	// TODO: Better Error handling
	if(!sht30_init(hi2c1)) {
		printf("ERROR\n");
	}

	while (1) {
		mcp9808_temp = read_mcp9808(hi2c1);
		sht30_read_temperature_and_humidity(hi2c1, &sht30_temperature, &sht30_humidity);
	}
}

void app_main(UART_HandleTypeDef huart2, I2C_HandleTypeDef hi2c1) {
	gpio_i2c1_init();
	temp_loop(huart2, hi2c1);
}
