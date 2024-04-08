cmake_minimum_required(VERSION 3.15.0)

# MSVC runtime library flags are selected by an abstraction.
cmake_policy(SET CMP0091 NEW) # MSVC runtime library flags are selected by an abstraction.

project(CSE)

# target_link_libraries() allows use with targets in other directories. Set to new in 3.13 or higher.
cmake_policy(SET CMP0079 NEW)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

# Only create Release and Debug configuration types
# Set a default build type if none was specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Release' as none was specified.")
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release")
else()
  set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)
endif()

# Build Architecture
if(NOT CSE_BUILD_ARCHITECTURE)
  if (WIN32)
    set(CSE_BUILD_ARCHITECTURE "32" CACHE STRING "Choose architecture size for the build." FORCE)
  else()
    set(CSE_BUILD_ARCHITECTURE "64" CACHE STRING "Choose architecture size for the build." FORCE)
  endif()
    # Set the possible values of build type for cmake-gui
    set_property(CACHE CSE_BUILD_ARCHITECTURE PROPERTY STRINGS "32" "64")
else()
  if(NOT WIN32 AND CSE_BUILD_ARCHITECTURE STREQUAL "32")
    message(WARNING "CSE_BUILD_ARCHITECTURE was set to '32', but is not supported on Unix systems.")
    set(CSE_BUILD_ARCHITECTURE "64" CACHE STRING "Choose architecture size for the build." FORCE)
  endif()
endif()

# Build OS
if(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
  set(CSE_OS_NAME "macos")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
  set(CSE_OS_NAME "win")
else()
  string(TOLOWER ${CMAKE_SYSTEM_NAME} CSE_OS_NAME)
endif()

# Build Compiler
if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
  set(CSE_COMPILER_NAME "gcc")
else()
  string(TOLOWER ${CMAKE_CXX_COMPILER_ID} CSE_COMPILER_NAME)
endif()

if(NOT DEFINED CSE_EXECUTABLE_DIRECTORY)
  set(CSE_EXECUTABLE_DIRECTORY ${CMAKE_BINARY_DIR})
endif()

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

include(BuildFlags)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

find_package(Git QUIET)

add_subdirectory(vendor)

# Common source files TODO: Move to library when dtypes.h circularity is resolved.
set(cse_common_source
  "${CSE_SOURCE_DIR}/src/cvpak.cpp"
  "${CSE_SOURCE_DIR}/src/dmpak.cpp"
  "${CSE_SOURCE_DIR}/src/envpak.cpp"
  "${CSE_SOURCE_DIR}/src/lookup.cpp"
  "${CSE_SOURCE_DIR}/src/messages.cpp"
  "${CSE_SOURCE_DIR}/src/msgtbl.cpp"
  "${CSE_SOURCE_DIR}/src/psychro.cpp"
  "${CSE_SOURCE_DIR}/src/rmkerr.cpp"
  "${CSE_SOURCE_DIR}/src/strpak.cpp"
  "${CSE_SOURCE_DIR}/src/tdpak.cpp"
  "${CSE_SOURCE_DIR}/src/xiopak.cpp"
  "${CSE_SOURCE_DIR}/src/nummeth.cpp"
)

add_subdirectory(src)

set_directory_properties(PROPERTIES VS_STARTUP_PROJECT CSE)

enable_testing()

add_subdirectory(test)
