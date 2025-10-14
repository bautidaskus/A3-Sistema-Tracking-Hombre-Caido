# README — Drivers (HAL)

**Objetivo:** brindar primitivas mínimas y deterministas para hardware (IMU, LoRa, GPIO/LED, timers, WDT).

## Qué expone
- `imu_accel`: `imu_init()`, `imu_read()`
- `lora_radio`: `lora_init()`, `lora_tx()`, `lora_rx()`
- (opcionales) `gpio_led`, `timer_*`, `watchdog_*`

## Reglas
- Nada de lógica de negocio, reintentos o políticas.
- Sin bloqueos indefinidos: usar **timeouts**.
- Sin `printf` en hot path; retornar códigos de error.
- Documentar peor caso temporal.

## Pruebas rápidas
- IMU: 100 lecturas/seg con tolerancias y outliers.
- LoRa: TX 5 paquetes de 14 B y medir tiempo de inicio de TX.
