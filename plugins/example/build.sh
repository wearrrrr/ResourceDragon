#!/bin/bash

# Build script for example plugin

set -e

echo "Building example plugin..."

# Get the directory of this script
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Create build directory
mkdir -p build
cd build

# Configure with CMake
cmake ..

# Build the plugin
make -j$(nproc)

echo "Plugin built successfully!"
echo "Plugin location: $(find . -name "*.so" -o -name "*.dll" | head -1)"

# Check if the plugin was built
if [ -f "../../../plugins/example_plugin.so" ] || [ -f "../../../plugins/example_plugin.dll" ]; then
    echo "Plugin successfully installed to plugins directory"
else
    echo "Warning: Plugin may not have been copied to plugins directory"
fi