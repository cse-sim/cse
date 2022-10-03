
macro(run_test case)
get_filename_component(test_name ${case} NAME_WE)
set(test_comment "! ")
add_test(NAME ${test_name}.error_handling
  COMMAND $<TARGET_FILE:CSE> -x${test_comment} -b -t1 "${test_name}"
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)
set_tests_properties(${test_name}.error_handling PROPERTIES WILL_FAIL $<$<NOT:$<STREQUAL:"minimumRunningFile","${test_name}">>:TRUE>)
endmacro()

set(files
    minimumRunningFile.cse
    missingWeatherFile.cse
)

foreach(file ${files})
  run_test(${file})
endforeach()
