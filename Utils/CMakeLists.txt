# CMakeList.txt : CMake project for OdbDesignServer
#

add_library(Utils SHARED "export.h" "Utils.cpp" "Utils.h" "ExitCode.h" "ThreadSafeQueue.h" "WorkQueueLoopThread.h" "Logger.h" "Logger.cpp" "CommandLineArgs.h" "CommandLineArgs.cpp" "bin2ascii.h" "ArchiveExtractor.cpp" "ArchiveExtractor.h" "libarchive_extract.cpp" "libarchive_extract.h" "str_trim.cpp" "str_trim.h")

# state that anybody linking to us needs to include the current source dir,
# while we don't.
target_include_directories(Utils
                           INTERFACE
                           $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
                           $<INSTALL_INTERFACE:include>)

# Link to LibArchive
#find_package(LibArchive REQUIRED)
#target_include_directories(OdbDesign PRIVATE ${LibArchive_INCLUDE_DIRS})
#target_link_libraries(OdbDesign PUBLIC ${LibArchive_LIBRARIES})

find_package(LibArchive REQUIRED)
target_link_libraries(Utils PRIVATE LibArchive::LibArchive) # CMake >= 3.17
