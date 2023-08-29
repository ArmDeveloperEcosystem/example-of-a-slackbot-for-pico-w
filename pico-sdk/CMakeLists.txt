cmake_minimum_required(VERSION 3.12)

include(pico_sdk_import.cmake)

project(picow_slack_bot)

pico_sdk_init()

set(FREERTOS_KERNEL_PATH ${CMAKE_CURRENT_LIST_DIR}/lib/FreeRTOS-Kernel)
include("${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake")
include(${CMAKE_CURRENT_LIST_DIR}/lib/coreHTTP/httpFilePaths.cmake)

add_library(coreHTTP
        ${HTTP_SOURCES}
)

target_include_directories(coreHTTP PUBLIC
        ${HTTP_INCLUDE_PUBLIC_DIRS}
        ${CMAKE_CURRENT_LIST_DIR}/config
)

add_library(cJSON
        ${CMAKE_CURRENT_LIST_DIR}/lib/cJSON/cJSON.c
)

target_include_directories(cJSON PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/lib/cJSON/
)

add_executable(picow_slack_bot
        ${CMAKE_CURRENT_LIST_DIR}/https_client.c
        ${CMAKE_CURRENT_LIST_DIR}/main.c
        ${CMAKE_CURRENT_LIST_DIR}/slack_client.c
        ${CMAKE_CURRENT_LIST_DIR}/tls_client.c
        ${CMAKE_CURRENT_LIST_DIR}/wss_client.c
)

target_include_directories(picow_slack_bot PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/config
)

target_compile_definitions(picow_slack_bot PRIVATE
        WIFI_SSID=\"${WIFI_SSID}\"
        WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
        SLACK_APP_TOKEN=\"${SLACK_APP_TOKEN}\"
        SLACK_BOT_TOKEN=\"${SLACK_BOT_TOKEN}\"
)

target_link_libraries(picow_slack_bot PUBLIC
        pico_cyw43_arch_lwip_sys_freertos
        pico_lwip_mbedtls
        pico_mbedtls
        pico_stdlib
        FreeRTOS-Kernel-Heap4 # FreeRTOS kernel and dynamic heap
        coreHTTP
        cJSON
)

pico_enable_stdio_usb(picow_slack_bot 1)
pico_enable_stdio_uart(picow_slack_bot 0)

pico_add_extra_outputs(picow_slack_bot)
