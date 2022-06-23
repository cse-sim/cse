message("Building CSE x64...")
execute_process(COMMAND ${CMAKE_COMMAND}
  --build ${CMAKE_CURRENT_SOURCE_DIR}/msvc/build64
  --config Release
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/msvc/build64
  RESULT_VARIABLE success
)
if (${success} MATCHES "0")
  message("Build 64-bit Successful!")
else()
  message(FATAL_ERROR "Build failed.")
endif()

#message("Building CSE...")
#execute_process(COMMAND ${CMAKE_COMMAND}
#  --build ${CMAKE_CURRENT_SOURCE_DIR}/msvc/build
#  --config Release
#  -- -m
#  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/msvc/build
#  RESULT_VARIABLE success
#)
#if (${success} MATCHES "0")
#  message("Build Successful!")
#else()
#  message(FATAL_ERROR "Build failed.")
#endif()
