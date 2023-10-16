# CMakeList.txt : CMake project for OdbDesignLib
#

add_library(OdbDesign SHARED "odbdesign_export.h" "ComponentLayerDirectory.cpp" "ComponentLayerDirectory.h" "EdaDataFile.cpp" "EdaDataFile.h" "LayerDirectory.cpp" "LayerDirectory.h" "NetlistFile.cpp" "NetlistFile.h" "FileArchive.cpp" "FileArchive.h" "StepDirectory.cpp" "StepDirectory.h" "Net.h" "Net.cpp" "Component.h" "Component.cpp" "Pin.h" "Pin.cpp" "PinConnection.h" "PinConnection.cpp" "Package.h" "Package.cpp" "Part.h" "Part.cpp" "Via.h" "Via.cpp" "Design.h" "Design.cpp" "BoardSide.h" "OdbDesign.h" "DesignCache.h" "DesignCache.cpp"   "win.h"   "proto/edadatafile.pb.h" "proto/edadatafile.pb.cc" "IProtoBuffable.h" "crow_win.h" "OdbFile.h" "OdbFile.cpp"  "OdbFileRecord.h" "OdbFileRecord.cpp"   "IOdbApp.h" "IOdbApp.cpp" "OdbAppBase.h" "OdbAppBase.cpp" "RouteController.h" "RouteController.cpp" "IOdbServerApp.h" "OdbServerAppBase.h" "OdbServerAppBase.cpp" "IOdbServerApp.cpp"
        MatrixFile.cpp
        MatrixFile.h
        MiscInfoFile.cpp
        MiscInfoFile.h
        "Constants.h"
        AttrListFile.cpp
        AttrListFile.h
        SysAttrFile.cpp
        SysAttrFile.h "RgbColor.h" "RgbColor.cpp" "StandardFontsFile.h" "StandardFontsFile.cpp")

target_compile_options(OdbDesign PRIVATE
    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
        -Wall>
    $<$<CXX_COMPILER_ID:MSVC>:
        /wd4250>)

## required for SWIG
#set_property(TARGET OdbDesign PROPERTY POSITION_INDEPENDENT_CODE ON)

target_link_libraries(OdbDesign PUBLIC Utils)


# state that anybody linking to us needs to include the current source dir, while we don't.
target_include_directories(OdbDesign
                           INTERFACE
                           $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
                           $<INSTALL_INTERFACE:include>)

# link to zlib (required for Crow HTTP compression)
find_package(ZLIB REQUIRED)
target_link_libraries(OdbDesign PRIVATE ZLIB::ZLIB)
add_compile_definitions(CROW_ENABLE_COMPRESSION)

# Link to Protobuf
find_package(Protobuf CONFIG REQUIRED)
#protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS protoc/edadatafile.proto)
#target_link_libraries(OdbDesign PUBLIC protobuf::libprotoc protobuf::libprotobuf protobuf::libprotobuf-lite)
target_link_libraries(OdbDesign PRIVATE protobuf::libprotobuf)
# mark the generated Protofbuf C++ files as generated
#set_source_files_properties(${PROTO_SRC} ${PROTO_HDRS} PROPERTIES GENERATED TRUE)
# add the generated Protobuf C++ files to the target
#target_sources(OdbDesign PRIVATE ${PROTO_SRCS} ${PROTO_HDRS})

# Python extension module build settings                           
if (PYTHON_MODULE_BUILD)
 #   # run swig to generate wrapper file
 #   add_custom_command(TARGET OdbDesign 
 #                      PRE_BUILD
 #                      COMMAND scripts/generate-python-module.ps1)

     if (MSVC)
        # make a copy of output library with name expected by Python module extensions
        add_custom_command(TARGET OdbDesign
                           POST_BUILD
                           COMMAND ${CMAKE_COMMAND}
                           ARGS -E copy $<TARGET_FILE:OdbDesign> $<TARGET_FILE_DIR:OdbDesign>/_PyOdbDesignLib.pyd
                           COMMENT Copying to Python module extension
                           )
    elseif(LINUX)
        # make a copy of output library with name expected by Python module extensions
        add_custom_command(TARGET OdbDesign
                           POST_BUILD
                           COMMAND ${CMAKE_COMMAND}
                           ARGS -E copy $<TARGET_FILE:OdbDesign> $<TARGET_FILE_DIR:OdbDesign>/_PyOdbDesignLib.so
                           COMMENT Copying to Python module extension
                           )
    endif()

    target_sources(OdbDesign PRIVATE "OdbDesignLib_wrap.cxx")

    # include and link to Python3                           
    find_package(Python3 COMPONENTS Development REQUIRED)
    target_include_directories(OdbDesign PRIVATE ${Python3_INCLUDE_DIRS})    
    target_link_libraries(OdbDesign ${Python3_LIBRARIES})
endif()