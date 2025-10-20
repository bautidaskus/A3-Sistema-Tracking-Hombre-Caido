idf_component_register(
  SRCS
    "../src/app/app.c"
    "../src/app/app_entry.c"
    "../src/drivers/imu_accel.c"
    "../src/drivers/lora_radio.c"
    "../src/services/alert_queue.c"
    "../src/services/fall_detector.c"
    "../src/services/pkt_codec.c"
  INCLUDE_DIRS
    "../src"
    "../src/app"
    "../src/drivers"
    "../src/services"
    "../../config"
  REQUIRES
    driver
)

target_compile_definitions(${COMPONENT_TARGET} PRIVATE APP_USE_FREERTOS=1)
