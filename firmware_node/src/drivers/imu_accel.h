#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct { int16_t ax, ay, az; } accel_raw_t;

// Inicializa el IMU (acelerómetro)
// Retorna true en éxito. Tiempo de ejecución acotado.
bool imu_init(void);

// Lee una muestra cruda del acelerómetro
// Debe ser no bloqueante o con tiempo acotado (O(100 us–1 ms) típico)
bool imu_read(accel_raw_t* out);

