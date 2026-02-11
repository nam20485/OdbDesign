# CMakeList.txt : CMake project for OdbDesignLib
#

add_library(OdbDesign SHARED 
        "odbdesign_export.h" "FileModel/Design/ComponentsFile.cpp" "FileModel/Design/ComponentsFile.h" "FileModel/Design/EdaDataFile.cpp" "FileModel/Design/EdaDataFile.h" "FileModel/Design/LayerDirectory.cpp" "FileModel/Design/LayerDirectory.h" "FileModel/Design/NetlistFile.cpp" "FileModel/Design/NetlistFile.h" "FileModel/Design/FileArchive.cpp" "FileModel/Design/FileArchive.h" "FileModel/Design/StepDirectory.cpp" "FileModel/Design/StepDirectory.h" "ProductModel/Net.h" "ProductModel/Net.cpp" "ProductModel/Component.h" "ProductModel/Component.cpp" "ProductModel/Pin.h" "ProductModel/Pin.cpp" "ProductModel/PinConnection.h" "ProductModel/PinConnection.cpp" "ProductModel/Package.h" "ProductModel/Package.cpp" "ProductModel/Part.h" "ProductModel/Part.cpp" "ProductModel/Via.h" "ProductModel/Via.cpp" "ProductModel/Design.h" "ProductModel/Design.cpp"  "OdbDesign.h" "App/DesignCache.h" "App/DesignCache.cpp"   "../Utils/win.h" "IProtoBuffable.h" "../Utils/crow_win.h" "FileModel/OdbFile.h" "FileModel/OdbFile.cpp"      "App/IOdbApp.h" "App/IOdbApp.cpp" "App/OdbAppBase.h" "App/OdbAppBase.cpp" "App/RouteController.h" "App/RouteController.cpp" "App/IOdbServerApp.h" "App/IOdbServerApp.cpp"
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
        "FileModel/parse_error.h"    "FileModel/parse_info.h" "FileModel/parse_info.cpp" "FileModel/parse_error.cpp" "FileModel/invalid_odb_error.h" "FileModel/invalid_odb_error.cpp"  "FileModel/Design/PropertyRecord.h" "FileModel/Design/PropertyRecord.cpp" "FileModel/Design/FeaturesFile.h" "FileModel/Design/FeaturesFile.cpp" "FileModel/Design/ContourPolygon.h" "FileModel/Design/ContourPolygon.cpp" "FileModel/Design/SymbolName.h" "FileModel/Design/SymbolName.cpp" "FileModel/Design/SymbolsDirectory.h" "FileModel/Design/SymbolsDirectory.cpp"         "FileModel/Design/StepHdrFile.h" "FileModel/Design/StepHdrFile.cpp" "FileModel/Design/AttributeLookupTable.h" "FileModel/Design/AttributeLookupTable.cpp" "FileModel/Design/ComponentHeightTracer.h" "FileModel/Design/ComponentHeightTracer.cpp"
        "App/RequestAuthenticationBase.h" "App/RequestAuthenticationBase.cpp" "App/BasicRequestAuthentication.h" "App/BasicRequestAuthentication.cpp" "FileModel/IStreamSaveable.h"  "FileModel/Design/ToolsFile.h" "FileModel/Design/ToolsFile.cpp")

# disable warning C4250: inheritance by dominance
target_compile_options(OdbDesign PUBLIC
        $<$<CXX_COMPILER_ID:MSVC>:
            /wd4250>)

target_link_libraries(OdbDesign PUBLIC Utils)

# Include dirs are PUBLIC so both OdbDesign and consumers get the source/binary dirs
# (needed for protobuf-generated headers that live in the binary dir).
target_include_directories(OdbDesign
                           PUBLIC
                           $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
                           $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
                           $<INSTALL_INTERFACE:include>)

# PCH
#if (NOT DEFINED ENV{CI})
    file (GLOB_RECURSE ODBDESIGN_HEADER_FILES "*.h")
    target_precompile_headers(OdbDesign PRIVATE ${ODBDESIGN_HEADER_FILES})
#endif()

# link to zlib (required for Crow HTTP compression)
find_package(ZLIB REQUIRED)
target_link_libraries(OdbDesign PRIVATE ZLIB::ZLIB)

target_compile_definitions(OdbDesign PUBLIC CROW_ENABLE_COMPRESSION)

# link to Crow
find_package(Crow CONFIG REQUIRED)
target_link_libraries(OdbDesign PRIVATE Crow::Crow)

# populate proto files cmake variable to all .proto files in "./protoc" directory (excluding subdirectories)
set(PROTO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/protoc")
file(GLOB PROTO_FILES "${PROTO_DIR}/*.proto")
message(STATUS "Found proto files: ${PROTO_FILES}")

# Link to Protobuf
find_package(Protobuf CONFIG REQUIRED)

# Generate protobuf files with export macro
protobuf_generate(
    TARGET OdbDesign
    PROTOS ${PROTO_FILES}
    IMPORT_DIRS ${PROTO_DIR}
    EXPORT_MACRO ODBDESIGN_EXPORT
)

target_link_libraries(OdbDesign PUBLIC protobuf::libprotobuf)

# workaround to remove error: "C++ command-line error: invalid macro definition: _CROW_ICD-NOTFOUND"
# (see https://github.com/CrowCpp/Crow/issues/661#issuecomment-1702544225)
list(REMOVE_ITEM _CROW_ICD "_CROW_ICD-NOTFOUND")
set_target_properties(Crow::Crow PROPERTIES INTERFACE_COMPILE_DEFINITIONS "${_CROW_ICD}")
