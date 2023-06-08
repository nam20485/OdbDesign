#FROM debian:bookworm-20230522-slim AS build
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

# copy source
COPY . /src/OdbDesign
WORKDIR /src/OdbDesign

# generate SWIG python bindings
RUN chmod +x scripts/generate-python-module.sh
RUN scripts/generate-python-module.sh

# configure & build using presets
# linux-release
RUN cmake --preset linux-release
RUN cmake --build --preset linux-release

# much smaller runtime image
#FROM python:3.11.4-bullseye AS run
FROM debian:bookworm-20230522-slim

# copy PyOdbDesignServer files
COPY --from=build /src/OdbDesign/OdbDesignServer OdbDesignServer

# copy PyOdbDesignLib files
COPY --from=build /src/OdbDesign/out/build/linux-release/OdbDesignLib/libOdbDesign.so /OdbDesignServer/PyOdbDesignLib/_PyOdbDesignLib.so
COPY --from=build /src/OdbDesign/PyOdbDesignLib/PyOdbDesignLib.py /OdbDesignServer/PyOdbDesignLib/
RUN touch /OdbDesignServer/PyOdbDesignLib/__init__.py

RUN apt-get update && \
    apt-get install -y --no-install-recommends \       
        python3-dev \
        python3-pip

RUN python3 -m pip install -r /OdbDesignServer/requirements.txt --break-system-packages

# run
WORKDIR /OdbDesignServer
EXPOSE 8000
CMD ["gunicorn", "--bind", ":8000", "--workers", "3", "OdbDesignServer.wsgi:application"]
