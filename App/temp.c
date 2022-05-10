
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_gpio.h"

#define MCP9808_REG_DEVID 0x07
#define MCP9808_REG_MANID 0x06
#define MCP9808_DEVID 	  0x40000
#define MCP9808_MANID     0x54

#define MCP9808_REG_TEMP  0x05
#define MCP9808_REG_CONF  0x01
#define MCP9808_ADDR      0x18

#define word(x,y) (((x) << 8) | (y))

static void read_addr(I2C_HandleTypeDef hi2c1, uint8_t REG);
uint16_t read_temp(I2C_HandleTypeDef hi2c1);

static uint8_t readBuffer[5];

uint16_t read_temp(I2C_HandleTypeDef hi2c1) {
	read_addr(hi2c1, MCP9808_REG_TEMP);
	uint16_t t = word(readBuffer[0], readBuffer[1]);
	return t;
}

static void read_addr(I2C_HandleTypeDef hi2c1, uint8_t REG) {
	HAL_StatusTypeDef ret;
	uint8_t readReg = REG;

	ret = HAL_I2C_Master_Transmit(&hi2c1, MCP9808_ADDR<<1, &readReg, 1, 2000);
	ret = HAL_I2C_Master_Receive(&hi2c1, MCP9808_ADDR<<1, readBuffer, 2, 2000);
	// TODO: Add more sophisticated error handling
	if (ret != HAL_OK) {
		return;
	}
}
