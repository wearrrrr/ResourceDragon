#!/bin/bash
./build.sh "$@"

if (($? == 0)); then
    if [ "$1" = "-mingw" ]; then
        echo "Running mingw build with wine..."
        wine build-mingw/Win32/ResourceDragon.exe
    else
        build/ResourceDragon
    fi
fi
