FROM gcc:13.1.0 AS build

# install dependencies
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        build-essential \
        cmake

# copy source
RUN mkdir -p /src/OdbDesign
WORKDIR /src/OdbDesign
COPY . .

# build
RUN cmake -B ./build -DCMAKE_BUILD_TYPE=Release
RUN cmake --build ./build --config Release

# run
WORKDIR /src/OdbDesign/build/OdbDesignApp
ENTRYPOINT [ "./OdbDesignApp" ]
