FROM gcc:13.1.0 AS build

# install dependencies
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        build-essential \
        cmake \
        ninja-build \
        mingw-w64

# copy source
RUN mkdir -p /src/OdbDesign
WORKDIR /src/OdbDesign
COPY . .

# build
#RUN cmake -B ./build -DCMAKE_BUILD_TYPE=Release
#RUN cmake --build ./build --config Release

# configure & build using presets...

# linux-release
RUN cmake --preset linux-release
RUN cmake --build --preset linux-release

# linux-mingw-w64-release
RUN cmake --preset linux-mingw-w64-release
RUN cmake --build --preset linux-mingw-w64-release

# linux-debug
RUN cmake --preset linux-debug
RUN cmake --build --preset linux-debug

# linux-mingw-w64-debug
RUN cmake --preset linux-mingw-w64-debug
RUN cmake --build --preset linux-mingw-w64-debug

# run
WORKDIR /src/OdbDesign/out/build/linux-release/OdbDesignApp
ENTRYPOINT [ "./OdbDesignApp" ]
