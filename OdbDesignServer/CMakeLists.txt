# CMakeList.txt : CMake project for OdbDesignServer
#

add_executable(OdbDesignServer "main.cpp"   "Controllers/HelloWorldController.h" "Controllers/HelloWorldController.cpp"     "OdbDesignServerApp.h" "OdbDesignServerApp.cpp" "Controllers/StepsEdaDataController.cpp" "Controllers/StepsEdaDataController.h" "OdbDesignServer.h"   )

target_compile_options(OdbDesignServer PRIVATE
    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
        -Wall>
    $<$<CXX_COMPILER_ID:MSVC>:
        /wd4250>)

# link to zlib (required for Crow HTTP compression)
find_package(ZLIB REQUIRED)
target_link_libraries(OdbDesignServer PRIVATE ZLIB::ZLIB)

# link to Crow
add_compile_definitions(CROW_ENABLE_COMPRESSION)
find_package(Crow CONFIG REQUIRED)
target_link_libraries(OdbDesignServer PRIVATE Crow::Crow)

# link to OdbDesign library
target_link_libraries(OdbDesignServer PUBLIC OdbDesign)

if (NOT CI)
    # TODO: add POST_BUILD commands for Linux (dest. directory will be ${CMAKE_CURRENT_BINARY_DIR})
    # copy templates directory to output directory 
    add_custom_command(
            TARGET OdbDesignServer POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                    ${CMAKE_SOURCE_DIR}/OdbDesignServer/templates
                    #${CMAKE_CURRENT_BINARY_DIR}/templates)
                    ${PROJECT_BINARY_DIR}/templates)

    # copy designs directory to output directory 
    add_custom_command(
            TARGET OdbDesignServer POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                    ${CMAKE_SOURCE_DIR}/OdbDesignServer/designs
                    #${CMAKE_CURRENT_BINARY_DIR}/designs)
                    ${PROJECT_BINARY_DIR}/designs)
endif()
