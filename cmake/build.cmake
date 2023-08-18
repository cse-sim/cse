if (NOT DEFINED TARGET_NAME)
  set(TARGET_NAME CSE)
endif ()
message("Building ${TARGET_NAME}...")

if ("${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "Windows")
  set(BUILD_ARGS -m)
else()
  set(BUILD_ARGS -j)
endif ()

include(cmake/utility.cmake)
if (NOT DEFINED BUILD_DIRECTORY)
  set_build_directory()
endif ()

execute_process(COMMAND ${CMAKE_COMMAND}
  --build .
  --config ${CONFIGURATION}
  --target ${TARGET_NAME}
  -- ${BUILD_ARGS}
  WORKING_DIRECTORY ${BUILD_DIRECTORY}
  RESULT_VARIABLE success
)
if (${success} MATCHES "0")
  message("Build Successful!")
else()
  message(FATAL_ERROR "Build failed.")
endif()
