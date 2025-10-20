#include "firmware_node/src/app/app.h"

#if APP_USE_FREERTOS
#include "config/board_pins.h"
#endif
#include "config/fall_params.h"
#include "config/radio_params.h"
#include "firmware_node/src/drivers/imu_accel.h"
#include "firmware_node/src/drivers/lora_radio.h"
#include "firmware_node/src/services/alert_queue.h"
#include "firmware_node/src/services/fall_detector.h"
#include "firmware_node/src/services/pkt_codec.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#if APP_USE_FREERTOS
#include "FreeRTOS.h"
#include "task.h"
#include "driver/gpio.h"
#endif
#include "esp_log.h"

#if defined(__unix__) || defined(__APPLE__)
#include <time.h>
#endif

#ifndef APP_SAMPLE_QUEUE_CAP
#define APP_SAMPLE_QUEUE_CAP 4U
#endif

#ifndef APP_SAMPLE_PERIOD_MS
#define APP_SAMPLE_PERIOD_MS (1000U / FALL_FS_HZ)
#endif

#ifndef APP_ALERT_TIMEOUT_MS
#define APP_ALERT_TIMEOUT_MS 50U
#endif

#ifndef APP_BLINK_PERIOD_MS
#define APP_BLINK_PERIOD_MS 500U
#endif

#if !APP_USE_FREERTOS
#include <stdio.h>
#define APP_DEMO_ITER_SAMPLE 500U
#define APP_DEMO_ITER_TX     200U
#define APP_DEMO_ITER_BLINK  20U
#endif

typedef struct {
  bool drivers_ready;
#if !APP_USE_FREERTOS
  uint32_t sample_iterations;
  uint32_t tx_iterations;
  uint32_t blink_iterations;
#endif
} app_ctx_t;

static app_ctx_t s_app_ctx;
static const char* TAG_APP = "app_node";

static void app_delay_ms(uint32_t ms) {
#if APP_USE_FREERTOS
  vTaskDelay(pdMS_TO_TICKS(ms));
#elif defined(__unix__) || defined(__APPLE__)
  struct timespec ts;
  ts.tv_sec = (time_t)(ms / 1000U);
  ts.tv_nsec = (long)((ms % 1000U) * 1000000L);
  nanosleep(&ts, NULL);
#else
  volatile uint32_t spin = ms * 4000U;
  while (spin-- > 0U) {
    (void)spin;
  }
#endif
}

static uint32_t sample_period_ms(void) {
  uint32_t period = APP_SAMPLE_PERIOD_MS;
  if (period == 0U) period = 10U;
  return period;
}

static void log_alert(const fall_event_t* evt) {
#ifndef APP_SUPPRESS_LOGS
#if !APP_USE_FREERTOS
  printf("[ALERTA] epoch=%ums peak=%d idle=%ums\n",
         (unsigned)evt->epoch_ms,
         (int)evt->ax_peak_centi_g,
         (unsigned)evt->idle_ms);
  fflush(stdout);
#else
  ESP_LOGI(TAG_APP, "ALERTA epoch=%u peak=%d idle=%u",
           (unsigned)evt->epoch_ms,
           (int)evt->ax_peak_centi_g,
           (unsigned)evt->idle_ms);
#endif
#else
  (void)evt;
#endif
}

void app_init(void) {
  memset(&s_app_ctx, 0, sizeof(s_app_ctx));

  bool imu_ok = imu_init();
  bool lora_ok = lora_init(LORA_FREQ_HZ, LORA_SF, LORA_BW_KHZ, LORA_POUT_DBM, LORA_CRC_ON);

  fall_detector_init(NULL);
  alert_queue_init(APP_SAMPLE_QUEUE_CAP);

  s_app_ctx.drivers_ready = imu_ok && lora_ok;

#if !APP_USE_FREERTOS
  s_app_ctx.sample_iterations = APP_DEMO_ITER_SAMPLE;
  s_app_ctx.tx_iterations = APP_DEMO_ITER_TX;
  s_app_ctx.blink_iterations = APP_DEMO_ITER_BLINK;
#else
  if (s_app_ctx.drivers_ready) {
    gpio_config_t led_cfg = {
      .pin_bit_mask = 1ULL << BOARD_STATUS_LED,
      .mode = GPIO_MODE_OUTPUT,
      .pull_up_en = GPIO_PULLUP_DISABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&led_cfg);

    xTaskCreatePinnedToCore(tsk_sample_detect, "sample_detect", 4096, NULL, configMAX_PRIORITIES - 2, NULL, 1);
    xTaskCreatePinnedToCore(tsk_alert_tx, "alert_tx", 4096, NULL, configMAX_PRIORITIES - 1, NULL, 1);
    xTaskCreatePinnedToCore(tsk_blink, "blink", 2048, NULL, tskIDLE_PRIORITY + 1, NULL, 1);
  } else {
    ESP_LOGE(TAG_APP, "Drivers init failed (imu=%d lora=%d)", imu_ok, lora_ok);
  }
#endif
}

void tsk_sample_detect(void* arg) {
  (void)arg;
  if (!s_app_ctx.drivers_ready) return;

#if APP_USE_FREERTOS
  for (;;) {
#else
  for (uint32_t i = 0; i < s_app_ctx.sample_iterations; ++i) {
#endif
    accel_raw_t sample;
    if (imu_read(&sample)) {
      fall_event_t evt;
      if (fall_detector_feed(&sample, &evt)) {
        alert_queue_push(&evt);
      }
    }
    app_delay_ms(sample_period_ms());
  }
}

void tsk_alert_tx(void* arg) {
  (void)arg;
  if (!s_app_ctx.drivers_ready) return;

  uint8_t tx_buf[32];

#if APP_USE_FREERTOS
  for (;;) {
#else
  for (uint32_t i = 0; i < s_app_ctx.tx_iterations; ++i) {
#endif
    fall_event_t evt;
    if (alert_queue_pop(&evt, APP_ALERT_TIMEOUT_MS)) {
      size_t len = pkt_encode_alert(&evt, tx_buf, sizeof(tx_buf));
      if (len > 0U && lora_tx(tx_buf, len, LORA_TX_TIMEOUT_MS)) {
        log_alert(&evt);
      }
    } else {
      app_delay_ms(5U);
    }
  }
}

void tsk_blink(void* arg) {
  (void)arg;
  if (!s_app_ctx.drivers_ready) return;

#if APP_USE_FREERTOS
  for (;;) {
#else
  for (uint32_t i = 0; i < s_app_ctx.blink_iterations; ++i) {
#endif
#ifndef APP_SUPPRESS_LOGS
#if !APP_USE_FREERTOS
    printf("[LED] toggle\n");
    fflush(stdout);
#endif
#endif
#if APP_USE_FREERTOS
    static bool led_state = false;
    led_state = !led_state;
    gpio_set_level(BOARD_STATUS_LED, led_state);
#endif
    app_delay_ms(APP_BLINK_PERIOD_MS);
  }
}
