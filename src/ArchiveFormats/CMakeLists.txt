add_library(ArchiveFormats
    ExeFile.cpp

    HSP/hsp.cpp
    NitroPlus/npa.cpp
    NitroPlus/npk.cpp
    NitroPlus/mpk.cpp
    PFS/pfs.cpp
    SonicAdv/pak.cpp
    Touhou/thdat.cpp
    XP3/xp3.cpp
    Zip/zip.cpp
)

target_include_directories(ArchiveFormats PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(ArchiveFormats PRIVATE util)

install(TARGETS ArchiveFormats
    LIBRARY DESTINATION lib
)
