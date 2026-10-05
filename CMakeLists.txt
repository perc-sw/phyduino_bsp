# Repository CMake entry point. Two builds, chosen by preset:
#
#   cmake --workflow --preset phyduino_pico   Arm verification build: core,
#       variant, libraries and every sketch compiled for the board with strict
#       warnings, using the flags in platform/platform.txt + boards.txt and the
#       committed SDK bundle.          Guide: docs/maintenance/cmake-verify.md
#
#   cmake --workflow --preset host-tests      PC-side unit tests: the same
#       sources compiled for this PC against a fake pico-sdk.
#                                      Guide: docs/maintenance/host-tests.md
#
# Neither is part of the release zip. (The SDK bundle itself is built by the
# separate project in sdk-builder/.)
cmake_minimum_required(VERSION 3.21)

option(PHYDUINO_HOST_TESTS "Build the PC-side unit tests instead of the Arm build" OFF)
option(PHYDUINO_WERROR "Treat warnings in Phyduino code as errors" ON)

if(PHYDUINO_HOST_TESTS)
    project(phyduino_host_tests LANGUAGES C CXX)
    get_filename_component(PHYDUINO_PLATFORM_DIR "${CMAKE_CURRENT_LIST_DIR}/platform" ABSOLUTE)
    enable_testing()
    add_subdirectory(tests/host)
    return()
endif()

if(NOT CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/cmake/arm-none-eabi.cmake")
endif()

project(phyduino_verify LANGUAGES C CXX)

set(PHYDUINO_BOARD "phyduino_pico" CACHE STRING "Board id from platform/boards.txt")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(cmake/phyduino.cmake)
phyduino_setup(${PHYDUINO_BOARD})

# Optional .uf2 output, using the same picotool as the Arduino build.
arduino_property(picotool_dir tools.picotool.path)
find_program(PHYDUINO_PICOTOOL picotool HINTS "${picotool_dir}")

file(GLOB sketch_dirs LIST_DIRECTORIES true
    "${CMAKE_CURRENT_LIST_DIR}/tests/sketches/*"
    "${CMAKE_CURRENT_LIST_DIR}/platform/libraries/*/examples/*")
foreach(dir IN LISTS sketch_dirs)
    if(IS_DIRECTORY "${dir}")
        phyduino_add_sketch("${dir}")
    endif()
endforeach()
