#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Inicializa el módulo LoRa con parámetros básicos
bool lora_init(uint32_t freq_hz, uint8_t sf, uint8_t bw_khz, int8_t pwr_dbm, bool crc_on);

// Transmite un buffer. Debe respetar timeout_ms (no bloquear indefinidamente)
bool lora_tx(const uint8_t* buf, size_t len, uint32_t timeout_ms);

// Recibe en un buffer con timeout_ms. Retorna true si hay paquete válido.
bool lora_rx(uint8_t* buf, size_t maxlen, uint32_t timeout_ms);

