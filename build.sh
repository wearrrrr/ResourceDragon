cmake -B build -G Ninja
cd build
ninja -j12

cd ..
printf "\x1B[1;32mCompiled successfully!\n\x1B[1;30mOutput files are in $PWD/build/\n"