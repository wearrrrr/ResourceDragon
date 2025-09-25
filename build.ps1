$ErrorActionPreference = "Stop"

$env:CC = "clang-cl"
$env:CXX = "clang-cl"

if (-Not (Test-Path "build-win32")) {
    New-Item -ItemType Directory -Path "build-win32" | Out-Null
}

cmake -B build -DCMAKE_BUILD_TYPE=Release -G "NMake Makefiles" `
      -DCMAKE_C_COMPILER=$env:CC -DCMAKE_CXX_COMPILER=$env:CXX

# Build
cmake --build build-win32 --config Release
