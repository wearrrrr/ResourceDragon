#!/bin/bash

NPROC=$(nproc)
if [ "$NPROC" -lt 1 ]; then NPROC=1; fi

DEBUG=OFF
MINGW=OFF
EMSCRIPTEN=OFF
ALPINE=OFF
RERUN_CMAKE=false

BUILD_DIR=build/release

C=clang
CXX=clang++

for arg in "$@"; do
    if [ "$arg" = "-debug" ]; then
        printf "Building as debug...\n"
        DEBUG=ON
        BUILD_DIR=build/debug
    elif [ "$arg" = "-cmake" ]; then
        printf "Re-running cmake...\n"
        RERUN_CMAKE=true
    elif [ "$arg" = "-mingw" ]; then
        printf "Building targeting Mingw...\n"
        MINGW=ON
        BUILD_DIR=build-mingw
    elif [ "$arg" = "-emscripten" ]; then
        printf "Building targeting Emscripten...\n"
        EMSCRIPTEN=ON
        BUILD_DIR=build-emscripten
    elif [ "$arg" = "-alpine" ]; then
        printf "Building targeting Alpine...\n"
        ALPINE=ON
        BUILD_DIR=build-alpine
    fi
done

# Check for the existence of the build dir, if it doesn't exist, we need to rerun cmake.
if [ ! -d $BUILD_DIR ]; then
    RERUN_CMAKE=true
fi

rm $BUILD_DIR/ResourceDragon

if [ "$MINGW" = "ON" ]; then
    build-scripts/build_mingw.sh "$@"
else
    if [ "$RERUN_CMAKE" = "true" ]; then
        rm ${BUILD_DIR}/ResourceDragon
        if [ $EMSCRIPTEN = "ON" ]; then
            emcmake cmake -B ${BUILD_DIR} -G Ninja -DDEBUG=${DEBUG} -DEMSCRIPTEN=${EMSCRIPTEN}
        else
            cmake -B ${BUILD_DIR} -G Ninja -DDEBUG=${DEBUG} -DALPINE=${ALPINE} -DCMAKE_C_COMPILER=${C} -DCMAKE_CXX_COMPILER=${CXX}
        fi;
    fi

    cd ${BUILD_DIR}
    ninja -j$NPROC
    cd ../../

    if [ -f ${BUILD_DIR}/ResourceDragon ] || ([ $EMSCRIPTEN = "ON" ] && [ -f build-emscripten/ResourceDragon.wasm ]); then
        printf "\x1B[1;32mCompiled successfully!\nOutput files are in $PWD/build/\x1B[0m \n"
        exit 0
    else
        printf "\x1B[1;31mBuild Failed!! Check the build output.\n"
        exit -1
    fi
fi
