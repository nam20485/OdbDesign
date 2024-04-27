# Build Instructions

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
