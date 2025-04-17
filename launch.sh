#!/bin/bash
./build.sh

if (($? == 0)); then
    build/ResourceDragon
fi
