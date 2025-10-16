#include "firmware_node/src/services/alert_queue.h"

// Nota: Implementación mínima sin RTOS para arrancar.
// Para integración con FreeRTOS, reemplazar por una QueueHandle_t.

#define AQ_MAX_CAP 8

static fall_event_t s_buf[AQ_MAX_CAP];
static unsigned s_cap = 0;
static unsigned s_head = 0; // push idx
static unsigned s_tail = 0; // pop idx
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
    // Política MVP: descartar el más antiguo para no bloquear
    s_tail = (s_tail + 1) % s_cap;
    s_count--;
  }
  s_buf[s_head] = *e;
  s_head = (s_head + 1) % s_cap;
  s_count++;
  return true;
}

bool alert_queue_pop(fall_event_t* e, uint32_t timeout_ms) {
  (void)timeout_ms; // Sin espera en stub
  if (!e || s_cap == 0) return false;
  if (s_count == 0) return false;
  *e = s_buf[s_tail];
  s_tail = (s_tail + 1) % s_cap;
  s_count--;
  return true;
}

