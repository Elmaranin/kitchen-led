cmake_minimum_required(VERSION 3.5)
set(PROJECT_VER "0.4.0" CACHE STRING "Firmware version")
if(NOT PROJECT_VER MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+$")
    message(FATAL_ERROR "PROJECT_VER must use the format major.minor.patch.")
endif()

if(NOT EXISTS "$ENV{IDF_PATH}/components/esp8266")
    message(FATAL_ERROR "Set IDF_PATH to ESP8266_RTOS_SDK v3.4, not ESP32 ESP-IDF.")
endif()

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(kitchen_led)
