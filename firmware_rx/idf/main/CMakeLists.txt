idf_component_register(
  SRCS
    "../src/app_rx.c"
    "../src/app_rx_entry.c"
    "../../firmware_node/src/drivers/lora_radio.c"
    "../../firmware_node/src/services/pkt_codec.c"
  INCLUDE_DIRS
    "../src"
    "../../firmware_node/src"
    "../../firmware_node/src/drivers"
    "../../firmware_node/src/services"
    "../../config"
  REQUIRES
    driver
)

target_compile_definitions(${COMPONENT_TARGET} PRIVATE APP_USE_FREERTOS=1)
