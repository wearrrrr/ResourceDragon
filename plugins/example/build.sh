#!/bin/bash

MINGW=OFF
BUILD_DIR="build"

for arg in "$@"; do
    if [ "$arg" = "-mingw" ]; then
        printf "Building for mingw...\n"
        MINGW=ON
        BUILD_DIR="build-mingw"
    fi
done



set -e

echo "Building example plugin..."

# Create build directory
mkdir -p $BUILD_DIR
cd $BUILD_DIR

if [ "$MINGW" = "ON" ]; then
    x86_64-w64-mingw32-cmake .. -G Ninja
else
    cmake .. -G Ninja
fi
ninja -j$(nproc)

echo "Plugin built successfully!"
echo "Plugin location: $(find ../../ -name "*.so" -o -name "*.dll" | head -1)"

# Check if the plugin was built
if [ -f "../../../plugins/example_plugin.so" ] || [ -f "../../../plugins/example_plugin.dll" ]; then
    echo "Plugin successfully installed to plugins directory"
else
    echo "Warning: Plugin may not have been copied to plugins directory"
fi
