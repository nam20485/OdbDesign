#!/bin/bash
set -e

sudo apt update
sudo apt install -y -qq --no-install-recommends \
                    git \
                    cmake \
                    ninja-build \
                    build-essential \
                    docker.io \
                    docker-compose-v2

git clone https://github.com/microsoft/vcpkg $HOME/vcpkg
$HOME/vcpkg/bootstrap-vcpkg.sh