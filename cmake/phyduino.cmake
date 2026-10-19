# CMake verification build of the Phyduino platform.
#
# Builds the same sources as the Arduino IDE (core, variant, libraries,
# sketches) against the same SDK bundle, with flags read from platform.txt /
# boards.txt (arduino_properties.cmake), plus strict warnings on our own code.
# Nothing here ships: the release zip contains only platform/.
#
#   phyduino_setup(<board id> [<menu>=<option>...])
#       Creates targets phyduino_core and phyduino_lib_<Library> for one board,
#       with the given Tools menu options (e.g. os=freertos).
#   phyduino_add_sketch(<sketch dir>)
#       Builds <name>.elf (+ .uf2 if picotool is found) from a sketch folder.
#       A sketch that includes FreeRTOS.h is skipped unless os=freertos or
#       os=cmsis; one that includes cmsis_os2.h unless os=cmsis.
#
# Guide: docs/maintenance/cmake-verify.html
include_guard(GLOBAL)
include(${CMAKE_CURRENT_LIST_DIR}/arduino_properties.cmake)


get_filename_component(PHYDUINO_PLATFORM_DIR "${CMAKE_CURRENT_LIST_DIR}/../platform" ABSOLUTE)

# Flags applied to code we own (not vendored ArduinoCore-API or the SDK).
set(PHYDUINO_STRICT_FLAGS -Wall -Wextra)
if(PHYDUINO_WERROR)
    list(APPEND PHYDUINO_STRICT_FLAGS -Werror)
endif()

# --- Board setup -----------------------------------------------------------------
function(phyduino_setup board)
    arduino_load_properties("${PHYDUINO_PLATFORM_DIR}" "${board}" ${ARGN})

    # Values the Arduino IDE would provide at build time.
    arduino_set_property(runtime.platform.path "${PHYDUINO_PLATFORM_DIR}")
    arduino_set_property(runtime.ide.version   "10607")
    arduino_set_property(build.arch            "RP2")
    arduino_set_property(build.path            "${CMAKE_BINARY_DIR}")

    arduino_property(core    build.core)
    arduino_property(variant build.variant)
    arduino_property(sdk     build.sdk.path)
    if(NOT EXISTS "${sdk}/lib/libpico.a")
        message(FATAL_ERROR "SDK bundle missing: ${sdk} (see docs/maintenance/update-pico-sdk.html)")
    endif()
    file(STRINGS "${sdk}/compile_flags.txt" freertos REGEX "^-DPHYDUINO_FREERTOS=1$")
    if(freertos)
        set(PHYDUINO_FREERTOS ON PARENT_SCOPE)
    else()
        set(PHYDUINO_FREERTOS OFF PARENT_SCOPE)
    endif()
    arduino_property_args(os_flags build.os.flags)
    if("-DPHYDUINO_CMSIS_RTOS2=1" IN_LIST os_flags)
        set(PHYDUINO_CMSIS_RTOS2 ON PARENT_SCOPE)
    else()
        set(PHYDUINO_CMSIS_RTOS2 OFF PARENT_SCOPE)
    endif()

    # Compile flags shared by C and C++, minus what CMake handles itself
    # (-c, -MMD) and Arduino's default -w (we want warnings here).
    arduino_property_args(common
        compiler.cpu.flags compiler.optimize.flags compiler.sdk.flags
        compiler.arduino.defines build.extra_flags build.os.flags)
    arduino_property_args(c_only   compiler.c.flags)
    arduino_property_args(cpp_only compiler.cpp.flags)
    foreach(list_var c_only cpp_only)
        list(REMOVE_ITEM ${list_var} -c -MMD -w)
    endforeach()

    add_library(phyduino_flags INTERFACE)
    target_compile_options(phyduino_flags INTERFACE
        ${common}
        "$<$<COMPILE_LANGUAGE:C>:${c_only}>"
        "$<$<COMPILE_LANGUAGE:CXX>:${cpp_only}>")
    target_include_directories(phyduino_flags INTERFACE
        "${PHYDUINO_PLATFORM_DIR}/cores/${core}"
        "${PHYDUINO_PLATFORM_DIR}/variants/${variant}")

    # Link settings: same pieces as recipe.c.combine.pattern in platform.txt.
    arduino_property_args(link_cpu compiler.cpu.flags compiler.optimize.flags compiler.ldflags)
    set(PHYDUINO_LINK_OPTIONS
        ${link_cpu} "@${sdk}/link_flags.txt" "-L${sdk}" "-T${sdk}/memmap.ld" PARENT_SCOPE)
    set(PHYDUINO_SDK_PATH "${sdk}" PARENT_SCOPE)
    arduino_property_args(os_libs build.os.libs)
    set(PHYDUINO_OS_LIBS "${os_libs}" PARENT_SCOPE)
    arduino_property(chip build.chip)
    set(PHYDUINO_CHIP "${chip}" PARENT_SCOPE)

    # Core + variant: one static library, like Arduino's core.a.
    file(GLOB_RECURSE core_sources CONFIGURE_DEPENDS
        "${PHYDUINO_PLATFORM_DIR}/cores/${core}/*.c"
        "${PHYDUINO_PLATFORM_DIR}/cores/${core}/*.cpp"
        "${PHYDUINO_PLATFORM_DIR}/variants/${variant}/*.c"
        "${PHYDUINO_PLATFORM_DIR}/variants/${variant}/*.cpp")
    add_library(phyduino_core STATIC ${core_sources})
    target_link_libraries(phyduino_core PUBLIC phyduino_flags)
    _phyduino_strict(phyduino_core "${core_sources}")

    # One static library per bundled library (Arduino layout: <Lib>/src).
    file(GLOB lib_dirs LIST_DIRECTORIES true "${PHYDUINO_PLATFORM_DIR}/libraries/*")
    set(libraries "")
    foreach(dir IN LISTS lib_dirs)
        if(NOT IS_DIRECTORY "${dir}/src")
            continue()
        endif()
        get_filename_component(name "${dir}" NAME)
        file(GLOB_RECURSE lib_sources CONFIGURE_DEPENDS "${dir}/src/*.c" "${dir}/src/*.cpp")
        add_library(phyduino_lib_${name} STATIC ${lib_sources})
        target_include_directories(phyduino_lib_${name} PUBLIC "${dir}/src")
        target_link_libraries(phyduino_lib_${name} PUBLIC phyduino_core)
        _phyduino_strict(phyduino_lib_${name} "${lib_sources}")
        list(APPEND libraries "${name}")
    endforeach()
    set(PHYDUINO_LIBRARIES "${libraries}" PARENT_SCOPE)
    message(STATUS "Phyduino: board ${board} [${ARGN}], SDK bundle ${sdk}, libraries: ${libraries}")
