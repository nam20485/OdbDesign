# CMakeList.txt : CMake project for OdbDesignTests
#

enable_testing()

add_executable(OdbDesignTests
  OdbDesignTests.h
 "FileArchiveLoadFixture.h" "FileArchiveLoadFixture.cpp"
  "FileArchiveLoadTests.cpp"
 "TestTests.cpp")

target_link_libraries(OdbDesignTests PRIVATE GTest::gtest_main)

# link to OdbDesign library
target_link_libraries(OdbDesignTests PRIVATE OdbDesign)

include(GoogleTest)
gtest_discover_tests(OdbDesignTests)