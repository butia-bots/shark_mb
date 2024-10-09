cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)

project(SharkMBEmergencyButton)

set(PICO_BOARD pico)

set(PICO_CXX_ENABLE_EXCEPTIONS 1)
pico_sdk_init()

add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} pico_stdlib )
pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)
pico_add_extra_outputs(${PROJECT_NAME})