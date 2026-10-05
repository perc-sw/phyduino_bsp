# Reads Arduino platform property files so the CMake build uses the same
# settings as the Arduino build (platform.txt and boards.txt stay the only
# place they are written).
#
#   arduino_load_properties(<platform dir> <board id>)
#       Loads platform.txt, then boards.txt entries for <board id> (prefix
#       stripped), then platform.local.txt. Later files win, as in Arduino.
#   arduino_set_property(<key> <value>)
#       Adds or overrides a property (e.g. runtime.platform.path).
#   arduino_property(<out var> <key>)
#       Value of <key> with every {other.key} reference expanded.
#   arduino_property_args(<out var> <key>...)
#       Expanded values split into a CMake list of arguments (shell quoting rules).

function(arduino_set_property key value)
    set_property(GLOBAL PROPERTY "ARDUINO_PROP_${key}" "${value}")
endfunction()

function(_arduino_load_file path board)
    if(NOT EXISTS "${path}")
        return()
    endif()
    file(STRINGS "${path}" lines)
    foreach(line IN LISTS lines)
        if(NOT line MATCHES "^([A-Za-z0-9_.-]+)=(.*)$")
            continue()  # comment, blank or menu line
        endif()
        set(key "${CMAKE_MATCH_1}")
        set(value "${CMAKE_MATCH_2}")
        if(board)
            if(NOT key MATCHES "^${board}\\.(.+)$")
                continue()  # another board's setting
            endif()
            set(key "${CMAKE_MATCH_1}")
        endif()
        arduino_set_property("${key}" "${value}")
    endforeach()
endfunction()

function(arduino_load_properties platform_dir board)
    if(NOT EXISTS "${platform_dir}/boards.txt")
        message(FATAL_ERROR "No boards.txt in ${platform_dir}")
    endif()
    file(STRINGS "${platform_dir}/boards.txt" known REGEX "^${board}\\.name=")
    if(NOT known)
        message(FATAL_ERROR "Board '${board}' is not defined in ${platform_dir}/boards.txt")
    endif()
    _arduino_load_file("${platform_dir}/platform.txt" "")
    _arduino_load_file("${platform_dir}/boards.txt" "${board}")
    _arduino_load_file("${platform_dir}/platform.local.txt" "")
endfunction()

function(arduino_property out key)
    get_property(value GLOBAL PROPERTY "ARDUINO_PROP_${key}")
    set(depth 0)
    while(value MATCHES "{([A-Za-z0-9_.-]+)}")
        set(ref "${CMAKE_MATCH_1}")
        get_property(sub GLOBAL PROPERTY "ARDUINO_PROP_${ref}")  # undefined -> empty
        string(REPLACE "{${ref}}" "${sub}" value "${value}")
        math(EXPR depth "${depth} + 1")
        if(depth GREATER 50)
            message(FATAL_ERROR "Property '${key}' references itself")
        endif()
    endwhile()
    set(${out} "${value}" PARENT_SCOPE)
endfunction()

function(arduino_property_args out)
    set(result "")
    foreach(key IN LISTS ARGN)
        arduino_property(value "${key}")
        separate_arguments(args UNIX_COMMAND "${value}")
        list(APPEND result ${args})
    endforeach()
    set(${out} "${result}" PARENT_SCOPE)
endfunction()
