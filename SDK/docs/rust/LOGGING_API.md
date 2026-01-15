# Rust Logging API

The Rust FFI bindings for ResourceDragon's logging system support both simple string logging and formatted logging with type-safe argument conversion.

## Simple String Logging

```rust
use crate::api::HOST_API;

// Log an info message
HOST_API.log("Application started");

// Log a warning
HOST_API.warn("Low memory");

// Log an error
HOST_API.error("Connection failed");
```

## Formatted Logging

### Method 1: Direct API (Recommended)

```rust
use crate::api::{HOST_API, RdLogArg};

HOST_API.log_fmt(
    "Opened '{}' with {} entries ({} bytes)",
    &[
        RdLogArg::string("test.pak"),
        RdLogArg::usize(100),
        RdLogArg::u64(2048),
    ],
);

HOST_API.warn_fmt(
    "Memory usage: {} MB / {} MB",
    &[
        RdLogArg::u64(512),
        RdLogArg::u64(1024),
    ],
);

HOST_API.error_fmt(
    "Failed to read file: {}",
    &[RdLogArg::string(error_msg)],
);
```

### Method 2: Using the Macro (Alternative)

```rust
use crate::{rd_log, api::RdLogArg as Arg};

rd_log!(
    info,
    "Opened '{}' with {} entries",
    Arg::string("test.pak"),
    Arg::usize(100)
);

rd_log!(warn, "Low memory");
rd_log!(error, "Connection failed");
```

## RdLogArg Type Conversions

The `RdLogArg` type provides constructors for all supported types:

### Strings
```rust
RdLogArg::string("hello")           // &str
RdLogArg::string(&my_string)        // &String
```

### Integers (Signed)
```rust
RdLogArg::i64(value)                // i64
RdLogArg::isize(value)              // isize
// Also automatically converts from i8, i16, i32
```

### Integers (Unsigned)
```rust
RdLogArg::u64(value)                // u64
RdLogArg::usize(value)              // usize
// Also automatically converts from u8, u16, u32
```

### Floats
```rust
RdLogArg::f64(3.14159)              // f64
// Also automatically converts from f32
```

### Booleans
```rust
RdLogArg::bool(true)                // bool
RdLogArg::bool(success)
```

## Complete Example

```rust
impl<'a: 'static> RdFormat<'a> for MyArchive {
    fn try_open(buffer: &[u8], file_name: &CStr) -> Option<Self> {
        // ... parsing logic ...
        
        let fname = file_name.to_str().unwrap_or("<invalid>");
        let entry_count = entries.len();
        let total_size = buffer.len();
        let is_compressed = check_compression(&buffer);
        let compression_ratio = 0.75;
        
        // Old style: simple message
        HOST_API.log("Archive opened successfully");
        
        // New style: formatted with multiple types
        HOST_API.log_fmt(
            "Opened '{}': {} entries, {} bytes, compressed={}, ratio={}",
            &[
                RdLogArg::string(fname),
                RdLogArg::usize(entry_count),
                RdLogArg::usize(total_size),
                RdLogArg::bool(is_compressed),
                RdLogArg::f64(compression_ratio),
            ],
        );
        
        Some(MyArchive { /* ... */ })
    }
}
```

## Output Format

The formatted logging uses `{}` as placeholders, similar to Rust's `format!` macro:

```rust
HOST_API.log_fmt("Value: {}", &[RdLogArg::i64(42)]);
// Output: [INFO] Value: 42

HOST_API.warn_fmt("File '{}' size: {} bytes", &[
    RdLogArg::string("test.dat"),
    RdLogArg::usize(1024),
]);
// Output: [WARN] File 'test.dat' size: 1024 bytes
```

## API Compatibility

The Rust bindings use the same C ABI as the C and C++ logging systems:

- **C**: `rd_log_fmtv(RD_LogLevel, const char*, const RD_LogArg*, size_t)`
- **C++**: `Logger::log(const char*, Args&&...)`
- **Rust**: `HOST_API.log_fmt(&str, &[RdLogArg])`

All three produce identical output when given the same format string and arguments.
