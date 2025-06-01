#! /bin/bash

sudo apt update
sudo apt install    git \
                    cmake \
                    ninja-build \
                    build-essential \
                    docker.io \
                    docker-compose-v2

git clone https://github.com/microsoft/vcpkg $HOME/vcpkg
$HOME/vcpkg/bootstrap-vcpkg.sh