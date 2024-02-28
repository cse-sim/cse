if (NOT DEFINED TARGET_NAME)
  set(TARGET_NAME "all")
endif ()
message("Building ${TARGET_NAME}...")

include(cmake/utility.cmake)
if (NOT DEFINED BUILD_DIRECTORY)
  set_build_directory()
endif ()

execute_process(COMMAND ${CMAKE_COMMAND}
  --build .
  --config ${CONFIGURATION}
  --target ${TARGET_NAME}
  -j
  WORKING_DIRECTORY ${BUILD_DIRECTORY}
  RESULT_VARIABLE success
)
if (${success} MATCHES "0")
  message("Build Successful!")
else()
  message(FATAL_ERROR "Build failed.")
endif()
