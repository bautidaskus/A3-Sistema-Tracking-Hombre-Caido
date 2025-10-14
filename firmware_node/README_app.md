# README — Aplicación (RTOS y Orquestación)

**Objetivo:** crear tareas, configurar prioridades/timers/colas y orquestar el flujo.

## Tareas (Nodo móvil)
- `tsk_fall_sample` (ALTA): lee IMU 100–200 Hz
- `tsk_fall_detect` (MUY ALTA): confirma patrón
- `tsk_alert_tx` (MÁXIMA): empaqueta y **LoRa TX**
- `tsk_housekeeping` (BAJA): LED/log

## Estados
`INIT → RUN → ERROR(retry)`

## Garantías RT
- Confirmación → `lora_tx()` ≤ 300 ms (A3: < 1 s).
- Timeouts cortos + backoff breve; nada bloquea detectores.
- Opción B (detector en sample) para menor jitter.

## Pruebas rápidas
- Smoke completo IMU→Detector→Queue→TX→RX→UI.
- Medición de latencia con timestamps.
