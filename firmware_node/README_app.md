# README — Aplicación (RTOS y Orquestación)

Objetivo: crear tareas mínimas, configurar prioridades y orquestar el flujo con foco RT.

## Tareas (Nodo móvil, MVP)
- `tsk_sample_detect` (ALTA): lee IMU a 100 Hz y corre el detector.
- `tsk_alert_tx` (MÁXIMA): toma evento de la cola, codifica y llama a `lora_tx()`.
- `tsk_blink` (BAJA): LED/log básico.

## Estados
`INIT → RUN → ERROR(retry)`

## Garantías RT (MVP)
- Detección confirmada → inicio de TX ≤ 300 ms (límite curso: < 1 s).
- Timeouts cortos + reintento breve (opcional, 1 vez) en radio.
- Nada bloquea la tarea de sampleo/detección.

## Prueba rápida
- Flujo IMU → Detector → Queue → TX → RX → UI y medir latencia básica.

