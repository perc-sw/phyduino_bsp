# sdk-builder: compiles the pico-sdk into a self-contained bundle that the
# Arduino core links against. Output: platform/system/<board>/
#
# Usage (from this folder):  cmake --workflow --preset phyduino_pico
# Recipe:                    docs/maintenance/update-pico-sdk.html
cmake_minimum_required(VERSION 3.20)

if(NOT PHYDUINO_BOARD)
    message(FATAL_ERROR "Set PHYDUINO_BOARD (use a preset from CMakePresets.json)")
endif()

set(REPO_ROOT              ${CMAKE_CURRENT_LIST_DIR}/..)
set(PICO_SDK_PATH          ${REPO_ROOT}/extern/pico-sdk)
set(PICO_BOARD             ${PHYDUINO_BOARD})
set(PICO_BOARD_HEADER_DIRS ${CMAKE_CURRENT_LIST_DIR}/boards)
set(BUNDLE_DIR             ${REPO_ROOT}/platform/system/${PHYDUINO_BOARD})

include(${PICO_SDK_PATH}/pico_sdk_init.cmake)
project(phyduino_sdk_builder C CXX ASM)
pico_sdk_init()

# Deterministic archives (no timestamps), so the same sources always give the
# same committed libpico.a.
foreach(lang C CXX ASM)
    set(CMAKE_${lang}_ARCHIVE_CREATE "<CMAKE_AR> qcD <TARGET> <LINK_FLAGS> <OBJECTS>")
    set(CMAKE_${lang}_ARCHIVE_APPEND "<CMAKE_AR> qD <TARGET> <LINK_FLAGS> <OBJECTS>")
    set(CMAKE_${lang}_ARCHIVE_FINISH "<CMAKE_RANLIB> -D <TARGET>")
endforeach()

# --- SDK components included in libpico.a -----------------------------------
# Anything listed here can be used directly from a sketch (#include "hardware/pwm.h").
# Components that do not exist on the current chip are skipped.
set(SDK_COMPONENTS
    pico_stdlib
    pico_multicore
    pico_unique_id
    pico_bootrom
    pico_flash
    pico_i2c_slave
    pico_rand
    hardware_adc
    hardware_dma
    hardware_exception
    hardware_flash
    hardware_i2c
    hardware_interp
    hardware_pio
    hardware_pwm
    hardware_rtc
    hardware_spi
    hardware_uart
    hardware_watchdog
    tinyusb_device
)
foreach(component IN LISTS SDK_COMPONENTS)
    if(TARGET ${component})
        list(APPEND SDK_LIBS ${component})
    else()
        message(STATUS "sdk-builder: ${component} not available on ${PICO_PLATFORM}, skipped")
    endif()
endforeach()

# --- libpico.a ---------------------------------------------------------------
add_library(pico STATIC ${CMAKE_CURRENT_LIST_DIR}/empty.c)
target_link_libraries(pico PUBLIC ${SDK_LIBS})
target_include_directories(pico PUBLIC ${CMAKE_CURRENT_LIST_DIR}/config)
# Strip machine-specific paths so a rebuild elsewhere gives the same library.
# gcc uses the last matching map, so the build dir (inside sdk-builder/) comes last.
target_compile_options(pico PRIVATE
    -ffile-prefix-map=${PICO_SDK_PATH}=pico-sdk
    -ffile-prefix-map=${CMAKE_CURRENT_LIST_DIR}=sdk-builder
    -ffile-prefix-map=${CMAKE_BINARY_DIR}=build
)

# --- probe -------------------------------------------------------------------
# Never built. Exists so CMake can resolve the full set of include dirs,
# defines and link options an SDK executable would get.
add_executable(probe EXCLUDE_FROM_ALL ${CMAKE_CURRENT_LIST_DIR}/empty.c)
target_link_libraries(probe pico)

# Arduino's Serial owns USB and the UARTs, so SDK stdio has no drivers.
# Set on both targets so the exported defines match what was compiled.
foreach(target pico probe)
    pico_enable_stdio_usb(${target} 0)
    pico_enable_stdio_uart(${target} 0)
endforeach()

file(GENERATE OUTPUT ${CMAKE_BINARY_DIR}/bundle_inputs.cmake CONTENT
"set(BOARD        [==[${PHYDUINO_BOARD}]==])
set(PLATFORM     [==[${PICO_PLATFORM}]==])
set(SDK_PATH     [==[${PICO_SDK_PATH}]==])
set(BINARY_DIR   [==[${CMAKE_BINARY_DIR}]==])
set(BOARD_HEADER [==[${CMAKE_CURRENT_LIST_DIR}/boards/${PHYDUINO_BOARD}.h]==])
set(LIBPICO      [==[$<TARGET_FILE:pico>]==])
set(BOOT2_OBJ    [==[$<TARGET_OBJECTS:bs2_default_library>]==])
set(INCLUDE_DIRS [==[$<TARGET_PROPERTY:probe,INCLUDE_DIRECTORIES>]==])
set(DEFINES      [==[$<TARGET_PROPERTY:probe,COMPILE_DEFINITIONS>]==])
set(LINK_OPTIONS [==[$<TARGET_PROPERTY:probe,LINK_OPTIONS>]==])
")

add_custom_target(bundle ALL
    COMMAND ${CMAKE_COMMAND}
            -DINPUTS=${CMAKE_BINARY_DIR}/bundle_inputs.cmake
            -DOUT=${BUNDLE_DIR}
            -P ${CMAKE_CURRENT_LIST_DIR}/export.cmake
    COMMENT "Writing SDK bundle to ${BUNDLE_DIR}"
    VERBATIM
)
add_dependencies(bundle pico bs2_default_library)
