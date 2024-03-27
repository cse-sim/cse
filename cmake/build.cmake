if (NOT DEFINED TARGET_NAME)
  if ("${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "Windows")
    set(TARGET_NAME "ALL_BUILD") # Needed for MSVC (not necessarily all Windows builds)
  else ()
    set(TARGET_NAME "all")
  endif ()
endif ()
message("Building ${TARGET_NAME}...")

include(cmake/utility.cmake)
set_build_configuration()
if (NOT DEFINED BUILD_DIRECTORY)
  set(BUILD_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/builds/${BUILD_CONFIGURATION}")
endif ()

include(ProcessorCount)
ProcessorCount(N)
if(NOT N EQUAL 0)
  set(parallel_jobs ${N})
else()
  set(parallel_jobs 1)
endif()

execute_process(COMMAND ${CMAKE_COMMAND}
  --build .
  --config ${CONFIGURATION}
  --target ${TARGET_NAME}
  -j ${parallel_jobs}
  WORKING_DIRECTORY ${BUILD_DIRECTORY}
  RESULT_VARIABLE success
)
if (${success} MATCHES "0")
  message("Build Successful!")
else()
  message(FATAL_ERROR "Build failed.")
endif()
