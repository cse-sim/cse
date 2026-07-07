# Derives version information from git tags and generates a version header.
# Follows the pattern from bigladder/atheneum-forge for generic reuse.
#
# Can be used as a -P script (pass PROJECT_SOURCE_DIR, PROJECT_BINARY_DIR, and
# optionally GIT_EXECUTABLE via -D) or included from another script to obtain
# ${PROJECT_NAME}_git_tag and related version variables.
# configure_file is only called when PROJECT_BINARY_DIR is defined.

# Fallbacks when used outside a configured CMake project
if(NOT DEFINED PROJECT_NAME)
  set(PROJECT_NAME "CSE")
endif()

if(NOT DEFINED PROJECT_SOURCE_DIR)
  get_filename_component(PROJECT_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
endif()

macro(git_command args output_variable)
  execute_process(
    COMMAND ${GIT_EXECUTABLE} ${args}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    RESULT_VARIABLE _exit_status
    ERROR_VARIABLE _error_output
    OUTPUT_VARIABLE _temp
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  if(NOT _exit_status MATCHES "0")
    message(SEND_ERROR "Unable to retrieve ${output_variable}.")
    message(FATAL_ERROR "${_error_output}")
  endif()
  set(${output_variable} "${_temp}")
endmacro()

macro(process_git_version)
  if(NOT DEFINED GIT_EXECUTABLE)
    find_package(Git)
  else()
    set(GIT_FOUND TRUE)
  endif()

  if(GIT_FOUND)
    # Branch (CI environment variable takes precedence)
    if(DEFINED ENV{CI_GIT_BRANCH})
      set(${PROJECT_NAME}_git_branch "$ENV{CI_GIT_BRANCH}")
    else()
      git_command("rev-parse;--abbrev-ref;HEAD" ${PROJECT_NAME}_git_branch)
    endif()

    # Commit SHA
    git_command("rev-parse;--verify;--short;HEAD" ${PROJECT_NAME}_git_sha)

    # Tag and build number
    git_command("tag" _tag_list)
    if(_tag_list MATCHES "^$")
      set(${PROJECT_NAME}_git_tag "v0.0.0")
      git_command("rev-list;--count;HEAD" ${PROJECT_NAME}_git_build_number)
    else()
      git_command("describe;--tags;--abbrev=0" ${PROJECT_NAME}_git_tag)
      git_command("rev-list;--count;HEAD;^${${PROJECT_NAME}_git_tag}" ${PROJECT_NAME}_git_build_number)
    endif()

    # Dirty state
    git_command("diff;--shortstat" ${PROJECT_NAME}_git_status)
    if(${PROJECT_NAME}_git_status MATCHES "changed")
      set(${PROJECT_NAME}_git_status "dirty")
    else()
      unset(${PROJECT_NAME}_git_status)
    endif()

    # Parse SemVer components
    set(_semver_normal "v(0|[1-9][0-9]*)\\.(0|[1-9][0-9]*)\\.(0|[1-9][0-9]*)")
    set(_semver_prerelease "(-((0|[1-9][0-9]*|[0-9]*[a-zA-Z-][0-9a-zA-Z-]*)(\\.(0|[1-9][0-9]*|[0-9]*[a-zA-Z-][0-9a-zA-Z-]*))*))")
    set(_semver_regex "^${_semver_normal}${_semver_prerelease}?$")

    if(NOT ${PROJECT_NAME}_git_tag MATCHES "${_semver_regex}")
      message(FATAL_ERROR "Tag \"${${PROJECT_NAME}_git_tag}\" does not conform to SemVer (expected v<major>.<minor>.<patch>[-<prerelease>]).")
    endif()

    string(REGEX REPLACE "${_semver_regex}" "\\1" ${PROJECT_NAME}_version_major "${${PROJECT_NAME}_git_tag}")
    string(REGEX REPLACE "${_semver_regex}" "\\2" ${PROJECT_NAME}_version_minor "${${PROJECT_NAME}_git_tag}")
    string(REGEX REPLACE "${_semver_regex}" "\\3" ${PROJECT_NAME}_version_patch "${${PROJECT_NAME}_git_tag}")

    if(${PROJECT_NAME}_git_tag MATCHES "^${_semver_normal}${_semver_prerelease}$")
      string(REGEX REPLACE "${_semver_regex}" "\\4" ${PROJECT_NAME}_version_prerelease "${${PROJECT_NAME}_git_tag}")
    else()
      set(${PROJECT_NAME}_version_prerelease "")
    endif()

    # Build metadata
    if(NOT ${PROJECT_NAME}_git_build_number MATCHES "^0$")
      set(${PROJECT_NAME}_version_meta
        "+${${PROJECT_NAME}_git_branch}.${${PROJECT_NAME}_git_sha}.${${PROJECT_NAME}_git_build_number}")
      if(DEFINED ${PROJECT_NAME}_git_status)
        string(APPEND ${PROJECT_NAME}_version_meta ".${${PROJECT_NAME}_git_status}")
      endif()
    else()
      if(DEFINED ${PROJECT_NAME}_git_status)
        set(${PROJECT_NAME}_version_meta "+${${PROJECT_NAME}_git_status}")
      else()
        set(${PROJECT_NAME}_version_meta "")
      endif()
    endif()

  else()
    message(WARNING "Failed to find git. Unable to establish version information for ${PROJECT_NAME}.")
    set(${PROJECT_NAME}_version_major "0")
    set(${PROJECT_NAME}_version_minor "0")
    set(${PROJECT_NAME}_version_patch "0")
    set(${PROJECT_NAME}_version_prerelease "")
    set(${PROJECT_NAME}_version_meta "+git.not.found")
    set(${PROJECT_NAME}_git_tag "v0.0.0")
  endif()

  set(${PROJECT_NAME}_version
    "v${${PROJECT_NAME}_version_major}.${${PROJECT_NAME}_version_minor}.${${PROJECT_NAME}_version_patch}${${PROJECT_NAME}_version_prerelease}${${PROJECT_NAME}_version_meta}")
endmacro()

process_git_version()

message("Building ${PROJECT_NAME} ${${PROJECT_NAME}_version_major}.${${PROJECT_NAME}_version_minor}.${${PROJECT_NAME}_version_patch}${${PROJECT_NAME}_version_prerelease}${${PROJECT_NAME}_version_meta}")

if(DEFINED PROJECT_BINARY_DIR)
  configure_file(
    "${PROJECT_SOURCE_DIR}/src/csevrsn.in.h"
    "${PROJECT_BINARY_DIR}/src/csevrsn.h"
  )
endif()
