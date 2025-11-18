# CMakeList.txt : CMake project for OdbDesignServer
#

add_executable(OdbDesignServer 
    "main.cpp"   
    "Controllers/HelloWorldController.h" 
    "Controllers/HelloWorldController.cpp"     
    "OdbDesignServerApp.h" 
    "OdbDesignServerApp.cpp" 
    "OdbDesignServer.h"      
    "Controllers/FileUploadController.h" 
    "Controllers/FileUploadController.cpp" 
    "Controllers/FileModelController.h" 
    "Controllers/FileModelController.cpp" 
    "Controllers/HealthCheckController.h" 
    "Controllers/HealthCheckController.cpp" 
    "Controllers/DesignsController.h" 
    "Controllers/DesignsController.cpp" 
    "Services/OdbDesignServiceImpl.h"
    "Services/OdbDesignServiceImpl.cpp"
)

## PCH
#if (NOT DEFINED ENV{CI})
	file (GLOB_RECURSE ODBDESIGN_SERVER_HEADER_FILES "*.h")
	target_precompile_headers(OdbDesignServer PRIVATE ${ODBDESIGN_SERVER_HEADER_FILES})
#endif()

# link to OdbDesign library (which includes gRPC)
target_link_libraries(OdbDesignServer PUBLIC OdbDesign)

# gRPC - link directly to ensure all symbols are available
find_package(gRPC CONFIG REQUIRED)
target_link_libraries(OdbDesignServer PRIVATE gRPC::grpc++ gRPC::grpc++_reflection)

