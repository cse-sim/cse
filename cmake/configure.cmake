include(cmake/utility.cmake)
set_build_configuration()
if (NOT DEFINED BUILD_DIRECTORY)
    set(BUILDS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/builds")
    set(BUILD_DIRECTORY "${BUILDS_DIRECTORY}/${BUILD_CONFIGURATION}")
    set(EXECUTABLE_DIRECTORY ${BUILDS_DIRECTORY})
endif ()

file(MAKE_DIRECTORY ${BUILD_DIRECTORY})

set(configure_command ${CMAKE_COMMAND} ${CMAKE_CURRENT_SOURCE_DIR} -DCSE_BUILD_ARCHITECTURE=${BUILD_ARCHITECTURE})

if (DEFINED EXECUTABLE_DIRECTORY)
    set(configure_command ${configure_command} -DCSE_EXECUTABLE_DIRECTORY=${EXECUTABLE_DIRECTORY})
endif ()

if (DEFINED BUILD_DOCS_WITH_ALL)
    set(configure_command ${configure_command} -DCSE_BUILD_DOCUMENTATION=BuildWithAll)
endif ()

if ("${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "Windows")
    if (${COMPILER_ID} STREQUAL "msvc")
        # Explicitly set versions to guarantee computational stability
        #    toolset version (-T, version)
        #    SDK version (CMAKE_SYSTEM_VERSION)
        #        Supported list of SDK versions: https://developer.microsoft.com/en-us/windows/downloads/sdk-archive/
        set(configure_command ${configure_command} -T v142,version=14.29.16.11 -A ${TARGET_VS_ARCHITECTURE} -DCMAKE_SYSTEM_VERSION=10.0.26100.0)
    elseif(${COMPILER_ID} STREQUAL "clang")
        set(configure_command ${configure_command} -T ClangCL -A ${TARGET_VS_ARCHITECTURE})
    endif ()
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