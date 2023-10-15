enable_testing()

add_executable(OdbDesignTests
  OdbDesignTests.h
  OdbDesignTests.cpp
)
target_link_libraries(OdbDesignTests PUBLIC GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(OdbDesignTests)