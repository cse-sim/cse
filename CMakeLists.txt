cmake_policy(SET CMP0091 NEW) # MSVC runtime library flags are selected by an abstraction.
project(CSE)

cmake_minimum_required(VERSION 3.10.0)

# target_link_libraries() allows use with targets in other directories. Set to new in 3.13 or higher.
cmake_policy(SET CMP0079 NEW)

set (CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

# Only create Release and Debug configuration types
set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)

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
)

add_subdirectory(src)

set_directory_properties(PROPERTIES VS_STARTUP_PROJECT CSE)

enable_testing()

add_subdirectory(test)
