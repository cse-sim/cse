if (${compiler_name} STREQUAL "MSVC")
  execute_process(
    COMMAND "${compiler_path}" -c -EP -nologo -I "${include_dir}" -Tc ${input_path}
    OUTPUT_FILE ${output_path}
    RESULT_VARIABLE exit_status
)
else()
  execute_process(
    COMMAND "${compiler_path}"  -E -P -I "${include_dir}" -
    INPUT_FILE "${input_path}"
    OUTPUT_FILE "${output_path}"
    RESULT_VARIABLE exit_status
)
endif()

if(NOT "${exit_status}" STREQUAL "0")
  message(FATAL_ERROR "C Preprocessor failed (${exit_status}): '${output_string}'")
endif()
