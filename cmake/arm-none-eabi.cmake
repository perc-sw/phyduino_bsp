# CMake toolchain file: Arm GNU toolchain (arm-none-eabi) for the CMake verification build.
#
# Compiler location, first match wins:
#   1. -DPHYDUINO_TOOLCHAIN_PATH=<dir containing arm-none-eabi-gcc>
#   2. compiler.path in platform/platform.local.txt (same file the Arduino dev setup uses)
#   3. arm-none-eabi-gcc on PATH
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
# The bare-metal toolchain cannot link a test program without a linker script.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

if(NOT PHYDUINO_TOOLCHAIN_PATH)
    set(local_txt "${CMAKE_CURRENT_LIST_DIR}/../platform/platform.local.txt")
    if(EXISTS "${local_txt}")
        file(STRINGS "${local_txt}" line REGEX "^compiler\\.path=")
        if(line MATCHES "^compiler\\.path=(.+)$")
            string(REGEX REPLACE "[/\\\\]+$" "" path "${CMAKE_MATCH_1}")
            set(PHYDUINO_TOOLCHAIN_PATH "${path}" CACHE PATH "Directory containing arm-none-eabi-gcc")
        endif()
    endif()
endif()

if(PHYDUINO_TOOLCHAIN_PATH)
    set(prefix "${PHYDUINO_TOOLCHAIN_PATH}/")
else()
    set(prefix "")
endif()
if(CMAKE_HOST_WIN32)
    set(exe ".exe")
else()
    set(exe "")
endif()

set(CMAKE_C_COMPILER   "${prefix}arm-none-eabi-gcc${exe}")
set(CMAKE_CXX_COMPILER "${prefix}arm-none-eabi-g++${exe}")
set(CMAKE_ASM_COMPILER "${prefix}arm-none-eabi-gcc${exe}")
set(CMAKE_AR           "${prefix}arm-none-eabi-ar${exe}" CACHE FILEPATH "")
set(CMAKE_SIZE         "${prefix}arm-none-eabi-size${exe}" CACHE FILEPATH "")
