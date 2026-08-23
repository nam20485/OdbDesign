# Debian 13 (Trixie) slim - amd64
# Version: 13.3-slim (trixie-slim)
# NOTE: the previous digest (b6e2a152...) actually resolved to Debian 12
# (bookworm, glibc 2.36), which is too old for the CI-built binaries
# (they require GLIBC_2.38 / GLIBCXX_3.4.32) -> CrashLoopBackOff.
# Digest below is the real debian:13.3-slim manifest list (trixie, glibc 2.41).
FROM --platform=$BUILDPLATFORM debian@sha256:34cd9e9fd437c0a095ec39cb2e73422c9f30821b0d0848ed74fd0d43bae4d958 AS build

ARG OWNER=nam20485
ARG VCPKG_BINARY_SOURCES=""
# USE_VCPKG_CACHE=1 enables read-only consumption of the GitHub Packages vcpkg
# binary cache (local dev / consumer builds). USE_VCPKG_CACHE=0 keeps the legacy
# write-enabled behavior (CI uploads via the in-image NuGet source).
#
# Tokens are supplied as BuildKit secrets (see the RUN --mount=type=secret below),
# NOT as ARGs, so they are not passed as build args, not printed in build logs,
# and kept out of the final image. (They do exist in the *build* stage's layer
# contents — e.g. the generated NuGet config — so avoid exporting build-stage
# layers to a shared cache; the published `run` stage contains no token.)
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
        if [ ! -s /run/secrets/github_token ]; then \
            echo "ERROR: USE_VCPKG_CACHE=0 requires the 'github_token' build secret, but it is missing/empty." >&2; \
            echo "       Provide GITHUB_TOKEN (GitHub PAT with write:packages scope) in the build environment." >&2; \
            exit 1; \
        fi; \
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
# match the linux-dynamic-release preset's triplet so the pre-install is what
# the configure step actually consumes (manifest mode would install it anyway)
RUN mkdir -p /src/OdbDesign
WORKDIR /src/OdbDesign
COPY ./vcpkg.json .
COPY ./vcpkg-configuration.json .
RUN ${VCPKG_ROOT}/vcpkg install --triplet x64-linux-dynamic
# RUN --mount=type=cache,target=/root/.cache \
#     ${VCPKG_ROOT}/vcpkg install

# copy source
COPY . .

# configure & build using presets
# linux-dynamic-release (shared protobuf/gRPC runtime — the static linux-release
# preset loads two protobuf copies and crashes; see docs/linux-dynamic-release-plan.md)
RUN cmake --preset linux-dynamic-release
RUN cmake --build --preset linux-dynamic-release
# # linux-debug
# RUN cmake --preset linux-debug
# RUN cmake --build --preset linux-debug

# much smaller runtime image
# Debian 13 (Trixie) slim - amd64
# Version: 13.3-slim (trixie-slim)
# NOTE: keep in sync with the build stage digest (see note above).
# FROM --platform=$TARGETPLATFORM debian@sha256:1d3c811171a08a5adaa4a163fbafd96b61b87aa871bbc7aa15431ac275d3d430 AS run
FROM debian@sha256:34cd9e9fd437c0a095ec39cb2e73422c9f30821b0d0848ed74fd0d43bae4d958 AS run
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
COPY --from=build /src/OdbDesign/out/build/linux-dynamic-release/OdbDesignLib/*.so ./bin/
COPY --from=build /src/OdbDesign/out/build/linux-dynamic-release/Utils/*.so ./bin/
COPY --from=build /src/OdbDesign/out/build/linux-dynamic-release/OdbDesignServer/OdbDesignServer ./bin/
COPY --from=build /src/OdbDesign/out/build/linux-dynamic-release/OdbDesignServer/*.so ./bin/
# vcpkg shared libraries (protobuf, gRPC, libarchive, ...) required by the
# linux-dynamic-release build; discovered via LD_LIBRARY_PATH=/OdbDesign/bin
COPY --from=build /src/OdbDesign/out/build/linux-dynamic-release/vcpkg_installed/x64-linux-dynamic/lib/*.so* ./bin/
# OpenSSL 3 provider modules (resolved relative to libcrypto.so's directory)
COPY --from=build /src/OdbDesign/out/build/linux-dynamic-release/vcpkg_installed/x64-linux-dynamic/lib/ossl-modules ./bin/ossl-modules
# gRPC service config (loaded by RunGrpcServer from exeDir/config.json)
COPY --from=build /src/OdbDesign/OdbDesignServer/config.json ./bin/config.json
COPY --from=build /src/OdbDesign/out/build/linux-dynamic-release/OdbDesignTests/OdbDesignTests ./bin/
# COPY --from=build /src/OdbDesign/out/build/linux-dynamic-release/OdbDesignApp/OdbDesignApp ./bin/
# COPY --from=build /src/OdbDesign/out/build/linux-dynamic-release/OdbDesignApp/*.so ./bin/

# copy templates directory
COPY --from=build /src/OdbDesign/OdbDesignServer/templates/* ./templates

# set ownership to non-root user
RUN chmod +x ./bin/OdbDesignServer && \
    chown --recursive odbdesign:odbdesign /OdbDesign

USER odbdesign

# Docker health check using existing HTTP endpoint. Runs inside the container
# as the image's configured user (odbdesign, the final USER above) — HEALTHCHECK
# placement does not affect that; kept after USER purely for readability of the
# non-root runtime stage.
HEALTHCHECK --interval=30s --timeout=3s --start-period=10s --retries=3 \
  CMD curl -f http://localhost:8888/healthz/live || exit 1

# create designs directory
# required to be volume mounted!
#RUN mkdir ./designs

# run
ENV LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/OdbDesign/bin
# ENV ODBDESIGN_SERVER_REQUEST_USERNAME=${ODBDESIGN_SERVER_REQUEST_USERNAME}
# ENV ODBDESIGN_SERVER_REQUEST_PASSWORD=${ODBDESIGN_SERVER_REQUEST_PASSWORD}
ENTRYPOINT [ "./bin/OdbDesignServer", "--designs-dir", "./designs", "--templates-dir", "./templates" ]
