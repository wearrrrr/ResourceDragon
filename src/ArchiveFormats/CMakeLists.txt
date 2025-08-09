add_library(ArchiveFormats STATIC
    ExeFile.cpp
    sha1.c

    HSP/hsp.cpp
    NitroPlus/npa.cpp
    NitroPlus/npk.cpp
    NitroPlus/mpk.cpp
    PFS/pfs.cpp
    SonicAdv/pak.cpp
    # Touhou/pbg.cpp
    XP3/xp3.cpp
)

target_include_directories(ArchiveFormats PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(ArchiveFormats PRIVATE thlib util)

install(TARGETS ArchiveFormats
    LIBRARY DESTINATION lib
)
