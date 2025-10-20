#include "firmware_node/src/app/app.h"

#if APP_USE_FREERTOS
void app_main(void) {
  app_init();
}
#endif

