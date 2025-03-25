rm -rf build/ResourceDragon

cmake -B build -G Ninja
cd build
ninja -j12

cd ..
if ! [ ! -f build/ResourceDragon ]; then
    printf "\x1B[1;32mCompiled successfully!\n\x1B[1;30mOutput files are in $PWD/build/\n"
    exit 0
else
    printf "\x1B[1;31mBuild Failed!! Check the build output.\n"
    exit -1
fi
