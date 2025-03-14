﻿# CMakeList.txt : CMake project for OdbDesignServer
#

add_library(Utils SHARED "utils_export.h"   "ExitCode.h" "ThreadSafeQueue.h" "WorkQueueLoopThread.h" "Logger.h" "Logger.cpp" "CommandLineArgs.h" "CommandLineArgs.cpp" "bin2ascii.h" "ArchiveExtractor.cpp" "ArchiveExtractor.h" "libarchive_extract.cpp" "libarchive_extract.h" "str_utils.cpp" "str_utils.h"  "IJsonable.h" "IJsonable.cpp" "CrowReturnable.h" "JsonCrowReturnable.h" "timestamp.h" "timestamp.cpp" "StopWatch.h" "StopWatch.cpp" "UrlEncoding.h" "UrlEncoding.cpp" "StringVector.h" "equals_within.h" "equals_within.cpp" "crow_win.h" "fastmove.h" "fastmove.cpp" "FileReader.h" "FileReader.cpp" "EnumMap.h" "EnumMap.cpp" "CrossPlatform.h" "CrossPlatform.cpp" "macros.h")

# state that anybody linking to us needs to include the current source dir,
# while we don't.
target_include_directories(Utils
                           INTERFACE
                           $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
                           $<INSTALL_INTERFACE:include>)

## PCH
#if (NOT DEFINED ENV{CI})
    file (GLOB_RECURSE UTILS_HEADER_FILES "*.h")
    target_precompile_headers(Utils PRIVATE ${UTILS_HEADER_FILES})
#endif()

# Link to LibArchive
find_package(LibArchive REQUIRED)
target_link_libraries(Utils PRIVATE LibArchive::LibArchive)
