#include "firmware_node/src/drivers/lora_radio.h"

#include "config/system_config.h"

#if APP_USE_FREERTOS

#include "config/board_pins.h"

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdlib.h>
#include <string.h>

#define SX1276_REG_FIFO              0x00
#define SX1276_REG_OP_MODE           0x01
#define SX1276_REG_FRF_MSB           0x06
#define SX1276_REG_FRF_MID           0x07
#define SX1276_REG_FRF_LSB           0x08
#define SX1276_REG_PA_CONFIG         0x09
#define SX1276_REG_OCP               0x0B
#define SX1276_REG_LNA               0x0C
#define SX1276_REG_FIFO_ADDR_PTR     0x0D
#define SX1276_REG_FIFO_TX_BASE_ADDR 0x0E
#define SX1276_REG_FIFO_RX_BASE_ADDR 0x0F
#define SX1276_REG_FIFO_RX_CURRENT   0x10
#define SX1276_REG_IRQ_FLAGS         0x12
#define SX1276_REG_IRQ_FLAGS_MASK    0x11
#define SX1276_REG_MODEM_CONFIG1     0x1D
#define SX1276_REG_MODEM_CONFIG2     0x1E
#define SX1276_REG_MODEM_CONFIG3     0x26
#define SX1276_REG_PAYLOAD_LENGTH    0x22
#define SX1276_REG_HOP_PERIOD        0x24
#define SX1276_REG_DETECTION_OPTIMIZE 0x31
#define SX1276_REG_DETECTION_THRESHOLD 0x37
#define SX1276_REG_DIO_MAPPING1      0x40
#define SX1276_REG_VERSION           0x42
#define SX1276_REG_PA_DAC            0x4D

#define SX1276_MODE_LONG_RANGE_MODE  0x80
#define SX1276_MODE_SLEEP            0x00
#define SX1276_MODE_STDBY            0x01
#define SX1276_MODE_TX               0x03
#define SX1276_MODE_RXCONTINUOUS     0x05
#define SX1276_MODE_RXSINGLE         0x06

#define SX1276_IRQ_TX_DONE           0x08
#define SX1276_IRQ_RX_DONE           0x40
#define SX1276_IRQ_RX_TIMEOUT        0x80
#define SX1276_IRQ_PAYLOAD_CRC_ERROR 0x20

static const char* TAG = "lora_sx1276";

static spi_device_handle_t s_spi = NULL;
static bool s_lora_ready = false;
static bool s_bus_ready = false;

static esp_err_t spi_write_reg(uint8_t reg, uint8_t value) {
  spi_transaction_t t = {
    .flags = SPI_TRANS_USE_TXDATA,
    .length = 16,
  };
  t.tx_data[0] = reg | 0x80;
  t.tx_data[1] = value;
  return spi_device_transmit(s_spi, &t);
}

static esp_err_t spi_read_reg(uint8_t reg, uint8_t* out, size_t len) {
  if (!out || len == 0) return ESP_ERR_INVALID_ARG;
  uint8_t header = reg & 0x7F;
  spi_transaction_t t = {
    .length = 8 * (len + 1),
  };
  uint8_t* tx = malloc(len + 1);
  uint8_t* rx = malloc(len + 1);
  if (!tx || !rx) {
    free(tx);
    free(rx);
    return ESP_ERR_NO_MEM;
  }
  tx[0] = header;
  memset(tx + 1, 0, len);
  t.tx_buffer = tx;
  t.rx_buffer = rx;
  esp_err_t ret = spi_device_transmit(s_spi, &t);
  if (ret == ESP_OK) {
    memcpy(out, rx + 1, len);
  }
  free(tx);
  free(rx);
  return ret;
}

static esp_err_t spi_write_fifo(const uint8_t* data, size_t len) {
  if (len == 0) return ESP_OK;
  uint8_t header = SX1276_REG_FIFO | 0x80;
  spi_transaction_t t = {
    .length = 8 * (len + 1),
  };
  uint8_t* tx = malloc(len + 1);
  if (!tx) return ESP_ERR_NO_MEM;
  tx[0] = header;
  memcpy(tx + 1, data, len);
  t.tx_buffer = tx;
  esp_err_t ret = spi_device_transmit(s_spi, &t);
  free(tx);
  return ret;
}

