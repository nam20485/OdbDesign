# CMakeList.txt : CMake project for OdbDesignApp, include source and define
# project specific logic here.
#

# Add source to this project's executable.
add_executable (OdbDesignApp "OdbDesignApp.cpp" "OdbDesignApp.h"  )

## PCH
#if (NOT DEFINED ENV{CI})
	file (GLOB_RECURSE ODBDESIGN_APP_HEADER_FILES "*.h")
	target_precompile_headers(OdbDesignApp PRIVATE ${ODBDESIGN_APP_HEADER_FILES})
#endif()

# link to OdbDesign library
target_link_libraries(OdbDesignApp PRIVATE OdbDesign)

# TODO: Add tests and install targets if needed.
