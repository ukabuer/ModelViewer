find_program(
    SOKOL_SHDC sokol-shdc
    HINTS ${SOKOL_SHDC_DIR}
)
if (NOT SOKOL_SHDC)
  message(FATAL_ERROR "Cannot find sokol-shdc, please set \"SOKOL_SHDC_DIR\" to a directory containing it.")
endif ()

function(process_shaders)
  cmake_parse_arguments(
      PARSED_ARGS # prefix of output variables
      "" # list of names of the boolean arguments (only defined ones will be true)
      "DIR;TARGET" # list of names of mono-valued arguments
      "" # list of names of multi-valued arguments (output variables are lists)
      ${ARGN} # arguments of the function to parse, here we take the all original ones
  )

  if (NOT PARSED_ARGS_DIR)
    message(ERROR " You must provide a dir")
    return()
  endif ()

  if (NOT PARSED_ARGS_TARGET)
    message(ERROR " You must provide a target")
    return()
  endif ()

  file(GLOB shader_files "${PARSED_ARGS_DIR}/*.glsl")

  set(DEPS "")
  foreach (shader_file ${shader_files})
    set(SHADER_SRC ${shader_file})
    add_custom_command(
        OUTPUT
        ${SHADER_SRC}.h
        DEPENDS
        ${SHADER_SRC}
        COMMAND
        ${SOKOL_SHDC} --input ${SHADER_SRC} --output ${SHADER_SRC}.h --slang glsl330
        COMMENT
        "Generating shader headers with sokol-shdc"
        VERBATIM
    )
    list(APPEND DEPS ${SHADER_SRC}.h)
  endforeach ()

  set(${PARSED_ARGS_TARGET} ${DEPS} PARENT_SCOPE)
endfunction()
