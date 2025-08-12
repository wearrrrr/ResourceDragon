#!/bin/bash
./build.sh "$@"

if (($? == 0)); then
    if [ "$1" = "-mingw" ]; then
        echo "Running mingw build with wine..."
        wine build-mingw/Win32/ResourceDragon.exe
    else if [ "$1" = "-emscripten" ]; then
        exit 0
    else
        build/ResourceDragon
    fi
    fi
fi
