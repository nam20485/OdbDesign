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
# -P preserves vcpkg's dev symlinks (libfoo.so -> libfoo.so.N -> real file) so
# each lib ships once; zip -y below stores them as links
cp -P ./out/build/linux-dynamic-release/vcpkg_installed/x64-linux-dynamic/lib/*.so* ./artifacts
# OpenSSL 3 provider modules (resolved relative to libcrypto.so's dir)
cp -r ./out/build/linux-dynamic-release/vcpkg_installed/x64-linux-dynamic/lib/ossl-modules ./artifacts/
#cp ./out/build/linux-dynamic-release/OdbDesignServer/*.so ./artifacts
cd ./artifacts
zip -r -y ./artifacts.zip ./*.so* ./ossl-modules ./OdbDesignServer
