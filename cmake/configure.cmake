message("Making build directory: ${CMAKE_CURRENT_SOURCE_DIR}/msvc/build")
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/msvc)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/msvc/build)
message("Generating project files...")
execute_process(COMMAND ${CMAKE_COMMAND} ${CMAKE_CURRENT_SOURCE_DIR} -T v142,version=14.29.16.11 -A Win32 -DCMAKE_SYSTEM_VERSION=10.0.20348.0
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/msvc/build
  RESULT_VARIABLE success
)
if (NOT ${success} MATCHES "0")
  message(FATAL_ERROR "Generation step failed.")
endif()