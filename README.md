# OdbDesign

A free open source cross-platform C++ library for parsing ODB++ Design archives and building net list product models exposed via a REST API, all packaged inside of a Docker image.

## Overview

### Current Implementation State

The diagram describes the current state of parser implementation and data availability. Green color describes areas of the ODB++ arechive file that are implemented and have their data available for use. Red color describes areas that are not parsed so their data is not yet available.

![image](https://github.com/nam20485/OdbDesign/assets/1829396/6c45831b-af62-4c45-b17c-137bc1f730aa)

_Some areas of the archive are usually empty and not generaly useful or needed for most cases, i.e. input/, output/, and user/ directories._

### Architecture

## Building from Source

## Integration

There are two interfaces that allow use of the library in other applications.

### C++ Shared Library

### REST API

## License

This project is free and open source under the MIT [license](https://github.com/nam20485/OdbDesign/blob/c0c8b6e4b93e1c7d4d5e65c7ad25157c883f8bfb/LICENSE).

## Contact

* [GitHub](https://github.com/nam20485/odbdesign)
* [Email Nathan (maintainer)](mailto:nmiller217@gmail.com?subject=OdbDesign)
* [LinkedIn](https://www.linkedin.com/in/namiller/)
