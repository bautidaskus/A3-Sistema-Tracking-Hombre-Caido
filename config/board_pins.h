#pragma once

#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/spi_master.h"

// TTGO LoRa32 (SX1276 + ESP32) pin mapping (868/915 MHz variant)
// https://github.com/LilyGO/TTGO-LORA32-V2.0

// I2C bus shared with onboard OLED (can be reused for MPU9250)
#define BOARD_I2C_PORT         I2C_NUM_0
#define BOARD_I2C_SDA          GPIO_NUM_21
#define BOARD_I2C_SCL          GPIO_NUM_22

// LoRa (SX1276) SPI interface
#define BOARD_LORA_SPI_HOST    SPI2_HOST
#define BOARD_LORA_PIN_MOSI    GPIO_NUM_27
#define BOARD_LORA_PIN_MISO    GPIO_NUM_19
#define BOARD_LORA_PIN_SCK     GPIO_NUM_5
#define BOARD_LORA_PIN_CS      GPIO_NUM_18
#define BOARD_LORA_PIN_RST     GPIO_NUM_14
#define BOARD_LORA_PIN_DIO0    GPIO_NUM_26
#define BOARD_LORA_PIN_DIO1    GPIO_NUM_33

// Optional LED indicator (on-board GPIO25)
#define BOARD_STATUS_LED       GPIO_NUM_25

// I2C clock frequency
#define BOARD_I2C_FREQ_HZ      400000
