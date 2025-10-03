# Generate CSE Version header

if(DEFINED ENV{CI_GIT_BRANCH})
  set(GIT_BRANCH "$ENV{CI_GIT_BRANCH}")
else()
  execute_process(
    COMMAND git rev-parse --abbrev-ref HEAD
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    RESULT_VARIABLE git_branch_exit_status
    OUTPUT_VARIABLE GIT_BRANCH
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  if (NOT ${git_branch_exit_status} MATCHES "0")
    message(FATAL_ERROR "GIT_BRANCH is not accessible." )
  endif()
endif()

execute_process(
  COMMAND ${GIT_EXECUTABLE} rev-parse --verify --short HEAD
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  RESULT_VARIABLE git_sha_exit_status
  OUTPUT_VARIABLE GIT_SHA
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

if (NOT ${git_sha_exit_status} MATCHES "0")
    message(FATAL_ERROR "GIT_SHA is not accessible." )
endif()

execute_process(
  COMMAND ${GIT_EXECUTABLE} describe --tags --abbrev=0
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  RESULT_VARIABLE git_tag_exit_status
  OUTPUT_VARIABLE GIT_TAG
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

if (NOT ${git_tag_exit_status} MATCHES "0")
  message(FATAL_ERROR "GIT_TAG is not accessible." )
endif()

if (GIT_TAG MATCHES "^v[0-9]+\\.[0-9]+\\.[0-9]+(\\-[0-9A-Za-z-]+)?$")
  string(REGEX REPLACE "^v([0-9]+)\\.[0-9]+\\.[0-9]+(\\-[0-9A-Za-z-]+)?$" "\\1" CSEVRSN_MAJOR "${GIT_TAG}")
  string(REGEX REPLACE "^v[0-9]+\\.([0-9]+)\\.[0-9]+(\\-[0-9A-Za-z-]+)?$" "\\1" CSEVRSN_MINOR "${GIT_TAG}")
  string(REGEX REPLACE "^v[0-9]+\\.[0-9]+\\.([0-9]+)(\\-[0-9A-Za-z-]+)?$" "\\1" CSEVRSN_PATCH "${GIT_TAG}")
  if (GIT_TAG MATCHES "^v[0-9]+\\.[0-9]+\\.[0-9]+(\\-[0-9A-Za-z-]+)$")
    string(REGEX REPLACE "^v[0-9]+\\.[0-9]+\\.[0-9]+(\\-[0-9A-Za-z-]+)$" "\\1" CSEVRSN_PRERELEASE "${GIT_TAG}")
  endif()
elseif(GIT_TAG MATCHES "^cse\\.[0-9]+$")
  # old version scheme
  set(CSEVRSN_MAJOR "0")
  string(REGEX REPLACE "^cse\\.([0-9]+)$" "\\1" CSEVRSN_MINOR "${GIT_TAG}")
  set(CSEVRSN_PATCH "0")
else()
  message(FATAL_ERROR "GIT_TAG ${GIT_TAG} must have format 'v<major>.<minor>.<patch>' or 'v<major>.<minor>.<patch>-<prerelease>'." )
endif()

execute_process(
  COMMAND ${GIT_EXECUTABLE} rev-list --count HEAD ^${GIT_TAG}
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  RESULT_VARIABLE git_build_exit_status
  OUTPUT_VARIABLE GIT_BUILD
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

if (NOT ${git_build_exit_status} MATCHES "0")
  message(FATAL_ERROR "GIT_BUILD value ${GIT_BUILD} is not accessible." )
endif()

# Check for modified ("dirty") state
execute_process(
  COMMAND ${GIT_EXECUTABLE} diff --shortstat
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  RESULT_VARIABLE git_stat_exit_status
  OUTPUT_VARIABLE GIT_STATUS
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

if (${GIT_STATUS} MATCHES "changed")
  set(GIT_STATUS ".dirty")
endif()

if(NOT ${GIT_BUILD} MATCHES "^0$")
  set(CSEVRSN_META "+${GIT_BRANCH}.${GIT_SHA}.${GIT_BUILD}${GIT_STATUS}")
else()
  set(CSEVRSN_META "")
endif()
#TODO: Add OS, compiler, and target architecture Ex. ${TARGET_OS}-${CMAKE_CXX_COMPILER_ID}-${CSE_BUILD_ARCHITECTURE}
message("Building CSE ${CSEVRSN_MAJOR}.${CSEVRSN_MINOR}.${CSEVRSN_PATCH}${CSEVRSN_PRERELEASE}${CSEVRSN_META}")

configure_file(
  "${PROJECT_SOURCE_DIR}/src/csevrsn.in.h"
  "${PROJECT_BINARY_DIR}/src/csevrsn.h"
)
