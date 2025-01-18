# OdbDesign

A free open source cross-platform C++ library for parsing ODB++ Design archives, accessing their data, and building net list product models. Exposed via a REST API packaged inside of a Docker image.

## Skip to Build and Running Instructions

Sounds great! Now how do I build and run it?

[Run](#running)

[Build](#building-from-source)

Documentation for the currently-released version of the source code is available [here](https://nam20485.github.io/OdbDesign/api).

## Need Help?

Feel free to message me in the project's Gitter chat room: [odbdesign:gitter.im](https://app.gitter.im/#/room/#odbdesign:gitter.im)

You can also create an issue: <https://github.com/nam20485/OdbDesign/issues>

Or you can start a discussion: <https://github.com/nam20485/OdbDesign/discussions>

## Key Features

OdbDesign ODB++ parser is differentiated from other offerings by these key features:

1. Performance
1. Cross-Platform Flexibility
1. Expertise
1. Security

### Performance

OdbDesign is implemented in C++ and is designed and optimized to be fast. Unlike higher-level languages that use interpreters or virtual machines, that means it is compiled into native code for your machine, whether your platform is Windows, Linux, or Mac.

>The parser is also multi-threaded to take advantage of modern multi-core CPUs.

### Cross-Platform Flexibility

The library itself can run directly on all major platforms (i.e. Windows, Linux, and Mac) and also exposes a REST API so that parsed data can be accessed from any application or programming language that can connect to a REST API. The REST API and library are also exposed via a Docker image that can be run on any platform that supports Docker.

>Both options support running the parser on a different machine than the one running the application that needs the parsed data. This allows the parser to be run on a high-performance server or workstation while the application that needs the data runs on a low-power device like a Raspberry Pi, mobile device, or the web.

### Expertise

The maintainer has well over a decade of experience in the PCB Manufacturing and hardware industry. Specifically he has worked on the development of ODB++ parsing and viewer applications for some of the established key players in the industry. Time spent working under the designer of the ODB++ specification has given him a unique perspective on the ODB++ format and how it is used in the industry. This experience is also leveraged to make the parser as fast and efficient as possible.

### Security

All code, dependency packages, and Docker images are scanned for security vulnerabilities, using extended security scanning rule profiles (these are more secure than the default base scanning rule profiles).

>The project has earned a high score on the [OpenSSF Security Scorecard](https://securityscorecards.dev/#what-is-openssf-scorecard). Details can be seen in the [Project Security](#project-security) section below.

It is built using the latest available version of the C++ standard and is compiled with the latest available compiler versions. The parser is also built using the latest available versions of all of its dependencies and is regularly updated to use the latest versions of those dependencies as they are released.

>These checks are run against all branches starting with development, so there is no chance of a security vulnerability being introduced into the main and release branches. Docker's Scout Suite is used to scan the Docker image for security vulnerabilities, and GitHub's CodeQL is used to scan the code for security vulnerabilities.

## Overview

### Current Implementation State

The diagram describes the current state of parser implementation and data availability. Green color describes areas of the ODB++ archive file that are implemented and have their data available for use. ~~Red color describes areas that are not parsed so their data is not yet available.~~ **All areas of the file are now parsed, and have their data available for use.**

![ODB++ file hierarchy implementation state diagram](<odb++ file hierarchy (implemented).png>)

### Project Security

#### OpenSSF Security Scorecard

[OpenSSF Security Scorecard](https://securityscorecards.dev/#what-is-openssf-scorecard) assesses open source projects for security risks through a series of automated checks.

[![OpenSSF Scorecard](https://api.securityscorecards.dev/projects/github.com/nam20485/OdbDesign/badge)](https://securityscorecards.dev/viewer/?uri=github.com/nam20485/OdbDesign)

>The majority of tested projects have a much lower score, so OdbDesign's 7.8 score represents a remarkably high level of security.

### CI/CD Build

#### Branches

#### `development`

| Step                        | Status  |
|-----------------------------|---------|
| Build              | [![CMake Build Multi-Platform](https://github.com/nam20485/OdbDesign/actions/workflows/cmake-multi-platform.yml/badge.svg?branch=development)](https://github.com/nam20485/OdbDesign/actions/workflows/cmake-multi-platform.yml) |
| Docker Image                | [![Docker Publish](https://github.com/nam20485/OdbDesign/actions/workflows/docker-publish.yml/badge.svg?branch=development)](https://github.com/nam20485/OdbDesign/actions/workflows/docker-publish.yml) |
| Security Code Scan          | [![CodeQL Security Scan](https://github.com/nam20485/OdbDesign/actions/workflows/codeql.yml/badge.svg?branch=development)](https://github.com/nam20485/OdbDesign/actions/workflows/codeql.yml) |
| Docker Security Scan        | [![Docker Scout Scan](https://github.com/nam20485/OdbDesign/actions/workflows/docker-scout-scan.yml/badge.svg?branch=development)](https://github.com/nam20485/OdbDesign/actions/workflows/docker-scout-scan.yml) |
| Dependency Review Scan      | [![Dependency Review](https://github.com/nam20485/OdbDesign/actions/workflows/dependency-review.yml/badge.svg?branch=development)](https://github.com/nam20485/OdbDesign/actions/workflows/dependency-review.yml) |
| Upload SBOM                 | [![SBOM Generate and Submit](https://github.com/nam20485/OdbDesign/actions/workflows/sbom-generate-submit.yml/badge.svg?branch=development)](https://github.com/nam20485/OdbDesign/actions/workflows/sbom-generate-submit.yml) |

#### `main`

| Step                        | Status   |
|-----------------------------|----------|
| Build                       | [![CMake Build Multi-Platform](https://github.com/nam20485/OdbDesign/actions/workflows/cmake-multi-platform.yml/badge.svg?branch=main)](https://github.com/nam20485/OdbDesign/actions/workflows/cmake-multi-platform.yml) |
| Docker Image                | [![Docker Publish](https://github.com/nam20485/OdbDesign/actions/workflows/docker-publish.yml/badge.svg?branch=main)](https://github.com/nam20485/OdbDesign/actions/workflows/docker-publish.yml) |
| Security Code Scan          | [![CodeQL Security Scan](https://github.com/nam20485/OdbDesign/actions/workflows/codeql.yml/badge.svg?branch=development)](https://github.com/nam20485/OdbDesign/actions/workflows/codeql.yml) |
| Docker Security Scan        | [![Docker Scout Scan](https://github.com/nam20485/OdbDesign/actions/workflows/docker-scout-scan.yml/badge.svg?branch=main)](https://github.com/nam20485/OdbDesign/actions/workflows/docker-scout-scan.yml) |
| Dependency Review Scan      | [![Dependency Review](https://github.com/nam20485/OdbDesign/actions/workflows/dependency-review.yml/badge.svg?branch=main)](https://github.com/nam20485/OdbDesign/actions/workflows/dependency-review.yml) |
| Upload SBOM                 | [![SBOM Generate and Submit](https://github.com/nam20485/OdbDesign/actions/workflows/sbom-generate-submit.yml/badge.svg?branch=main)](https://github.com/nam20485/OdbDesign/actions/workflows/sbom-generate-submit.yml) |

#### `release`

| Step                        | Status   |
|-----------------------------|----------|
| Build                       | [![CMake Build Multi-Platform](https://github.com/nam20485/OdbDesign/actions/workflows/cmake-multi-platform.yml/badge.svg?branch=release)](https://github.com/nam20485/OdbDesign/actions/workflows/cmake-multi-platform.yml) |
| Docker Image                | [![Docker Publish](https://github.com/nam20485/OdbDesign/actions/workflows/docker-publish.yml/badge.svg?branch=release)](https://github.com/nam20485/OdbDesign/actions/workflows/docker-publish.yml) |
| Security Code Scan          | [![CodeQL Security Scan](https://github.com/nam20485/OdbDesign/actions/workflows/codeql.yml/badge.svg?branch=development)](https://github.com/nam20485/OdbDesign/actions/workflows/codeql.yml) |
| Docker Security Scan        | [![Docker Scout Scan](https://github.com/nam20485/OdbDesign/actions/workflows/docker-scout-scan.yml/badge.svg?branch=release)](https://github.com/nam20485/OdbDesign/actions/workflows/docker-scout-scan.yml) |
| Dependency Review Scan      | [![Dependency Review](https://github.com/nam20485/OdbDesign/actions/workflows/dependency-review.yml/badge.svg?branch=release)](https://github.com/nam20485/OdbDesign/actions/workflows/dependency-review.yml) |
| Upload SBOM                 | [![SBOM Generate and Submit](https://github.com/nam20485/OdbDesign/actions/workflows/sbom-generate-submit.yml/badge.svg?branch=release)](https://github.com/nam20485/OdbDesign/actions/workflows/sbom-generate-submit.yml) |

### Architecture

The OdbDesign parser is built as a C++ shared library on all three platforms. An executable running the server links to the library and provides the REST API for accessing the data the library parses. The REST API server can be run by invoking the executable directly or by running the Docker image. The server executable and library can be run on Windows, Linux, or MacOS and the Docker image can be run on any platform that supports Docker.

Documentation for the currently-released version of the source code is available [here](https://nam20485.github.io/OdbDesign/api).

[Insert Diagram Image]

#### Library

FileModel vs. ProductModel...

## Running

There are three different ways to use the project. You can:

1. Build the C++ shared library and use it in your own application
1. Build the C++ library and REST API server application and then run the application
1. Run the REST API server (which contains the library inside) from the Docker image.

The instructions for building the C++ shared library and application are below in the [Building from Source](#building-from-source) section. The instructions for running the application or REST API server are here.

### Running the C++ Application

The C++ application REST API server can be run by executing the `OdbDesignServer` or `OdbDesignServer.exe` executable (depending on your platform).

The executable can be found in the following directory:

* `~/src/OdbDesign/out/build/x64-release`                         *(Linux/MacOS)*
* `C:\Users\<YourName>\Source\OdbDesign\out\build\x64-release`    *(Windows)*

See the [Building from Source](#building-from-source) section for more details on building the C++ application and the build output directory.

If successful, the REST API server will be running and listening on port 8888. You can access the REST API at `http://localhost:8888`.

### Running the REST API Server

From the root of the source directory run:

`$ docker compose up`

If successful, the REST API server will be running and listening on port 8888. You can access the REST API at `http://localhost:8888`. The Swagger UI is available at `http://localhost:8080/swagger`.

## Building from Source

Documentation for the currently-released version of the source code is available [here](https://nam20485.github.io/OdbDesign/api).

### Build Dependencies

> If you are building on Windows and have a modern version of Visual Studio installed then all of the dependencies listed below are already installed on your system (except for maybe Docker). You can skip to the next section.

> If you are building on a Linux system then the dependencies listed below can be installed using your package manager. For example on Ubuntu you can install them (except for vcpkg and Docker) using the following command:

`$ sudo apt install git cmake ninja-build build-essential`

* git ([install instructions for your platform](https://git-scm.com/book/en/v2/Getting-Started-Installing-Git))
* vcpkg ([install instructions for your platform](https://vcpkg.io/en/getting-started.html))
* CMake ([install instructions for your platform](https://cliutils.gitlab.io/modern-cmake/chapters/intro/installing.html))
* Ninja ([install instructions for your platform](https://ninja-build.org/))
* Docker ([install instructions for your platform](https://docs.docker.com/get-docker/)) (*optional*)

git, CMake, and ninja can all be installed via the `sudo apt install` command listed above. vcpkg can be installed by following the instructions below. Docker is optional but can be installed via the following command:

`$ sudo apt install docker.io docker-compose-v2`

#### vcpkg

```Bash
$ git clone https://github.com/microsoft/vcpkg
$ .\vcpkg\bootstrap-vcpkg.bat
```

Then add the following line to your shell profile (e.g. ~/.bashrc, ~/.zshrc, etc.):

`export VCPKG_ROOT=/path/to/vcpkg`

Make sure to restart your shell or source the profile after adding the line.

`source ~/.bashrc`

### Source Code

Get the source code by cloning the GitHub repository:

```Bash
$ git clone https://github.com/nam20485/OdbDesign.git
```

### Build

Open your favorite terminal and change to the directory where you cloned the source code. This should be something similar to one of the below depending on your platform and where you cloned the source code:

* `~/src/OdbDesign`                         *(Linux/MacOS)*
* `C:\Users\<YourName>\Source\OdbDesign`    *(Windows)*

Then run the commands from one or more of the sections below, based on what you would like to build...

#### CMake C++ Project

##### Visual Studio

* Open the directory in Visual Studio (this will open the directory as a CMake project)
* Select a Configuration preset:
  * `x64 Release`
  * `x64 Debug`
* Select a target:
  * `OdbDesign.dll` *(parser shared library)*
  * `OdbDesignServer.exe` *(REST API server executable)*
* Build *(i.e. Build->Rebuild All or CTRL+SHIFT+B)*

##### Command-Line CMake

###### Windows

```Bash
$ cmake --preset x64-release
$ cmake --build --preset x64-release
```

###### Linux

```Bash
$ cmake --preset linux-release
$ cmake --build --preset linux-release

```

This builds the C++ shared library and the REST API server executable. See the [Running the C++ Application](#running-the-c%2b%2b-application) section for more details.

The build output can be found in the following directory:

* `~/src/OdbDesign/out/build/x64-release`                         *(Linux/MacOS)*
* `C:\Users\<YourName>\Source\OdbDesign\out\build\x64-release`    *(Windows)*

>The `x64-release` directory will be different if you selected a different configuration preset (`x64-release` vs. `x64-debug` or `linux-release` vs. `linux-debug`). The `x64-release` directory will contain the shared library and the server executable. Make sure to copy the dependencies (.dll files on Windows, .so files on Linux, .dylib files on MacOS) to the same directory as the executable if you want to copy it somewhere else.

#### Docker Image for REST API Server

From the root of the source directory...

```Bash
$ docker compose up
```

This builds the Docker image and runs it. See the [Running the REST API Server](#running-the-rest-api-server) section for more details.

## Integration into Other Applications

There are three separate interfaces that allow use of the library in other applications.

### C++ Shared Library

Build the C++ shared library and link it into your own C++ application. See the [Building from Source](#building-from-source) section for more details on building it.

### REST API

Run the REST API server and access the data via the REST API. See the [Running the REST API Server](#running-the-rest-api-server) section for more details on running it.

### Google Protobuf Protocol Buffers

Data objects returned from the parser library support serialization to and from [Google Protobuf protocol buffers](https://protobuf.dev/). This allows the data to be easily shared with other applications and programming languages that support protocol buffers. Google Protobuf is a highly optimized binary encoding so it is fast and small.

>The protocol buffer definitions are included in the library so they can be used to generate code for other languages - currently C++ bindings are built and used in the library.

## Contributing

The project is happy to accept pull requests if you would like to contribute. Please open an issue to discuss your proposed changes before submitting a pull request. Pull requests are accepted against the `development` branch. Please make sure your pull request is up to date with the latest changes in the `development` branch before submitting it.

All contributed code must be covered by accompanying test cases. Please make sure your changes are covered by test cases before submitting a pull request.

See [docs/CONTRIBUTING.md](CONTRIBUTING.md) for more details.

## License

This project is free and open source under the MIT [license](https://github.com/nam20485/OdbDesign/blob/c0c8b6e4b93e1c7d4d5e65c7ad25157c883f8bfb/LICENSE).

## Credit

Thanks to the following open source projects that are used to create OdbDesign:

* [Crow](https://crowcpp.org/master/)
* [Google Protobuf](https://protobuf.dev/)
* [CMake](https://cmake.org/)
* [vcpkg](https://vcpkg.io/en/)
* [libarchive](https://www.libarchive.org/)
* [OpenSSL](https://www.openssl.org/)
* [zlib](https://www.zlib.net/)
* [Python 3](https://www.python.org/)

OdbDesign would not be possible without the hard work of their developers, staff, and supporters.

## Contact

If you are interested in using the parser in your application or code, or have any questions about it please do not hesitate to contact me.

>If you need support for integration of the parser into your own product and/or need its feature set extended, I am currently available for consulting.

* [Email me (maintainer)](mailto:nmiller217@gmail.com?subject=OdbDesign)
* [GitHub](https://github.com/nam20485/odbdesign)
* [LinkedIn](https://www.linkedin.com/in/namiller/)
* [OdbDesign Website](https://nam20485.github.io/OdbDesign/)
* Gitter chat room: [odbdesign:gitter.im](https://app.gitter.im/#/room/#odbdesign:gitter.im)

## ODB++

* [ODB++ Format Home](https://odbplusplus.com/design/)
* [ODB++ Format Documentation & Resources](https://odbplusplus.com/design/our-resources/)
* [ODB++ Format Specification v8.1 update 3](https://odbplusplus.com//wp-content/uploads/sites/2/2021/02/odb_spec_user.pdf)

*ODB++ is a registered trademark of Siemens and © Siemens 2021*
