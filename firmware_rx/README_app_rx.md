# README — Nodo Receptor (RX LoRa)

**Objetivo:** escuchar P2P, decodificar alertas y mostrarlas.

## Tareas
- `tsk_lora_rx` (ALTA): `lora_rx()` + `pkt_decode_alert()`
- `tsk_ui` (MEDIA/BAJA): serial/display

## Flujo
1) Llega paquete → `pkt_decode_alert()` valida CRC y TYPE=0xFA.  
2) Mostrar “ALERTA HOMBRE CAÍDO” + timestamp + RSSI.

## Pruebas rápidas
- Contar recibidos con CRC OK vs. errores.
- Tasa de paquetes perdidos bajo SF7/BW125k.
