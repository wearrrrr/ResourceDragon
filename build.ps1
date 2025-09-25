$ErrorActionPreference = "Stop"
$nproc = $env:NUMBER_OF_PROCESSORS

if (-Not (Test-Path "build-win32")) {
    New-Item -ItemType Directory -Path "build-win32" | Out-Null
}

cmake -B build-win32 -DCMAKE_BUILD_TYPE=Release -G Ninja `
      -DCMAKE_ASM_NASM_COMPILER="C:/Program Files/NASM/nasm.exe" `
      -DCMAKE_C_COMPILER="clang-cl" -DCMAKE_CXX_COMPILER="clang-cl" `
      -DCMAKE_C_FLAGS="-msse4.1 -Wno-unsafe-buffer-usage" `
      -DCMAKE_CXX_FLAGS="-msse4.1 -Wno-unsafe-buffer-usage" `
      -DAOM_TARGET_CPU=generic `
      -DSDL_IMAGE_AVIF=OFF -DOPUS_X86_MAY_HAVE_AVX=OFF

ninja -C build-win32 -j $nproc
