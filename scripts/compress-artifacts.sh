#!/bin/sh

# COPY --from=build /src/OdbDesign/out/build/linux-dynamic-release/OdbDesignLib/*.so .
# COPY --from=build /src/OdbDesign/out/build/linux-dynamic-release/Utils/*.so .
# COPY --from=build /src/OdbDesign/out/build/linux-dynamic-release/OdbDesignServer/OdbDesignServer .

mkdir ./artifacts
cp ./out/build/linux-dynamic-release/OdbDesignLib/*.so ./artifacts
cp ./out/build/linux-dynamic-release/Utils/*.so ./artifacts
cp ./out/build/linux-dynamic-release/OdbDesignServer/OdbDesignServer ./artifacts
# vcpkg shared libraries (protobuf, gRPC, ...) required at runtime by the
# linux-dynamic-release build; Dockerfile.prebuilt copies *.so*
cp ./out/build/linux-dynamic-release/vcpkg_installed/x64-linux-dynamic/lib/*.so* ./artifacts
#cp ./out/build/linux-dynamic-release/OdbDesignServer/*.so ./artifacts
cd ./artifacts
zip -r -y ./artifacts.zip ./*.so* ./OdbDesignServer
