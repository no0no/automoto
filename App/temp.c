
#define MCP9808_REG_TEMP (0x05)
#define MCP9808_REG_CONF (0x01)
#define MCP9808_ADDR	 (0x30)

void temp_loop(UART_HandleTypeDef huart2, I2C_HandleTypeDef hi2c1) {
	while(1) {
		continue;
	}
}



int read_temp(uint8_t reg, uint8_t *data, I2C_HandleTypeDef hi2c1) {
	if (HAL_I2C_Master_Transmit(&hi2c1, MCP9808_ADDR, &reg, 1, HAL_MAX_DELAY) != HAL_OK) {
		return 0;
	}
	if(HAL_I2C_Master_Receive(&hi2c1,MCP9808ADDRESS,data,2,HAL_MAX_DELAY) != HAL_OK) {
		return 0;
	}

	return 1;
}
