# CMakeList.txt : CMake project for OdbDesignLib
#

add_library(OdbDesign SHARED 
        "odbdesign_export.h" "FileModel/Design/ComponentsFile.cpp" "FileModel/Design/ComponentsFile.h" "FileModel/Design/EdaDataFile.cpp" "FileModel/Design/EdaDataFile.h" "FileModel/Design/LayerDirectory.cpp" "FileModel/Design/LayerDirectory.h" "FileModel/Design/NetlistFile.cpp" "FileModel/Design/NetlistFile.h" "FileModel/Design/FileArchive.cpp" "FileModel/Design/FileArchive.h" "FileModel/Design/StepDirectory.cpp" "FileModel/Design/StepDirectory.h" "ProductModel/Net.h" "ProductModel/Net.cpp" "ProductModel/Component.h" "ProductModel/Component.cpp" "ProductModel/Pin.h" "ProductModel/Pin.cpp" "ProductModel/PinConnection.h" "ProductModel/PinConnection.cpp" "ProductModel/Package.h" "ProductModel/Package.cpp" "ProductModel/Part.h" "ProductModel/Part.cpp" "ProductModel/Via.h" "ProductModel/Via.cpp" "ProductModel/Design.h" "ProductModel/Design.cpp"  "OdbDesign.h" "App/DesignCache.h" "App/DesignCache.cpp"   "../Utils/win.h" "IProtoBuffable.h" "../Utils/crow_win.h" "FileModel/OdbFile.h" "FileModel/OdbFile.cpp"      "App/IOdbApp.h" "App/IOdbApp.cpp" "App/OdbAppBase.h" "App/OdbAppBase.cpp" "App/RouteController.h" "App/RouteController.cpp" "App/IOdbServerApp.h" "App/OdbServerAppBase.h" "App/OdbServerAppBase.cpp" "App/IOdbServerApp.cpp"
        FileModel/Design/MatrixFile.cpp
        FileModel/Design/MatrixFile.h
        FileModel/Design/MiscInfoFile.cpp
        FileModel/Design/MiscInfoFile.h
        "Constants.h"
        FileModel/Design/AttrListFile.cpp
        FileModel/Design/AttrListFile.h        
         "FileModel/Design/RgbColor.h" "FileModel/Design/RgbColor.cpp"
         "FileModel/Design/StandardFontsFile.h" "FileModel/Design/StandardFontsFile.cpp"
         "enums.h"
         "App/OdbDesignArgs.h" "App/OdbDesignArgs.cpp"
        "ProtoBuf/edadatafile.pb.h" "ProtoBuf/edadatafile.pb.cc" 
        "ProtoBuf/filearchive.pb.h" "ProtoBuf/filearchive.pb.cc" 
        "ProtoBuf/netlistfile.pb.h" "ProtoBuf/netlistfile.pb.cc" 
        "ProtoBuf/layerdirectory.pb.h" "ProtoBuf/layerdirectory.pb.cc" 
        "ProtoBuf/design.pb.h" "ProtoBuf/design.pb.cc"
        "ProtoBuf/matrixfile.pb.h" "ProtoBuf/matrixfile.pb.cc"
        "ProtoBuf/color.pb.h" "ProtoBuf/color.pb.cc"
        "ProtoBuf/miscinfofile.pb.h" "ProtoBuf/miscinfofile.pb.cc"
        "ProtoBuf/standardfontsfile.pb.h" "ProtoBuf/standardfontsfile.pb.cc"
        "ProtoBuf/stepdirectory.pb.h" "ProtoBuf/stepdirectory.pb.cc"
        "ProtoBuf/enums.pb.h" "ProtoBuf/enums.pb.cc"
        "ProtoBuf/featuresfile.pb.h" "ProtoBuf/featuresfile.pb.cc"
        "ProtoBuf/attrlistfile.pb.h" "ProtoBuf/attrlistfile.pb.cc"
        "ProtoBuf/symbolsdirectory.pb.h" "ProtoBuf/symbolsdirectory.pb.cc"
        "ProtoBuf/symbolname.pb.h" "ProtoBuf/symbolname.pb.cc"
        "ProtoBuf/stephdrfile.pb.h" "ProtoBuf/stephdrfile.pb.cc"
        "ProtoBuf/pin.pb.h" "ProtoBuf/pin.pb.cc"
        "ProtoBuf/pinconnection.pb.h" "ProtoBuf/pinconnection.pb.cc"
        "ProtoBuf/part.pb.h" "ProtoBuf/part.pb.cc"
        "ProtoBuf/component.pb.h" "ProtoBuf/component.pb.cc"
        "ProtoBuf/net.pb.h" "ProtoBuf/net.pb.cc"
        "ProtoBuf/via.pb.h" "ProtoBuf/via.pb.cc"
        "ProtoBuf/package.pb.h" "ProtoBuf/package.pb.cc"
        "FileModel/parse_error.h"    "FileModel/parse_info.h" "FileModel/parse_info.cpp" "FileModel/parse_error.cpp" "FileModel/invalid_odb_error.h" "FileModel/invalid_odb_error.cpp" "ProtoBuf/common.pb.h" "ProtoBuf/common.pb.cc" "ProtoBuf/componentsfile.pb.h" "ProtoBuf/componentsfile.pb.cc" "FileModel/Design/PropertyRecord.h" "FileModel/Design/PropertyRecord.cpp" "FileModel/Design/FeaturesFile.h" "FileModel/Design/FeaturesFile.cpp" "FileModel/Design/ContourPolygon.h" "FileModel/Design/ContourPolygon.cpp" "FileModel/Design/SymbolName.h" "FileModel/Design/SymbolName.cpp" "FileModel/Design/SymbolsDirectory.h" "FileModel/Design/SymbolsDirectory.cpp" "FileModel/Design/StepHdrFile.h" "FileModel/Design/StepHdrFile.cpp" "FileModel/Design/AttributeLookupTable.h" "FileModel/Design/AttributeLookupTable.cpp"
 "App/RequestAuthenticationBase.h" "App/RequestAuthenticationBase.cpp" "App/BasicRequestAuthentication.h" "App/BasicRequestAuthentication.cpp" "FileModel/IStreamSaveable.h" )

