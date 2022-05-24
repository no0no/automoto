
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

#define SHT30_ADDR		  0x44
#define SHT30_SINGLE_SHOT 0x24

#define word(x,y) (((x) << 8) | (y))

static void read_addr(I2C_HandleTypeDef hi2c1, uint8_t REG, uint8_t addr, uint8_t* buffer);
uint16_t read_mcp9808(I2C_HandleTypeDef hi2c1);
uint16_t read_sht30(I2C_HandleTypeDef hi2c1);

static uint8_t read_buffer[2];
static uint8_t sht30_buffer[2];

uint16_t read_sht30(I2C_HandleTypeDef hi2c1) {
	read_addr(hi2c1, SHT30_SINGLE_SHOT, SHT30_ADDR, sht30_buffer);
	uint16_t t = word(sht30_buffer[0], sht30_buffer[1]);
	return t;
}

uint16_t read_mcp9808(I2C_HandleTypeDef hi2c1) {
	read_addr(hi2c1, MCP9808_REG_TEMP, MCP9808_ADDR, read_buffer);
	uint16_t t = word(read_buffer[0], read_buffer[1]);
	return t;
}

static void read_addr(I2C_HandleTypeDef hi2c1, uint8_t reg, uint8_t addr, uint8_t* buffer) {
	HAL_StatusTypeDef ret;
	uint8_t read_reg = reg;

	ret = HAL_I2C_Master_Transmit(&hi2c1, addr<<1, &read_reg, 1, 2000);
	ret = HAL_I2C_Master_Receive(&hi2c1, addr<<1, buffer, 2, 2000);
	// TODO: Add more sophisticated error handling
	if (ret != HAL_OK) {
		return;
	}
}
