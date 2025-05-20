# Lua with CMake support 

This is Lua 5.4.7, released on 13 Jun 2024.

For installation instructions, license details, and
further information about Lua, see
[doc/readme.html](doc/readme.html).

## CMake options

This project supports all of the standard CMake options, and should be configured
just like any other CMake-based project.  Out of source builds are supported.

### Platform configuration

The platform should already be known to CMake and this option should not typically
be needed.  However, the autodetected platform may be explicitly overridden if
required.

| Option                        | Description                                    |
|-------------------------------|------------------------------------------------|
| `-DLUA_PLATFORM=Autodetect`   | Automatically determine the platform [default] |
| `-DLUA_PLATFORM=POSIX`        | POSIX platform (UNIX and UNIX-like systems)    |
| `-DLUA_PLATFORM=Windows`      | Windows platform                               |
| `-DLUA_PLATFORM=Freestanding` | "Freestanding" platform (e.g. embedded)        |

### Library configuration

Lua may be built as a static library or as a shared (dynamic) library.  Additionally,
Lua may load additional libraries at runtime if configured to use a platform-specific
library loading mechanism.

| Option                     | Description                                                      |
|----------------------------|------------------------------------------------------------------|
| `-DBUILD_SHARED_LIBS=ON`   | Build shared (dynamic) library                                   |
| `-DBUILD_SHARED_LIBS=OFF`  | Build static library [default]                                   |
| `-DLUA_LIBRARY_LOADER=ON`  | Enable library loading [default with POSIX or Windows platforms] |
| `-DLUA_LIBRARY_LOADER=OFF` | Disable library loading [default on all other platforms]         |

### Language configuration

It is possible to configure several aspects of the behaviour of the Lua runtime.  If
compiled with a C++ compiler, exceptions may be used for error handling in place of
longjmp.  It is also possible to restrict Lua to only use C89 features if required.
Additional consistency checking may also be enabled for debugging purposes.

| Option                       | Description                                               |
|------------------------------|-----------------------------------------------------------|
| `-DLUA_LANGUAGE_SUPPORT=C`   | Compile Lua with the C compiler [default]                 |
| `-DLUA_LANGUAGE_SUPPORT=CXX` | Compile Lua with the C++ compiler                         |
| `-DLUA_USE_LONGJMP=ON`       | Use longjmp for error handling [C++ only]                 |
| `-DLUA_USE_LONGJMP=OFF`      | Use exceptions for error handling [default, C++ only]     |
| `-DLUA_USE_C89=ON`           | Restrict C language usage to C89 [C only]                 |
| `-DLUA_USE_C89=OFF`          | Do not restrict C language usage to C89 [default, C only] |
| `-DLUA_USE_APICHECK=ON`      | Enable additional consistency checks for debugging        |
| `-DLUA_USE_APICHECK=OFF`     | Do not enable additional consistency checks for debugging |

### Line editor

The interactive `lua` command may optionally use a line editor library for improved
usability including a command history.  By default, the GNU Readline library will be
used  if found, or else the BSD Editline library will be used if found.  The specific
library to use may be manually selected.

| Option                         | Description                                           |
|--------------------------------|-------------------------------------------------------|
| `-DLUA_LINE_EDITOR=Autodetect` | Automatically determine line editor library [default] |
| `-DLUA_LINE_EDITOR=Readline`   | Use GNU Readline library                              |
| `-DLUA_LINE_EDITOR=Editline`   | Use BSD Editline library                              |
