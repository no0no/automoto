#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_gpio.h"
#include "main.h"

#define I2C1_SCL_Pin GPIO_PIN_8
#define I2C1_SCL_GPIO_Port GPIOB
#define I2C1_SDA_Pin GPIO_PIN_9
#define I2C1_SDA_GPIO_Port GPIOB

#define MCP9808_REG_DEVID 0x07
#define MCP9808_REG_MANID 0x06
#define MCP9808_DEVID 0x40000
#define MCP9808_MANID 0x54

#define MCP9808_REG_TEMP 0x05
#define MCP9808_REG_CONF 0x01
#define MCP9808_ADDR (0x18)

typedef struct {
	I2C_HandleTypeDef* instance;
	uint16_t sdaPin;
	GPIO_TypeDef* sdaPort;
	uint16_t sclPin;
	GPIO_TypeDef* sclPort;
} I2C_Module_t;

void temp_loop(UART_HandleTypeDef huart2, I2C_Module_t* mod);
static void I2C_ClearBusyFlagErratum(I2C_Module_t* i2c, uint32_t timeout);
static uint8_t wait_for_gpio_state_timeout(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState state, uint32_t timeout);
static void read_addr(I2C_Module_t* mod, uint8_t REG);
static uint16_t read_temp(I2C_Module_t* mod);

char buffer[64];
uint8_t readBuffer[5];

#define word(x,y) (((x) << 8) | (y))

static uint8_t wait_for_gpio_state_timeout(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState state, uint32_t timeout) {
    uint32_t Tickstart = HAL_GetTick();
    uint8_t ret = 1;
    /* Wait until flag is set */
    for(;(state != HAL_GPIO_ReadPin(port, pin)) && (1 == ret);) {
        /* Check for the timeout */
        if (timeout != HAL_MAX_DELAY) {
            if ((timeout == 0U) || ((HAL_GetTick() - Tickstart) > timeout)) {
                ret = 0;
            }
        }
        asm("nop");
    }
    return ret;
}

