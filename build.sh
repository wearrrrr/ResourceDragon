#!/bin/bash

NPROC=$(nproc)
if [ "$NPROC" -lt 1 ]; then NPROC=1; fi

DEBUG=OFF
EMSCRIPTEN=OFF

for arg in "$@"; do
    if [ "$arg" = "-debug" ]; then
        printf "Building as debug...\n"
        DEBUG=ON
        break
    fi
    if [ "$arg" = "-emscripten" ]; then
        printf "Building targeting Emscripten...\n"
        EMSCRIPTEN=ON
        break
    fi
done

rm build/ResourceDragon
if [ $EMSCRIPTEN = "ON" ]; then
    emcmake cmake -B build -G Ninja -DDEBUG=${DEBUG} -DEMSCRIPTEN=${EMSCRIPTEN}
else
    cmake -B build -G Ninja -DDEBUG=${DEBUG}
fi;

cd build
ninja -j$NPROC
cd ..

if [ -f build/ResourceDragon ] || [ -f build/ResourceDragon.wasm ]; then
    printf "\x1B[1;32mCompiled successfully!\nOutput files are in $PWD/build/\n\x1B[m"
    exit 0
else
    printf "\x1B[1;31mBuild Failed!! Check the build output.\n"
    exit -1
fi