static esp_err_t spi_read_fifo(uint8_t* data, size_t len) {
  if (len == 0) return ESP_OK;
  uint8_t header = SX1276_REG_FIFO & 0x7F;
  spi_transaction_t t = {
    .length = 8 * (len + 1),
  };
  uint8_t* tx = malloc(len + 1);
  uint8_t* rx = malloc(len + 1);
  if (!tx || !rx) {
    free(tx);
    free(rx);
    return ESP_ERR_NO_MEM;
  }
  tx[0] = header;
  memset(tx + 1, 0, len);
  t.tx_buffer = tx;
  t.rx_buffer = rx;
  esp_err_t ret = spi_device_transmit(s_spi, &t);
  if (ret == ESP_OK) {
    memcpy(data, rx + 1, len);
  }
  free(tx);
  free(rx);
  return ret;
}

static void sx1276_reset(void) {
  gpio_set_level(BOARD_LORA_PIN_RST, 0);
  vTaskDelay(pdMS_TO_TICKS(10));
  gpio_set_level(BOARD_LORA_PIN_RST, 1);
  vTaskDelay(pdMS_TO_TICKS(10));
}

static bool ensure_spi_bus(void) {
  if (s_bus_ready) return true;

  spi_bus_config_t buscfg = {
    .mosi_io_num = BOARD_LORA_PIN_MOSI,
    .miso_io_num = BOARD_LORA_PIN_MISO,
    .sclk_io_num = BOARD_LORA_PIN_SCK,
    .max_transfer_sz = 64,
  };
  esp_err_t err = spi_bus_initialize(BOARD_LORA_SPI_HOST, &buscfg, SPI_DMA_DISABLED);
  if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
    ESP_LOGE(TAG, "spi_bus_initialize failed (%d)", err);
    return false;
  }
  s_bus_ready = true;
  return true;
}

static bool attach_device(void) {
  if (s_spi) return true;
  spi_device_interface_config_t devcfg = {
    .mode = 0,
    .clock_speed_hz = 8 * 1000 * 1000,
    .spics_io_num = BOARD_LORA_PIN_CS,
    .queue_size = 2,
    .flags = SPI_DEVICE_HALFDUPLEX,
  };
  if (spi_bus_add_device(BOARD_LORA_SPI_HOST, &devcfg, &s_spi) != ESP_OK) {
    ESP_LOGE(TAG, "spi_bus_add_device failed");
    return false;
  }
  return true;
}

static uint8_t bw_to_reg(uint8_t bw_khz) {
  switch (bw_khz) {
    case 7:   return 0x00;
    case 10:  return 0x10;
    case 15:  return 0x20;
    case 20:  return 0x30;
    case 31:  return 0x40;
    case 41:  return 0x50;
    case 62:  return 0x60;
    case 125: return 0x70;
    case 250: return 0x80;
    case 500: return 0x90;
    default:  return 0x70;
  }
}

static uint8_t sf_to_reg(uint8_t sf) {
  if (sf < 6) sf = 6;
  if (sf > 12) sf = 12;
  return (sf << 4);
}

