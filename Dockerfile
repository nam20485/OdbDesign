# Debian 13 (Trixie) slim - amd64
# Version: 13.3-slim (trixie-slim)
# Reduces vulnerabilities: 2 HIGH, 1 MEDIUM, 4 LOW vs Debian 12
FROM --platform=$BUILDPLATFORM debian@sha256:b6e2a152f22a40ff69d92cb397223c906017e1391a73c952b588e51af8883bf8 AS build

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
COPY ./vcpkg-configuration.json .
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
# Debian 13 (Trixie) slim - amd64
# Version: 13.3-slim (trixie-slim)
# Reduces vulnerabilities: 2 HIGH, 1 MEDIUM, 4 LOW vs Debian 12
# FROM --platform=$TARGETPLATFORM debian@sha256:b6e2a152f22a40ff69d92cb397223c906017e1391a73c952b588e51af8883bf8 AS run
FROM debian@sha256:b6e2a152f22a40ff69d92cb397223c906017e1391a73c952b588e51af8883bf8 AS run
# ARG ODBDESIGN_SERVER_REQUEST_USERNAME=""
# ARG ODBDESIGN_SERVER_REQUEST_PASSWORD=""
LABEL org.opencontainers.image.source=https://github.com/nam20485/OdbDesign \
    org.opencontainers.image.authors=https://github.com/nam20485 \
    org.opencontainers.image.description="A free open source cross-platform C++ library for parsing ODB++ Design archives and accessing their data. Exposed via a REST API packaged inside of a Docker image. The OdbDesign Docker image runs the OdbDesignServer REST API server executable, listening on port 8888." \
    org.opencontainers.image.licenses=AGPL-3.0-only \
    org.opencontainers.image.url=https://nam20485.github.io/OdbDesign \
    org.opencontainers.image.documentation=https://github.com/nam20485/OdbDesign?tab=readme-ov-file \
    org.opencontainers.image.title="OdbDesign Server"

EXPOSE 8888 50051

# Docker health check using existing HTTP endpoint
HEALTHCHECK --interval=30s --timeout=3s --start-period=10s --retries=3 \
  CMD curl -f http://localhost:8888/healthz/live || exit 1

# install dependencies (curl for healthcheck, 7z for archive extraction)
RUN apt-get update && \
    apt-get install -y -q --no-install-recommends \
    curl \
    apt-transport-https \
    ca-certificates \
    p7zip-full \
    && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

# # --- gRPC health check (easy to disable: comment out the two blocks below) ---
# # Download grpc_health_probe binary
# ARG GRPC_HEALTH_PROBE_VERSION=v0.4.24
# RUN curl -sL -o /bin/grpc_health_probe \
#       https://github.com/grpc-ecosystem/grpc-health-probe/releases/download/${GRPC_HEALTH_PROBE_VERSION}/grpc_health_probe-linux-amd64 && \
#     chmod +x /bin/grpc_health_probe

# # gRPC-specific healthcheck (comment out to disable, re-enable HTTP-only HEALTHCHECK above)
# HEALTHCHECK --interval=30s --timeout=5s --start-period=10s --retries=3 \
#   CMD /bin/grpc_health_probe -addr=localhost:50051 || exit 1

# test 7z install
RUN 7z -h

# create non-root user
RUN groupadd --gid 10001 odbdesign && \
    useradd --uid 10001 --gid odbdesign --create-home --shell /usr/sbin/nologin odbdesign

RUN mkdir --parents /OdbDesign/bin /OdbDesign/templates /OdbDesign/designs
WORKDIR /OdbDesign

# copy binaries
COPY --from=build /src/OdbDesign/out/build/linux-release/OdbDesignLib/*.so ./bin/
COPY --from=build /src/OdbDesign/out/build/linux-release/Utils/*.so ./bin/
COPY --from=build /src/OdbDesign/out/build/linux-release/OdbDesignServer/OdbDesignServer ./bin/
COPY --from=build /src/OdbDesign/out/build/linux-release/OdbDesignServer/*.so ./bin/
COPY --from=build /src/OdbDesign/out/build/linux-release/OdbDesignTests/OdbDesignTests ./bin/
# COPY --from=build /src/OdbDesign/out/build/linux-release/OdbDesignApp/OdbDesignApp ./bin/
# COPY --from=build /src/OdbDesign/out/build/linux-release/OdbDesignApp/*.so ./bin/

# copy templates directory
COPY --from=build /src/OdbDesign/OdbDesignServer/templates/* ./templates

# set ownership to non-root user
RUN chmod +x ./bin/OdbDesignServer && \
    chown --recursive odbdesign:odbdesign /OdbDesign

USER odbdesign

# create designs directory
# required to be volume mounted!
#RUN mkdir ./designs

# run
ENV LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/OdbDesign/bin
# ENV ODBDESIGN_SERVER_REQUEST_USERNAME=${ODBDESIGN_SERVER_REQUEST_USERNAME}
# ENV ODBDESIGN_SERVER_REQUEST_PASSWORD=${ODBDESIGN_SERVER_REQUEST_PASSWORD}
ENTRYPOINT [ "./bin/OdbDesignServer", "--designs-dir", "./designs", "--templates-dir", "./templates" ]
