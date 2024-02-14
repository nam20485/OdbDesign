FROM debian:bookworm-20240211@sha256:4482958b4461ff7d9fabc24b3a9ab1e9a2c85ece07b2db1840c7cbc01d053e90 AS build

ARG OWNER=nam20485
ARG GITHUB_TOKEN="PASSWORD"
ARG VCPKG_BINARY_SOURCES=""

# install dependencies
RUN apt-get update && \
    apt-get install -y -q --no-install-recommends \
        curl \
        apt-transport-https \
        ca-certificates \
        cmake \            
        g++ \
        ninja-build \
        build-essential \
        git \        
        zip \
        unzip \
        tar  \
        pkg-config \
        mono-complete \
        linux-libc-dev \   
        && \
    rm -rf /var/lib/apt/lists/*

# install vcpkg
ENV VCPKG_ROOT=/root/src/github/microsoft/vcpkg
RUN git clone https://github.com/Microsoft/vcpkg.git ${VCPKG_ROOT}
WORKDIR ${VCPKG_ROOT}
RUN ./bootstrap-vcpkg.sh

# set vcpkg to use NuGet for binary caching
ENV VCPKG_BINARY_SOURCES=${VCPKG_BINARY_SOURCES}

# setup NuGet to use GitHub Packages as a source so vcpkg binary cache can use it
RUN mono `./vcpkg fetch nuget | tail -n 1` \
        sources add \
        -source "https://nuget.pkg.github.com/${OWNER}/index.json" \
        -storepasswordincleartext \
        -name "GitHub" \
        -username ${OWNER} \
        -password "${GITHUB_TOKEN}"

RUN mono `./vcpkg fetch nuget | tail -n 1` \
        setapikey "${GITHUB_TOKEN}" \
        -source "https://nuget.pkg.github.com/${OWNER}/index.json"

# pre-install vcpgk packages BEFORE cmake configure
RUN mkdir -p /src/OdbDesign
WORKDIR /src/OdbDesign
COPY ./vcpkg.json .
RUN ${VCPKG_ROOT}/vcpkg install
# RUN --mount=type=cache,target=/root/.cache \
#     ${VCPKG_ROOT}/vcpkg install

# copy source
COPY . .

# configure & build using presets
# linux-release
RUN cmake --preset linux-release
RUN cmake --build --preset linux-release
# # linux-debug
# RUN cmake --preset linux-debug
# RUN cmake --build --preset linux-debug

# much smaller runtime image
FROM debian:bookworm-20240211-slim@sha256:d02c76d82364cedca16ba3ed6f9102406fa9fa8833076a609cabf14270f43dfc AS run
LABEL org.opencontainers.image.source=https://github.com/nam20485/OdbDesign
LABEL org.opencontainers.image.authors=https://github.com/nam20485
LABEL org.opencontainers.image.description="The OdbDesign Docker image runs the OdbDesignServer REST API server executable, listening on port 8888."
LABEL org.opencontainers.image.licenses=MIT
EXPOSE 8888

RUN mkdir --parents /OdbDesign/bin
WORKDIR /OdbDesign/bin

# copy binaries
COPY --from=build /src/OdbDesign/out/build/linux-release/OdbDesignLib/*.so .
COPY --from=build /src/OdbDesign/out/build/linux-release/Utils/*.so .
COPY --from=build /src/OdbDesign/out/build/linux-release/OdbDesignServer/OdbDesignServer .
COPY --from=build /src/OdbDesign/out/build/linux-release/OdbDesignServer/*.so .
COPY --from=build /src/OdbDesign/out/build/linux-release/OdbDesignTests/OdbDesignTests .

# copy templates directory
RUN mkdir ./templates
COPY --from=build /src/OdbDesign/OdbDesignServer/templates/* ./templates

# create designs directory
RUN mkdir ./designs

# run
ENV LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/OdbDesign/bin
RUN chmod +x ./OdbDesignServer
ENTRYPOINT [ "./OdbDesignServer" ]
