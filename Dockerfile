# Debian 13 (Trixie) slim - amd64
# Version: 13.3-slim (trixie-slim)
# Reduces vulnerabilities: 2 HIGH, 1 MEDIUM, 4 LOW vs Debian 12
FROM --platform=$BUILDPLATFORM debian@sha256:346fa035ca82052ce8ec3ddb9df460b255507acdeb1dc880a8b6930e778a553c AS build

ARG OWNER=nam20485
ARG VCPKG_BINARY_SOURCES=""
# USE_VCPKG_CACHE=1 enables read-only consumption of the GitHub Packages vcpkg
# binary cache (local dev / consumer builds). USE_VCPKG_CACHE=0 keeps the legacy
# write-enabled behavior (CI uploads via the in-image NuGet source).
#
# Tokens are supplied as BuildKit secrets (see the RUN --mount=type=secret below),
# NOT as ARGs, so their values never appear in build logs or the layer cache:
#   id=github_token     GitHub PAT (write:packages)  — used by the USE_VCPKG_CACHE=0 path
#   id=nuget_auth_token GitHub PAT (read:packages)   — used by the USE_VCPKG_CACHE=1 path
ARG USE_VCPKG_CACHE=0

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

# When USE_VCPKG_CACHE=1 (local dev): generate a read-only nuget config so vcpkg
# downloads pre-built packages from GitHub Packages without uploading.
# When USE_VCPKG_CACHE=0 (CI / default): set up the in-image write-enabled NuGet
# source so vcpkg uploads built binaries to GitHub Packages.
RUN --mount=type=secret,id=github_token \
    --mount=type=secret,id=nuget_auth_token \
    if [ "${USE_VCPKG_CACHE}" = "1" ]; then \
        if [ ! -s /run/secrets/nuget_auth_token ]; then \
            echo "ERROR: USE_VCPKG_CACHE=1 requires the 'nuget_auth_token' build secret, but it is missing/empty." >&2; \
            echo "       Provide NUGET_AUTH_TOKEN (GitHub PAT with read:packages scope) in the build environment." >&2; \
            exit 1; \
        fi; \
        mkdir -p /etc/vcpkg && \
        printf '<?xml version="1.0" encoding="utf-8"?>\n<configuration>\n    <packageSources>\n        <clear />\n        <add key="GitHubPackages-OdbDesign" value="https://nuget.pkg.github.com/%s/index.json" />\n    </packageSources>\n    <packageSourceCredentials>\n        <GitHubPackages-OdbDesign>\n            <add key="Username" value="%s" />\n            <add key="ClearTextPassword" value="%s" />\n        </GitHubPackages-OdbDesign>\n    </packageSourceCredentials>\n</configuration>\n' "${OWNER}" "${OWNER}" "$(cat /run/secrets/nuget_auth_token)" \
            > /etc/vcpkg/local.nuget.config; \
    else \
        mono `./vcpkg fetch nuget | tail -n 1` \
            sources add \
            -source "https://nuget.pkg.github.com/${OWNER}/index.json" \
            -storepasswordincleartext \
            -name "GitHub" \
            -username ${OWNER} \
            -password "$(cat /run/secrets/github_token)" && \
        mono `./vcpkg fetch nuget | tail -n 1` \
            setapikey "$(cat /run/secrets/github_token)" \
            -source "https://nuget.pkg.github.com/${OWNER}/index.json"; \
    fi

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
FROM --platform=$TARGETPLATFORM debian@sha256:346fa035ca82052ce8ec3ddb9df460b255507acdeb1dc880a8b6930e778a553c AS run
# ARG ODBDESIGN_SERVER_REQUEST_USERNAME=""
# ARG ODBDESIGN_SERVER_REQUEST_PASSWORD=""
LABEL org.opencontainers.image.source=https://github.com/nam20485/OdbDesign \
    org.opencontainers.image.authors=https://github.com/nam20485 \
    org.opencontainers.image.description="A free open source cross-platform C++ library for parsing ODB++ Design archives and accessing their data. Exposed via a REST API (port 8888) and a gRPC API (port 50051) packaged inside of a Docker image. The OdbDesign Docker image runs the OdbDesignServer executable, which starts both servers in the same process and shares a single DesignCache." \
    org.opencontainers.image.licenses=AGPL-3.0-only \
    org.opencontainers.image.url=https://nam20485.github.io/OdbDesign \
    org.opencontainers.image.documentation=https://github.com/nam20485/OdbDesign?tab=readme-ov-file \
    org.opencontainers.image.title="OdbDesign Server"

EXPOSE 8888 50051

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
# gRPC service config (loaded by RunGrpcServer from exeDir/config.json)
COPY --from=build /src/OdbDesign/OdbDesignServer/config.json ./bin/config.json
COPY --from=build /src/OdbDesign/out/build/linux-release/OdbDesignTests/OdbDesignTests ./bin/
# COPY --from=build /src/OdbDesign/out/build/linux-release/OdbDesignApp/OdbDesignApp ./bin/
# COPY --from=build /src/OdbDesign/out/build/linux-release/OdbDesignApp/*.so ./bin/


# copy templates directory
RUN mkdir -p ./templates
COPY --from=build /src/OdbDesign/OdbDesignServer/templates/* ./templates

# create designs directory
# required to be volume mounted!
#RUN mkdir ./designs

# run
ENV LD_LIBRARY_PATH=/OdbDesign/bin
# ENV ODBDESIGN_SERVER_REQUEST_USERNAME=${ODBDESIGN_SERVER_REQUEST_USERNAME}
# ENV ODBDESIGN_SERVER_REQUEST_PASSWORD=${ODBDESIGN_SERVER_REQUEST_PASSWORD}
RUN chmod +x ./bin/OdbDesignServer
ENTRYPOINT [ "./bin/OdbDesignServer", "--designs-dir", "./designs", "--templates-dir", "./templates" ]
