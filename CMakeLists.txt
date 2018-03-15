project(CSE)

cmake_minimum_required(VERSION 3.10.0)

set (CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

# Only create Release and Debug configuration types
set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)

include(BuildFlags)
#include(cotire)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_subdirectory(vendor)
add_subdirectory(src)
add_subdirectory(bin)

#include(ctest)

add_subdirectory(test)
