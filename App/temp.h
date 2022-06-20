#ifndef TEMP_H_
#define TEMP_H_

uint16_t read_mcp9808(I2C_HandleTypeDef hi2c1);
bool sht30_init(I2C_HandleTypeDef hi2c1);
bool sht30_read_temperature_and_humidity(I2C_HandleTypeDef hi2c1, float *temperature, float *humidity);


#endif /* TEMP_H_ */
