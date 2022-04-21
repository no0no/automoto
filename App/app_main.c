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

void init_button();
void sample_sensors(UART_HandleTypeDef huart2, TIM_HandleTypeDef htim10);
void sample_button(UART_HandleTypeDef huart2);
void delay_us(uint16_t us, TIM_HandleTypeDef htim10);
void set_pin_input(GPIO_TypeDef *gpio, uint16_t gpio_pin);
void set_pin_output(GPIO_TypeDef *gpio, uint16_t gpio_pin);
void temp_loop(UART_HandleTypeDef huart2, I2C_Module_t* mod);
static void I2C_ClearBusyFlagErratum(I2C_Module_t* i2c, uint32_t timeout);
static uint8_t wait_for_gpio_state_timeout(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState state, uint32_t timeout);

char buffer[64];

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
	HAL_StatusTypeDef ret;

	uint8_t temp_data[2];
	temp_data[0] = 0;
	temp_data[1] = 0;

	uint8_t dev_id[2];
	dev_id[0] = 0;
	dev_id[1] = 0;

	while (1) {

		// ret = HAL_I2C_Mem_Read(&hi2c1, MCP9808_ADDR | 0x01, MCP9808_REG_TEMP, 1, temp_data, 2, 50);

		ret = HAL_I2C_Mem_Read(mod->instance, MCP9808_ADDR, MCP9808_REG_DEVID, I2C_MEMADD_SIZE_8BIT, dev_id, 2, 1000);
		if (ret != HAL_OK) {
			I2C_ClearBusyFlagErratum(mod, 1000);
			// mod->instance->Instance->CR1 |= (1<<15);
			// mod->instance->Instance->CR1 &= ~(1<<15);
		}
		ret = HAL_I2C_Mem_Read(mod->instance, MCP9808_ADDR, MCP9808_REG_TEMP, I2C_MEMADD_SIZE_8BIT, temp_data, 2, 1000);
		if (ret != HAL_OK) {
			SET_BIT(mod->instance->Instance->CR1, I2C_CR1_SWRST);
		}
		HAL_Delay(500);
	}
}

void app_main(UART_HandleTypeDef huart2, TIM_HandleTypeDef htim10, I2C_HandleTypeDef hi2c1) {

	// init_button();
	// sample_sensors(huart2, htim10);
	I2C_Module_t mod;
	mod.instance = &hi2c1;
	mod.sdaPin = I2C1_SDA_Pin;
	mod.sdaPort = I2C1_SDA_GPIO_Port;
	mod.sclPin = I2C1_SCL_Pin;
	mod.sclPort = I2C1_SCL_GPIO_Port;
	temp_loop(huart2, &mod);
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
