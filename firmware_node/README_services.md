# README — Servicios (Dominio)

Objetivo: proveer lógica mínima, determinista y sin dependencias de hardware.

## fall_detector (MVP)
- Entrada: `accel_raw_t { int16_t ax, ay, az; }` (IMU en crudo) o |a| en centi-g.
- Salida: `fall_event_t` cuando se cumple patrón simple: pico > `Athr` seguido de inmovilidad `Tidle_ms`.
- API propuesta:
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
  uint8_t fs_hz;          // 100 Hz (MVP)
} fall_cfg_t;

void fall_detector_init(const fall_cfg_t* cfg);
bool fall_detector_feed(const accel_raw_t* s, fall_event_t* out_event);
```
- Complejidad: O(1) por muestra; sin asignación dinámica; sin `printf`.

## alert_queue
- Desacopla detector (productor) de radio TX (consumidor).
- Implementada sobre `QueueHandle_t` cuando `APP_USE_FREERTOS=1`, con política de descartar el más antiguo si la cola está llena para no bloquear la tarea de detección.
- API propuesta:
```
void alert_queue_init(size_t capacity);
bool alert_queue_push(const fall_event_t* e);                // ISR-safe si aplica
bool alert_queue_pop (fall_event_t* e, uint32_t timeout_ms); // timeout corto
```
- Implementación típica: envoltura sobre `QueueHandle_t` de FreeRTOS (a definir en app).

## pkt_codec (uplink)
- Formato mínimo (MVP):
  - `TYPE=0xFA` (1B), `VER=0x01` (1B)
  - `epoch_ms` (4B LE)
  - `ax_peak_centi_g` (2B LE)
  - `idle_ms` (2B LE)
  - `CRC8` (1B, polinomio 0x07) — opcionalmente XOR simple si se complica.
- API propuesta:
```
size_t pkt_encode_alert(const fall_event_t* e, uint8_t* out, size_t max);
bool   pkt_decode_alert(const uint8_t* in, size_t len, fall_event_t* e);
```

Notas
- Los servicios no conocen hardware; sólo estructuras de datos.
- Parámetros por defecto en `config/fall_params.h` y `config/radio_params.h`.
