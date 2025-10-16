# A3 - Sistema de Tracking con Alerta de Hombre Caído

Sistema de tiempo real para detección y alerta de caídas humanas utilizando sensores IMU y comunicación LoRa.

## Descripción (MVP simple)

Objetivo mínimo: que el nodo portátil detecte una caída y envíe una alerta por LoRa, y que el receptor la muestre sin problemas. Sin complejidad extra.

- Nodo móvil: IMU a 100 Hz, detector simple “pico + 500 ms de inmovilidad”, y envío inmediato de alerta.
- Nodo receptor: escucha LoRa, valida paquete y muestra “ALERTA HOMBRE CAÍDO”.
- Sin ACK; opcional 1 reintento rápido si el radio está ocupado.
- Objetivo RT: detección confirmada → inicio de TX ≤ 300 ms (límite curso: < 1 s).

## Arquitectura mínima

```
Aplicación (App)    → Orquestación RTOS (tareas, prioridades)
Servicios (Dominio) → Detector, cola de alertas, codec de paquete
Drivers (HAL)       → IMU, LoRa, GPIO/LED (timeouts explícitos)
```

Tareas sugeridas (MVP):
- `tsk_sample_detect` (ALTA): lee IMU a 100 Hz y corre el detector.
- `tsk_alert_tx` (MÁXIMA): toma evento de la cola, codifica y llama a `lora_tx()`.
- `tsk_blink` (BAJA): LED de estado/log básico.

Receptor (MVP):
- `tsk_lora_rx` (ALTA): `lora_rx()` + `pkt_decode_alert()`.
- `tsk_ui` (MEDIA/BAJA): muestra la alerta (serial/LED).

## Estructura del proyecto

```
a3-hombre-caido/
  docs/
    A3_Arquitectura_Capas.md
  firmware_node/
    README_drivers.md
    README_services.md
    README_app.md
    src/
      drivers/
        imu_accel.h
        lora_radio.h
      services/
        fall_detector.h
        alert_queue.h
        pkt_codec.h
      app/
        app.h
        app.c
  firmware_rx/
    README_app_rx.md
  config/
    README_config.md
    fall_params.h
    radio_params.h
  README.md
```

## Pasos para el MVP

1) Drivers: `imu_accel` y `lora_radio` con timeouts (stub inicial).  
2) Servicios: `fall_detector` (umbral simple) + `alert_queue` + `pkt_codec` (paquete mínimo).  
3) App: crear tareas mínimas, prioridades y cola; medir latencia básica.  
4) Receptor: `lora_rx` + `pkt_decode_alert` + mostrar alerta.

## Documentación

- `docs/A3_Arquitectura_Capas.md`: diseño simplificado del MVP.  
- `firmware_node/README_drivers.md`: contratos de HAL.  
- `firmware_node/README_services.md`: APIs de servicios.  
- `firmware_node/README_app.md`: tareas y prioridades.  
- `firmware_rx/README_app_rx.md`: nodo receptor.  
- `config/README_config.md`: parámetros del sistema.

## Requisitos

- FreeRTOS.  
- Hardware: IMU (acelerómetro), módulo LoRa.  
- Frecuencia de muestreo: 100 Hz (MVP).  
- Radio: 915 MHz, SF7, BW125 kHz (orientativo).

Este proyecto es parte del curso “Sistemas de Tiempo Real” - UNLP.
