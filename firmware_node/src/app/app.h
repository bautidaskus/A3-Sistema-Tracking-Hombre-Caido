#pragma once

#include <stdint.h>

#include "config/system_config.h"

// Inicialización de la aplicación (creación de tareas/colas/timers)
void app_init(void);

// Tareas (MVP)
void tsk_sample_detect(void* arg);
void tsk_alert_tx(void* arg);
void tsk_blink(void* arg);
