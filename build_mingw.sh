x86_64-w64-mingw32-cmake -B build-mingw -G Ninja

rm build-mingw/ResourceDragon.exe

cd build-mingw
ninja -j12
mkdir -p Win32
cp -r ../fonts/ ./Win32/fonts/
cp ResourceDragon.exe ./Win32/
cp src/ArchiveFormats/libArchiveFormats.dll ./Win32/
cp src/GUI/libGUI.dll ./Win32/
cp src/Scripting/libScripting.dll ./Win32/
cp vendored/SDL/SDL3.dll ./Win32/
cp vendored/SDL_image/SDL3_image.dll ./Win32/
cp vendored/SDL_mixer/SDL3_mixer.dll ./Win32/
cp _deps/freetype-build/libfreetype.dll ./Win32/
cd Win32
cp /usr/x86_64-w64-mingw32/bin/libwinpthread-1.dll ./
cp /usr/x86_64-w64-mingw32/bin/libstdc++-6.dll ./
cp /usr/x86_64-w64-mingw32/bin/libssp-0.dll ./
cp /usr/x86_64-w64-mingw32/bin/libgcc_s_seh-1.dll ./
cp /usr/x86_64-w64-mingw32/bin/zlib1.dll ./

cd ../../

if ! [ ! -f build-mingw/Win32/ResourceDragon.exe ]; then
    printf "\x1B[1;32mCompiled successfully!\n\x1B[1;30mOutput files are in $PWD/build-mingw/Win32/\n"
    exit 0
else
    printf "\x1B[1;31mBuild Failed!! Check the build output.\n"
    exit -1
fi
