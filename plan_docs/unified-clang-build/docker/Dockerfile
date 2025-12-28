FROM ubuntu:24.04 AS build-env
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y \
    clang lld llvm cmake ninja-build git curl zip unzip tar pkg-config python3 \
    gcc-mingw-w64-x86-64 g++-mingw-w64-x86-64 libssl-dev lzma-dev libxml2-dev patch xz-utils \
    && rm -rf /var/lib/apt/lists/*
ENV VCPKG_ROOT=/opt/vcpkg
RUN git clone https://github.com/microsoft/vcpkg.git $VCPKG_ROOT && $VCPKG_ROOT/bootstrap-vcpkg.sh
WORKDIR /src