# A3 — Arquitectura por Capas (MVP)

Este documento resume las capas, responsabilidades, APIs y flujo mínimo para el MVP del sistema de Alerta de Hombre Caído. La idea es implementar rápido, con foco en tiempo real y sin complejidad extra.

---

## Visión general
- Drivers (HAL): IMU y LoRa. Primitivas mínimas con timeouts. Sin lógica de negocio.
- Servicios (Dominio): detector simple de caída, cola de alertas, codec de paquete.
- Aplicación (App): tareas RTOS, prioridades y orquestación.

Regla de dependencia: App → Servicios → Drivers.

---

## Drivers (HAL)
Responsabilidad: acceso a hardware con tiempos acotados.
- imu_accel
  - `bool imu_init(void);`
  - `bool imu_read(accel_raw_t* out);` // lectura cruda, O(us–ms)
  - Implementación ESP-IDF: I2C @400 kHz sobre GPIO21/22 leyendo MPU9250 (ver `config/board_pins.h`).
- lora_radio
  - `bool lora_init(uint32_t freq, uint8_t sf, uint8_t bw_khz, int8_t pwr_dbm, bool crc_on);`
  - `bool lora_tx(const uint8_t* buf, size_t len, uint32_t timeout_ms);`
  - `bool lora_rx(uint8_t* buf, size_t maxlen, uint32_t timeout_ms);`
  - Implementación ESP-IDF: SPI 8 MHz (GPIO5/18/19/27) + RST=14, DIO0=26.

Reglas: sin bloqueos indefinidos, sin `printf` en hot path, retornar `bool`/códigos.

---

## Servicios (Dominio)
### fall_detector (MVP)
Patrón: pico > umbral seguido de inmovilidad por Tidle.
```
typedef struct { int16_t ax, ay, az; } accel_raw_t;

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
bool fall_detector_feed(const accel_raw_t* s, fall_event_t* out_event);
```
O(1) por muestra. Sin asignación dinámica.

### alert_queue
Desacopla detector (productor) de TX (consumidor).
```
void alert_queue_init(size_t capacity);
bool alert_queue_push(const fall_event_t* e);
bool alert_queue_pop (fall_event_t* e, uint32_t timeout_ms);
```

### pkt_codec (uplink)
Formato mínimo:
- TYPE=0xFA (1B), VER=0x01 (1B)
- epoch_ms (4B LE), ax_peak_centi_g (2B LE), idle_ms (2B LE)
- CRC8 (1B, polinomio 0x07) — opcional XOR si hace falta simplificar
```
size_t pkt_encode_alert(const fall_event_t* e, uint8_t* out, size_t max);
bool   pkt_decode_alert(const uint8_t* in, size_t len, fall_event_t* e);
```

---

## Aplicación (RTOS y Orquestación)
Nodo móvil (MVP):
- `tsk_sample_detect` (ALTA): lee IMU a 100 Hz y corre el detector.
- `tsk_alert_tx` (MÁXIMA): pop cola → encode → `lora_tx()`.
- `tsk_blink` (BAJA): LED/log básico.

Receptor (MVP):
- `tsk_lora_rx` (ALTA): `lora_rx()` + `pkt_decode_alert()`
- `tsk_ui` (MEDIA/BAJA): muestra alerta

Estados: INIT → RUN → ERROR(retry sencillo).

---

## Tiempo real (MVP)
- Métrica: detección confirmada → inicio de TX ≤ 300 ms (límite curso: < 1 s)
- Prioridad: TX > Sample/Detect > Blink
- Timeouts cortos en radio. Sin bloqueos indefinidos.

---

## Pruebas mínimas
- Unitarias: `pkt_encode/decode` con un vector dorado.
- Integración: flujo IMU → Detector → Queue → TX → RX → UI.
- Medir latencia: timestamp en confirmación y antes de `lora_tx()`.

---

## Configuración
- `config/fall_params.h`: umbrales y ventanas por defecto.
- `config/radio_params.h`: freq, SF, BW, potencia, CRC, timeouts.
- `FreeRTOSConfig.h`: stacks, prioridades, colas (cuando se integre RTOS).
