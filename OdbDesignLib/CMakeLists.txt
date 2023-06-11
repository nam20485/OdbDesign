# CMakeList.txt : CMake project for OdbDesignLib, include source and define
# project specific logic here.
#

add_library(OdbDesign SHARED "export.h" "ComponentLayerDirectory.cpp" "ComponentLayerDirectory.h" "EdaDataFile.cpp" "EdaDataFile.h" "LayerDirectory.cpp" "LayerDirectory.h" "NetlistFile.cpp" "NetlistFile.h" "FileArchive.cpp" "FileArchive.h" "StepDirectory.cpp" "StepDirectory.h" "string_trim.h" "macros.h" "Net.h" "Net.cpp" "Component.h" "Component.cpp" "Pin.h" "Pin.cpp" "PinConnection.h" "PinConnection.cpp" "Package.h" "Package.cpp" "Part.h" "Part.cpp" "Via.h" "Via.cpp" "Design.h" "Design.cpp" "enums.h" "string_trim.cpp")
set_property(TARGET OdbDesign PROPERTY POSITION_INDEPENDENT_CODE ON)

# state that anybody linking to us needs to include the current source dir,
# while we don't.
target_include_directories(OdbDesign
                           INTERFACE
                           $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
                           $<INSTALL_INTERFACE:include>
                           )

if (CMAKE_PYTHON_BUILD)      
 #   add_custom_command(TARGET OdbDesign 
 #                      PRE_BUILD
 #                      COMMAND scripts/generate-python-module.ps1
 #                      )

    target_sources(OdbDesign PRIVATE "OdbDesignLib_wrap.cxx")

    # include and link to Python3                           
    find_package(Python3 COMPONENTS Development REQUIRED)
    target_include_directories(OdbDesign PUBLIC ${Python3_INCLUDE_DIRS})
    #include_directories(${Python3_INCLUDE_DIRS})
    target_link_libraries(OdbDesign ${Python3_LIBRARIES})
endif()