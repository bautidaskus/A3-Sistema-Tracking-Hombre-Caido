#include "SimpleLora.h"

namespace SimpleLora {

static Config g_cfg;

bool begin(const Config& cfg) {
  g_cfg = cfg;

  LoRa.setPins(g_cfg.ss, g_cfg.rst, g_cfg.dio0);
  SPI.begin(g_cfg.sck, g_cfg.miso, g_cfg.mosi, g_cfg.ss);

  if (!LoRa.begin(g_cfg.frequency)) return false;

  LoRa.setSignalBandwidth(g_cfg.bandwidth);
  LoRa.setSpreadingFactor(g_cfg.spreading);
  LoRa.setCodingRate4(g_cfg.codingRate);
  LoRa.setSyncWord(g_cfg.syncWord);

  if (g_cfg.usePABoost) {
  #ifdef PA_OUTPUT_PA_BOOST_PIN
    LoRa.setTxPower(g_cfg.txPower, PA_OUTPUT_PA_BOOST_PIN);
  #else
    LoRa.setTxPower(g_cfg.txPower);
  #endif
  } else {
    LoRa.setTxPower(g_cfg.txPower);
  }

  LoRa.receive();
  return true;
}

bool send(const uint8_t* data, size_t len) {
  LoRa.idle();
  (void)LoRa.beginPacket();
  size_t written = LoRa.write(data, len);
  int endOk = LoRa.endPacket();   // bloquea hasta terminar TX
  LoRa.receive();
  return (written == len) && (endOk == 1);
}

int receive(uint8_t* buf, size_t maxLen,
            unsigned long timeoutMs, int* rssiOut, float* snrOut) {
  LoRa.receive();
  unsigned long t0 = millis();
  for (;;) {
    int packetSize = LoRa.parsePacket();
    if (packetSize > 0) {
      int i = 0;
      while (LoRa.available() && i < (int)maxLen) buf[i++] = (uint8_t)LoRa.read();
      if (rssiOut) *rssiOut = LoRa.packetRssi();
      if (snrOut)  *snrOut  = LoRa.packetSnr();
      return i;
    }
    if (timeoutMs > 0 && (millis() - t0) >= timeoutMs) return 0;
    delay(1); yield();
  }
}

String receiveString(unsigned long timeoutMs, int* rssiOut, float* snrOut) {
  static char sbuf[256];
  int n = receive(reinterpret_cast<uint8_t*>(sbuf), sizeof(sbuf)-1,
                  timeoutMs, rssiOut, snrOut);
  if (n <= 0) return String();
  sbuf[n] = '\0';
  return String(sbuf);
}

} // namespace SimpleLora
