# CMakeList.txt : CMake project for OdbDesignServer
#

add_executable(OdbDesignServer "main.cpp"   "Controllers/HelloWorldController.h" "Controllers/HelloWorldController.cpp"     "OdbDesignServerApp.h" "OdbDesignServerApp.cpp" "Controllers/StepsEdaDataController.cpp" "Controllers/StepsEdaDataController.h" "OdbDesignServer.h"   )

# link to Crow
find_package(Crow CONFIG REQUIRED)
target_link_libraries(OdbDesignServer PRIVATE Crow::Crow)

# link to OdbDesign library
target_link_libraries(OdbDesignServer PRIVATE OdbDesign)

# workaround to remove error: "C++ command-line error: invalid macro definition: _CROW_ICD-NOTFOUND"
# (see https://github.com/CrowCpp/Crow/issues/661#issuecomment-1702544225)
list(REMOVE_ITEM _CROW_ICD "_CROW_ICD-NOTFOUND")
set_target_properties(Crow::Crow PROPERTIES INTERFACE_COMPILE_DEFINITIONS "${_CROW_ICD}")