endfunction()

# Strict warnings on our sources; vendored ArduinoCore-API (core/api) compiles quietly.
function(_phyduino_strict target sources)
    foreach(src IN LISTS sources)
        if(src MATCHES "/api/")
            set_source_files_properties("${src}" TARGET_DIRECTORY ${target} PROPERTIES COMPILE_OPTIONS -w)
        else()
            set_source_files_properties("${src}" TARGET_DIRECTORY ${target}
                PROPERTIES COMPILE_OPTIONS "${PHYDUINO_STRICT_FLAGS}")
        endif()
    endforeach()
endfunction()

# --- Sketches ----------------------------------------------------------------------
# Like the Arduino builder, but without its automatic function prototypes:
# functions must be declared before use (plain C++). Libraries are linked when
# the sketch #includes <Library.h>.
function(phyduino_add_sketch dir)
    get_filename_component(name "${dir}" NAME)
    file(GLOB inos CONFIGURE_DEPENDS "${dir}/*.ino")
    if(NOT inos)
        message(FATAL_ERROR "No .ino file in ${dir}")
    endif()
    # Main .ino first, then the others alphabetically (Arduino's order).
    list(REMOVE_ITEM inos "${dir}/${name}.ino")
    list(SORT inos)
    list(PREPEND inos "${dir}/${name}.ino")

    set(cpp "${CMAKE_BINARY_DIR}/sketches/${name}/${name}.ino.cpp")
    set(text "#include <Arduino.h>\n")
    set(used_libs "")
    foreach(ino IN LISTS inos)
        file(READ "${ino}" body)
        if(NOT PHYDUINO_FREERTOS AND body MATCHES "#include[ \t]*[<\"]FreeRTOS\\.h[>\"]")
            message(STATUS "Phyduino: ${name} needs FreeRTOS, skipped (build with os=freertos)")
            return()
        endif()
        if(NOT PHYDUINO_CMSIS_RTOS2 AND body MATCHES "#include[ \t]*[<\"]cmsis_os2\\.h[>\"]")
            message(STATUS "Phyduino: ${name} needs CMSIS-RTOS2, skipped (build with os=cmsis)")
            return()
        endif()
        string(APPEND text "#line 1 \"${ino}\"\n${body}\n")
        foreach(lib IN LISTS PHYDUINO_LIBRARIES)
            if(body MATCHES "#include[ \t]*[<\"]${lib}\\.h[>\"]")
                list(APPEND used_libs phyduino_lib_${lib})
            endif()
        endforeach()
    endforeach()
    # Rewrite only on change, so unchanged sketches are not recompiled.
    set(previous "")
    if(EXISTS "${cpp}")
        file(READ "${cpp}" previous)
    endif()
    if(NOT previous STREQUAL text)
        file(WRITE "${cpp}" "${text}")
    endif()
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${inos})

    add_executable(${name} "${cpp}")
    set_target_properties(${name} PROPERTIES SUFFIX ".elf")
    target_include_directories(${name} PRIVATE "${dir}")
    target_compile_options(${name} PRIVATE ${PHYDUINO_STRICT_FLAGS})
    target_link_options(${name} PRIVATE ${PHYDUINO_LINK_OPTIONS}
        "LINKER:-Map,$<TARGET_FILE_DIR:${name}>/${name}.map")
    list(REMOVE_DUPLICATES used_libs)
    target_link_libraries(${name} PRIVATE
        "${PHYDUINO_SDK_PATH}/lib/boot2.o"
        -Wl,--start-group
        ${used_libs} phyduino_core ${PHYDUINO_OS_LIBS}
        -Wl,--whole-archive "${PHYDUINO_SDK_PATH}/lib/libpico.a" -Wl,--no-whole-archive
        stdc++ m c
        -Wl,--end-group)

    add_custom_command(TARGET ${name} POST_BUILD
        COMMAND "${CMAKE_SIZE}" "$<TARGET_FILE:${name}>"
        VERBATIM)
    if(PHYDUINO_PICOTOOL)
        add_custom_command(TARGET ${name} POST_BUILD
            COMMAND "${PHYDUINO_PICOTOOL}" uf2 convert "$<TARGET_FILE:${name}>"
                    "$<TARGET_FILE_DIR:${name}>/${name}.uf2" --family ${PHYDUINO_CHIP}
            VERBATIM)
    endif()
endfunction()
