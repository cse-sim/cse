
macro(error_test case error_string)
get_filename_component(test_name ${case} NAME_WE)
set(test_comment "! ")
add_test(NAME ${test_name}.Error
  COMMAND $<TARGET_FILE:CSE> -x${test_comment} -b -t1 "${test_name}"
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)
set_tests_properties(${test_name}.Error PROPERTIES WILL_FAIL TRUE)

add_test(NAME ${test_name}.ErrorMatch
    COMMAND ${CMAKE_COMMAND} -Dfile=test/error_handling/no_file.err -Derror_string=P0003 -P cmake/string-finder.cmake # $<TARGET_FILE:error_comp> "${CMAKE_CURRENT_SOURCE_DIR}/${test_name}.err" "${error_code}" 
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  )
set_tests_properties(${test_name}.ErrorMatch PROPERTIES DEPENDS ${test_name}.Error )
endmacro()

error_test(missing_weather_file "S0263: No wfName given in input file")
error_test(no_file "P0003: Cannot open file")
