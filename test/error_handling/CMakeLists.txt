
macro(error_test case)
get_filename_component(test_name ${case} NAME_WE)
set(test_comment "! ")
add_test(NAME ${test_name}.Error
  COMMAND $<TARGET_FILE:CSE> -x${test_comment} -b -t1 "${test_name}"
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)
set_tests_properties(${test_name}.Error PROPERTIES WILL_FAIL TRUE)
endmacro()

macro(error_code_finder case error_code)
  get_filename_component(test_name ${case} NAME_WE)
  add_test(NAME ${test_name}.ErrorCode
    COMMAND $<TARGET_FILE:error_comp> "${CMAKE_CURRENT_SOURCE_DIR}/${test_name}.err" "${error_code}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  )
  set_tests_properties(${test_name}.ErrorCode PROPERTIES DEPENDS ${test_name}.Error )
endmacro()

set(MAP_no_file "P0003")
set(MAP_missing_weather_file "S0263")

set(files
  missing_weather_file
  no_file
)

foreach(file ${files})
  error_test(${file})
  error_code_finder(${file} ${MAP_${file}})
endforeach()
