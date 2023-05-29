FROM gcc:13.1.0

RUN apt-get update && \
    apt-get install -y --no-install-recommends build-essential \
                                               cmake \
                                               ninja-build

RUN mkdir -p /src/OdbDesign
WORKDIR /src/OdbDesign
COPY . .

RUN cmake -B ./build -DCMAKE_BUILD_TYPE=Release
RUN cmake --build ./build --config Release

ENTRYPOINT ["./build/OdbDesignApp/OdbDesignApp"]