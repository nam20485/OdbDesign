$PROTOC = 'C:\Source\github\nam20485\OdbDesign\out\build\x64-debug\vcpkg_installed\x64-windows\tools\protobuf\protoc'

. $PROTOC --cpp_out=../Protobuf --error_format=msvs *.proto
