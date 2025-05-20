set(manual_pages
        lua.1
        luac.1)

set(html_docs
        contents.html
        index.css
        logo.gif
        lua.css
        manual.css
        manual.html
        OSIApproved_100X125.png
        readme.html)

foreach (man IN LISTS manual_pages)
    string(REPLACE ".1" "${PROJECT_VERSION_MAJOR}${PROJECT_VERSION_MINOR}.1"
            manver "${man}")
    install(FILES ${man}
            RENAME ${manver}
            DESTINATION "${CMAKE_INSTALL_MANDIR}/man1"
            COMPONENT "runtime")
endforeach ()

install(FILES ${html_docs}
        DESTINATION "${CMAKE_INSTALL_DOCDIR}${PROJECT_VERSION_MAJOR}${PROJECT_VERSION_MINOR}"
        COMPONENT "runtime")
