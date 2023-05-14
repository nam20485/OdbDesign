# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.15)

# require C++0x17 compiler
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("OdbDesign")

if (MSVC)
    # control where the static and shared libraries are built so that on windows
    # we don't need to tinker with the path to run the executable
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")
endif()

if (MSVC)
    # warning level 4
    add_compile_options(/W4)
    # treat warnings as errors
    add_compile_options(/WX)
    # ignore warning C4251: 'identifier' : class 'type' needs to have dll-interface to be used by clients of class 'type2'
    add_compile_options(/wd4251)
else()
    # additional warnings    
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Include sub-projects.
add_subdirectory("OdbDesignApp")
add_subdirectory("OdbDesignLib")

target_link_libraries(OdbDesignApp PUBLIC OdbDesign)