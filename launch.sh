#!/bin/bash
./build.sh "$@"

MINGW=false
EMSCRIPTEN=false
ALPINE=false
DEBUG=false

for arg in "$@"; do
    if [ "$arg" = "-debug" ]; then
        DEBUG=true
    elif [ "$arg" = "-mingw" ]; then
        MINGW=true
    elif [ "$arg" = "-emscripten" ]; then
        EMSCRIPTEN=true
    elif [ "$arg" = "-alpine" ]; then
        ALPINE=true
    fi
done;

if (($? == 0)); then
    if [ "$MINGW" = true ]; then
        echo "Running mingw build with wine..."
        wine build-mingw/Win32/ResourceDragon.exe
    elif [ "$EMSCRIPTEN" = true ]; then
        exit 0
    elif [ "$ALPINE" = true ]; then
        build-alpine/ResourceDragon
    elif [ "$DEBUG" = true ]; then
        build/debug/ResourceDragon
    else
        build/release/ResourceDragon
    fi
fi
