# sdk-builder: compiles the pico-sdk into a self-contained bundle that the
# Arduino core links against. Output: platform/system/<board>/, or
# platform/system/<board>_freertos/ with PHYDUINO_OS=freertos (the SDK plus the
# FreeRTOS kernel, for Tools > Operating System > FreeRTOS, and the CMSIS-RTOS2
# layer in lib/libcmsis_os2.a for the CMSIS-RTOS2 option).
#
# Usage (from this folder):  cmake --workflow --preset phyduino_pico
# Recipe:                    docs/maintenance/update-pico-sdk.html
cmake_minimum_required(VERSION 3.20)

if(NOT PHYDUINO_BOARD)
    message(FATAL_ERROR "Set PHYDUINO_BOARD (use a preset from CMakePresets.json)")
endif()
set(PHYDUINO_OS "none" CACHE STRING "Operating system compiled into the bundle: none or freertos")
if(NOT PHYDUINO_OS MATCHES "^(none|freertos)$")
    message(FATAL_ERROR "PHYDUINO_OS must be none or freertos, not '${PHYDUINO_OS}'")
endif()

set(REPO_ROOT              ${CMAKE_CURRENT_LIST_DIR}/..)
set(PICO_SDK_PATH          ${REPO_ROOT}/extern/pico-sdk)
set(PICO_BOARD             ${PHYDUINO_BOARD})
set(PICO_BOARD_HEADER_DIRS ${CMAKE_CURRENT_LIST_DIR}/boards)
set(BUNDLE_DIR             ${REPO_ROOT}/platform/system/${PHYDUINO_BOARD})
if(PHYDUINO_OS STREQUAL "freertos")
    string(APPEND BUNDLE_DIR "_freertos")
endif()

include(${PICO_SDK_PATH}/pico_sdk_init.cmake)
project(phyduino_sdk_builder C CXX ASM)
pico_sdk_init()

# --- FreeRTOS (PHYDUINO_OS=freertos) -------------------------------------------
# The RP2040 port's library.cmake defines the FreeRTOS-Kernel-* targets. Linking
# one adds PICO_CONFIG_RTOS_ADAPTER_HEADER to every SDK source, so SDK mutexes
# and sleep_ms() cooperate with the scheduler; that is why FreeRTOS needs its
# own build of the SDK rather than a library compiled with the sketch.
if(PHYDUINO_OS STREQUAL "freertos")
    set(FREERTOS_KERNEL_PATH           ${REPO_ROOT}/extern/FreeRTOS-Kernel)
    set(FREERTOS_CONFIG_FILE_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/freertos)
    if(PICO_PLATFORM STREQUAL "rp2040")
        include(${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/library.cmake)
    else()
        message(FATAL_ERROR "sdk-builder: no FreeRTOS port wired up for ${PICO_PLATFORM} yet")
    endif()
endif()

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
if(PHYDUINO_OS STREQUAL "freertos")
    list(APPEND SDK_COMPONENTS FreeRTOS-Kernel-Heap3)  # heap_3: FreeRTOS allocates with malloc()
endif()
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

# --- libcmsis_os2.a (PHYDUINO_OS=freertos) -------------------------------------
# ARM's CMSIS-RTOS2 API on FreeRTOS (cmsis-rtos2/README.md). A separate archive,
# linked only by the CMSIS-RTOS2 menu option: libpico.a is linked whole, and
# plain FreeRTOS sketches should not get the wrapper's weak hook functions.
# Compiled with the include dirs and defines of an SDK executable (probe, below)
# but not linked to the SDK, which would add the SDK's sources to the archive.
# CMSIS_device_header only supplies the CMSIS compiler intrinsics
# (cmsis_device_rp2040.h).
if(PHYDUINO_OS STREQUAL "freertos")
    set(CMSIS_RTOS2_DIR ${CMAKE_CURRENT_LIST_DIR}/cmsis-rtos2)
    add_library(cmsis_os2 STATIC
        ${CMSIS_RTOS2_DIR}/CMSIS-FreeRTOS/cmsis_os2.c
        ${CMSIS_RTOS2_DIR}/os_tick_rp2040.c
    )
    target_include_directories(cmsis_os2 PRIVATE
        ${CMSIS_RTOS2_DIR}
        ${CMSIS_RTOS2_DIR}/CMSIS_6
        ${CMSIS_RTOS2_DIR}/CMSIS-FreeRTOS
        ${PICO_SDK_PATH}/src/rp2_common/cmsis/stub/CMSIS/Core/Include
        $<TARGET_PROPERTY:probe,INCLUDE_DIRECTORIES>
    )
    target_compile_definitions(cmsis_os2 PRIVATE
        CMSIS_device_header="cmsis_device_rp2040.h"
        $<TARGET_PROPERTY:probe,COMPILE_DEFINITIONS>
    )
    target_compile_options(cmsis_os2 PRIVATE
        $<TARGET_PROPERTY:probe,COMPILE_OPTIONS>
        -ffile-prefix-map=${PICO_SDK_PATH}=pico-sdk
        -ffile-prefix-map=${FREERTOS_KERNEL_PATH}=FreeRTOS-Kernel
        -ffile-prefix-map=${CMAKE_CURRENT_LIST_DIR}=sdk-builder
        -ffile-prefix-map=${CMAKE_BINARY_DIR}=build
    )
    add_dependencies(cmsis_os2 pico)   # generated SDK headers (config_autogen.h)
    set(CMSIS_OS2_LIB "$<TARGET_FILE:cmsis_os2>")
endif()

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

# Lets the core and sketches tell the two bundles apart (#if PHYDUINO_FREERTOS).
if(PHYDUINO_OS STREQUAL "freertos")
    target_compile_definitions(pico PUBLIC PHYDUINO_FREERTOS=1)
    target_compile_options(pico PRIVATE -ffile-prefix-map=${FREERTOS_KERNEL_PATH}=FreeRTOS-Kernel)
endif()

file(GENERATE OUTPUT ${CMAKE_BINARY_DIR}/bundle_inputs.cmake CONTENT
"set(BOARD        [==[${PHYDUINO_BOARD}]==])
set(PLATFORM     [==[${PICO_PLATFORM}]==])
set(SDK_PATH     [==[${PICO_SDK_PATH}]==])
set(BINARY_DIR   [==[${CMAKE_BINARY_DIR}]==])
set(OS           [==[${PHYDUINO_OS}]==])
set(FREERTOS_PATH [==[${FREERTOS_KERNEL_PATH}]==])
set(CMSIS_RTOS2_DIR [==[${CMSIS_RTOS2_DIR}]==])
set(CMSIS_OS2_LIB [==[${CMSIS_OS2_LIB}]==])
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
if(TARGET cmsis_os2)
    add_dependencies(bundle cmsis_os2)
endif()
