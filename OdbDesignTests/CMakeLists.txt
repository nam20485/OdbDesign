# CMakeList.txt : CMake project for OdbDesignTests
#

add_executable(OdbDesignTests
    "Fixtures/FileArchiveLoadFixture.h" 
    "Fixtures/FileArchiveLoadFixture.cpp"     
    "TestTests.cpp"
    "DesignCacheLoadTests.cpp"
    "Fixtures/DesignNameValueParamTest.h"   
    "FileArchiveTests.cpp"
  "ProtobufSerializationTests.cpp" "FileReaderTests.cpp" "ArchiveTests.cpp" "CrossPlatformTests.cpp" "Fixtures/TestDataFixture.h" "Fixtures/TestDataFixture.cpp" "ArchiveExtractorTests.cpp")

target_link_libraries(OdbDesignTests PRIVATE GTest::gtest_main GTest::gmock_main)

# link to OdbDesign library
target_link_libraries(OdbDesignTests PRIVATE OdbDesign)

include(GoogleTest)
gtest_discover_tests(OdbDesignTests)

## PCH
#if (NOT DEFINED ENV{CI})
    file (GLOB_RECURSE OdbDesignTests_HEADER_FILES "*.h")
    target_precompile_headers(OdbDesignTests PRIVATE ${OdbDesignTests_HEADER_FILES})
#endif()