static void I2C_ClearBusyFlagErratum(I2C_Module_t* i2c, uint32_t timeout) {
    GPIO_InitTypeDef GPIO_InitStructure;

    I2C_HandleTypeDef* handler = NULL;

    handler = i2c->instance;

    // 1. Clear PE bit.
    CLEAR_BIT(handler->Instance->CR1, I2C_CR1_PE);

    //  2. Configure the SCL and SDA I/Os as General Purpose Output Open-Drain, High level (Write 1 to GPIOx_ODR).
    HAL_I2C_DeInit(handler);

    GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStructure.Pull = GPIO_NOPULL;

    GPIO_InitStructure.Pin = i2c->sclPin;
    HAL_GPIO_Init(i2c->sclPort, &GPIO_InitStructure);

    GPIO_InitStructure.Pin = i2c->sdaPin;
    HAL_GPIO_Init(i2c->sdaPort, &GPIO_InitStructure);

    // 3. Check SCL and SDA High level in GPIOx_IDR.
    HAL_GPIO_WritePin(i2c->sdaPort, i2c->sdaPin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(i2c->sclPort, i2c->sclPin, GPIO_PIN_SET);

    wait_for_gpio_state_timeout(i2c->sclPort, i2c->sclPin, GPIO_PIN_SET, timeout);
    wait_for_gpio_state_timeout(i2c->sdaPort, i2c->sdaPin, GPIO_PIN_SET, timeout);

    // 4. Configure the SDA I/O as General Purpose Output Open-Drain, Low level (Write 0 to GPIOx_ODR).
    HAL_GPIO_WritePin(i2c->sdaPort, i2c->sdaPin, GPIO_PIN_RESET);

    // 5. Check SDA Low level in GPIOx_IDR.
    wait_for_gpio_state_timeout(i2c->sdaPort, i2c->sdaPin, GPIO_PIN_RESET, timeout);

    // 6. Configure the SCL I/O as General Purpose Output Open-Drain, Low level (Write 0 to GPIOx_ODR).
    HAL_GPIO_WritePin(i2c->sclPort, i2c->sclPin, GPIO_PIN_RESET);

    // 7. Check SCL Low level in GPIOx_IDR.
    wait_for_gpio_state_timeout(i2c->sclPort, i2c->sclPin, GPIO_PIN_RESET, timeout);

    // 8. Configure the SCL I/O as General Purpose Output Open-Drain, High level (Write 1 to GPIOx_ODR).
    HAL_GPIO_WritePin(i2c->sclPort, i2c->sclPin, GPIO_PIN_SET);

    // 9. Check SCL High level in GPIOx_IDR.
    wait_for_gpio_state_timeout(i2c->sclPort, i2c->sclPin, GPIO_PIN_SET, timeout);

    // 10. Configure the SDA I/O as General Purpose Output Open-Drain , High level (Write 1 to GPIOx_ODR).
    HAL_GPIO_WritePin(i2c->sdaPort, i2c->sdaPin, GPIO_PIN_SET);

    // 11. Check SDA High level in GPIOx_IDR.
    wait_for_gpio_state_timeout(i2c->sdaPort, i2c->sdaPin, GPIO_PIN_SET, timeout);

    // 12. Configure the SCL and SDA I/Os as Alternate function Open-Drain.
    GPIO_InitStructure.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStructure.Alternate = GPIO_AF4_I2C2;

    GPIO_InitStructure.Pin = i2c->sclPin;
    HAL_GPIO_Init(i2c->sclPort, &GPIO_InitStructure);

    GPIO_InitStructure.Pin = i2c->sdaPin;
    HAL_GPIO_Init(i2c->sdaPort, &GPIO_InitStructure);

    // 13. Set SWRST bit in I2Cx_CR1 register.
    SET_BIT(handler->Instance->CR1, I2C_CR1_SWRST);
    asm("nop");

    /* 14. Clear SWRST bit in I2Cx_CR1 register. */
    CLEAR_BIT(handler->Instance->CR1, I2C_CR1_SWRST);
    asm("nop");

    /* 15. Enable the I2C peripheral by setting the PE bit in I2Cx_CR1 register */
    SET_BIT(handler->Instance->CR1, I2C_CR1_PE);
    asm("nop");

    // Call initialization function.
    HAL_I2C_Init(handler);
}

void temp_loop(UART_HandleTypeDef huart2, I2C_Module_t* mod) {

	uint16_t temp;
	while (1) {
		temp = read_temp(mod);
	}
}

static uint16_t read_temp(I2C_Module_t* mod) {
	read_addr(mod, MCP9808_REG_TEMP);
	uint16_t t = word(readBuffer[0], readBuffer[1]);
	return t;
}

static void read_addr(I2C_Module_t* mod, uint8_t REG) {
	HAL_StatusTypeDef ret;
	uint8_t readReg = REG;

	ret = HAL_I2C_Master_Transmit(mod->instance, MCP9808_ADDR<<1, &readReg, 1, 2000);
	if (ret == HAL_BUSY) {
		I2C_ClearBusyFlagErratum(mod, 1000);
		__HAL_RCC_I2C1_FORCE_RESET();
		HAL_Delay(2);
		__HAL_RCC_I2C1_RELEASE_RESET();
	}
	ret = HAL_I2C_Master_Receive(mod->instance, MCP9808_ADDR<<1, readBuffer, 2, 2000);
}

void app_main(UART_HandleTypeDef huart2, I2C_HandleTypeDef hi2c1) {
	I2C_Module_t mod;
	mod.instance = &hi2c1;
	mod.sdaPin = I2C1_SDA_Pin;
	mod.sdaPort = I2C1_SDA_GPIO_Port;
	mod.sclPin = I2C1_SCL_Pin;
	mod.sclPort = I2C1_SCL_GPIO_Port;
	temp_loop(huart2, &mod);
}
