$ErrorActionPreference = "Stop"
$nproc = $env:NUMBER_OF_PROCESSORS

if (-Not (Test-Path "build-win32")) {
    New-Item -ItemType Directory -Path "build-win32" | Out-Null
}

cmake -B build-win32 -DCMAKE_BUILD_TYPE=Release -G Ninja `
      -DCMAKE_C_COMPILER="clang-cl" -DCMAKE_CXX_COMPILER="clang-cl" `

ninja -C build-win32 -j $nproc
