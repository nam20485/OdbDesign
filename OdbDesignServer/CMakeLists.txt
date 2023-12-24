# CMakeList.txt : CMake project for OdbDesignServer
#

add_executable(OdbDesignServer "main.cpp"   "Controllers/HelloWorldController.h" "Controllers/HelloWorldController.cpp"     "OdbDesignServerApp.h" "OdbDesignServerApp.cpp" "OdbDesignServer.h"      "Controllers/FileUploadController.h" "Controllers/FileUploadController.cpp" "Controllers/FileModelController.h" "Controllers/FileModelController.cpp")

## PCH
file (GLOB_RECURSE ODBDESIGN_SERVER_HEADER_FILES "*.h")
target_precompile_headers(OdbDesignServer PRIVATE ${ODBDESIGN_SERVER_HEADER_FILES})

# link to OdbDesign library
target_link_libraries(OdbDesignServer PRIVATE OdbDesign)
