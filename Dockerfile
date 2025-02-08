FROM --platform=$BUILDPLATFORM debian:bookworm-20240722-slim@sha256:5f7d5664eae4a192c2d2d6cb67fc3f3c7891a8722cd2903cc35aa649a12b0c8d AS build

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
        python3 \
        && \
    apt-get clean && \
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
FROM --platform=$BUILDPLATFORM debian:bookworm-20240722-slim@sha256:5f7d5664eae4a192c2d2d6cb67fc3f3c7891a8722cd2903cc35aa649a12b0c8d AS run
# ARG ODBDESIGN_SERVER_REQUEST_USERNAME=""
# ARG ODBDESIGN_SERVER_REQUEST_PASSWORD=""
LABEL org.opencontainers.image.source=https://github.com/nam20485/OdbDesign \
      org.opencontainers.image.authors=https://github.com/nam20485 \
      org.opencontainers.image.description="A free open source cross-platform C++ library for parsing ODB++ Design archives and accessing their data. Exposed via a REST API packaged inside of a Docker image. The OdbDesign Docker image runs the OdbDesignServer REST API server executable, listening on port 8888." \
      org.opencontainers.image.licenses=MIT \    
      org.opencontainers.image.url=https://nam20485.github.io/OdbDesign \ 
      org.opencontainers.image.documentation=https://github.com/nam20485/OdbDesign?tab=readme-ov-file \
      org.opencontainers.image.title="OdbDesign Server"

EXPOSE 8888

# install dependencies (7z command)
RUN apt-get update && \
    apt-get install -y -q --no-install-recommends \
        curl \
        apt-transport-https \
        ca-certificates \        
        p7zip-full \  
        && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

# test 7z install
RUN 7z -h

RUN mkdir --parents /OdbDesign/bin
WORKDIR /OdbDesign

# copy binaries
COPY --from=build /src/OdbDesign/out/build/linux-release/OdbDesignLib/*.so ./bin/
COPY --from=build /src/OdbDesign/out/build/linux-release/Utils/*.so ./bin/
COPY --from=build /src/OdbDesign/out/build/linux-release/OdbDesignServer/OdbDesignServer ./bin/
COPY --from=build /src/OdbDesign/out/build/linux-release/OdbDesignServer/*.so ./bin/
COPY --from=build /src/OdbDesign/out/build/linux-release/OdbDesignTests/OdbDesignTests ./bin/

# copy templates directory
RUN mkdir -p ./templates
COPY --from=build /src/OdbDesign/OdbDesignServer/templates/* ./templates

# create designs directory
# required to be volume mounted!
#RUN mkdir ./designs

# run
ENV LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/OdbDesign/bin
# ENV ODBDESIGN_SERVER_REQUEST_USERNAME=${ODBDESIGN_SERVER_REQUEST_USERNAME}
# ENV ODBDESIGN_SERVER_REQUEST_PASSWORD=${ODBDESIGN_SERVER_REQUEST_PASSWORD}
RUN chmod +x ./bin/OdbDesignServer
ENTRYPOINT [ "./bin/OdbDesignServer", "--designs-dir", "./designs", "--templates-dir", "./templates" ]
