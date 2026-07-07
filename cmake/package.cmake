# cmake/package.cmake
# Creates a release archive containing a built executable.
#
# Usage:
#   cmake [-DBUILD_DIRECTORY=<dir>] [-DRELEASE_TAG=<tag>] -P cmake/package.cmake
#
# Optional:
#   BUILD_DIRECTORY    Path to the build output directory. Defaults to "build"
#                      relative to the project root.
#   RELEASE_TAG        Version string used in the archive name (e.g., "v1.2.3"
#                      or "v1.2.3+main.abc1234.5" for a dev build). When
#                      omitted, derived from git via CSEVersion.cmake using the
#                      full version including build metadata.
#
# The executable name, compiler identifier, and build architecture are all read
# from the build directory's CMakeCache.txt (CSE_EXECUTABLE_NAME,
# CSE_COMPILER_NAME, and CSE_BUILD_ARCHITECTURE), ensuring the archive always
# matches what was actually built.
#
# Output:
#   <name>-<tag>-<os>-<arch>[-<compiler>].zip     (Windows)
#   <name>-<tag>-<os>-<arch>[-<compiler>].tar.gz  (macOS / Linux)

if(NOT DEFINED BUILD_DIRECTORY)
  get_filename_component(BUILD_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/../build" ABSOLUTE)
endif()

if(NOT EXISTS "${BUILD_DIRECTORY}/CMakeCache.txt")
  message(FATAL_ERROR "No CMakeCache.txt found in BUILD_DIRECTORY: ${BUILD_DIRECTORY}")
endif()

load_cache("${BUILD_DIRECTORY}" READ_WITH_PREFIX _cache_
  CSE_EXECUTABLE_NAME
  CSE_BUILD_ARCHITECTURE
  CSE_COMPILER_NAME
)

if(NOT _cache_CSE_EXECUTABLE_NAME)
  message(FATAL_ERROR "CSE_EXECUTABLE_NAME not found in ${BUILD_DIRECTORY}/CMakeCache.txt — has the project been configured?")
endif()

if(NOT DEFINED RELEASE_TAG)
  include("${CMAKE_CURRENT_LIST_DIR}/CSEVersion.cmake")
  set(RELEASE_TAG "${CSE_version}")
endif()

if(WIN32)
  set(_os "windows")
  set(_exe_suffix ".exe")
elseif(APPLE)
  set(_os "macos")
  set(_exe_suffix "")
else()
  set(_os "linux")
  set(_exe_suffix "")
endif()

if(_cache_CSE_BUILD_ARCHITECTURE STREQUAL "32")
  set(_arch "x86")
else()
  cmake_host_system_information(RESULT _arch QUERY OS_PLATFORM)
  string(TOLOWER "${_arch}" _arch)
  if(_arch STREQUAL "amd64")  # Windows reports AMD64; normalize to x86_64
    set(_arch "x86_64")
  endif()
endif()

set(_exe "${_cache_CSE_EXECUTABLE_NAME}${_exe_suffix}")
set(_canonical_exe "cse${_exe_suffix}")

if(NOT EXISTS "${BUILD_DIRECTORY}/${_exe}")
  message(FATAL_ERROR "Executable not found: ${BUILD_DIRECTORY}/${_exe}")
endif()

if(NOT _exe STREQUAL _canonical_exe)
  execute_process(
    COMMAND ${CMAKE_COMMAND} -E copy "${BUILD_DIRECTORY}/${_exe}" "${BUILD_DIRECTORY}/${_canonical_exe}"
    RESULT_VARIABLE _result
  )
  if(_result)
    message(FATAL_ERROR "Failed to copy ${_exe} to ${_canonical_exe}")
  endif()
endif()

set(_archive_base "cse-${_os}-${_arch}")
if(DEFINED _cache_CSE_COMPILER_NAME AND NOT _cache_CSE_COMPILER_NAME STREQUAL "")
  string(APPEND _archive_base "-${_cache_CSE_COMPILER_NAME}")
endif()
string(APPEND _archive_base "-${RELEASE_TAG}")

if(WIN32)
  set(_archive "${BUILD_DIRECTORY}/${_archive_base}.zip")
  execute_process(
    COMMAND ${CMAKE_COMMAND} -E tar cf "${_archive}" --format=zip -- "${_canonical_exe}"
    WORKING_DIRECTORY "${BUILD_DIRECTORY}"
    RESULT_VARIABLE _result
  )
else()
  set(_archive "${BUILD_DIRECTORY}/${_archive_base}.tar.gz")
  execute_process(
    COMMAND ${CMAKE_COMMAND} -E tar czf "${_archive}" -- "${_canonical_exe}"
    WORKING_DIRECTORY "${BUILD_DIRECTORY}"
    RESULT_VARIABLE _result
  )
endif()

if(_result)
  message(FATAL_ERROR "Failed to create archive: ${_archive}")
endif()

if(NOT _exe STREQUAL _canonical_exe)
  file(REMOVE "${BUILD_DIRECTORY}/${_canonical_exe}")
endif()

message(STATUS "Created: ${_archive}")
