# ResourceDragon Plugin API Documentation

This document describes the plugin API for ResourceDragon, which allows you to create custom archive format handlers as dynamically loaded plugins.

## Overview

The plugin system allows you to extend ResourceDragon with support for custom archive formats without modifying the core application. Plugins are compiled as shared libraries (.so on Linux, .dll on Windows) and loaded at runtime.

## Plugin Structure

A plugin must export the following symbols:

### Required Exports

1. **Plugin Metadata**
   ```cpp
   extern "C" const char* RD_PluginName = "Your Plugin Name";
   extern "C" const char* RD_PluginVersion = "1.0.0";
   ```

2. **Plugin Functions**
   ```cpp
   RD_EXPORT bool RD_PluginInit(HostAPI* api);
   RD_EXPORT void RD_PluginShutdown();
   RD_EXPORT const ArchiveFormatVTable* RD_GetArchiveFormat(struct sdk_ctx* ctx);
   ```

## API Reference

### HostAPI Structure

The `HostAPI` structure provides access to host application functions:

```cpp
struct HostAPI {
    sdk_ctx* (*get_sdk_context)();
    LogFn_t log;
    LogFn_t warn;
    LogFn_t error;
};
```

- `get_sdk_context()`: Returns the SDK context for this plugin
- `log(ctx, msg, ...)`: Log an informational message
- `warn(ctx, msg, ...)`: Log a warning message
- `error(ctx, msg, ...)`: Log an error message

### SDK Context

The `sdk_ctx` structure contains plugin runtime information:

```cpp
struct sdk_ctx {
    int version;
    struct Logger* logger;
    struct ArchiveFormatWrapper* archiveFormat;
};
```

### Archive Format VTable

The core of your plugin is the `ArchiveFormatVTable` which defines how your format behaves:

```cpp
typedef struct ArchiveFormatVTable {
    ArchiveHandle (*New)(struct sdk_ctx* ctx);
    void (*Delete)(ArchiveHandle inst);
    int (*CanHandleFile)(ArchiveHandle inst, u8* buffer, u64 size, const char* ext);
    ArchiveBaseHandle (*TryOpen)(ArchiveHandle inst, u8* buffer, u64 size, const char* file_name);
    const char *(*GetTag)(ArchiveHandle inst);
    const char *(*GetDescription)(ArchiveHandle inst);
} ArchiveFormatVTable;
```

#### VTable Functions

- **New**: Create a new instance of your archive handler
- **Delete**: Clean up an archive handler instance
- **CanHandleFile**: Return non-zero if your plugin can handle the given file
- **TryOpen**: Attempt to open a file as your archive format
- **GetTag**: Return a short identifier for your format (e.g., "ZIP", "TAR")
- **GetDescription**: Return a human-readable description

### Archive Base VTable

When `TryOpen` succeeds, it returns an `ArchiveBaseHandle` containing:

```cpp
struct ArchiveBaseVTable {
    void (*Destroy)(ArchiveInstance inst);
    usize (*GetEntryCount)(ArchiveInstance inst);
    const char* (*GetEntryName)(ArchiveInstance inst, usize index);
    usize (*GetEntrySize)(ArchiveInstance inst, usize index);
    u8* (*OpenStream)(ArchiveInstance inst, usize index, usize* out_size);
};
```

- **Destroy**: Clean up the opened archive instance
- **GetEntryCount**: Return the number of files in the archive
- **GetEntryName**: Return the name of the file at the given index
- **GetEntrySize**: Return the size of the file at the given index
- **OpenStream**: Extract and return the data for the file at the given index

## Building a Plugin

### CMakeLists.txt Example

```cmake
cmake_minimum_required(VERSION 3.10)
project(YourPlugin VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 23)

# SDK paths
set(SDK_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/../../SDK)
set(SDK_UTIL_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/../../SDK/util)

add_library(your_plugin SHARED
    main.cpp
)

target_include_directories(your_plugin PRIVATE
    ${SDK_INCLUDE_DIR}
    ${SDK_UTIL_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/../..
)

set_target_properties(your_plugin PROPERTIES
    PREFIX ""
    OUTPUT_NAME "your_plugin"
)

if(WIN32)
    set_target_properties(your_plugin PROPERTIES SUFFIX ".dll")
else()
    set_target_properties(your_plugin PROPERTIES SUFFIX ".so")
endif()

set_target_properties(your_plugin PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/../../plugins
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/../../plugins
)
```

### Build Process

1. Create your plugin directory in `plugins/your_plugin_name/`
2. Write your plugin implementation
3. Create a CMakeLists.txt file
4. Build with: `mkdir build && cd build && cmake .. && make`
5. The resulting shared library will be placed in the `plugins/` directory

## Example Plugin

See `plugins/example/example_plugin.cpp` for a complete working example that demonstrates:

- Plugin initialization and shutdown
- File format detection based on extension
- Virtual archive creation with mock entries
- Memory management for extracted data

## Plugin Loading

Plugins are automatically loaded from the `plugins/` directory when ResourceDragon starts. The application will:

1. Scan for shared libraries (.so/.dll files)
2. Load each library and check for required exports
3. Initialize plugins that pass validation
4. Register their archive formats with the main application

## Error Handling

- Use the logging functions provided in the HostAPI for debugging
- Return `false` from `RD_PluginInit` if initialization fails
- Ensure proper cleanup in `RD_PluginShutdown`
- Handle null pointers and invalid parameters gracefully

## Memory Management

- Plugins are responsible for managing their own memory
- Data returned by `OpenStream` should be allocated with `new[]` or `malloc`
- The caller will free the memory, so ensure it's compatible
- Clean up all resources in the `Destroy` function

## Platform Considerations

### Windows
- Use `.dll` extension
- Export functions with `__declspec(dllexport)`
- Consider calling conventions (`__cdecl`)

### Linux
- Use `.so` extension
- Compile with `-fPIC`
- Use symbol visibility controls

## Debugging Tips

1. Use the logging functions extensively during development
2. Check that all required symbols are exported correctly
3. Verify memory allocation/deallocation pairs
4. Test with various file sizes and edge cases
5. Use a debugger to step through plugin initialization

## Common Pitfalls

- Forgetting to export required symbols
- Memory leaks in OpenStream or cleanup functions
- Not handling edge cases in CanHandleFile
- Incorrect VTable function signatures
- Platform-specific compilation issues

## Support

For questions about the plugin API or help with implementation, please refer to the ResourceDragon documentation or open an issue on the project repository.
