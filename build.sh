#!/usr/bin/env bash

if [ "$PWD" != "$(dirname "$(realpath "$0")")" ]; then
    echo "Run script from the source directory"
    exit;
fi

BUILD_TYPE=${BUILD_TYPE:-RELEASE}
DOKANY_DIR=${DOKANY_DIR:-"../dokany/dokan_fuse"}

for arg in "$@"
do
    if [ "$arg" = "--mingw64" ]; then
        mkdir -p build-mingw64-${BUILD_TYPE,,}
        cmake -S . -B ./build-mingw64-${BUILD_TYPE,,} \
            -DCMAKE_TOOLCHAIN_FILE=./CMakeModules/Toolchain-mingw64.cmake \
            -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
            -DENABLE_FUSE=1 \
            -DFUSE_DIR=$DOKANY_DIR
        make -j 4 -C build-mingw64-${BUILD_TYPE,,}

    elif [ "$arg" = "--native" ]; then
        mkdir -p build-native-${BUILD_TYPE,,}
        cmake -S . -B ./build-native-${BUILD_TYPE,,} \
            -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
            -DENABLE_FUSE=1
        make -j 4 -C build-native-${BUILD_TYPE,,}

    elif [ "$arg" = "--clean" ]; then
        rm -r build-*

    fi
done
