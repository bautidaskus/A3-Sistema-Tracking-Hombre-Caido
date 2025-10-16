#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "firmware_node/src/drivers/imu_accel.h"

typedef struct {
  uint32_t epoch_ms;
  int16_t  ax_peak_centi_g;
  uint16_t idle_ms;
} fall_event_t;

typedef struct {
  int16_t Athr_centi_g;   // ~220 (≈2.2 g)
  int16_t Ithr_centi_g;   // ~20  (≈0.2 g)
  uint16_t Tidle_ms;      // 500..1000 ms
  uint8_t fs_hz;          // 100 Hz
} fall_cfg_t;

void fall_detector_init(const fall_cfg_t* cfg);

// Alimenta el detector con una muestra. Retorna true si se confirma evento y
// escribe en out_event.
bool fall_detector_feed(const accel_raw_t* s, fall_event_t* out_event);

