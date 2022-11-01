message("Building CSE...")
set(BUILD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/msvc/build)
if (DEFINED ARCHITECTUREBUILD)
  if (${ARCHITECTUREBUILD} STREQUAL "x64")
    set(BUILD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/msvc/build64)
  endif()
endif()
execute_process(COMMAND ${CMAKE_COMMAND}
  --build ${BUILD_DIR}
  --config Release
  --target CSE
  -- -m
  WORKING_DIRECTORY ${BUILD_DIR}
  RESULT_VARIABLE success
)
if (${success} MATCHES "0")
  message("Build Successful!")
else()
  message(FATAL_ERROR "Build failed.")
endif()
