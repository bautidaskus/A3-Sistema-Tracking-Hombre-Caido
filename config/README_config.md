# README — Configuración

Una sola fuente de verdad para parámetros del sistema (MVP):

- `fall_params.h`: umbrales y ventanas del detector (pico, inmovilidad, fs).
- `radio_params.h`: 915 MHz, SF7, BW125 kHz, potencia y timeouts.
- `FreeRTOSConfig.h`: tamaños de stack, prioridades, colas (cuando se integre RTOS).

> Evitar duplicar constantes en el código; incluir estos headers.

