# CMakeList.txt : CMake project for OdbDesignServer
#

add_executable(OdbDesignServer
    "main.cpp"
    "Controllers/HelloWorldController.h"
    "Controllers/HelloWorldController.cpp"
    "OdbDesignServerApp.h"
    "OdbDesignServerApp.cpp"

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
    "Config/GrpcServiceConfig.h"
    "Config/GrpcServiceConfig.cpp"
    "OdbServerAppBase.h"
    "OdbServerAppBase.cpp"
)

## PCH
#if (NOT DEFINED ENV{CI})
#	file (GLOB_RECURSE ODBDESIGN_SERVER_HEADER_FILES "*.h")
#	target_precompile_headers(OdbDesignServer PRIVATE ${ODBDESIGN_SERVER_HEADER_FILES})
#endif()

# link to OdbDesign library
target_link_libraries(OdbDesignServer PUBLIC OdbDesign)


# ===== gRPC Configuration (moved from OdbDesignLib) =====

# populate grpc proto files cmake variable to all .proto files in "${CMAKE_CURRENT_SOURCE_DIR}/protoc/grpc" directory
set(PROTO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../OdbDesignLib/protoc")
set(GRPC_PROTO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/protoc/grpc")
file(GLOB GRPC_PROTO_FILES "${GRPC_PROTO_DIR}/*.proto")
message(STATUS "Found grpc proto files for OdbDesignServer: ${GRPC_PROTO_FILES}")

set(GRPC_CODEGEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/grpc")

# Precompute generated source/header file paths for later use
set(GRPC_PROTO_SRCS "")
set(GRPC_PROTO_HDRS "")
foreach(proto_file ${GRPC_PROTO_FILES})
    get_filename_component(proto_name ${proto_file} NAME_WE)
    list(APPEND GRPC_PROTO_SRCS
        ${GRPC_CODEGEN_DIR}/${proto_name}.pb.cc
        ${GRPC_CODEGEN_DIR}/${proto_name}.grpc.pb.cc)
    list(APPEND GRPC_PROTO_HDRS
        ${GRPC_CODEGEN_DIR}/${proto_name}.pb.h
        ${GRPC_CODEGEN_DIR}/${proto_name}.grpc.pb.h)
endforeach()

# Find packages
find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)

# Generate protobuf messages for gRPC proto files
protobuf_generate(
    TARGET OdbDesignServer
    PROTOS ${GRPC_PROTO_FILES}
    IMPORT_DIRS ${GRPC_PROTO_DIR} ${PROTO_DIR}
    PROTOC_OUT_DIR ${GRPC_CODEGEN_DIR}
    APPEND_PATH
    #EXPORT_MACRO ODBDESIGN_EXPORT
)

# Generate gRPC service stubs
protobuf_generate(
    TARGET OdbDesignServer
    LANGUAGE grpc
    GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc
    PLUGIN "protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>"
    PROTOS ${GRPC_PROTO_FILES}
    IMPORT_DIRS ${GRPC_PROTO_DIR} ${PROTO_DIR}
    PROTOC_OUT_DIR ${GRPC_CODEGEN_DIR}
    APPEND_PATH
)

# Link gRPC libraries
target_link_libraries(OdbDesignServer PUBLIC gRPC::gpr gRPC::grpc gRPC::grpc++ gRPC::grpc++_reflection)

# Include generated gRPC files directory
target_include_directories(OdbDesignServer PRIVATE ${CMAKE_CURRENT_BINARY_DIR} ${GRPC_CODEGEN_DIR})

# mark the generated Protobuf C++ files as generated
set_source_files_properties(${GRPC_PROTO_SRCS} ${GRPC_PROTO_HDRS} PROPERTIES GENERATED TRUE)

# Copy config.json to executable directory so it can be found at runtime
# Use CMAKE_RUNTIME_OUTPUT_DIRECTORY if set (for MSVC), otherwise use executable's output directory
if(CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set(CONFIG_DEST_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
else()
    # Get the executable's output directory
    get_target_property(CONFIG_DEST_DIR OdbDesignServer RUNTIME_OUTPUT_DIRECTORY)
    if(NOT CONFIG_DEST_DIR)
        # Fallback to binary directory if RUNTIME_OUTPUT_DIRECTORY not set
        set(CONFIG_DEST_DIR "${CMAKE_CURRENT_BINARY_DIR}")
    endif()
endif()

file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/config.json"
     DESTINATION "${CONFIG_DEST_DIR}"
     FILE_PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)

if (WIN32)
    # Ensure the generated sources compile with the import macro defined
    set_source_files_properties(
        ${GRPC_PROTO_SRCS}
        PROPERTIES COMPILE_DEFINITIONS "ODBDESIGN_EXPORT=__declspec(dllimport)"
    )
endif()

# add the generated Protobuf C++ files to the target
target_sources(OdbDesignServer PRIVATE ${GRPC_PROTO_SRCS} ${GRPC_PROTO_HDRS})

