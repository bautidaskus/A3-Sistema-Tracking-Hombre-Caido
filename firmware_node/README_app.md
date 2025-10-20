# README — Aplicación (RTOS y Orquestación)

Objetivo: crear tareas mínimas, configurar prioridades y orquestar el flujo con foco RT. En la versión ESP-IDF (`APP_USE_FREERTOS=1`) `app_init()` configura los drivers reales (MPU9250 + SX1276), inicializa la cola y crea las tareas con prioridades acordes.

## Tareas (Nodo móvil, MVP)
- `tsk_sample_detect` (ALTA): lee IMU a 100 Hz y corre el detector (ejecuta en núcleo 1).
- `tsk_alert_tx` (MÁXIMA): toma evento de la cola, codifica y llama a `lora_tx()`.
- `tsk_blink` (BAJA): indica estado por GPIO25 (LED onboard) cuando se compila para ESP32.

## Estados
`INIT → RUN → ERROR(retry)`

## Garantías RT (MVP)
- Detección confirmada → inicio de TX ≤ 300 ms (límite curso: < 1 s).
- Timeouts cortos + reintento breve (opcional, 1 vez) en radio.
- Nada bloquea la tarea de sampleo/detección.

## Prueba rápida
- Flujo IMU → Detector → Queue → TX → RX → UI y medir latencia básica.
