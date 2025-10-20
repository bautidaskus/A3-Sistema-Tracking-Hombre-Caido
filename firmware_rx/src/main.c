#include "firmware_rx/src/app_rx.h"

#if !APP_USE_FREERTOS
#include <stddef.h>

int main(void) {
  app_rx_init();
  tsk_lora_rx(NULL);
  tsk_ui(NULL);
  return 0;
}
#endif
