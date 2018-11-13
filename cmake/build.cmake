message("Making build directory: ${CMAKE_CURRENT_SOURCE_DIR}/msvc/build")
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/msvc)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/msvc/build)
message("Generating project files...")
execute_process(COMMAND ${CMAKE_COMMAND} ${CMAKE_CURRENT_SOURCE_DIR} -T v141_xp
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/msvc/build
  RESULT_VARIABLE success
)
if (NOT ${success} MATCHES "0")
  message("Generation step failed.")
  return()
endif()

message("Building CSE...")
execute_process(COMMAND ${CMAKE_COMMAND}
  --build ${CMAKE_CURRENT_SOURCE_DIR}/msvc/build
  --config Release
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/msvc/build
  RESULT_VARIABLE success
)
if (${success} MATCHES "0")
  message("Build Successful!")
else()
  message("Build failed.")
endif()
