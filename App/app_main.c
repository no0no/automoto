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
	uint16_t temp;
	while (1) {
		temp = read_temp(hi2c1);
	}
}

void app_main(UART_HandleTypeDef huart2, I2C_HandleTypeDef hi2c1) {
	gpio_i2c1_init();
	temp_loop(huart2, hi2c1);
}
