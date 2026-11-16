if(NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/../.secrets/ota_secret.h")
    message(FATAL_ERROR "Run python3 tools/configure.py before building to create the local OTA token.")
endif()

idf_component_register(SRCS "main.c" "kitchen_led.c" "ws2812.S" "network.c" "wifi_policy.c" "ota_server.c"
                       INCLUDE_DIRS "." "../.secrets"
                       EMBED_TXTFILES "update.html")
