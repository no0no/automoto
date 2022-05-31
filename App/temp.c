
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <stdbool.h>

#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_gpio.h"

#define MCP9808_REG_DEVID 0x07
#define MCP9808_REG_MANID 0x06
#define MCP9808_DEVID 	  0x40000
#define MCP9808_MANID     0x54

#define MCP9808_REG_TEMP  0x05
#define MCP9808_REG_CONF  0x01
#define MCP9808_ADDR      0x18

#define SHT3X_I2C_DEVICE_ADDRESS_ADDR_PIN_LOW 0x44

typedef enum
{
	SHT3X_COMMAND_MEASURE_HIGHREP_STRETCH = 0x2c06,
	SHT3X_COMMAND_CLEAR_STATUS = 0x3041,
	SHT3X_COMMAND_SOFT_RESET = 0x30A2,
	SHT3X_COMMAND_HEATER_ENABLE = 0x306d,
	SHT3X_COMMAND_HEATER_DISABLE = 0x3066,
	SHT3X_COMMAND_READ_STATUS = 0xf32d,
	SHT3X_COMMAND_FETCH_DATA = 0xe000,
	SHT3X_COMMAND_MEASURE_HIGHREP_10HZ = 0x2737,
	SHT3X_COMMAND_MEASURE_LOWREP_10HZ = 0x272a
} sht30_command_t;

#define word(x,y) (((x) << 8) | (y))

static void read_addr(I2C_HandleTypeDef hi2c1, uint8_t REG, uint8_t addr, uint8_t* buffer);
uint16_t read_mcp9808(I2C_HandleTypeDef hi2c1);
bool sht30_read_temperature_and_humidity(I2C_HandleTypeDef hi2c1, float *temperature, float *humidity);
static bool sht30_send_command(I2C_HandleTypeDef hi2c1, sht30_command_t command);
bool sht30_set_header_enable(I2C_HandleTypeDef hi2c1, bool enable);
bool sht30_init(I2C_HandleTypeDef hi2c1);
static uint8_t calculate_crc(const uint8_t *data, size_t length);
static uint16_t uint8_to_uint16(uint8_t msb, uint8_t lsb);

static uint8_t read_buffer[2];
static uint8_t sht30_buffer[2];

static bool sht30_send_command(I2C_HandleTypeDef hi2c1, sht30_command_t command) {
	uint8_t command_buffer[2] = {(command & 0xff00u) >> 8u, command & 0xffu};

	if (HAL_I2C_Master_Transmit(&hi2c1, SHT3X_I2C_DEVICE_ADDRESS_ADDR_PIN_LOW << 1u, command_buffer, sizeof(command_buffer), 30) != HAL_OK) {
		return false;
	}

	return true;
}

bool sht30_init(I2C_HandleTypeDef hi2c1) {
	assert(hi2c1.Init.NoStretchMode == I2C_NOSTRETCH_DISABLE);
	// TODO: Assert i2c frequency is not too high

	uint8_t status_reg_and_checksum[3];
	if (HAL_I2C_Mem_Read(&hi2c1, SHT3X_I2C_DEVICE_ADDRESS_ADDR_PIN_LOW << 1u, SHT3X_COMMAND_READ_STATUS, 2, (uint8_t*)&status_reg_and_checksum,
					  sizeof(status_reg_and_checksum), 30) != HAL_OK) {
		return false;
	}

	uint8_t calculated_crc = calculate_crc(status_reg_and_checksum, 2);

	if (calculated_crc != status_reg_and_checksum[2]) {
		return false;
	}

	return true;
}

static uint16_t uint8_to_uint16(uint8_t msb, uint8_t lsb) {
	return (uint16_t)((uint16_t)msb << 8u) | lsb;
}

bool sht30_set_header_enable(I2C_HandleTypeDef hi2c1, bool enable) {
	if (enable) {
		return sht30_send_command(hi2c1, SHT3X_COMMAND_HEATER_ENABLE);
	} else {
		return sht30_send_command(hi2c1, SHT3X_COMMAND_HEATER_DISABLE);
	}
}

static uint8_t calculate_crc(const uint8_t *data, size_t length) {
	uint8_t crc = 0xff;
	for (size_t i = 0; i < length; i++) {
		crc ^= data[i];
		for (size_t j = 0; j < 8; j++) {
			if ((crc & 0x80u) != 0) {
				crc = (uint8_t)((uint8_t)(crc << 1u) ^ 0x31u);
			} else {
				crc <<= 1u;
			}
		}
	}
	return crc;
}

bool sht30_read_temperature_and_humidity(I2C_HandleTypeDef hi2c1, float *temperature, float *humidity) {
	bool status = sht30_send_command(hi2c1, SHT3X_COMMAND_MEASURE_HIGHREP_STRETCH);

	if (!status) {
		return false;
	}

	HAL_Delay(1);

	uint8_t buffer[6];
	if (HAL_I2C_Master_Receive(&hi2c1, SHT3X_I2C_DEVICE_ADDRESS_ADDR_PIN_LOW << 1u, buffer, sizeof(buffer), 30) != HAL_OK) {
		return false;
	}

	uint8_t temperature_crc = calculate_crc(buffer, 2);
	uint8_t humidity_crc = calculate_crc(buffer + 3, 2);
	if (temperature_crc != buffer[2] || humidity_crc != buffer[5]) {
		return false;
	}

	int16_t temperature_raw = (int16_t)uint8_to_uint16(buffer[0], buffer[1]);
	uint16_t humidity_raw = uint8_to_uint16(buffer[3], buffer[4]);

	*temperature = -45.0f + 175.0f * (float)temperature_raw / 65535.0f;
	*humidity = 100.0f * (float)humidity_raw / 65535.0f;

	return true;
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
