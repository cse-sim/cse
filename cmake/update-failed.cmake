set(testLog "Testing/Temporary/LastTestsFailed.log")
if(EXISTS ${testLog})
  file(STRINGS ${testLog} failedTests)
  foreach(test ${failedTests})
    string(REGEX REPLACE "[0-9]+:[^;].*.(Regression|Run)" "\\1" test_type "${test}")
    if(${test_type} MATCHES "Regression")
      string(REGEX REPLACE "[0-9]+:([^;].*).Regression" "\\1" test_name "${test}")
      string(TOUPPER ${test_name} test_name)
      message("Updating: ${test_name}.REP")
      execute_process(COMMAND ${CMAKE_COMMAND} -E copy "${test_name}.REP" "${ref_dir}/${test_name}.REP"
        WORKING_DIRECTORY ${test_dir}
        RESULT_VARIABLE success
      )
    endif()
  endforeach()
else()
  message("Could not find test log at ${testLog}")
endif()
