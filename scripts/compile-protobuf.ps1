$PROTOC = 'C:\Source\github\nam20485\OdbDesign\out\build\x64-debug\vcpkg_installed\x64-windows\tools\protobuf\protoc'
$DLL_EXPORT = 'ODBDESIGN_EXPORT'

. $PROTOC --cpp_out=dllexport_decl=${DLL_EXPORT}:../Protobuf --error_format=msvs *.proto
