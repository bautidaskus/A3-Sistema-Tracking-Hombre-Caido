#include "firmware_rx/src/app_rx.h"

#include "config/radio_params.h"
#include "firmware_node/src/drivers/lora_radio.h"
#include "firmware_node/src/services/fall_detector.h"
#include "firmware_node/src/services/pkt_codec.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#if APP_USE_FREERTOS
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "esp_log.h"
#else
#if defined(__unix__) || defined(__APPLE__)
#include <time.h>
#endif
#endif

#if !APP_USE_FREERTOS
#define APP_RX_POLL_ITER 200U
#endif

typedef struct {
  bool ready;
#if APP_USE_FREERTOS
  QueueHandle_t evt_queue;
#else
  bool event_pending;
  fall_event_t last_event;
#endif
} app_rx_ctx_t;

static app_rx_ctx_t s_rx_ctx;
static const char* TAG_RX = "app_rx";

#if APP_USE_FREERTOS
static void rx_log(const fall_event_t* evt) {
  ESP_LOGI(TAG_RX, "ALERTA RX epoch=%u peak=%d idle=%u",
           (unsigned)evt->epoch_ms,
           (int)evt->ax_peak_centi_g,
           (unsigned)evt->idle_ms);
}
#else
static void rx_delay_ms(uint32_t ms) {
#if defined(__unix__) || defined(__APPLE__)
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
#endif

void app_rx_init(void) {
  memset(&s_rx_ctx, 0, sizeof(s_rx_ctx));
  s_rx_ctx.ready = lora_init(LORA_FREQ_HZ, LORA_SF, LORA_BW_KHZ, LORA_POUT_DBM, LORA_CRC_ON);
#if APP_USE_FREERTOS
  if (s_rx_ctx.ready) {
    s_rx_ctx.evt_queue = xQueueCreate(4, sizeof(fall_event_t));
    xTaskCreatePinnedToCore(tsk_lora_rx, "rx_lora", 4096, NULL, configMAX_PRIORITIES - 2, NULL, 1);
    xTaskCreatePinnedToCore(tsk_ui, "rx_ui", 2048, NULL, tskIDLE_PRIORITY + 1, NULL, 1);
  } else {
    ESP_LOGE(TAG_RX, "LoRa init failed");
  }
#endif
}

void tsk_lora_rx(void* arg) {
  (void)arg;
  if (!s_rx_ctx.ready) return;

  const size_t expected_len = 1U + 1U + 4U + 2U + 2U + 1U;
  uint8_t rx_buf[32];

#if APP_USE_FREERTOS
  for (;;) {
#else
  for (uint32_t i = 0; i < APP_RX_POLL_ITER; ++i) {
#endif
    if (lora_rx(rx_buf, sizeof(rx_buf), LORA_RX_TIMEOUT_MS)) {
      fall_event_t evt;
      if (pkt_decode_alert(rx_buf, expected_len, &evt)) {
#if APP_USE_FREERTOS
        if (s_rx_ctx.evt_queue) {
          xQueueSend(s_rx_ctx.evt_queue, &evt, 0);
        }
#else
        s_rx_ctx.last_event = evt;
        s_rx_ctx.event_pending = true;
#endif
      }
    }
#if !APP_USE_FREERTOS
    else {
      rx_delay_ms(20U);
    }
#endif
  }
}

void tsk_ui(void* arg) {
  (void)arg;
  if (!s_rx_ctx.ready) return;

#if APP_USE_FREERTOS
  fall_event_t evt;
  for (;;) {
    if (s_rx_ctx.evt_queue && xQueueReceive(s_rx_ctx.evt_queue, &evt, portMAX_DELAY) == pdTRUE) {
      rx_log(&evt);
    }
  }
#else
  for (uint32_t i = 0; i < APP_RX_POLL_ITER; ++i) {
    if (s_rx_ctx.event_pending) {
      s_rx_ctx.event_pending = false;
      printf("[RX] ALERTA HOMBRE CAIDO - epoch=%ums peak=%d idle=%ums\n",
             (unsigned)s_rx_ctx.last_event.epoch_ms,
             (int)s_rx_ctx.last_event.ax_peak_centi_g,
             (unsigned)s_rx_ctx.last_event.idle_ms);
      fflush(stdout);
    }
    rx_delay_ms(100U);
  }
#endif
}

