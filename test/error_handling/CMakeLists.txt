
macro(error_test case)
get_filename_component(test_name ${case} NAME_WE)
set(test_comment "! ")
add_test(NAME ${test_name}.Error
  COMMAND $<TARGET_FILE:CSE> -x${test_comment} -b -t1 "${test_name}"
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)
set_tests_properties(${test_name}.Error PROPERTIES WILL_FAIL TRUE)
endmacro()

macro(error_regression_test case)
  get_filename_component(test_name ${case} NAME_WE)
  add_test(NAME ${test_name}.ErrorRegression
    COMMAND $<TARGET_FILE:wcmp> "${CMAKE_CURRENT_SOURCE_DIR}/${test_name}.err" "${CMAKE_CURRENT_SOURCE_DIR}/ref/${test_name}.err" "!"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  )
  set_tests_properties(${test_name}.ErrorRegression PROPERTIES DEPENDS ${test_name}.Error)
endmacro()

set(files
    missing_weather_file.cse
)

foreach(file ${files})
  error_test(${file})
  error_regression_test(${file})
endforeach()
