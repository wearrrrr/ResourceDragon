#!/bin/bash
./build.sh "$@"

if (($? == 0)); then
    if [ "$@" = "-mingw"]; then
        wine build-mingw/Win32/ResourceDragon.exe
    else
        build/ResourceDragon
    fi
fi
