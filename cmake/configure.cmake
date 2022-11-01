set(ARCHBUILDFLAG Win32)
set(BUILD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/msvc/build)
if (DEFINED ARCHITECTUREBUILD)
  if (${ARCHITECTUREBUILD} STREQUAL "x64")
    set(ARCHBUILDFLAG x64)
    set(BUILD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/msvc/build64)
  endif()
endif()
message("Making build directory: ${BUILD_DIR}")
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/msvc)
file(MAKE_DIRECTORY ${BUILD_DIR})
message("Generating project files...")
execute_process(COMMAND ${CMAKE_COMMAND} ${CMAKE_CURRENT_SOURCE_DIR} -T v142,version=14.29.16.11 -A ${ARCHBUILDFLAG} -DCMAKE_SYSTEM_VERSION=10.0.20348.0
  WORKING_DIRECTORY ${BUILD_DIR}
  RESULT_VARIABLE success
)
if (NOT ${success} MATCHES "0")
  message(FATAL_ERROR "Generation step failed.")
endif()