#include "firmware_node/src/drivers/imu_accel.h"

#include "config/system_config.h"

#if APP_USE_FREERTOS

#include "config/board_pins.h"

#include "driver/i2c.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <limits.h>

#define MPU9250_ADDR            0x68
#define MPU9250_WHO_AM_I        0x75
#define MPU9250_PWR_MGMT_1      0x6B
#define MPU9250_ACCEL_CONFIG    0x1C
#define MPU9250_ACCEL_CONFIG2   0x1D
#define MPU9250_SMPLRT_DIV      0x19
#define MPU9250_ACCEL_XOUT_H    0x3B

#define MPU9250_ACCEL_FS_SEL_4G 0x08
#define MPU9250_LSB_PER_G_4G    8192

static const char* TAG = "imu_mpu9250";

static bool s_i2c_ready = false;

static esp_err_t i2c_write_reg(uint8_t reg, uint8_t value) {
  uint8_t data[2] = { reg, value };
  return i2c_master_write_to_device(
      BOARD_I2C_PORT, MPU9250_ADDR, data, sizeof(data), pdMS_TO_TICKS(20));
}

static esp_err_t i2c_read_reg(uint8_t reg, uint8_t* buf, size_t len) {
  return i2c_master_write_read_device(
      BOARD_I2C_PORT, MPU9250_ADDR, &reg, 1, buf, len, pdMS_TO_TICKS(20));
}

static bool ensure_i2c_bus(void) {
  if (s_i2c_ready) {
    return true;
  }

  i2c_config_t cfg = {
    .mode = I2C_MODE_MASTER,
    .sda_io_num = BOARD_I2C_SDA,
    .scl_io_num = BOARD_I2C_SCL,
    .sda_pullup_en = GPIO_PULLUP_ENABLE,
    .scl_pullup_en = GPIO_PULLUP_ENABLE,
    .master.clk_speed = BOARD_I2C_FREQ_HZ,
  };

  esp_err_t err = i2c_param_config(BOARD_I2C_PORT, &cfg);
  if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
    ESP_LOGE(TAG, "i2c_param_config failed (%d)", err);
    return false;
  }
  err = i2c_driver_install(BOARD_I2C_PORT, cfg.mode, 0, 0, 0);
  if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
    ESP_LOGE(TAG, "i2c_driver_install failed (%d)", err);
    return false;
  }

  s_i2c_ready = true;
  return true;
}

bool imu_init(void) {
  if (!ensure_i2c_bus()) {
    return false;
  }

  uint8_t who_am_i = 0;
  if (i2c_read_reg(MPU9250_WHO_AM_I, &who_am_i, 1) != ESP_OK) {
    ESP_LOGE(TAG, "WHO_AM_I read failed");
    return false;
  }

  if (who_am_i != 0x71 && who_am_i != 0x73) {
    ESP_LOGW(TAG, "Unexpected WHO_AM_I=0x%02X", who_am_i);
  }

  if (i2c_write_reg(MPU9250_PWR_MGMT_1, 0x01) != ESP_OK) {
    ESP_LOGE(TAG, "Failed to exit sleep");
    return false;
  }
  vTaskDelay(pdMS_TO_TICKS(10));

  if (i2c_write_reg(MPU9250_SMPLRT_DIV, 0x09) != ESP_OK) {
    ESP_LOGE(TAG, "Failed to set sample rate");
    return false;
  }

  if (i2c_write_reg(MPU9250_ACCEL_CONFIG, MPU9250_ACCEL_FS_SEL_4G) != ESP_OK) {
    ESP_LOGE(TAG, "Failed to set accel range");
    return false;
  }

  if (i2c_write_reg(MPU9250_ACCEL_CONFIG2, 0x03) != ESP_OK) {
    ESP_LOGW(TAG, "Failed to set accel bandwidth");
  }

  ESP_LOGI(TAG, "MPU9250 initialized (WHO_AM_I=0x%02X)", who_am_i);
  return true;
}

static int16_t raw_to_centi_g(int16_t raw) {
  int32_t scaled = ((int32_t)raw * 100) / MPU9250_LSB_PER_G_4G;
  if (scaled > INT16_MAX) return INT16_MAX;
  if (scaled < INT16_MIN) return INT16_MIN;
  return (int16_t)scaled;
}

bool imu_read(accel_raw_t* out) {
  if (!out) return false;
  if (!s_i2c_ready && !ensure_i2c_bus()) return false;

  uint8_t buf[6];
  if (i2c_read_reg(MPU9250_ACCEL_XOUT_H, buf, sizeof(buf)) != ESP_OK) {
    ESP_LOGE(TAG, "Accel read failed");
    return false;
  }

  int16_t raw_ax = (int16_t)((buf[0] << 8) | buf[1]);
  int16_t raw_ay = (int16_t)((buf[2] << 8) | buf[3]);
  int16_t raw_az = (int16_t)((buf[4] << 8) | buf[5]);

  out->ax = raw_to_centi_g(raw_ax);
  out->ay = raw_to_centi_g(raw_ay);
  out->az = raw_to_centi_g(raw_az);
  return true;
}

#else  // APP_USE_FREERTOS == 0 (host stub)

#include <stdlib.h>

static bool s_initialized = false;
static uint32_t s_sample_idx = 0;

static int16_t pseudo_noise(uint32_t seed) {
  const uint32_t hash = (seed * 1103515245u + 12345u) & 0x7FFFu;
  return (int16_t)((int32_t)hash % 9 - 4);
}

static accel_raw_t build_sample(uint32_t sample_idx) {
  accel_raw_t out = {0};
  out.az = 100 + pseudo_noise(sample_idx);
  if ((sample_idx % 100) == 10) {
    out.az = 450;
  } else if ((sample_idx % 100) > 10 && (sample_idx % 100) < 80) {
    out.az = 10 + pseudo_noise(sample_idx);
  }
  out.ax = pseudo_noise(sample_idx + 1U);
  out.ay = pseudo_noise(sample_idx + 2U);
  return out;
}

bool imu_init(void) {
  s_initialized = true;
  s_sample_idx = 0;
  return true;
}

bool imu_read(accel_raw_t* out) {
  if (!s_initialized || !out) return false;
  *out = build_sample(s_sample_idx++);
  return true;
}

#endif