# disable warning C4250: inheritance by dominance
target_compile_options(OdbDesign PUBLIC
#    $<$<OR:
#        $<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
#            >
        $<$<CXX_COMPILER_ID:MSVC>:
            /wd4250>)

target_link_libraries(OdbDesign PUBLIC Utils)

# state that anybody linking to us needs to include the current source dir, while we don't.
target_include_directories(OdbDesign
                           INTERFACE
                           $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
                           $<INSTALL_INTERFACE:include>)

# PCH
#if (NOT DEFINED ENV{CI})
    file (GLOB_RECURSE ODBDESIGN_HEADER_FILES "*.h")
    target_precompile_headers(OdbDesign PRIVATE ${ODBDESIGN_HEADER_FILES})
#endif()

# link to zlib (required for Crow HTTP compression)
find_package(ZLIB REQUIRED)
target_link_libraries(OdbDesign PRIVATE ZLIB::ZLIB)

find_package(OpenSSL REQUIRED)
target_link_libraries(OdbDesign PUBLIC OpenSSL::SSL OpenSSL::Crypto)

target_compile_definitions(OdbDesign PUBLIC CROW_ENABLE_COMPRESSION)
target_compile_definitions(OdbDesign PUBLIC CROW_ENABLE_SSL)

# link to Crow
find_package(Crow CONFIG REQUIRED)
target_link_libraries(OdbDesign PRIVATE Crow::Crow)

# Link to Protobuf
find_package(Protobuf CONFIG REQUIRED)
#protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS protoc/edadatafile.proto)
#target_link_libraries(OdbDesign PUBLIC protobuf::libprotoc protobuf::libprotobuf protobuf::libprotobuf-lite)
target_link_libraries(OdbDesign PUBLIC protobuf::libprotobuf)
# mark the generated Protofbuf C++ files as generated
#set_source_files_properties(${PROTO_SRC} ${PROTO_HDRS} PROPERTIES GENERATED TRUE)
# add the generated Protobuf C++ files to the target
#target_sources(OdbDesign PRIVATE ${PROTO_SRCS} ${PROTO_HDRS})

# # gRPC
# find_package(gRPC CONFIG REQUIRED)
# target_link_libraries(OdbDesign PUBLIC gRPC::grpc++)

# workaround to remove error: "C++ command-line error: invalid macro definition: _CROW_ICD-NOTFOUND"
# (see https://github.com/CrowCpp/Crow/issues/661#issuecomment-1702544225)
list(REMOVE_ITEM _CROW_ICD "_CROW_ICD-NOTFOUND")
set_target_properties(Crow::Crow PROPERTIES INTERFACE_COMPILE_DEFINITIONS "${_CROW_ICD}")

