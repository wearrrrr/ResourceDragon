#!/bin/bash

NPROC=$(nproc)
if [ "$NPROC" -lt 1 ]; then NPROC=1; fi

DEBUG=OFF
MINGW=OFF
EMSCRIPTEN=OFF

BUILD_DIR=build

for arg in "$@"; do
    if [ "$arg" = "-debug" ]; then
        printf "Building as debug...\n"
        DEBUG=ON
        break
    else if [ "$arg" = "-mingw" ]; then
        printf "Building targeting Mingw...\n"
        MINGW=ON
        BUILD_DIR=build-mingw
        break
    else if [ "$arg" = "-emscripten" ]; then
        printf "Building targeting Emscripten...\n"
        EMSCRIPTEN=ON
        BUILD_DIR=build-emscripten
        break
    fi
    fi
    fi
done

if [ "$MINGW" = "ON" ]; then
    build-scripts/build_mingw.sh "$@"
else
    rm ${BUILD_DIR}/ResourceDragon
    if [ $EMSCRIPTEN = "ON" ]; then
        emcmake cmake -B ${BUILD_DIR} -G Ninja -DDEBUG=${DEBUG} -DEMSCRIPTEN=${EMSCRIPTEN}
    else
        cmake -B ${BUILD_DIR} -G Ninja -DDEBUG=${DEBUG}
    fi;

    cd ${BUILD_DIR}
    ninja -j$NPROC
    cd ..

    if [ -f build/ResourceDragon ] || [ -f build/ResourceDragon.wasm ]; then
        printf "\x1B[1;32mCompiled successfully!\nOutput files are in $PWD/build/\x1B[0m \n"
        exit 0
    else
        printf "\x1B[1;31mBuild Failed!! Check the build output.\n"
        exit -1
    fi
fi
