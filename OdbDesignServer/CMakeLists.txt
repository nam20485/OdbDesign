# CMakeList.txt : CMake project for OdbDesignServer
#

add_executable(OdbDesignServer "main.cpp"   "Controllers/HelloWorldController.h" "Controllers/HelloWorldController.cpp"     "OdbDesignServerApp.h" "OdbDesignServerApp.cpp" "Controllers/StepsEdaDataController.cpp" "Controllers/StepsEdaDataController.h" "OdbDesignServer.h"      "Controllers/FileUploadController.h" "Controllers/FileUploadController.cpp")

## PCH
file (GLOB_RECURSE ODBDESIGN_SERVER_HEADER_FILES "*.h")
target_precompile_headers(OdbDesignServer PRIVATE ${ODBDESIGN_SERVER_HEADER_FILES})

# link to zlib (required for Crow HTTP compression)
find_package(ZLIB REQUIRED)
target_link_libraries(OdbDesign PRIVATE ZLIB::ZLIB)

# link to Crow
find_package(Crow CONFIG REQUIRED)
target_link_libraries(OdbDesignServer PRIVATE Crow::Crow)

# link to Oatpp
find_package(oatpp          1.3.0 REQUIRED)
find_package(oatpp-swagger  1.3.0 REQUIRED)
find_package(oatpp-zlib		1.3.0 REQUIRED)

target_link_libraries(OdbDesignServer PRIVATE oatpp::oatpp)
target_link_libraries(OdbDesignServer PRIVATE oatpp::oatpp-swagger)
target_link_libraries(OdbDesignServer PRIVATE oatpp::oatpp-zlib)

# link to OdbDesign library
target_link_libraries(OdbDesignServer PRIVATE OdbDesign)

# workaround to remove error: "C++ command-line error: invalid macro definition: _CROW_ICD-NOTFOUND"
# (see https://github.com/CrowCpp/Crow/issues/661#issuecomment-1702544225)
list(REMOVE_ITEM _CROW_ICD "_CROW_ICD-NOTFOUND")
set_target_properties(Crow::Crow PROPERTIES INTERFACE_COMPILE_DEFINITIONS "${_CROW_ICD}")
