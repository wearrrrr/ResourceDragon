/*
 * C11 _Generic logging test
 * Compile with: gcc -std=c11 test_c11_logging.c -o test_c11_logging
 */

#include "../../SDK/util/rd_log.h"
#include "../../SDK/util/rd_log_helpers.h"
#include <stdio.h>
#include <stddef.h>

/* Mock implementation of the logging functions for testing */
void rd_log(RD_LogLevel level, const char* msg, size_t len) {
    const char* level_str = level == RD_LOG_INFO ? "INFO" : 
                           level == RD_LOG_WARN ? "WARN" : "ERROR";
    printf("[%s] %.*s\n", level_str, (int)len, msg);
}

void rd_log_fmtv(RD_LogLevel level, const char* fmt_str, const RD_LogArg* args, size_t arg_count) {
    const char* level_str = level == RD_LOG_INFO ? "INFO" : 
                           level == RD_LOG_WARN ? "WARN" : "ERROR";
    
    printf("[%s] ", level_str);
    
    /* Simple {} placeholder replacement */
    size_t arg_idx = 0;
    for (const char* p = fmt_str; *p; p++) {
        if (*p == '{' && *(p + 1) == '}') {
            /* Found a placeholder */
            if (arg_idx < arg_count) {
                switch (args[arg_idx].type) {
                    case RD_LOG_ARG_STRING:
                        printf("%.*s", 
                               (int)args[arg_idx].value.string.size, 
                               args[arg_idx].value.string.data);
                        break;
                    case RD_LOG_ARG_BOOL:
                        printf("%s", args[arg_idx].value.boolean ? "true" : "false");
                        break;
                    case RD_LOG_ARG_S64:
                        printf("%lld", args[arg_idx].value.s64);
                        break;
                    case RD_LOG_ARG_U64:
                        printf("%llu", args[arg_idx].value.u64);
                        break;
                    case RD_LOG_ARG_F64:
                        printf("%g", args[arg_idx].value.f64);
                        break;
                }
                arg_idx++;
            }
            p++; /* Skip the '}' */
        } else if (*p == '{' && *(p + 1) == '{') {
            /* Escaped {{ becomes { */
            printf("{");
            p++;
        } else if (*p == '}' && *(p + 1) == '}') {
            /* Escaped }} becomes } */
            printf("}");
            p++;
        } else {
            /* Regular character */
            printf("%c", *p);
        }
    }
    printf("\n");
}

int main(void) {
    printf("=== C11 _Generic Logging Test ===\n\n");
    
    /* Test 1: Simple messages */
    printf("Test 1: Simple messages\n");
    RD_LOG_INFO("Application started");
    RD_LOG_WARN("Low memory warning");
    RD_LOG_ERROR("Connection failed");
    printf("\n");
    
    /* Test 2: Automatic type conversion with _Generic */
    printf("Test 2: Automatic type conversion\n");
    const char* filename = "test.pak";
    size_t file_size = 2048;
    int entry_count = 100;
    
    RD_LOG_INFO(
        "Opened '{}' ({} bytes, {} entries)",
        RD_LOG_ARG(filename),
        RD_LOG_ARG(file_size),
        RD_LOG_ARG(entry_count)
    );
    printf("\n");
    
    /* Test 3: Different numeric types */
    printf("Test 3: Different numeric types\n");
    char c = 'A';
    short s = -32;
    int i = 42;
    long l = 1234567L;
    long long ll = 9876543210LL;
    unsigned int ui = 255U;
    unsigned long ul = 999999UL;
    unsigned long long ull = 18446744073709551615ULL;
    
    RD_LOG_INFO(
        "Types: char={}, short={}, int={}, long={}, ll={}",
        RD_LOG_ARG(c),
        RD_LOG_ARG(s),
        RD_LOG_ARG(i),
        RD_LOG_ARG(l),
        RD_LOG_ARG(ll)
    );
    
    RD_LOG_INFO(
        "Unsigned: uint={}, ulong={}, ull={}",
        RD_LOG_ARG(ui),
        RD_LOG_ARG(ul),
        RD_LOG_ARG(ull)
    );
    printf("\n");
    
    /* Test 4: Floating point */
    printf("Test 4: Floating point\n");
    float f = 3.14159f;
    double d = 2.718281828;
    
    RD_LOG_INFO(
        "Pi={}, e={}",
        RD_LOG_ARG(f),
        RD_LOG_ARG(d)
    );
    printf("\n");
    
    /* Test 5: Boolean */
    printf("Test 5: Boolean\n");
    _Bool flag_true = 1;
    _Bool flag_false = 0;
    
    RD_LOG_INFO(
        "Flags: success={}, error={}",
        RD_LOG_ARG(flag_true),
        RD_LOG_ARG(flag_false)
    );
    printf("\n");
    
    /* Test 6: Pointers */
    printf("Test 6: Pointers\n");
    void* ptr = &file_size;
    const void* const_ptr = filename;
    
    RD_LOG_INFO(
        "Pointers: ptr={}, const_ptr={}",
        RD_LOG_ARG(ptr),
        RD_LOG_ARG(const_ptr)
    );
    printf("\n");
    
    /* Test 7: Mixed types */
    printf("Test 7: Mixed types in one call\n");
    RD_LOG_INFO(
        "Archive: name={}, size={}, count={}, ratio={}, valid={}",
        RD_LOG_ARG(filename),
        RD_LOG_ARG(file_size),
        RD_LOG_ARG(entry_count),
        RD_LOG_ARG(f),
        RD_LOG_ARG(flag_true)
    );
    printf("\n");
    
    /* Test 8: Compare with manual conversion */
    printf("Test 8: Manual vs Automatic comparison\n");
    printf("Manual:    ");
    RD_LOG_INFO(
        "size={}, count={}",
        rd_log_make_size(file_size),
        rd_log_make_s64(entry_count)
    );
    
    printf("Automatic: ");
    RD_LOG_INFO(
        "size={}, count={}",
        RD_LOG_ARG(file_size),
        RD_LOG_ARG(entry_count)
    );
    printf("\n");
    
    /* Test 9: Using size_t helper */
    printf("Test 9: size_t automatic detection\n");
    size_t bytes_read = 4096;
    size_t bytes_written = 2048;
    ptrdiff_t offset = -512;
    
    RD_LOG_INFO(
        "IO: read={}, written={}, offset={}",
        RD_LOG_ARG(bytes_read),
        RD_LOG_ARG(bytes_written),
        RD_LOG_ARG(offset)
    );
    printf("\n");
    
    printf("=== All tests completed! ===\n");
    return 0;
}
