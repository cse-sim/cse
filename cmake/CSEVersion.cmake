# Generate CSE Version header

execute_process(
  COMMAND git rev-parse --abbrev-ref HEAD
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  RESULT_VARIABLE git_branch_exit_status
  OUTPUT_VARIABLE GIT_BRANCH
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

if (NOT ${git_branch_exit_status} MATCHES "0")
  set(GIT_BRANCH "unknown-branch")
endif()

execute_process(
  COMMAND git rev-parse --verify --short HEAD
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  RESULT_VARIABLE git_sha_exit_status
  OUTPUT_VARIABLE GIT_SHA
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

if (NOT ${git_sha_exit_status} MATCHES "0")
  set(GIT_SHA "unknown-commit")
endif()

execute_process(
  COMMAND git describe --tags --abbrev=0
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  RESULT_VARIABLE git_tag_exit_status
  OUTPUT_VARIABLE GIT_TAG
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

if (NOT ${git_tag_exit_status} MATCHES "0")
  set(GIT_TAG "unknown-tag")
endif()

if (GIT_TAG MATCHES "^v[0-9]+\\.[0-9]+\\.[0-9]+$")
  string(REGEX REPLACE "^v([0-9]+)\\.[0-9]+\\.[0-9]+$" "\\1" CSEVRSN_MAJOR "${GIT_TAG}")
  string(REGEX REPLACE "^v[0-9]+\\.([0-9]+)\\.[0-9]+$" "\\1" CSEVRSN_MINOR "${GIT_TAG}")
  string(REGEX REPLACE "^v[0-9]+\\.[0-9]+\\.([0-9]+)$" "\\1" CSEVRSN_PATCH "${GIT_TAG}")
elseif(GIT_TAG MATCHES "^cse\\.[0-9]+$")
  # old version scheme
  set(CSEVRSN_MAJOR "0")
  string(REGEX REPLACE "^cse\\.([0-9]+)$" "\\1" CSEVRSN_MINOR "${GIT_TAG}")
  set(CSEVRSN_PATCH "0")
else()
  set(CSEVRSN_MAJOR "?")
  set(CSEVRSN_MINOR "?")
  set(CSEVRSN_PATCH "?")
endif()

execute_process(
  COMMAND git rev-list --count HEAD ^${GIT_TAG}
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  RESULT_VARIABLE git_build_exit_status
  OUTPUT_VARIABLE GIT_BUILD
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

if (NOT ${git_build_exit_status} MATCHES "0")
  set(GIT_BUILD "unknown-build-number")
endif()

if(NOT ${GIT_BUILD} MATCHES "^0$")
  set(CSEVRSN_META "+${GIT_BRANCH}.${GIT_SHA}.${GIT_BUILD}")
else()
  set(CSEVRSN_META "")
endif()

message("Building CSE ${CSEVRSN_MAJOR}.${CSEVRSN_MINOR}.${CSEVRSN_PATCH}${CSEVRSN_META}")

configure_file(
  "${PROJECT_SOURCE_DIR}/src/csevrsn.h.in"
  "${PROJECT_SOURCE_DIR}/src/csevrsn.h"
)

