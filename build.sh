#!/bin/bash

NPROC=$(nproc --ignore=2)
if [ "$NPROC" -lt 1 ]; then NPROC=1; fi

DEBUG=OFF
UBUNTU=OFF

for arg in "$@"; do
    if [ "$arg" = "-debug" ]; then
        printf "Building as debug...\n"
        DEBUG=ON
        break
    fi
    if [ "$arg" = "-ubuntu" ]; then
        printf "Building targeting Ubuntu 22.04...\n"
        UBUNTU=ON
        break
    fi
done

rm build/ResourceDragon
cmake -B build -G Ninja -DUBUNTU=${UBUNTU} -DDEBUG=${DEBUG}
cd build
ninja -j$NPROC
cd ..

if [ -f build/ResourceDragon ]; then
    printf "\x1B[1;32mCompiled successfully!\nOutput files are in $PWD/build/\n\x1B[m"
    exit 0
else
    printf "\x1B[1;31mBuild Failed!! Check the build output.\n"
    exit -1
fi
