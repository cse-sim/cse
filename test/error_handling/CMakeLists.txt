
macro(run_test case)
get_filename_component(test_name ${case} NAME_WE)
set(test_comment "! ")
add_test(NAME ${test_name}.error_handling
  COMMAND $<TARGET_FILE:CSE> -x${test_comment} -b -t1 "${test_name}"
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)
set_tests_properties(${test_name}.error_handling PROPERTIES WILL_FAIL TRUE)
endmacro()

macro(error_checking case)
  get_filename_component(test_name ${case} NAME_WE)
  add_test(NAME ${test_name}.error_checking
    COMMAND $<TARGET_FILE:wcmp> "${CMAKE_CURRENT_SOURCE_DIR}/${test_name}.err" "${CMAKE_CURRENT_SOURCE_DIR}/ref/${test_name}.err" "!"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  )
  set_tests_properties(${test_name}.error_checking PROPERTIES DEPENDS ${test_name}.error_handling)
endmacro()

set(files
    missingWeatherFile.cse
)

foreach(file ${files})
  run_test(${file})
  error_checking(${file})
endforeach()
