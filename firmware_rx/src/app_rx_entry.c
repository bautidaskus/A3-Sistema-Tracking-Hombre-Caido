#include "firmware_rx/src/app_rx.h"

#if APP_USE_FREERTOS
void app_main(void) {
  app_rx_init();
}
#endif

