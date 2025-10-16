#include "firmware_node/src/services/pkt_codec.h"
#include <string.h>

static uint8_t crc8(const uint8_t* data, size_t len) {
  uint8_t crc = 0x00; // inicialización habitual con polinomio 0x07
  for (size_t i = 0; i < len; ++i) {
    crc ^= data[i];
    for (int b = 0; b < 8; ++b) {
      if (crc & 0x80) crc = (crc << 1) ^ 0x07; else crc <<= 1;
    }
  }
  return crc;
}

size_t pkt_encode_alert(const fall_event_t* e, uint8_t* out, size_t max) {
  if (!e || !out) return 0;
  const size_t need = 1 + 1 + 4 + 2 + 2 + 1; // TYPE,VER,epoch,peak,idle,CRC
  if (max < need) return 0;
  size_t i = 0;
  out[i++] = PKT_TYPE_ALERT;
  out[i++] = PKT_VER;
  // epoch_ms (LE)
  out[i++] = (uint8_t)(e->epoch_ms & 0xFF);
  out[i++] = (uint8_t)((e->epoch_ms >> 8) & 0xFF);
  out[i++] = (uint8_t)((e->epoch_ms >> 16) & 0xFF);
  out[i++] = (uint8_t)((e->epoch_ms >> 24) & 0xFF);
  // ax_peak_centi_g (LE)
  out[i++] = (uint8_t)(e->ax_peak_centi_g & 0xFF);
  out[i++] = (uint8_t)((e->ax_peak_centi_g >> 8) & 0xFF);
  // idle_ms (LE)
  out[i++] = (uint8_t)(e->idle_ms & 0xFF);
  out[i++] = (uint8_t)((e->idle_ms >> 8) & 0xFF);
  // CRC8 sobre todo excepto el último byte (CRC)
  out[i] = crc8(out, i);
  i += 1;
  return i;
}

bool pkt_decode_alert(const uint8_t* in, size_t len, fall_event_t* e) {
  if (!in || !e) return false;
  const size_t need = 1 + 1 + 4 + 2 + 2 + 1;
  if (len < need) return false;
  if (in[0] != PKT_TYPE_ALERT || in[1] != PKT_VER) return false;
  const uint8_t crc_calc = crc8(in, need - 1);
  if (crc_calc != in[need - 1]) return false;
  size_t i = 2;
  // epoch_ms (LE)
  e->epoch_ms = (uint32_t)in[i] | ((uint32_t)in[i+1] << 8) | ((uint32_t)in[i+2] << 16) | ((uint32_t)in[i+3] << 24);
  i += 4;
  // ax_peak_centi_g (LE)
  e->ax_peak_centi_g = (int16_t)((uint16_t)in[i] | ((uint16_t)in[i+1] << 8));
  i += 2;
  // idle_ms (LE)
  e->idle_ms = (uint16_t)in[i] | ((uint16_t)in[i+1] << 8);
  return true;
}

