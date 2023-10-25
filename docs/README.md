# OdbDesign

A free open source cross-platform C++ library for parsing ODB++ Design archives, accessing their data, and building net list product models. Exposed via a REST API and packaged inside of a Docker image.

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

It is built using the latest available version of the C++ standard and is compiled with the latest available compiler versions. The parser is also built using the latest available versions of all of its dependencies and is regularly updated to use the latest versions of those dependencies as they are released.

>These checks are run against all branches starting with development, so there is no chance of a security vulnerability being introduced into the main and release branches. Docker's Scout Suite is used to scan the Docker image for security vulnerabilities, and GitHub's CodeQL is used to scan the code for security vulnerabilities.

## Overview

### Current Implementation State

The diagram describes the current state of parser implementation and data availability. Green color describes areas of the ODB++ archive file that are implemented and have their data available for use. Red color describes areas that are not parsed so their data is not yet available.

![image](https://github.com/nam20485/OdbDesign/assets/1829396/d55d0ea8-6ad9-446a-8659-9157636e1c9f)

### CI/CD Build

#### Branches

##### `development`

| Step               | Status  |
|--------------------|---------|
| Build              | [![CMake Build Multi-Platform](https://github.com/nam20485/OdbDesign/actions/workflows/cmake-multi-platform.yml/badge.svg?branch=development)](https://github.com/nam20485/OdbDesign/actions/workflows/cmake-multi-platform.yml) |
| Security Code Scan | [![CodeQL Security Scan](https://github.com/nam20485/OdbDesign/actions/workflows/codeql.yml/badge.svg?branch=development)](https://github.com/nam20485/OdbDesign/actions/workflows/codeql.yml) |
| Docker Image       | [![Docker Publish](https://github.com/nam20485/OdbDesign/actions/workflows/docker-publish.yml/badge.svg?branch=development)](https://github.com/nam20485/OdbDesign/actions/workflows/docker-publish.yml) |
| Docker Security Scan        | [![Docker Scout Scan](https://github.com/nam20485/OdbDesign/actions/workflows/docker-scout-scan.yml/badge.svg?branch=development)](https://github.com/nam20485/OdbDesign/actions/workflows/docker-scout-scan.yml) |

##### `main`

| Step               | Status   |
|--------------------|----------|
| Build              | [![CMake Build Multi-Platform](https://github.com/nam20485/OdbDesign/actions/workflows/cmake-multi-platform.yml/badge.svg?branch=main)](https://github.com/nam20485/OdbDesign/actions/workflows/cmake-multi-platform.yml) |
| Security Code Scan | [![CodeQL Security Scan](https://github.com/nam20485/OdbDesign/actions/workflows/codeql.yml/badge.svg?branch=development)](https://github.com/nam20485/OdbDesign/actions/workflows/codeql.yml) |
| Docker Image       | [![Docker Publish](https://github.com/nam20485/OdbDesign/actions/workflows/docker-publish.yml/badge.svg?branch=main)](https://github.com/nam20485/OdbDesign/actions/workflows/docker-publish.yml) |
| Docker Security Scan        | [![Docker Scout Scan](https://github.com/nam20485/OdbDesign/actions/workflows/docker-scout-scan.yml/badge.svg?branch=main)](https://github.com/nam20485/OdbDesign/actions/workflows/docker-scout-scan.yml) |

##### `release`

| Step               | Status   |
|--------------------|----------|
| Build              | [![CMake Build Multi-Platform](https://github.com/nam20485/OdbDesign/actions/workflows/cmake-multi-platform.yml/badge.svg?branch=release)](https://github.com/nam20485/OdbDesign/actions/workflows/cmake-multi-platform.yml) |
| Security Code Scan | [![CodeQL Security Scan](https://github.com/nam20485/OdbDesign/actions/workflows/codeql.yml/badge.svg?branch=development)](https://github.com/nam20485/OdbDesign/actions/workflows/codeql.yml) |
| Docker Image       | [![Docker Publish](https://github.com/nam20485/OdbDesign/actions/workflows/docker-publish.yml/badge.svg?branch=release)](https://github.com/nam20485/OdbDesign/actions/workflows/docker-publish.yml) |
| Docker Security Scan        | [![Docker Scout Scan](https://github.com/nam20485/OdbDesign/actions/workflows/docker-scout-scan.yml/badge.svg?branch=release)](https://github.com/nam20485/OdbDesign/actions/workflows/docker-scout-scan.yml) |

### Architecture

The OdbDesign parser is built as a C++ shared library on all three platforms. An executable running the server links to the library and provides the REST API for accessing the data the library parses. The REST API server can be run by invoking the executable directly or by running the Docker image. The server executable and library can be run on Windows, Linux, or MacOS and the Docker image can be run on any platform that supports Docker.

[Insert Diagram Image]

#### Library

FileModel vs. ProductModel...

## Building from Source

### Build Dependencies

> If you are building on Windows and have a modern version of Visual Studio installed then all of the dependencies listed below are already installed on your system (except for maybe Docker). You can skip to the next section.

> If you are building on a Linux system then the dependencies listed below can be installed using your package manager. For example on Ubuntu you can install them (except for vcpkg and Docker) using the following command:

`$ sudo apt install git cmake ninja-build build-essential`

* git ([install instructions for your platform](https://git-scm.com/book/en/v2/Getting-Started-Installing-Git))
* vcpkg ([install instructions for your platform](https://vcpkg.io/en/getting-started.html))
* CMake ([install instructions for your platform](https://cliutils.gitlab.io/modern-cmake/chapters/intro/installing.html))
* Ninja ([install instructions for your platform](https://ninja-build.org/))
* Docker ([install instructions for your platform](https://docs.docker.com/get-docker/)) (*optional*)

### Source Code

Get the source code by cloning the GitHub repository:

```Bash
$ git clone git@github.com:nam20485/OdbDesign.git
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
$ cmake --preset x64-debug
$ cmake --build --preset x64-release
```

###### Linux

```Bash
$ cmake --preset linux-debug
$ cmake --build --preset linux-release

```

#### Docker Image for REST API Server

```Bash
$ docker build -f Dockerfile_OdbDesignServer . -t odbdesign-server
```

## Running the REST API Server

## Integration into Other Applications

There are four interfaces that allow use of the library in other applications.

### C++ Shared Library

### REST API

### Python Object Interface

SWIG is used to create a Python interface to the C++ library. The interface is exposed as a Python package that can be installed using pip. Parsing is supported, but currently not all data is available to access via the Python interface.

>Using the Python interface for anything beyond basic parsing would require implementation of Python wrapper classes to hold the data you are interested in. A better route for Python integration is probably to use the REST API or to generate Python bindings automatically using the included Google Protobuf definitions.

### Google Protobuf Protocol Buffers

Data objects returned from the parser library support serialization to and from [Google Protobuf protocol buffers](https://protobuf.dev/). This allows the data to be easily shared with other applications and programming languages that support protocol buffers. Google Protobuf is a highly optimized binary encoding so it is fast and small.

>The protocol buffer definitions are included in the library so they can be used to generate code for other languages - currently C++ bindings are built and used in the library.

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

## ODB++

* [ODB++ Format Home](https://odbplusplus.com/design/)
* [ODB++ Format Documentation & Resources](https://odbplusplus.com/design/our-resources/)
* [ODB++ Format Specification v8.1 update 3](https://odbplusplus.com//wp-content/uploads/sites/2/2021/02/odb_spec_user.pdf)

*ODB++ is a registered trademark of Siemens and © Siemens 2021*
