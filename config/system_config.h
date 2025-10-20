#pragma once

#ifndef APP_USE_FREERTOS
#ifdef ESP_PLATFORM
#define APP_USE_FREERTOS 1
#else
#define APP_USE_FREERTOS 0
#endif
#endif
