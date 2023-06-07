FROM debian:bookworm-20230522-slim AS build

# install dependencies
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        gpg \
        curl \
        apt-transport-https \
        build-essential \
        ca-certificates \
        cmake \            
        g++ \
        ninja-build \
        python3-dev \
        # mingw-w64 \        
        swig      

# add MS PowerShell repo
RUN curl https://packages.microsoft.com/keys/microsoft.asc | gpg --yes --dearmor --output /usr/share/keyrings/microsoft.gpg
RUN sh -c 'echo "deb [arch=amd64 signed-by=/usr/share/keyrings/microsoft.gpg] https://packages.microsoft.com/repos/microsoft-debian-bullseye-prod bullseye main" > /etc/apt/sources.list.d/microsoft.list'

# install PowerShell
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        powershell

# copy source
RUN mkdir -p /src/OdbDesign
WORKDIR /src/OdbDesign
COPY . .

# generate SWIG python bindings
RUN chmod +x swig/generate-python-lib.ps1
RUN swig/generate-python-lib.ps1

# configure & build using presets
# linux-release
RUN cmake --preset linux-release
RUN cmake --build --preset linux-release
# linux-debug
RUN cmake --preset linux-debug
RUN cmake --build --preset linux-debug

# much smaller runtime image
FROM debian:bookworm-20230522-slim AS run

# install Python3
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        python3-dev

RUN mkdir /OdbDesign
WORKDIR /OdbDesign
COPY --from=build /src/OdbDesign/out/build/linux-release/OdbDesignLib/libOdbDesign.so .
COPY --from=build /src/OdbDesign/out/build/linux-release/OdbDesignApp/OdbDesignApp .
# copy to a format that Python expects for extension modules
RUN cp libOdbDesign.so _PyOdbDesignLib.so
COPY --from=build /src/OdbDesign/swig/PyOdbDesignLib.py .
ENV LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/OdbDesign/

# run
ENTRYPOINT [ "./OdbDesignApp" ]
