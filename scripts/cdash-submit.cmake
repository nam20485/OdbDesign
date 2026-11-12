# CTest driver script for CDash submission with an authentication token.
#
# my.cdash.org (CDash 5.5.2) requires authenticated submissions and reads the
# token ONLY from an "Authorization: Bearer <token>" HTTP header
# (SubmissionController: AuthTokenService::hash(request()->bearerToken())).
# Plain `ctest -T Submit` cannot send headers; only ctest_submit(HTTPHEADER)
# in a driver script like this one can. The token is never logged or
# committed - it arrives via the CDASH_SUBMIT_TOKEN environment variable.
#
# Run from the repository root:
#   ctest -S scripts/cdash-submit.cmake
#
# Required environment:
#   CDASH_SUBMIT_TOKEN       CDash authentication token ("Submit Only" scope,
#                            minted on the my.cdash.org profile page,
#                            "Authentication Tokens" section)
#   CTEST_SOURCE_DIRECTORY   repository root (for CTestConfig.cmake)
#   CTEST_BINARY_DIRECTORY   build root containing DartConfiguration.tcl
#   CTEST_SITE               dashboard site name, e.g. gha-ubuntu-24.04
#   CTEST_BUILD_NAME         dashboard build name, e.g. nam20485-linux-dynamic-release
# Optional environment:
#   CTEST_CONFIGURATION_TYPE multi-config generator config (Release); ignored
#                            by single-config generators
#   CTEST_TEST_TIMEOUT       per-test timeout seconds (default 1500)

if(NOT DEFINED ENV{CDASH_SUBMIT_TOKEN} OR "$ENV{CDASH_SUBMIT_TOKEN}" STREQUAL "")
  message(FATAL_ERROR "CDASH_SUBMIT_TOKEN is not set: my.cdash.org rejects untokened submissions")
endif()

set(CTEST_SOURCE_DIRECTORY "$ENV{CTEST_SOURCE_DIRECTORY}")
set(CTEST_BINARY_DIRECTORY "$ENV{CTEST_BINARY_DIRECTORY}")
set(CTEST_SITE "$ENV{CTEST_SITE}")
set(CTEST_BUILD_NAME "$ENV{CTEST_BUILD_NAME}")
set(CTEST_CONFIGURATION_TYPE "$ENV{CTEST_CONFIGURATION_TYPE}")
if("$ENV{CTEST_TEST_TIMEOUT}" STREQUAL "")
  set(CTEST_TEST_TIMEOUT 1500)
else()
  set(CTEST_TEST_TIMEOUT "$ENV{CTEST_TEST_TIMEOUT}")
endif()

# The build is already complete by the time this script runs (CI built it in
# an earlier step), so this is a test + submit only cycle.
ctest_start(Experimental)
ctest_test(BUILD "${CTEST_BINARY_DIRECTORY}")
ctest_submit(HTTPHEADER "Authorization: Bearer $ENV{CDASH_SUBMIT_TOKEN}" RETRY_COUNT 3 RETRY_DELAY 5)
