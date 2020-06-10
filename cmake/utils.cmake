find_program(
    SOKOL_SHDC sokol-shdc
    HINTS ${SOKOL_SHDC_DIR}
)
if (NOT SOKOL_SHDC)
  message(FATAL_ERROR "Cannot find sokol-shdc, please set \"SOKOL_SHDC_DIR\" to a directory containing it.")
endif ()

function(process_shaders directory)
  file(GLOB shader_files "${directory}/*.glsl")

  foreach (shader_file ${shader_files})
    set(SHADER_SRC ${shader_file})
    execute_process(
        COMMAND
        ${SOKOL_SHDC} --input ${SHADER_SRC} --output ${SHADER_SRC}.h --slang glsl330
    )
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${SHADER_SRC})
  endforeach ()
endfunction()
