# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#[=======================================================================[.rst:
FindEditline
------------

.. versionadded:: 3.26

Find the BSD Editline library.

Imported targets
^^^^^^^^^^^^^^^^

.. versionadded:: 3.26

This module defines the following :prop_tgt:`IMPORTED` targets:

``Editline::Editline``
  The Editline ``Editline`` library, if found.

Result variables
^^^^^^^^^^^^^^^^

This module will set the following variables in your project:

``Editline_FOUND``
  true if the Editline headers and libraries were found
``Editline_VERSION``
  Editline release version
``Editline_INCLUDE_DIRS``
  the directory containing the Editline headers
``Editline_LIBRARIES``
  Editline libraries to be linked

Cache variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``Editline_INCLUDE_DIR``
  the directory containing the Editline headers
``Editline_LIBRARY``
  the Editline library

.. versionadded:: 3.26
  Debug and Release variants are found separately.
#]=======================================================================]

# Written by Roger Leigh <rleigh@codelibre.net>

function(_Editline_GET_VERSION version_hdr)
    file(STRINGS ${version_hdr} _contents REGEX "^[ \t]*#define[ \t]+RL_READLINE_VERSION[ \t]*.*")
    if (_contents)
        string(REGEX REPLACE ".*#define[ \t]+RL_READLINE_VERSION[ \t]+0x([0-9A-F]+).*" "\\1" Editline_HEXVERSION "${_contents}")
        if (NOT Editline_HEXVERSION MATCHES "^[0-9A-F]+$")
            message(FATAL_ERROR "Version parsing failed for Editline_HEXVERSION!")
        endif ()

        string(SUBSTRING "${Editline_HEXVERSION}" 0 2 Editline_MAJOR)
        string(SUBSTRING "${Editline_HEXVERSION}" 2 2 Editline_MINOR)

        math(EXPR Editline_MAJOR "0x${Editline_MAJOR}" OUTPUT_FORMAT DECIMAL)
        math(EXPR Editline_MINOR "0x${Editline_MINOR}" OUTPUT_FORMAT DECIMAL)

        set(Editline_VERSION "${Editline_MAJOR}.${Editline_MINOR}" PARENT_SCOPE)
        set(Editline_VERSION_MAJOR "${Editline_MAJOR}" PARENT_SCOPE)
        set(Editline_VERSION_MINOR "${Editline_MINOR}" PARENT_SCOPE)
    else ()
        message(FATAL_ERROR "Include file ${version_hdr} does not exist or does not contain expected version information")
    endif ()
endfunction()

# Find include directory
find_path(Editline_INCLUDE_DIR
        NAMES "readline/readline.h"
        HINTS /usr/include/edit
        DOC "Editline include directory")
mark_as_advanced(Editline_INCLUDE_DIR)

if (Editline_INCLUDE_DIR AND EXISTS "${Editline_INCLUDE_DIR}/readline/readline.h")
    _Editline_GET_VERSION("${Editline_INCLUDE_DIR}/readline/readline.h")
endif ()

if (NOT Editline_LIBRARY)
    # Find all Editline libraries
    find_library(Editline_LIBRARY_RELEASE
            NAMES edit
            NAMES_PER_DIR
            HINTS /usr/lib
            DOC "Editline libraries (release)")
    find_library(Editline_LIBRARY_DEBUG
            NAMES editd
            NAMES_PER_DIR
            HINTS /usr/lib
            DOC "Editline libraries (debug)")
    include(SelectLibraryConfigurations)
    select_library_configurations(Editline)
    mark_as_advanced(Editline_LIBRARY_RELEASE Editline_LIBRARY_DEBUG)
endif ()

unset(Editline_VERSION_MAJOR)
unset(Editline_VERSION_MINOR)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Editline
        FOUND_VAR Editline_FOUND
        REQUIRED_VARS Editline_LIBRARY
        Editline_INCLUDE_DIR
        Editline_VERSION
        VERSION_VAR Editline_VERSION
        FAIL_MESSAGE "Failed to find Editline")

if (Editline_FOUND)
    set(Editline_INCLUDE_DIRS "${Editline_INCLUDE_DIR}")
    set(Editline_LIBRARIES "${Editline_LIBRARY}")

    # For header-only libraries
    if (NOT TARGET Editline::Editline)
        add_library(Editline::Editline UNKNOWN IMPORTED)
        if (Editline_INCLUDE_DIRS)
            set_target_properties(Editline::Editline PROPERTIES
                    INTERFACE_INCLUDE_DIRECTORIES "${Editline_INCLUDE_DIRS}")
        endif ()
        if (EXISTS "${Editline_LIBRARY}")
            set_target_properties(Editline::Editline PROPERTIES
                    IMPORTED_LINK_INTERFACE_LANGUAGES "CXX"
                    IMPORTED_LOCATION "${Editline_LIBRARY}")
        endif ()
        if (EXISTS "${Editline_LIBRARY_RELEASE}")
            set_property(TARGET Editline::Editline APPEND PROPERTY
                    IMPORTED_CONFIGURATIONS RELEASE)
            set_target_properties(Editline::Editline PROPERTIES
                    IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
                    IMPORTED_LOCATION_RELEASE "${Editline_LIBRARY_RELEASE}")
        endif ()
        if (EXISTS "${Editline_LIBRARY_DEBUG}")
            set_property(TARGET Editline::Editline APPEND PROPERTY
                    IMPORTED_CONFIGURATIONS DEBUG)
            set_target_properties(Editline::Editline PROPERTIES
                    IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
                    IMPORTED_LOCATION_DEBUG "${Editline_LIBRARY_DEBUG}")
        endif ()
    endif ()
endif ()
