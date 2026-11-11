---
title: OdbDesign ODB++ Parser
---

# OdbDesign ODB++ Parser

A free open source cross-platform C++ library for parsing ODB++ Design archives, accessing their data, and building net list product models. Exposed via a REST API packaged inside of a Docker image.

**UPDATE: LICENSE HAS CHANGED TO [AGPL V3.0](https://github.com/nam20485/OdbDesign/blob/development/LICENSE).** This is a non-permissive open source license that is not suitable for use in closed-source applications. Please contact me to obtain a license suitable for closed-source uses.

*[Skip to Build and Running Instructions](README.html#building-from-source)*

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

>The project has earned a high score on the [OpenSSF Security Scorecard](https://securityscorecards.dev/#what-is-openssf-scorecard). Details can be seen in the [Project Security](README.html#project-security) section of the README.

It is built using the latest available version of the C++ standard and is compiled with the latest available compiler versions. The parser is also built using the latest available versions of all of its dependencies and is regularly updated to use the latest versions of those dependencies as they are released.

>These checks are run against all branches starting with development, so there is no chance of a security vulnerability being introduced into the main and release branches. Docker's Scout Suite is used to scan the Docker image for security vulnerabilities, and GitHub's CodeQL is used to scan the code for security vulnerabilities.

## Project

* [README](README.md) &mdash; key features, architecture, build &amp; run instructions, integration guide

## Reference &amp; reports

* [API reference](api/) &mdash; Doxygen-generated reference for all projects: library (`OdbDesignLib`), server (`OdbDesignServer`), CLI (`OdbDesignApp`), utilities (`Utils`), and tests (`OdbDesignTests`)
* [Code coverage](coverage/) &mdash; per-branch HTML test coverage reports (`development` and `release` branches)
* [CDash dashboard](https://my.cdash.org/index.php?project=OdbDesign) &mdash; continuous integration build &amp; test results from all platforms
* [Dependencies &amp; SBOM](dependencies.md) &mdash; third-party software packages, dependency and SBOM reports, security scanning
* [Licensing &amp; IP protection](licensing.md) &mdash; the dual-license model, the commercial licensing process, and how the codebase is kept safe for closed-source use

## Community

* [Contact](contact.md) &mdash; email, chat, issues &amp; discussions, consulting

## More documentation

* [Build](BUILD.html) &nbsp;&middot;&nbsp; [Setup](README-SETUP.html) &nbsp;&middot;&nbsp; [Testing](TESTING.html) &nbsp;&middot;&nbsp; [Contributing](CONTRIBUTING.md) &nbsp;&middot;&nbsp; [Security](SECURITY.html) &nbsp;&middot;&nbsp; [Code of Conduct](CODE_OF_CONDUCT.md) &nbsp;&middot;&nbsp; [One-pager](one_pager.html) &nbsp;&middot;&nbsp; [Licensing](licensing.html)

* Internal working documents: [active plans](plan/README.md) &nbsp;&middot;&nbsp; [reference material](reference/README.md)

The source code is on GitHub: [nam20485/OdbDesign](https://github.com/nam20485/OdbDesign).
