## Set platform toolset (must be done before project statement)
set(CMAKE_GENERATOR_TOOLSET v120_xp)

project(CSE)

cmake_minimum_required(VERSION 3.10.0)

set (CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

# Only create Release and Debug configuration types
set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)

include(CompilerFlags)
#include(cotire)

add_subdirectory(vendor)
add_subdirectory(src)
