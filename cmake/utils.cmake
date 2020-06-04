function(embed_shaders)
  cmake_parse_arguments(
      PARSED_ARGS # prefix of output variables
      "" # list of names of the boolean arguments (only defined ones will be true)
      "CONFIG_FILE;TARGET;DIR" # list of names of mono-valued arguments
      "" # list of names of multi-valued arguments (output variables are lists)
      ${ARGN} # arguments of the function to parse, here we take the all original ones
  )

  if (NOT PARSED_ARGS_CONFIG_FILE)
    message(ERROR " You must provide a config file")
    return()
  endif ()

  if (NOT PARSED_ARGS_TARGET)
    message(ERROR " You must provide a target file")
    return()
  endif ()

  if (NOT PARSED_ARGS_DIR)
    message(ERROR " You must provide a dir file")
    return()
  endif ()

  file(GLOB shader_files "${PARSED_ARGS_DIR}/*.glsl")

  foreach (shader_file ${shader_files})
    get_filename_component(shader_name ${shader_file} NAME)
    file(READ ${shader_file} ${shader_name})
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${shader_file})
  endforeach ()

  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${PARSED_ARGS_CONFIG_FILE})
  configure_file(
      ${PARSED_ARGS_CONFIG_FILE}
      ${PARSED_ARGS_TARGET}
      ESCAPE_QUOTES
      NEWLINE_STYLE LF
  )
endfunction()
