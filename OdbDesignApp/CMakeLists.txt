# CMakeList.txt : CMake project for OdbDesignApp, include source and define
# project specific logic here.
#

# Add source to this project's executable.
add_executable (OdbDesignApp "OdbDesignApp.cpp" "OdbDesignApp.h")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET OdbDesignApp PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
