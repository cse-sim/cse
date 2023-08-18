if ("${BUILD_ARCHITECTURE}" STREQUAL "64")
    set(TARGET_VS_ARCHITECTURE x64)
else()
    set(TARGET_VS_ARCHITECTURE Win32)
endif()

include(cmake/utility.cmake)
if (NOT DEFINED BUILD_DIRECTORY)
    set_build_directory()
    set(EXECUTABLE_DIRECTORY ${BUILDS_DIRECTORY})
endif ()

if (NOT DEFINED CONFIGURATION)
    set(CONFIGURATION Release)
endif ()

file(MAKE_DIRECTORY ${BUILD_DIRECTORY})

set(configure_command ${CMAKE_COMMAND} ${CMAKE_CURRENT_SOURCE_DIR} -DCSE_BUILD_ARCHITECTURE=${BUILD_ARCHITECTURE})

if (DEFINED EXECUTABLE_DIRECTORY)
    set(configure_command ${configure_command} -DCSE_EXECUTABLE_DIRECTORY=${EXECUTABLE_DIRECTORY})
endif ()

if ("${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "Windows")
    set(configure_command ${configure_command} -T v142,version=14.29.16.11 -A ${TARGET_VS_ARCHITECTURE}  -DCMAKE_SYSTEM_VERSION=10.0.20348.0)
else()
    set(configure_command ${configure_command} -DCMAKE_BUILD_TYPE=${CONFIGURATION})
endif()

message("Generating project files...")
execute_process(COMMAND ${configure_command}
    WORKING_DIRECTORY ${BUILD_DIRECTORY}
    RESULT_VARIABLE success
)

if (NOT ${success} MATCHES "0")
  message(FATAL_ERROR "Generation step failed.")
endif()