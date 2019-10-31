#!/usr/bin/env bash

for arg in "$@"
do
    if [ "$arg" = "--mingw64" ]; then
        mkdir -p build-mingw64
        cmake -S . -B ./build-mingw64 \
            -DCMAKE_TOOLCHAIN_FILE=./CMakeModules/Toolchain-mingw64.cmake \
            -DCMAKE_BUILD_TYPE=RELEASE \
            -DENABLE_FUSE=1 \
            -DFUSE_DIR=../dokany/dokan_fuse
    fi
done
