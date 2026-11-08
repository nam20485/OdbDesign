## This file should be placed in the root directory of your project.
## Then modify the CMakeLists.txt file in the root directory of your
## project to incorporate the testing dashboard.
##
## # The following are required to submit to the CDash dashboard:
##   ENABLE_TESTING()
##   INCLUDE(CTest)

set(CTEST_PROJECT_NAME OdbDesign)
set(CTEST_NIGHTLY_START_TIME 00:00:00 UTC)

if(CMAKE_VERSION VERSION_GREATER 3.14)
  set(CTEST_SUBMIT_URL https://my.cdash.org/submit.php?project=OdbDesign)
  # my.cdash.org requires authenticated submissions (a real ctest -T Submit
  # without a token is rejected with "Invalid Token"). The token is appended
  # from the environment when present - CI passes the CDASH_TOKEN secret
  # through the submit step; local runs export CDASH_SUBMIT_TOKEN. The token
  # itself must never be committed to this file.
  if(DEFINED ENV{CDASH_SUBMIT_TOKEN})
    string(APPEND CTEST_SUBMIT_URL "&token=$ENV{CDASH_SUBMIT_TOKEN}")
  endif()
else()
  set(CTEST_DROP_METHOD "https")
  set(CTEST_DROP_SITE "my.cdash.org")
  set(CTEST_DROP_LOCATION "/submit.php?project=OdbDesign")
endif()

set(CTEST_DROP_SITE_CDASH TRUE)