static bool wait_for_irq(uint8_t mask, uint32_t timeout_ms) {
  int64_t start_us = esp_timer_get_time();
  while (true) {
    uint8_t irq = 0;
    if (spi_read_reg(SX1276_REG_IRQ_FLAGS, &irq, 1) != ESP_OK) {
      return false;
    }
    if (irq & mask) {
      spi_write_reg(SX1276_REG_IRQ_FLAGS, irq);
      return true;
    }
    if (timeout_ms > 0) {
      int64_t elapsed_ms = (esp_timer_get_time() - start_us) / 1000;
      if (elapsed_ms >= timeout_ms) {
        return false;
      }
    }
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

bool lora_init(uint32_t freq_hz, uint8_t sf, uint8_t bw_khz, int8_t pwr_dbm, bool crc_on) {
  if (!ensure_spi_bus()) return false;

  gpio_config_t rst_cfg = {
    .pin_bit_mask = 1ULL << BOARD_LORA_PIN_RST,
    .mode = GPIO_MODE_OUTPUT,
  };
  gpio_config(&rst_cfg);
  gpio_set_level(BOARD_LORA_PIN_RST, 1);

  gpio_config_t dio_cfg = {
    .pin_bit_mask = (1ULL << BOARD_LORA_PIN_DIO0) | (1ULL << BOARD_LORA_PIN_DIO1),
    .mode = GPIO_MODE_INPUT,
  };
  gpio_config(&dio_cfg);

  if (!attach_device()) return false;

  sx1276_reset();

  uint8_t version = 0;
  if (spi_read_reg(SX1276_REG_VERSION, &version, 1) != ESP_OK) {
    ESP_LOGE(TAG, "version read failed");
    return false;
  }
  if (version == 0x00 || version == 0xFF) {
    ESP_LOGE(TAG, "invalid SX1276 version 0x%02X", version);
    return false;
  }

  spi_write_reg(SX1276_REG_OP_MODE, SX1276_MODE_SLEEP | SX1276_MODE_LONG_RANGE_MODE);
  vTaskDelay(pdMS_TO_TICKS(10));
  spi_write_reg(SX1276_REG_OP_MODE, SX1276_MODE_STDBY | SX1276_MODE_LONG_RANGE_MODE);

  uint64_t frf = ((uint64_t)freq_hz << 19) / 32000000ULL;
  spi_write_reg(SX1276_REG_FRF_MSB, (frf >> 16) & 0xFF);
  spi_write_reg(SX1276_REG_FRF_MID, (frf >> 8) & 0xFF);
  spi_write_reg(SX1276_REG_FRF_LSB, frf & 0xFF);

  uint8_t modem1 = bw_to_reg(bw_khz) | 0x02;
  spi_write_reg(SX1276_REG_MODEM_CONFIG1, modem1);
  uint8_t modem2 = sf_to_reg(sf) | (crc_on ? 0x04 : 0x00);
  spi_write_reg(SX1276_REG_MODEM_CONFIG2, modem2);
  spi_write_reg(SX1276_REG_MODEM_CONFIG3, 0x04);

  spi_write_reg(SX1276_REG_PAYLOAD_LENGTH, 0x0F);
  spi_write_reg(SX1276_REG_FIFO_TX_BASE_ADDR, 0x80);
  spi_write_reg(SX1276_REG_FIFO_RX_BASE_ADDR, 0x00);

  uint8_t power = (uint8_t)((pwr_dbm < 2 ? 2 : (pwr_dbm > 17 ? 17 : pwr_dbm)) - 2);
  spi_write_reg(SX1276_REG_PA_CONFIG, 0x80 | power);
  spi_write_reg(SX1276_REG_PA_DAC, 0x84);
  spi_write_reg(SX1276_REG_OCP, 0x2B);
  spi_write_reg(SX1276_REG_LNA, 0x23);

  spi_write_reg(SX1276_REG_DIO_MAPPING1, 0x00);
  spi_write_reg(SX1276_REG_IRQ_FLAGS_MASK, ~(SX1276_IRQ_TX_DONE | SX1276_IRQ_RX_DONE | SX1276_IRQ_RX_TIMEOUT | SX1276_IRQ_PAYLOAD_CRC_ERROR));
  spi_write_reg(SX1276_REG_HOP_PERIOD, 0x00);
  spi_write_reg(SX1276_REG_DETECTION_OPTIMIZE, 0x03);
  spi_write_reg(SX1276_REG_DETECTION_THRESHOLD, 0x0A);

  s_lora_ready = true;
  ESP_LOGI(TAG, "SX1276 ready (ver=0x%02X)", version);
  return true;
}

bool lora_tx(const uint8_t* buf, size_t len, uint32_t timeout_ms) {
  if (!s_lora_ready || !buf || len == 0 || len > 255) return false;

  spi_write_reg(SX1276_REG_OP_MODE, SX1276_MODE_STDBY | SX1276_MODE_LONG_RANGE_MODE);
  spi_write_reg(SX1276_REG_IRQ_FLAGS, 0xFF);
  spi_write_reg(SX1276_REG_FIFO_ADDR_PTR, 0x80);
  spi_write_reg(SX1276_REG_PAYLOAD_LENGTH, (uint8_t)len);

  if (spi_write_fifo(buf, len) != ESP_OK) {
    ESP_LOGE(TAG, "FIFO write failed");
    return false;
  }

  spi_write_reg(SX1276_REG_OP_MODE, SX1276_MODE_TX | SX1276_MODE_LONG_RANGE_MODE);

  if (!wait_for_irq(SX1276_IRQ_TX_DONE, timeout_ms)) {
    ESP_LOGE(TAG, "TX timeout");
    spi_write_reg(SX1276_REG_OP_MODE, SX1276_MODE_STDBY | SX1276_MODE_LONG_RANGE_MODE);
    return false;
  }

  spi_write_reg(SX1276_REG_OP_MODE, SX1276_MODE_STDBY | SX1276_MODE_LONG_RANGE_MODE);
  return true;
}

bool lora_rx(uint8_t* buf, size_t maxlen, uint32_t timeout_ms) {
  if (!s_lora_ready || !buf || maxlen == 0) return false;

  spi_write_reg(SX1276_REG_OP_MODE, SX1276_MODE_STDBY | SX1276_MODE_LONG_RANGE_MODE);
  spi_write_reg(SX1276_REG_IRQ_FLAGS, 0xFF);
  spi_write_reg(SX1276_REG_FIFO_ADDR_PTR, 0x00);
  spi_write_reg(SX1276_REG_OP_MODE, SX1276_MODE_RXSINGLE | SX1276_MODE_LONG_RANGE_MODE);

  if (!wait_for_irq(SX1276_IRQ_RX_DONE | SX1276_IRQ_RX_TIMEOUT | SX1276_IRQ_PAYLOAD_CRC_ERROR, timeout_ms)) {
    ESP_LOGE(TAG, "RX wait error");
    return false;
  }

  uint8_t irq = 0;
  spi_read_reg(SX1276_REG_IRQ_FLAGS, &irq, 1);
  spi_write_reg(SX1276_REG_IRQ_FLAGS, irq);

  if (irq & SX1276_IRQ_RX_TIMEOUT) {
    ESP_LOGW(TAG, "RX timeout");
    return false;
  }
  if (irq & SX1276_IRQ_PAYLOAD_CRC_ERROR) {
    ESP_LOGW(TAG, "RX CRC error");
    return false;
  }

  uint8_t current = 0;
  uint8_t bytes = 0;
  spi_read_reg(SX1276_REG_FIFO_RX_CURRENT, &current, 1);
  spi_read_reg(SX1276_REG_PAYLOAD_LENGTH, &bytes, 1);
  if (bytes > maxlen) bytes = (uint8_t)maxlen;

  spi_write_reg(SX1276_REG_FIFO_ADDR_PTR, current);
  if (spi_read_fifo(buf, bytes) != ESP_OK) {
    return false;
  }

  spi_write_reg(SX1276_REG_OP_MODE, SX1276_MODE_STDBY | SX1276_MODE_LONG_RANGE_MODE);
  return true;
}

#else  // APP_USE_FREERTOS == 0 (host stub)

#include <string.h>

#define LORA_STUB_MAX_FRAME 64U

static bool s_initialized = false;
static uint8_t s_last_frame[LORA_STUB_MAX_FRAME];
static size_t s_last_len = 0;
static bool s_rx_pending = false;

bool lora_init(uint32_t freq_hz, uint8_t sf, uint8_t bw_khz, int8_t pwr_dbm, bool crc_on) {
  (void)freq_hz;
  (void)sf;
  (void)bw_khz;
  (void)pwr_dbm;
  (void)crc_on;
  s_initialized = true;
  s_last_len = 0;
  s_rx_pending = false;
  return true;
}

bool lora_tx(const uint8_t* buf, size_t len, uint32_t timeout_ms) {
  (void)timeout_ms;
  if (!s_initialized || !buf || len == 0) return false;
  if (len > LORA_STUB_MAX_FRAME) len = LORA_STUB_MAX_FRAME;
  memcpy(s_last_frame, buf, len);
  s_last_len = len;
  s_rx_pending = true;
  return true;
}

bool lora_rx(uint8_t* buf, size_t maxlen, uint32_t timeout_ms) {
  (void)timeout_ms;
  if (!s_initialized || !buf || maxlen == 0) return false;
  if (!s_rx_pending || s_last_len == 0) return false;
  size_t copy = s_last_len;
  if (copy > maxlen) copy = maxlen;
  memcpy(buf, s_last_frame, copy);
  s_rx_pending = false;
  return true;
}

#endif
