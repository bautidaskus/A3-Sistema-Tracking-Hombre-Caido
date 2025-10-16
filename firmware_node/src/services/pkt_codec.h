#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "firmware_node/src/services/fall_detector.h"

#define PKT_TYPE_ALERT 0xFA
#define PKT_VER        0x01

size_t pkt_encode_alert(const fall_event_t* e, uint8_t* out, size_t max);
bool   pkt_decode_alert(const uint8_t* in, size_t len, fall_event_t* e);

