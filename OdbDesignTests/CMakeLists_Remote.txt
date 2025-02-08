# CMakeList.txt : CMake project for OdbDesignTests
#

enable_testing()

add_executable(OdbDesignTests
    "Fixtures/FileArchiveLoadFixture.h" 
    "Fixtures/FileArchiveLoadFixture.cpp"     
    "TestTests.cpp"
    "DesignLoadTests.cpp"
    "Fixtures/DesignNameValueParamTest.h"
    "FileArchiveLoadTests.cpp"
)

target_link_libraries(OdbDesignTests PRIVATE GTest::gtest_main)

# link to OdbDesign library
target_link_libraries(OdbDesignTests PRIVATE OdbDesign)

include(GoogleTest)
gtest_discover_tests(OdbDesignTests)