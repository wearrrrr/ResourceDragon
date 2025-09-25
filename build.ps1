$ErrorActionPreference = "Stop"

if (-Not (Test-Path "build-win32")) {
    New-Item -ItemType Directory -Path "build-win32" | Out-Null
}

cmake -B build -DCMAKE_BUILD_TYPE=Release -G "NMake Makefiles" -DAOM_TARGET_CPU=generic `
      -DCMAKE_ASM_NASM_COMPILER="C:/ProgramData/chocolatey/bin/nasm.exe" `
      -DCMAKE_C_COMPILER="clang-cl" -DCMAKE_CXX_COMPILER="clang-cl" `
      -DSDL_IMAGE_AVIF=OFF

# Build
cmake --build build-win32 --config Release
