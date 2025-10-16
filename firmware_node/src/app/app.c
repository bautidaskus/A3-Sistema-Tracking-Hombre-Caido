#include "firmware_node/src/app/app.h"

// Nota: Este archivo es un esqueleto para guiar la implementación.
// No incluye FreeRTOS para mantenerlo portable. Reemplazar con includes reales
// y crear tareas/colas según el RTOS/MCU elegido.

/* Pseudocódigo sugerido (MVP)

app_init() {
  // 1) init drivers
  //    imu_init(); lora_init(LORA_FREQ_HZ, LORA_SF, LORA_BW_KHZ, LORA_POUT_DBM, LORA_CRC_ON);

  // 2) init services
  //    fall_detector_init( { Athr, Ithr, Tidle, fs } ); alert_queue_init(4);

  // 3) crear tareas (prioridades: TX > SampleDetect > Blink)
  //    xTaskCreate(tsk_alert_tx, ... alta);
  //    xTaskCreate(tsk_sample_detect, ... alta-1);
  //    xTaskCreate(tsk_blink, ... baja);
}

tsk_sample_detect(void*) {
  // loop: cada 10 ms (100 Hz)
  //   imu_read(&s);
  //   if (fall_detector_feed(&s, &evt)) { alert_queue_push(&evt); /* opcional: notificar a TX */ }
  //   delay_until(10 ms);
}

tsk_alert_tx(void*) {
  // loop:
  //   if (alert_queue_pop(&evt, 50)) {
  //     uint8_t buf[16]; size_t n = pkt_encode_alert(&evt, buf, sizeof(buf));
  //     if (!lora_tx(buf, n, LORA_TX_TIMEOUT_MS)) {
  //       /* opcional: un reintento breve */
  //     }
  //   }
}

tsk_blink(void*) {
  // loop: toggle LED cada 500 ms
}

*/

void app_init(void) {}
void tsk_sample_detect(void* arg) { (void)arg; }
void tsk_alert_tx(void* arg)     { (void)arg; }
void tsk_blink(void* arg)        { (void)arg; }

