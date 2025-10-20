#pragma once
#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>

namespace SimpleLora {

struct Config {
  long   frequency   = 915E6;
  int    sck         = 5;
  int    miso        = 19;
  int    mosi        = 27;
  int    ss          = 18;
  int    rst         = 14;
  int    dio0        = 26;
  long   bandwidth   = 125E3;
  uint8_t spreading  = 7;      // SF7..SF12
  uint8_t codingRate = 5;      // 5..8 -> (4/5 .. 4/8)
  int    txPower     = 14;
  bool   usePABoost  = true;
  uint8_t syncWord   = 0x12;
};

bool begin(const Config& cfg);

bool send(const uint8_t* data, size_t len);
inline bool sendString(const String& s) {
  return send(reinterpret_cast<const uint8_t*>(s.c_str()), s.length());
}

int receive(uint8_t* buf, size_t maxLen,
            unsigned long timeoutMs = 0,
            int* rssiOut = nullptr, float* snrOut = nullptr);

String receiveString(unsigned long timeoutMs = 0,
                     int* rssiOut = nullptr, float* snrOut = nullptr);

} // namespace SimpleLora
