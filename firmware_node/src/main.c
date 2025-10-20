#include "firmware_node/src/app/app.h"

#if !APP_USE_FREERTOS
#include <stddef.h>

int main(void) {
  app_init();
  tsk_sample_detect(NULL);
  tsk_alert_tx(NULL);
  tsk_blink(NULL);
  return 0;
}
#endif
