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
    "ArchiveSecurityTests.cpp"
    "Fixtures/TestUtils.h"
    "FeatureRecordSurfaceTests.cpp"
    "SymbolContractTests.cpp"
    "ConnectivityContractTests.cpp"
    "MatrixFileTests.cpp"
    "ComponentHeightPipelineTests.cpp"
    "DesignCacheThreadSafetyTests.cpp"
    "DesignCacheSingleFlightTests.cpp"
    "GrpcServiceConfigTests.cpp"
    "BasicRequestAuthenticationTests.cpp"
    "Utf8SanitizerTests.cpp"
    "HttpCachingTests.cpp"

    # Server component tests (OdbDesignServiceImpl, config, stream/batch
    # early-error handling). The server implementation is NOT compiled here:
    # OdbDesignServerLib (services, config, controllers, generated gRPC code)
    # is linked below so tests exercise the same composition as production
    # with exactly one protobuf descriptor pool.
    "GetStandardFontsTests.cpp"
    "GetLayerSymbolsTests.cpp"
    "RequestLoadDesignTests.cpp"
    "ResponseCacheTests.cpp"
    "DesignStreamTests.cpp"
    "DesignStreamMemoryTests.cpp"
    "DesignFetchBenchmarkTests.cpp"
    "AttributeLookupTableTests.cpp"
    "GrpcParseErrorDetailTests.cpp"
    "GrpcBatchStreamTests.cpp"
    "FileArchiveLoadTests.cpp"
    "ParserOnlyHeaderTests.cpp"
    "ToolsFileTests.cpp")

target_link_libraries(OdbDesignTests PRIVATE GTest::gtest_main GTest::gmock_main)

# link to OdbDesign library
target_link_libraries(OdbDesignTests PRIVATE OdbDesign)

# Server component tests link the real server library instead of compiling
# selected OdbDesignServer sources by hand. OdbDesignServerLib owns the
# services (OdbDesignServiceImpl, DesignStreamWalker, StreamSpoolCache),
# config (GrpcServiceConfig), controllers and the generated gRPC/protobuf
# code, and PUBLICly propagates OdbDesign, the gRPC targets and the include
# dirs for OdbDesignServer/ and the generated .pb.h files. Linking it keeps
# a single owner for production sources/codegen and exactly one protobuf
# descriptor pool in the test executable (single shared protobuf runtime
# under the x64-linux-dynamic triplet).
target_link_libraries(OdbDesignTests PRIVATE OdbDesignServerLib)

# Source-tree paths for tests (M0.3 golden fixtures, gitignored local design
# corpora) and for the "OdbDesignServer/..." style includes used by server
# component tests. Everything server-side (OdbDesignServer/, generated
# service.pb.h / service.grpc.pb.h) comes from OdbDesignServerLib's PUBLIC
# include dirs; only the repo root must be added here. The define must
# follow the REPO_ROOT computation above or the paths silently become
# cwd-relative.
get_filename_component(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
target_include_directories(OdbDesignTests PRIVATE ${REPO_ROOT})
target_compile_definitions(OdbDesignTests PRIVATE ODBDESIGN_SOURCE_DIR="${REPO_ROOT}")
# F004 fail-closed test (BasicRequestAuthenticationTests.cpp) spawns the real
# server binary to prove auth-enabled startup without credentials exits
# PreServerRunFailed and opens no listener. The genex tracks the actual
# output path across generators/output-dir changes (OdbDesignServerLib's
# PUBLIC include dirs already supply the OdbDesignServer/ and generated
# .pb.h headers, and OdbDesignServerLib owns all server sources + codegen —
# the hand-compiled sources and duplicate service.pb.cc sources are gone).
# The dependency orders the server executable build before the test target
# so the spawned binary exists even for target-scoped builds.
target_compile_definitions(OdbDesignTests PRIVATE
    ODBDESIGN_SERVER_EXECUTABLE_PATH="$<TARGET_FILE:OdbDesignServer>")
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

# The parser-only surface test must NOT receive the force-included fixture
# headers from the PCH: its point is proving the TU compiles with nothing but
# OdbDesignParser.h (no App/, no Crow).
set_source_files_properties("ParserOnlyHeaderTests.cpp" PROPERTIES SKIP_PRECOMPILE_HEADERS ON)
