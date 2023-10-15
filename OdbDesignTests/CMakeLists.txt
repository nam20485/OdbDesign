enable_testing()

add_executable(OdbDesignTests
  OdbDesignTests.h
  OdbDesignTests.cpp
)
target_link_libraries(OdbDesignTests PUBLIC GTest::gtest_main)

# link to OdbDesign library
target_link_libraries(OdbDesignTests PUBLIC OdbDesign)

include(GoogleTest)
gtest_discover_tests(OdbDesignTests)