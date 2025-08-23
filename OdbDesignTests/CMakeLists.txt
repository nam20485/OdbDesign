# CMakeList.txt : CMake project for OdbDesignTests
#

add_executable(OdbDesignTests
    "Fixtures/FileArchiveLoadFixture.h" 
    "Fixtures/FileArchiveLoadFixture.cpp"     
    "TestTests.cpp"
    "DesignCacheLoadTests.cpp"
    "Fixtures/DesignNameValueParamTest.h"   
    "FileArchiveTests.cpp"
    "ProtobufSerializationTests.cpp" 
    "FileReaderTests.cpp" 
    "ArchiveTests.cpp" 
    "CrossPlatformTests.cpp" 
    "Fixtures/TestDataFixture.h" 
    "Fixtures/TestDataFixture.cpp" 
    "ArchiveExtractorTests.cpp"
    "PerformanceTests.cpp"
    "EnhancedTests.cpp"
    "IntegrationTests.cpp"
    "Fixtures/TestUtils.h")

target_link_libraries(OdbDesignTests PRIVATE GTest::gtest_main GTest::gmock_main)

# link to OdbDesign library
target_link_libraries(OdbDesignTests PRIVATE OdbDesign)

include(GoogleTest)
# Defer test discovery to ctest time to avoid executing the test binary during build
# This prevents build failures when runtime dependencies or environment variables
# are not yet available at link/post-build time.
gtest_discover_tests(OdbDesignTests DISCOVERY_MODE PRE_TEST)

## PCH
#if (NOT DEFINED ENV{CI})
    file (GLOB_RECURSE OdbDesignTests_HEADER_FILES "*.h")
    target_precompile_headers(OdbDesignTests PRIVATE ${OdbDesignTests_HEADER_FILES})
#endif()