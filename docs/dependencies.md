---
title: Dependencies &amp; SBOM
---

# Dependencies &amp; SBOM

This page collects the third-party software the project depends on, and where its dependency, SBOM, and security reports live.

## Runtime dependencies

All third-party libraries are resolved and version-pinned by [vcpkg](https://vcpkg.io/) in
[`vcpkg.json`](https://github.com/nam20485/OdbDesign/blob/development/vcpkg.json) (manifest
mode, exact `version#port-version` overrides):

| Package | Pinned version | Upstream |
|---|---|---|
| [Crow](https://crowcpp.org/master/) | 1.3.3 | C++ micro web framework (REST API server) |
| [Protocol Buffers](https://protobuf.dev/) | 6.33.4#2 | data serialization (design wire format) |
| [gRPC](https://grpc.io/) | 1.81.1 | gRPC API server (with `codegen` feature) |
| [libarchive](https://www.libarchive.org/) | 3.8.7 | ODB++ `.tgz` archive extraction |
| [zlib](https://www.zlib.net/) | 1.3.2#2 | compression (also a protobuf feature) |

Transitive dependencies (e.g. OpenSSL, c-ares, abseil, re2, brought in by gRPC/Crow) are
resolved by vcpkg against the pinned registry baseline
(`4f6d4ae8` in [`vcpkg-configuration.json`](https://github.com/nam20485/OdbDesign/blob/development/vcpkg-configuration.json)).

## Build &amp; test tooling

* [CMake](https://cmake.org/) 3.21+ &mdash; build system, with presets per platform
* [vcpkg](https://vcpkg.io/) &mdash; dependency management (manifest mode, binary caching via GitHub Packages)
* [Ninja](https://ninja-build.org/) &mdash; build backend
* [GoogleTest](https://github.com/google/googletest) 1.17.0 &mdash; test framework

## Dependency &amp; SBOM reports

* [GitHub dependency graph](https://github.com/nam20485/OdbDesign/network/dependencies) &mdash; continuously updated from the vcpkg manifest by the `dependencygraph` integration; an SPDX SBOM can be exported from the same page ("Export SBOM").
* [SBOM Generate and Submit workflow](https://github.com/nam20485/OdbDesign/actions/workflows/sbom-generate-submit.yml) &mdash; generates an SPDX SBOM on every push and submits it to the GitHub dependency submission API.
* [Dependency Review workflow](https://github.com/nam20485/OdbDesign/actions/workflows/dependency-review.yml) &mdash; gates pull requests against vulnerable or unlicensed dependency changes.

## Security scanning

All code, dependency packages, and Docker images are scanned for security vulnerabilities:

* [OpenSSF Security Scorecard](https://securityscorecards.dev/viewer/?uri=github.com/nam20485/OdbDesign) &mdash; [![OpenSSF Scorecard](https://api.securityscorecards.dev/projects/github.com/nam20485/OdbDesign/badge)](https://securityscorecards.io/viewer/?uri=github.com/nam20485/OdbDesign)
* [Docker Scout](https://github.com/nam20485/OdbDesign/actions/workflows/docker-scout-scan.yml) &mdash; scans the published server image
* CodeQL and Codacy static analysis run on every pull request

## License

The project is licensed under [AGPL V3.0](https://github.com/nam20485/OdbDesign/blob/development/LICENSE). Licenses suitable for closed-source use are available &mdash; see [Contact](contact.md).
