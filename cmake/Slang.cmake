# Slang.cmake - helper for compiling Slang shaders to SPIR-V
#
# Provides:
#   add_slang_shaders(<target>
#       SHADERS  <file1.slang> [<file2.slang> ...]
#       [OUTPUT_DIR <dir>]   # defaults to ${CMAKE_CURRENT_BINARY_DIR}
#       [FLAGS <extra slangc flags...>])
#
# Each shader is compiled to <OUTPUT_DIR>/<name>.spv and attached to <target>
# as a build dependency, so the .spv files are (re)built whenever their source
# changes before <target> links.

find_program(SLANGC slangc REQUIRED)

function(add_slang_shaders TARGET)
    cmake_parse_arguments(SLANG "" "OUTPUT_DIR" "SHADERS;FLAGS" ${ARGN})

    if(NOT SLANG_OUTPUT_DIR)
        set(SLANG_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR})
    endif()
    if(NOT SLANG_FLAGS)
        set(SLANG_FLAGS -target spirv)
    endif()

    set(_outputs "")
    foreach(_shader ${SLANG_SHADERS})
        get_filename_component(_name ${_shader} NAME_WE)
        get_filename_component(_abs ${_shader} ABSOLUTE)
        set(_out ${SLANG_OUTPUT_DIR}/${_name}.spv)
        add_custom_command(
            OUTPUT ${_out}
            COMMAND ${SLANGC} ${_abs} ${SLANG_FLAGS} -o ${_out}
            DEPENDS ${_abs}
            COMMENT "Compiling shader ${_shader}"
            VERBATIM
        )
        list(APPEND _outputs ${_out})
    endforeach()

    add_custom_target(${TARGET}-shaders DEPENDS ${_outputs})
    add_dependencies(${TARGET} ${TARGET}-shaders)
endfunction()
