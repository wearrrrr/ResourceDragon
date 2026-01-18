# Logging API Examples

This document demonstrates the improved logging API with all the new features.

## C API Examples

### Basic Logging

```c
#include "rd_log.h"
#include "rd_log_helpers.h"

void basic_c_logging() {
    // Simple string logging
    RD_LOG_INFO("Server initialized");
    RD_LOG_WARN("Low memory");
    RD_LOG_ERROR("Connection failed");
}
```

### Formatted Logging with Arguments

```c
void formatted_c_logging() {
    const char* filename = "data.bin";
    size_t file_size = 1024;
    int entry_count = 42;
    
    // Traditional approach - explicit type conversion
    RD_LOG_INFO(
        "Opened '{}' ({} bytes, {} entries)",
        rd_log_make_cstring(filename),
        rd_log_make_size(file_size),
        rd_log_make_s64(entry_count)
    );
    
    // C11 approach - automatic type conversion with RD_LOG_ARG!
    #if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
    RD_LOG_INFO(
        "Opened '{}' ({} bytes, {} entries)",
        RD_LOG_ARG(filename),
        RD_LOG_ARG(file_size),
        RD_LOG_ARG(entry_count)
    );
    #endif
    
    // Pointer logging
    void* buffer = malloc(256);
    RD_LOG_INFO(
        "Allocated buffer at {}",
        rd_log_make_ptr(buffer)
    );
    
    // C11: Automatic!
    #if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
    RD_LOG_INFO("Allocated buffer at {}", RD_LOG_ARG(buffer));
    #endif
    free(buffer);
    
    // All numeric types
    RD_LOG_INFO(
        "Stats: u64={}, s64={}, f64={}, bool={}",
        rd_log_make_u64(12345ULL),
        rd_log_make_s64(-9876LL),
        rd_log_make_f64(3.14159),
        rd_log_make_bool(1)
    );
    
    // C11: Automatic type deduction!
    #if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
    unsigned long long big = 12345ULL;
    long long small = -9876LL;
    double pi = 3.14159;
    _Bool flag = 1;
    RD_LOG_INFO(
        "Stats: u64={}, s64={}, f64={}, bool={}",
        RD_LOG_ARG(big),
        RD_LOG_ARG(small),
        RD_LOG_ARG(pi),
        RD_LOG_ARG(flag)
    );
    #endif
}
```

### C11 _Generic Examples

```c
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L

void c11_logging_examples() {
    // RD_LOG_ARG automatically selects the right conversion function
    const char* name = "test.pak";
    size_t size = 2048;
    int count = 100;
    unsigned int flags = 0xFF;
    float ratio = 0.75f;
    double precise = 3.141592653589793;
    void* ptr = &size;
    
    // All types are automatically converted!
    RD_LOG_INFO(
        "Archive: name={}, size={}, count={}, flags={}, ratio={}, pi={}, ptr={}",
        RD_LOG_ARG(name),
        RD_LOG_ARG(size),
        RD_LOG_ARG(count),
        RD_LOG_ARG(flags),
        RD_LOG_ARG(ratio),
        RD_LOG_ARG(precise),
        RD_LOG_ARG(ptr)
    );
    
    // Mixed with explicit conversions (for custom types)
    struct { int x; int y; } point = {10, 20};
    RD_LOG_INFO(
        "Point: x={}, y={}, ptr={}",
        RD_LOG_ARG(point.x),
        RD_LOG_ARG(point.y),
        rd_log_make_ptr(&point)  // Explicit for struct pointer
    );
    
    // Works seamlessly with zero arguments too
    RD_LOG_INFO("Simple message");
    RD_LOG_WARN("Warning!");
    RD_LOG_ERROR("Error occurred");
}

#endif
```
```

### Structured Logging (New!)

```c
#include "rd_log_schema.h"

void structured_c_logging() {
    const char* user = "alice";
    int user_id = 1001;
    size_t bytes_sent = 4096;
    
    // Manual approach
    RD_LogField fields[] = {
        RD_LOG_FIELD_CSTR("user", user),
        RD_LOG_FIELD_INT("user_id", user_id),
        RD_LOG_FIELD_DATA("bytes", &bytes_sent, sizeof(bytes_sent))
    };
    rd_log_schema(RD_LOG_INFO, "user_activity", fields, 3);
    
    // Macro approach (automatic counting!)
    RD_LOG_SCHEMA(
        RD_LOG_INFO,
        "user_activity",
        RD_LOG_FIELD_CSTR("user", user),
        RD_LOG_FIELD_INT("user_id", user_id),
        RD_LOG_FIELD_DATA("bytes", &bytes_sent, sizeof(bytes_sent))
    );
}
```

## C++ API Examples

### Basic Logging

```cpp
#include "Logger.hpp"

