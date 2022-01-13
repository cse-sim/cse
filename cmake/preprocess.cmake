if (CMAKE_MSVC_COMPILER_ID MATCHES MSVC)
  execute_process(
    COMMAND "${compiler}" -c -EP -I "${include_dir}" -Tc ${input_path}
    OUTPUT_FILE ${output_path}
    RESULT_VARIABLE exit_status
)
else()
  execute_process(
    COMMAND "${compiler}"  -E -P -I "${include_dir}" -
    INPUT_FILE "${input_path}"
    OUTPUT_FILE "${output_path}"
    RESULT_VARIABLE exit_status
)
endif()

if(NOT "${exit_status}" STREQUAL "0")
  message(FATAL_ERROR "C Preprocessor failed (${exit_status}): '${output_string}'")
endif()
