#include "SimpleLora.h"
using namespace SimpleLora;

void setup() {
  Serial.begin(115200);

  Config cfg;
  cfg.frequency = 915E6; // o 868E6 según tu setup
  cfg.sck=5; cfg.miso=19; cfg.mosi=27; cfg.ss=18; cfg.rst=14; cfg.dio0=26;
  if (!begin(cfg)) {
    Serial.println("LoRa init FAILED");
    while (1) delay(1000);
  }
  Serial.println("LoRa OK (RX esperando)");
}

void loop() {
  int rssi; float snr;
  // timeoutMs=0 → espera indefinidamente
  String msg = receiveString(/*timeoutMs=*/0, &rssi, &snr);
  if (msg.length()) {
    Serial.print("RX: "); Serial.print(msg);
    Serial.print(" | RSSI="); Serial.print(rssi);
    Serial.print(" dBm SNR="); Serial.println(snr, 1);
  }
}