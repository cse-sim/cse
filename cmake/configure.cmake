if (X64)
  # Build 64-bit CSE
  message("Making x64 build directory: ${CMAKE_CURRENT_SOURCE_DIR}/msvc/build")
  file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/msvc)
  file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/msvc/build64)
  message("Generating x64 project files...")
  execute_process(COMMAND ${CMAKE_COMMAND} ${CMAKE_CURRENT_SOURCE_DIR} -T v142,version=14.29.16.11 -A x64 -DCMAKE_SYSTEM_VERSION=10.0.20348.0
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/msvc/build64
    RESULT_VARIABLE success
  )
  if (NOT ${success} MATCHES "0")
    message(FATAL_ERROR "Generation x64 step failed.")
  endif()

else()
# Build 32-bit CSE 
  message("Making x32 build directory: ${CMAKE_CURRENT_SOURCE_DIR}/msvc/build")
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
endif()
