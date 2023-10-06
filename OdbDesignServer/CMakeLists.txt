add_executable(OdbDesignServer "OdbDesignServer.cpp" "OdbDesignServer.h")

# copy templates directory to output directory 
add_custom_command(
        TARGET OdbDesignServer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_SOURCE_DIR}/OdbDesignServer/templates
                ${CMAKE_CURRENT_BINARY_DIR}/templates)
