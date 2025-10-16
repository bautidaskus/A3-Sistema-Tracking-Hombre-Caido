#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "firmware_node/src/services/fall_detector.h"

// Cola de eventos de caída (envoltura sobre FreeRTOS Queue)
void alert_queue_init(size_t capacity);
bool alert_queue_push(const fall_event_t* e);
bool alert_queue_pop (fall_event_t* e, uint32_t timeout_ms);

