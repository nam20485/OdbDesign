# OdbDesign

A free open source cross-platform C++ library for parsing ODB++ Design archives, accessing their data, and building net list product models. Exposed via a REST API and packaged inside of a Docker image.

## Key Features

OdbDesign ODB++ parser is differentiated from other offerings by three key features:

1. Performance
1. Cross Platform Flexibility
1. Expertise

### Performance

OdbDesign is implemented in C++ and is designed and optimized to be fast. Unlike higher-level languages that use interpreters or virtual machines, that means it is compiled into native code for your machine, whether your platform is Windows, Linux, or Mac. The parser is also multi-threaded to take advantage of modern multi-core CPUs.

### Cross-Platform Flexibility

The library itself can run directly on all major platforms (i.e. Windows, Linux, and Mac) and also exposes a REST API so that parsed data can be accessed from any application or programming language that can connect to a REST API. The REST API and library are also exposed via a Docker image that can be run on any platform that supports Docker.

Both options support running the parser on a different machine than the one running the application that needs the parsed data. This allows the parser to be run on a high-performance server or workstation while the application that needs the data runs on a low-power device like a Raspberry Pi, mobile device, or the web.

### Expertise

The maintainer has well over a decade of experience in the PCB Manufacting and hardware industry. Specifically he has worked on the development of ODB++ parsing and viewer applications for some of the established key players in the industry. Time spent working under the designer of the ODB++ specification has given him a unique perspective on the ODB++ format and how it is used in the industry. This experience is also leveraged to make the parser as fast and efficient as possible.

## Overview

### Current Implementation State

The diagram describes the current state of parser implementation and data availability. Green color describes areas of the ODB++ archive file that are implemented and have their data available for use. Red color describes areas that are not parsed so their data is not yet available.

![image](https://github.com/nam20485/OdbDesign/assets/1829396/2b902d94-c9ba-40cb-9b79-e0e6c4ba775a)

### Architecture

## Building from Source

### CMake C++ Project

### Docker Image

## Integration

There are four interfaces that allow use of the library in other applications.

### C++ Shared Library

### REST API

### Python Object Interface

SWIG is used to create a Python interface to the C++ library. The interface is exposed as a Python package that can be installed using pip. Parsing is supported, but currently not all data is available to access via the Python interface. Using the Python interface for anything beyond basic parsing would require implementation of Python wrapper classes to hold the data you are interested in. A better route for Python integration is probably to use the REST API or to generate Python bindings automatically using the included Goolge Protobuf definitions.

### Google Protobuf Protocol Buffers

Data objects returned from the parser library support serialization to and from [Google Protobuf protocol buffers](https://protobuf.dev/). This allows the data to be easily shared with other applications and programming languages that support protocol buffers. This is a highly optimized binary encoding so it is fast and small. The protocol buffer definitions are included in the library so they can be used to generate code for other languages.

## License

This project is free and open source under the MIT [license](https://github.com/nam20485/OdbDesign/blob/c0c8b6e4b93e1c7d4d5e65c7ad25157c883f8bfb/LICENSE).

## Contact

If you are interested in using the parser in your application or code, or have any questions about it please do not hesistate to contact me. If you need support for integration of the parser into your own product and/or need its feature set extended, I am available for consulting at very reasonable fees.

* [Email me (maintainer)](mailto:nmiller217@gmail.com?subject=OdbDesign)
* [GitHub](https://github.com/nam20485/odbdesign)
* [LinkedIn](https://www.linkedin.com/in/namiller/)

## ODB++

* [ODB++ Format Home](https://odbplusplus.com/design/)
* [ODB++ Format Documentation & Resources](https://odbplusplus.com/design/our-resources/)
* [ODB++ Format Specification v8.1 update 3](https://odbplusplus.com//wp-content/uploads/sites/2/2021/02/odb_spec_user.pdf)

_ODB++ is a registered trademark of Siemens and Mentor Graphics Corporation._