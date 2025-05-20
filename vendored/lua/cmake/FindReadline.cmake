# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#[=======================================================================[.rst:
FindReadline
------------

.. versionadded:: 3.26

Find the GNU Readline library.

Imported targets
^^^^^^^^^^^^^^^^

.. versionadded:: 3.26

This module defines the following :prop_tgt:`IMPORTED` targets:

``Readline::Readline``
  The Readline ``readline`` library, if found.

Result variables
^^^^^^^^^^^^^^^^

This module will set the following variables in your project:

``Readline_FOUND``
  true if the Readline headers and libraries were found
``Readline_VERSION``
  Readline release version
``Readline_INCLUDE_DIRS``
  the directory containing the Readline headers
``Readline_LIBRARIES``
  Readline libraries to be linked

Cache variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``Readline_INCLUDE_DIR``
  the directory containing the Readline headers
``Readline_LIBRARY``
  the Readline library

.. versionadded:: 3.26
  Debug and Release variants are found separately.
#]=======================================================================]

# Written by Roger Leigh <rleigh@codelibre.net>

function(_Readline_GET_VERSION version_hdr)
    file(STRINGS ${version_hdr} _contents REGEX "^[ \t]*#define[ \t]+RL_READLINE_VERSION[ \t]*.*")
    if (_contents)
        string(REGEX REPLACE ".*#define[ \t]+RL_READLINE_VERSION[ \t]+0x([0-9A-F]+).*" "\\1" Readline_HEXVERSION "${_contents}")
        if (NOT Readline_HEXVERSION MATCHES "^[0-9A-F]+$")
            message(FATAL_ERROR "Version parsing failed for Readline_HEXVERSION!")
        endif ()

        string(SUBSTRING "${Readline_HEXVERSION}" 0 2 Readline_MAJOR)
        string(SUBSTRING "${Readline_HEXVERSION}" 2 2 Readline_MINOR)

        math(EXPR Readline_MAJOR "0x${Readline_MAJOR}" OUTPUT_FORMAT DECIMAL)
        math(EXPR Readline_MINOR "0x${Readline_MINOR}" OUTPUT_FORMAT DECIMAL)

        set(Readline_VERSION "${Readline_MAJOR}.${Readline_MINOR}" PARENT_SCOPE)
        set(Readline_VERSION_MAJOR "${Readline_MAJOR}" PARENT_SCOPE)
        set(Readline_VERSION_MINOR "${Readline_MINOR}" PARENT_SCOPE)
    else ()
        message(FATAL_ERROR "Include file ${version_hdr} does not exist or does not contain expected version information")
    endif ()
endfunction()

# Find include directory
find_path(Readline_INCLUDE_DIR
        NAMES "readline/readline.h"
        DOC "Readline include directory")
mark_as_advanced(Readline_INCLUDE_DIR)

if (Readline_INCLUDE_DIR AND EXISTS "${Readline_INCLUDE_DIR}/readline/readline.h")
    _Readline_GET_VERSION("${Readline_INCLUDE_DIR}/readline/readline.h")
endif ()

if (NOT Readline_LIBRARY)
    # Find all Readline libraries
    find_library(Readline_LIBRARY_RELEASE
            NAMES readline
            NAMES_PER_DIR
            DOC "Readline libraries (release)")
    find_library(Readline_LIBRARY_DEBUG
            NAMES readlined
            NAMES_PER_DIR
            DOC "Readline libraries (debug)")
    include(SelectLibraryConfigurations)
    select_library_configurations(Readline)
    mark_as_advanced(Readline_LIBRARY_RELEASE Readline_LIBRARY_DEBUG)
endif ()

unset(Readline_VERSION_MAJOR)
unset(Readline_VERSION_MINOR)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Readline
        FOUND_VAR Readline_FOUND
        REQUIRED_VARS Readline_LIBRARY
        Readline_INCLUDE_DIR
        Readline_VERSION
        VERSION_VAR Readline_VERSION
        FAIL_MESSAGE "Failed to find Readline")

if (Readline_FOUND)
    set(Readline_INCLUDE_DIRS "${Readline_INCLUDE_DIR}")
    set(Readline_LIBRARIES "${Readline_LIBRARY}")

    # For header-only libraries
    if (NOT TARGET Readline::Readline)
        add_library(Readline::Readline UNKNOWN IMPORTED)
        if (Readline_INCLUDE_DIRS)
            set_target_properties(Readline::Readline PROPERTIES
                    INTERFACE_INCLUDE_DIRECTORIES "${Readline_INCLUDE_DIRS}")
        endif ()
        if (EXISTS "${Readline_LIBRARY}")
            set_target_properties(Readline::Readline PROPERTIES
                    IMPORTED_LINK_INTERFACE_LANGUAGES "CXX"
                    IMPORTED_LOCATION "${Readline_LIBRARY}")
        endif ()
        if (EXISTS "${Readline_LIBRARY_RELEASE}")
            set_property(TARGET Readline::Readline APPEND PROPERTY
                    IMPORTED_CONFIGURATIONS RELEASE)
            set_target_properties(Readline::Readline PROPERTIES
                    IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
                    IMPORTED_LOCATION_RELEASE "${Readline_LIBRARY_RELEASE}")
        endif ()
        if (EXISTS "${Readline_LIBRARY_DEBUG}")
            set_property(TARGET Readline::Readline APPEND PROPERTY
                    IMPORTED_CONFIGURATIONS DEBUG)
            set_target_properties(Readline::Readline PROPERTIES
                    IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
                    IMPORTED_LOCATION_DEBUG "${Readline_LIBRARY_DEBUG}")
        endif ()
    endif ()
endif ()
