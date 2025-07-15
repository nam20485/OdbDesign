$PROTOC = 'C:\Source\github\nam20485\OdbDesign\out\build\x64-debug\vcpkg_installed\x64-windows\tools\protobuf\protoc'
$DLL_EXPORT = 'ODBDESIGN_EXPORT'
$_GRPC_CPP_PLUGIN_EXECUTABLE = 'C:\Program Files (x86)\grpc\bin\grpc_cpp_plugin.exe'

. $PROTOC --cpp_out=dllexport_decl=${DLL_EXPORT}:../Protobuf --error_format=msvs *.proto `
          --grpc_out=../Protobuf --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"

# COMMAND ${_PROTOBUF_PROTOC}
# ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
#   --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
#   -I "${hw_proto_path}"
#   --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
#   "${hw_proto}"
