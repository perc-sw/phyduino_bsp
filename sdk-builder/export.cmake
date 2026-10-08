# Writes the SDK bundle (platform/system/<board>/) from the values CMakeLists.txt
# collected in bundle_inputs.cmake. Run by the `bundle` target; not meant to be
# run by hand.
#
#   include/            every SDK include dir merged into one tree
#   lib/libpico.a       SDK + TinyUSB objects (link with --whole-archive)
#   lib/boot2.o         boot stage 2 (padded + checksummed)
#   memmap.ld           linker script (+ the .ld files it INCLUDEs)
#   compile_flags.txt   -D defines, one per line (gcc @file)
#   link_flags.txt      linker options, one per line (gcc @file)
#   GENERATED.md        provenance
cmake_minimum_required(VERSION 3.21)
include(${INPUTS})

file(REMOVE_RECURSE ${OUT})
file(MAKE_DIRECTORY ${OUT}/include ${OUT}/lib)

# --- 1. Headers --------------------------------------------------------------
set(collisions "")
foreach(dir IN LISTS INCLUDE_DIRS)
    if(NOT IS_DIRECTORY "${dir}")
        continue()
    endif()
    file(GLOB_RECURSE headers RELATIVE "${dir}" "${dir}/*.h")
    foreach(header IN LISTS headers)
        set(dst "${OUT}/include/${header}")
        if(EXISTS "${dst}")
            file(SHA256 "${dir}/${header}" new_hash)
            file(SHA256 "${dst}" old_hash)
            if(NOT new_hash STREQUAL old_hash)
                list(APPEND collisions "${header} (from ${dir})")
            endif()
        else()
            configure_file("${dir}/${header}" "${dst}" COPYONLY)
        endif()
    endforeach()
endforeach()
# config_autogen.h #includes absolute build-machine paths. Copy each target
# into include/ and make the #include relative.
set(autogen "${OUT}/include/pico/config_autogen.h")
file(STRINGS "${autogen}" lines)
set(text "")
foreach(line IN LISTS lines)
    if(line MATCHES "^#include \"(.+)\"$")
        set(path "${CMAKE_MATCH_1}")
        if(path STREQUAL BOARD_HEADER)
            set(rel "boards/${BOARD}.h")
        elseif(path MATCHES "/include/([^/].*)$")
            set(rel "${CMAKE_MATCH_1}")
        else()
            message(FATAL_ERROR "Don't know where to put ${path} from config_autogen.h")
        endif()
        configure_file("${path}" "${OUT}/include/${rel}" COPYONLY)
        set(line "#include \"${rel}\"")
    endif()
    string(APPEND text "${line}\n")
endforeach()
file(WRITE "${autogen}" "${text}")

if(collisions)
    list(JOIN collisions "\n  " collision_text)
    message(FATAL_ERROR "Two SDK include dirs provide different files with the same name:\n  ${collision_text}")
endif()

# --- 2. Libraries ------------------------------------------------------------
list(LENGTH BOOT2_OBJ boot2_count)
if(NOT boot2_count EQUAL 1)
    message(FATAL_ERROR "Expected one boot stage 2 object, got: ${BOOT2_OBJ}")
endif()
file(COPY_FILE "${LIBPICO}"   "${OUT}/lib/libpico.a")
file(COPY_FILE "${BOOT2_OBJ}" "${OUT}/lib/boot2.o")

# --- 3. Compile defines ------------------------------------------------------
set(compile_flags "")
foreach(define IN LISTS DEFINES)
    if(define STREQUAL "")
        continue()
    endif()
    # gcc @file syntax: backslash-escape quotes and spaces.
    string(REPLACE "\\" "\\\\" define "${define}")
    string(REPLACE "\"" "\\\"" define "${define}")
    string(REPLACE " "  "\\ "  define "${define}")
    string(APPEND compile_flags "-D${define}\n")
endforeach()
file(WRITE ${OUT}/compile_flags.txt "${compile_flags}")

# --- 4. Link options + linker script ----------------------------------------
set(link_flags "")
set(linker_script "")
foreach(option IN LISTS LINK_OPTIONS)
    if(option MATCHES "^LINKER:(.*)$")
        set(option "-Wl,${CMAKE_MATCH_1}")
    elseif(option MATCHES "^SHELL:(.*)$")
        set(option "${CMAKE_MATCH_1}")
    endif()

    if(option MATCHES "^-Wl,--script=(.*)$")
        set(linker_script "${CMAKE_MATCH_1}")     # path passed by platform.txt instead
    elseif(option STREQUAL "-Wl,-L${BINARY_DIR}")
        # build-tree search path; platform.txt adds -L for the bundle folder
    elseif(NOT option STREQUAL "")
        list(APPEND link_flags "${option}")
    endif()
endforeach()
list(REMOVE_DUPLICATES link_flags)    # the SDK adds some --wrap options twice
list(JOIN link_flags "\n" link_flags)
if(NOT EXISTS "${linker_script}")
    message(FATAL_ERROR "Linker script not found in link options: '${linker_script}'")
endif()
file(WRITE ${OUT}/link_flags.txt "${link_flags}\n")
file(COPY_FILE "${linker_script}" "${OUT}/memmap.ld")
file(GLOB generated_ld "${BINARY_DIR}/*.ld")
foreach(ld IN LISTS generated_ld)
    get_filename_component(name "${ld}" NAME)
    file(COPY_FILE "${ld}" "${OUT}/${name}")
endforeach()

# --- 5. Provenance -----------------------------------------------------------
function(git_describe repo out)
    execute_process(COMMAND git -C "${repo}" describe --tags --always
                    OUTPUT_VARIABLE desc OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(COMMAND git -C "${repo}" rev-parse HEAD
                    OUTPUT_VARIABLE hash OUTPUT_STRIP_TRAILING_WHITESPACE)
    set(${out} "`${desc}` (${hash})" PARENT_SCOPE)
endfunction()
git_describe("${SDK_PATH}" sdk_version)
git_describe("${SDK_PATH}/lib/tinyusb" tinyusb_version)

file(WRITE ${OUT}/GENERATED.md
"# GENERATED — do not edit

This folder is the pico-sdk prebuilt for the **${BOARD}** board (${PLATFORM}).
It is written by `sdk-builder/` and committed so the Arduino IDE works straight
after cloning.

| Input    | Version |
|----------|---------|
| pico-sdk | ${sdk_version} |
| TinyUSB  | ${tinyusb_version} |
| Board    | `sdk-builder/boards/${BOARD}.h` |
| USB      | `sdk-builder/config/tusb_config.h` |

Regenerate (from `sdk-builder/`):

    cmake --workflow --preset ${BOARD}

See `docs/maintenance/update-pico-sdk.html`.
")
message(STATUS "SDK bundle written: ${OUT}")
