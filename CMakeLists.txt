# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required(VERSION 3.21)

# require C++0x17 compiler
set(MY_CXX_STANDARD 20)
set(CXX_STANDARD ${MY_CXX_STANDARD})
set(CMAKE_CXX_STANDARD ${MY_CXX_STANDARD})
set(CMAKE_CXX_STANDARD_REQUIRED True)

# required for SWIG python wrapper
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

## treat warnings as errors (https://cmake.org/cmake/help/latest/variable/CMAKE_COMPILE_WARNING_AS_ERROR.html#variable:CMAKE_COMPILE_WARNING_AS_ERROR)
#set(CMAKE_COMPILE_WARNING_AS_ERROR True)

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

# increase warnings in a compiler-specific manner
if (MSVC)
    # warning level 4
    add_compile_options(/W4)
#    # treat warnings as errors
#    add_compile_options(/WX)
    # ignore C4251: 'identifier' : class 'type' needs to have dll-interface to be used by clients of class 'type2'
    add_compile_options(/wd4251)
    # ignore C4100: 'identifier': unreferenced formal parameter
    add_compile_options(/wd4100)
else()
    # additional warnings    
    add_compile_options(-Wall -Wextra -Wpedantic)
    # treat warnings as errors
    #add_compile_options(-Werror)
    # ignore warning: unused parameter 'self'
    add_compile_options(-Wno-unused-parameter)
endif()

# Include sub-projects.
add_subdirectory("OdbDesignLib")
#add_subdirectory("OdbDesignApp")
add_subdirectory("OdbDesignServer")
add_subdirectory("Utils")

# link to OdbDesign library in both executable targets
#target_link_libraries(OdbDesignApp PUBLIC OdbDesign)
target_link_libraries(OdbDesignServer PUBLIC OdbDesign)

# link to Utils lib in all targets
#target_link_libraries(OdbDesignApp PUBLIC Utils)
#target_link_libraries(OdbDesignServer PUBLIC Utils)
#target_link_libraries(OdbDesign PUBLIC Utils)
