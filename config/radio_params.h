#pragma once

// Parámetros por defecto de radio (orientativos para MVP)

#define LORA_FREQ_HZ        915000000UL  // 915 MHz
#define LORA_SF             7            // SF7
#define LORA_BW_KHZ         125          // BW 125 kHz
#define LORA_POUT_DBM       15           // Potencia de salida
#define LORA_CRC_ON         1            // CRC activado

// Timeouts sugeridos (ms)
#define LORA_TX_TIMEOUT_MS   100
#define LORA_RX_TIMEOUT_MS   200

