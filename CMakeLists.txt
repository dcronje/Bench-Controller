# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.0)
set(toolchainVersion 13_3_Rel1)
set(picotoolVersion 2.1.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/cjson)
add_library(cjson lib/cjson/cJSON.c)

# Include FreeRTOS Kernel if needed
include($ENV{FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake)

project(bench-controller C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib/onewire)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib/lcdgfx)

add_executable(bench-controller 
    main.cpp
    src/wifi.cpp
    src/settings.cpp
    src/dhcpserver.c
    src/httpserver.cpp
    src/control.cpp
    src/sensors.cpp
    src/compressor-status.cpp
    src/display.cpp
    src/interaction.cpp
)

pico_set_program_name(bench-controller "bench-controller")
pico_set_program_version(bench-controller "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(bench-controller 1)
pico_enable_stdio_usb(bench-controller 0)

# Add the standard include files to the build
target_include_directories(bench-controller PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${CMAKE_CURRENT_LIST_DIR}/lib/lcdgfx/src
)

# Add the standard library to the build
target_link_libraries(bench-controller
    pico_stdlib
    pico_cyw43_arch_lwip_sys_freertos
    pico_lwip_http
    pico_lwip_mdns
    hardware_i2c
    FreeRTOS-Kernel-Heap4
    cjson
    pico_one_wire
    lcdgfx
)

pico_add_extra_outputs(bench-controller)

