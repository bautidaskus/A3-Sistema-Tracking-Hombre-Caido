#include <Arduino.h>
#include "SimpleLora.h"                 // tu wrapper
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

using namespace SimpleLora;

static void TxTask(void *pv);           // prototipo

void setup() {
  Serial.begin(115200);
  delay(500);

  // Config TTGO LoRa32 v1 (ajusta si tu placa difiere)
  Config cfg;
  cfg.frequency = 915E6;                // o 868E6 si usás 868
  cfg.sck=5; cfg.miso=19; cfg.mosi=27;  // SPI
  cfg.ss=18; cfg.rst=14; cfg.dio0=26;
  cfg.bandwidth = 125E3;
  cfg.spreading = 7;                    // SF7
  cfg.codingRate = 5;                   // 4/5
  cfg.syncWord = 0x12;                  // ¡igual en RX!

  if (!begin(cfg)) {
    Serial.println("LoRa init FAILED");
    for(;;) delay(1000);
  }
  Serial.println("LoRa OK - creando task TX");

  // Crea la tarea de transmisión (stack 4 KB, prio 2, core 1)
  xTaskCreatePinnedToCore(
      TxTask, "TxTask",
      4096, nullptr, 2, nullptr, 1);
}

void loop() {
  // vacío: todo corre en FreeRTOS
}

static void TxTask(void *pv) {
  const TickType_t period = pdMS_TO_TICKS(1000); // cada 1 s
  TickType_t last = xTaskGetTickCount();
  uint32_t seq = 0;

  for(;;){
    char buf[64];
    int n = snprintf(buf, sizeof(buf), "HELLO %lu", (unsigned long)seq++);

    bool ok = send(reinterpret_cast<const uint8_t*>(buf), (size_t)n);
    Serial.printf("[TX] %s | %s\r\n", buf, ok ? "OK" : "FAIL");

    vTaskDelayUntil(&last, period); // periodicidad exacta
  }
}
