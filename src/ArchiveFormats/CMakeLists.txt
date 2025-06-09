add_library(ArchiveFormats
    ElfFile.cpp
    ExeFile.cpp

    HSP/hsp.cpp
    NitroPlus/mpk.cpp
    NitroPlus/npa.cpp
    PFS/pfs.cpp
    SonicAdv/pak.cpp
    Touhou/thdat.cpp
    XP3/xp3.cpp
    Zip/zip.cpp
)

target_include_directories(ArchiveFormats PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
