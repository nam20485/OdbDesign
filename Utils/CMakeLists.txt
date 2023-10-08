# CMakeList.txt : CMake project for OdbDesignServer
#

add_library(Utils SHARED "export.h" "Utils.cpp" "Utils.h" "ExitCode.h" "ThreadSafeQueue.h" "WorkQueueLoopThread.h" "Logger.h" "Logger.cpp" "CommandLineArgs.h" "CommandLineArgs.cpp")

# state that anybody linking to us needs to include the current source dir,
# while we don't.
target_include_directories(Utils
                           INTERFACE
                           $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
                           $<INSTALL_INTERFACE:include>)
