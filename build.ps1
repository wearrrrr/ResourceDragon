$ErrorActionPreference = "Stop"

if (-Not (Test-Path "build-win32")) {
    New-Item -ItemType Directory -Path "build-win32" | Out-Null
}

cmake -B build -DCMAKE_BUILD_TYPE=Release -G "NMake Makefiles" -DAOM_TARGET_CPU=generic `
      -DCMAKE_C_COMPILER="clang-cl" -DCMAKE_CXX_COMPILER="clang-cl"

# Build
cmake --build build-win32 --config Release
