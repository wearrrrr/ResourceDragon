NPROC=$(nproc --ignore=2)
if [ "$NPROC" -lt 1 ]; then NPROC=1; fi

rm build/ResourceDragon
cmake -B build -G Ninja
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
