#!/bin/bash
./build.sh "$@"
BUILD_RESULT=$?

if [ $BUILD_RESULT = -1 ]; then
    printf "\x1B[1;31mBuild failed! Can't continue creating appimage...\n\x1B[m"
else
    mkdir -p build/AppDir/usr/share/applications/
    mkdir -p build/AppDir/usr/share/icons/hicolor/256x256/apps/
    cp ./resource-dragon.png build/AppDir/usr/share/icons/hicolor/256x256/apps/resource-dragon.png
    cp ResourceDragon.desktop build/AppDir/usr/share/applications/
    cd build
    NO_STRIP=true linuxdeploy --appdir AppDir --output appimage --executable ResourceDragon
fi
