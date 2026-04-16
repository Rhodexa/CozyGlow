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
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(Console C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(Console
        Console.c
        faders.c
        buttons.c
        leds.c
)

pico_set_program_name(Console "Console")
pico_set_program_version(Console "0.1")

# stdio over UART and USB are both disabled:
#   - UART0 is used for MIDI at 31250 baud, not stdio
#   - USB is managed directly by TinyUSB (MIDI or CDC depending on switch)
pico_enable_stdio_uart(Console 0)
pico_enable_stdio_usb(Console 1)

# Add the standard library to the build
target_link_libraries(Console
        pico_stdlib
        hardware_uart
        hardware_adc
        hardware_gpio
        hardware_pio
)

# Add the standard include files to the build
target_include_directories(Console PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)

pico_add_extra_outputs(Console)

