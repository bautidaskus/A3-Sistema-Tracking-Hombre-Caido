# README — Nodo Receptor (LoRa RX)

Objetivo: escuchar P2P, decodificar alertas y mostrarlas.

## Tareas (MVP)
- `tsk_lora_rx` (ALTA): `lora_rx()` + `pkt_decode_alert()`.
- `tsk_ui` (MEDIA/BAJA): imprimir/LED “ALERTA HOMBRE CAÍDO”.

## Flujo
1) Llega paquete → `pkt_decode_alert()` valida TYPE/VER y campos.
2) Mostrar “ALERTA HOMBRE CAÍDO” + timestamp + RSSI (si disponible).

## Pruebas rápidas
- Contar recibidos con CRC OK vs. errores.
- Tasa de paquetes perdidos bajo SF7/BW125k.