void basic_cpp_logging() {
    // Simple const char* (optimized path, no strlen call)
    Logger::log("Application started");
    
    // std::string_view
    std::string_view sv = "Processing...";
    Logger::log(sv);
    
    // std::string
    std::string msg = "Task complete";
    Logger::log(msg);
}
```

### Automatic Type Conversion

```cpp
void automatic_cpp_logging() {
    std::string filename = "archive.pak";
    size_t file_size = 2048;
    int entries = 100;
    
    // No manual conversion needed! Types are deduced automatically
    Logger::log("Opened '{}' ({} bytes, {} entries)", 
                filename, file_size, entries);
    
    // Works with all numeric types
    uint64_t big_num = 0xDEADBEEF;
    double ratio = 0.75;
    bool success = true;
    
    Logger::warn("Stats: num={}, ratio={}, ok={}", 
                 big_num, ratio, success);
    
    // Pointers
    void* ptr = this;
    Logger::log("Object at {}", ptr);
    
    // Vectors are automatically stringified
    std::vector<int> data = {1, 2, 3, 4, 5};
    Logger::log("Data: {}", data);  // Output: "Data: { 1, 2, 3, 4, 5 }"
}
```

### Mixed Types

```cpp
void mixed_types() {
    std::string name = "config.json";
    const char* status = "valid";
    size_t size = 512;
    float version = 1.5f;
    
    // Mix and match any types
    Logger::log("Loaded {} (status: {}, size: {}, version: {})",
                name, status, size, version);
}
```

### Compile-Time Validation (C++20)

```cpp
void validated_logging() {
    // ✓ Valid - braces match
    Logger::log("Value: {}", 42);
    Logger::log("Escaped {{ braces }} and value: {}", 123);
    
    // ✗ Compile error in C++20: unmatched braces
    // Logger::log("Missing close: {", 42);
    // Logger::log("Extra close: }", 42);
    // Logger::log("Nested: {{}", 42);
}
```

## Rust FFI Example

```rust
use std::ffi::CString;

#[repr(C)]
struct RD_LogArg {
    arg_type: u32,
    value: RD_LogArgValue,
}

#[repr(C)]
union RD_LogArgValue {
    string: RD_LogStringArg,
    boolean: u8,
    s64: i64,
    u64: u64,
    f64: f64,
}

#[repr(C)]
struct RD_LogStringArg {
    data: *const u8,
    size: usize,
}

extern "C" {
    fn rd_log_fmtv(level: u32, fmt: *const u8, args: *const RD_LogArg, count: usize);
}

fn log_from_rust() {
    let filename = CString::new("test.dat").unwrap();
    let size: u64 = 1024;
    
    unsafe {
        let args = [
            // String argument
            RD_LogArg {
                arg_type: 0, // RD_LOG_ARG_STRING
                value: RD_LogArgValue {
                    string: RD_LogStringArg {
                        data: filename.as_ptr() as *const u8,
                        size: filename.as_bytes().len(),
                    }
                }
            },
            // u64 argument
            RD_LogArg {
                arg_type: 3, // RD_LOG_ARG_U64
                value: RD_LogArgValue { u64: size }
            }
        ];
        
        let fmt = CString::new("Rust opened '{}' ({} bytes)").unwrap();
        rd_log_fmtv(0, fmt.as_ptr() as *const u8, args.as_ptr(), 2);
    }
}
```

## Key Improvements

1. **No more F0 variants** - All macros now handle zero arguments
2. **New type helpers** - `rd_log_make_size()`, `rd_log_make_ptr()`, `rd_log_make_ssize()`
3. **C11 _Generic support** - Automatic type conversion with `RD_LOG_ARG()` and `RD_LOG_INFO/WARN/ERROR()`
4. **Structured logging** - Easy-to-use macros for structured data
5. **Optimized C++** - Reduced allocations, const char* fast path
6. **C++20 validation** - Compile-time format string checking
7. **100% C ABI compatible** - All improvements work with pure C or any FFI
## Migration Guide

### C89/C99 Code (Traditional)
```c
RD_LOG_INFO("No args");
RD_LOG_INFO("With args: {}", rd_log_make_u64((unsigned long long)size));
```
### C11 Code (Modern)
```c
RD_LOG_INFO("No args");
RD_LOG_INFO("With args: {}", RD_LOG_ARG(size));
```

### C++
```cpp
Logger::log("message");  // Direct const char* overload
```

## Comparison Table

| Feature | C89/C99 | C11 | C++ |
|---------|---------|-----|-----|
| Type conversion | Manual (`rd_log_make_*`) | Automatic (`RD_LOG_ARG`) | Automatic (templates) |
| Zero-arg handling | Need `*F0` variants | Single macro | Single function |
| Compile-time checks | None | Type safety via `_Generic` | Format validation (C++20) |
| Custom types | Via manual helpers | Via manual helpers | Automatic stringification |
| Macro names | `RD_LOG_INFOF` | `RD_LOG_INFO` | `Logger::log` | `Logger::log("message");  // Direct const char* overload
```
