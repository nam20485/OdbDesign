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
    "Fixtures/TestUtils.h"
    "FeatureRecordSurfaceTests.cpp"
    "SymbolContractTests.cpp"
    #"GetLayerSymbolsTests.cpp"
    "MatrixFileTests.cpp"
    
    "../OdbDesignServer/Config/GrpcServiceConfig.cpp"
    "../OdbDesignServer/Services/OdbDesignServiceImpl.cpp" "GetLayerSymbolsTests.cpp" "GrpcBatchStreamTests.cpp" "FileArchiveLoadTests.cpp")

target_link_libraries(OdbDesignTests PRIVATE GTest::gtest_main GTest::gmock_main)

# link to OdbDesign library
target_link_libraries(OdbDesignTests PRIVATE OdbDesign)

# Link gRPC libraries for batch stream tests
find_package(gRPC CONFIG REQUIRED)
target_link_libraries(OdbDesignTests PRIVATE gRPC::gpr gRPC::grpc gRPC::grpc++ gRPC::grpc++_reflection)

# Include directories for OdbDesignServer sources and generated gRPC files
# The generated gRPC files are in OdbDesignServer's binary directory
get_filename_component(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
set(ODBDESIGN_SERVER_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/../OdbDesignServer")
set(GRPC_CODEGEN_DIR "${ODBDESIGN_SERVER_BINARY_DIR}/grpc")
target_include_directories(OdbDesignTests PRIVATE 
    ${REPO_ROOT}                                      # For OdbDesignServer/Services/... includes (repo root)
    ${ODBDESIGN_SERVER_BINARY_DIR}                   # For service.grpc.pb.h (generated files)
    ${GRPC_CODEGEN_DIR})                             # For grpc generated files

# Add generated gRPC sources (required for OdbDesignServiceImpl which uses these types)
set(GRPC_PROTO_SRCS
    ${GRPC_CODEGEN_DIR}/service.pb.cc
    ${GRPC_CODEGEN_DIR}/service.grpc.pb.cc)
set_source_files_properties(${GRPC_PROTO_SRCS} PROPERTIES GENERATED TRUE)
target_sources(OdbDesignTests PRIVATE ${GRPC_PROTO_SRCS})

# Ensure OdbDesignServer is built first so gRPC sources are generated
add_dependencies(OdbDesignTests OdbDesignServer)

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
