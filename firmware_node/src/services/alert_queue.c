#include "config/system_config.h"
#include "firmware_node/src/services/alert_queue.h"

#if APP_USE_FREERTOS

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

static QueueHandle_t s_queue = NULL;

void alert_queue_init(size_t capacity) {
  if (capacity == 0) capacity = 1;
  if (capacity > 16) capacity = 16;
  if (s_queue) {
    vQueueDelete(s_queue);
  }
  s_queue = xQueueCreate((UBaseType_t)capacity, sizeof(fall_event_t));
}

bool alert_queue_push(const fall_event_t* e) {
  if (!s_queue || !e) return false;
  if (xQueueSend(s_queue, e, 0) == pdTRUE) {
    return true;
  }
  fall_event_t dropped;
  xQueueReceive(s_queue, &dropped, 0);
  return xQueueSend(s_queue, e, 0) == pdTRUE;
}

bool alert_queue_pop(fall_event_t* e, uint32_t timeout_ms) {
  if (!s_queue || !e) return false;
  TickType_t ticks = timeout_ms ? pdMS_TO_TICKS(timeout_ms) : 0;
  return xQueueReceive(s_queue, e, ticks) == pdTRUE;
}

#else

#if defined(__unix__) || defined(__APPLE__)
#include <time.h>
#endif

#define AQ_MAX_CAP 8

static fall_event_t s_buf[AQ_MAX_CAP];
static unsigned s_cap = 0;
static unsigned s_head = 0;
static unsigned s_tail = 0;
static unsigned s_count = 0;

void alert_queue_init(size_t capacity) {
  if (capacity == 0) capacity = 1;
  if (capacity > AQ_MAX_CAP) capacity = AQ_MAX_CAP;
  s_cap = (unsigned)capacity;
  s_head = s_tail = s_count = 0;
}

bool alert_queue_push(const fall_event_t* e) {
  if (!e || s_cap == 0) return false;
  if (s_count >= s_cap) {
    s_tail = (s_tail + 1) % s_cap;
    s_count--;
  }
  s_buf[s_head] = *e;
  s_head = (s_head + 1) % s_cap;
  s_count++;
  return true;
}

static void sleep_ms(uint32_t ms) {
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

bool alert_queue_pop(fall_event_t* e, uint32_t timeout_ms) {
  if (!e || s_cap == 0) return false;

  uint32_t waited = 0;
  const uint32_t step_ms = 5;
  while (s_count == 0) {
    if (timeout_ms == 0) return false;
    if (waited >= timeout_ms) return false;
    sleep_ms(step_ms);
    waited += step_ms;
  }

  *e = s_buf[s_tail];
  s_tail = (s_tail + 1) % s_cap;
  s_count--;
  return true;
}

#endif
