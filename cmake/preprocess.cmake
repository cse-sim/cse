execute_process(
  COMMAND "${compiler}" -c -EP -nologo -I "${source_dir}" -Tc "${source_dir}/${file}.def"
  OUTPUT_FILE "${out_dir}/${file}.i"
  RESULT_VARIABLE exit_status
)

if(NOT "${exit_status}" STREQUAL "0")
  message(FATAL_ERROR "C Preprocessor failed (${exit_status}): '${output_string}'")
endif()
