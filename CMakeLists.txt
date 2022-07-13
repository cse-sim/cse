project(CSE)

cmake_minimum_required(VERSION 3.10.0)

set (CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

# Only create Release and Debug configuration types
set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)

include(BuildFlags)
#include(cotire)

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
