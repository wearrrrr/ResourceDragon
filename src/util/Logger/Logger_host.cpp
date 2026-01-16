#include "../../../SDK/util/rd_log_schema.h"
#include <mutex>
#include <cstring>
#include <string>
#include <cstdint>
#include <cctype>
#include <iterator>
#include <fmt/format.h>

#define LOG_COLOR "\x1b[1;34m"
#define WARN_COLOR "\x1b[1;33m"
#define ERROR_COLOR "\x1b[1;31m"
#define RESET  "\x1B[0;1m"

#define PREFIX "[ResourceDragon] "
#define LOG_PREFIX   LOG_COLOR PREFIX RESET
#define WARN_PREFIX  WARN_COLOR PREFIX RESET
#define ERROR_PREFIX ERROR_COLOR PREFIX RESET

namespace {
    std::mutex g_log_mutex;

    void write_with_prefix(RD_LogLevel level, std::string_view msg) {
        const char* prefix;
        switch (level) {
            case RD_LOG_LVL_INFO:  prefix = LOG_PREFIX; break;
            case RD_LOG_LVL_WARN:  prefix = WARN_PREFIX; break;
            case RD_LOG_LVL_ERROR: prefix = ERROR_PREFIX; break;
            default: prefix = PREFIX; break;
        }

        printf("%s", prefix);
        fwrite(msg.data(), 1, msg.size(), stdout);
        puts(RESET);
    }
}

extern "C" void rd_log(RD_LogLevel level, const char* msg, size_t len) {
    if (!msg) return;
    std::string_view sv(msg, len ? len : strlen(msg));
    write_with_prefix(level, sv);
}

extern "C" void rd_log_fmtv(RD_LogLevel level, const char* fmt_str, const RD_LogArg* args, size_t arg_count) {
    if (!fmt_str) return;

    size_t fmt_len = std::strlen(fmt_str);
    std::string formatted;
    formatted.reserve(fmt_len + 16);

    auto append_arg = [&](const RD_LogArg& arg) {
        switch (arg.type) {
            case RD_LOG_ARG_STRING: {
                const char* data = arg.value.string.data;
                size_t size = arg.value.string.size;
                if (data && size) {
                    formatted.append(data, size);
                } else if (data) {
                    formatted.append(data);
                }
                break;
            }
            case RD_LOG_ARG_BOOL:
                formatted += (arg.value.boolean != 0) ? "true" : "false";
                break;
            case RD_LOG_ARG_S64:
                formatted += std::to_string(arg.value.s64);
                break;
            case RD_LOG_ARG_U64:
                formatted += std::to_string(arg.value.u64);
                break;
            case RD_LOG_ARG_F64:
                formatted += std::to_string(arg.value.f64);
                break;
            default:
                formatted += "<invalid arg>";
                break;
        }
    };

    size_t arg_index = 0;
    for (size_t i = 0; i < fmt_len; ++i) {
        char ch = fmt_str[i];
        if (ch == '{') {
            if (i + 1 < fmt_len && fmt_str[i + 1] == '{') {
                formatted.push_back('{');
                ++i;
                continue;
            }

            size_t close = i + 1;
            bool found = false;
            while (close < fmt_len) {
                if (fmt_str[close] == '}') {
                    found = true;
                    break;
                }
                if (fmt_str[close] == '{') {
                    break;
                }
                ++close;
            }

            if (!found) {
                formatted.push_back('{');
                continue;
            }

            if (close == i + 1) {
                if (arg_index < arg_count) {
                    append_arg(args[arg_index++]);
                } else {
                    formatted += "<missing arg>";
                }
                i = close;
            } else {
                formatted.append(fmt_str + i, close - i + 1);
                i = close;
            }
        } else if (ch == '}') {
            if (i + 1 < fmt_len && fmt_str[i + 1] == '}') {
                formatted.push_back('}');
                ++i;
            } else {
                formatted.push_back('}');
            }
        } else {
            formatted.push_back(ch);
        }
    }

    std::string_view result(formatted);
    write_with_prefix(level, result);
}

extern "C" void rd_log_schema(
    RD_LogLevel level,
    const char* schema_name,
    const RD_LogField* fields,
    size_t field_count
) {
    const char* safe_schema = schema_name ? schema_name : "<null>";
    fmt::memory_buffer buffer;
    fmt::format_to(std::back_inserter(buffer), "[schema {}] ", safe_schema);

    auto append_hex = [&](const uint8_t* bytes, size_t size) {
        fmt::format_to(std::back_inserter(buffer), "0x");
        for (size_t i = 0; i < size; ++i) {
            fmt::format_to(std::back_inserter(buffer), "{:02x}", bytes[i]);
        }
    };

    auto append_value = [&](const void* data, size_t size) {
        if (!data || size == 0) {
            fmt::format_to(std::back_inserter(buffer), "<empty>");
            return;
        }
        const uint8_t* bytes = (uint8_t*)data;
        bool printable = true;
        for (size_t i = 0; i < size; ++i) {
            unsigned char ch = static_cast<unsigned char>(bytes[i]);
            if (!isprint(ch)) {
                printable = false;
                break;
            }
        }
        if (printable) {
            std::string text((char*)bytes, size);
            fmt::format_to(std::back_inserter(buffer), "\"{}\"", text);
        } else {
            append_hex(bytes, size);
        }
    };

    if (!fields || field_count == 0) {
        fmt::format_to(std::back_inserter(buffer), "<no fields>");
    } else {
        for (size_t i = 0; i < field_count; ++i) {
            if (i > 0) {
                fmt::format_to(std::back_inserter(buffer), ", ");
            }
            const RD_LogField &field = fields[i];
            const char* name = field.name ? field.name : "<unnamed>";
            fmt::format_to(std::back_inserter(buffer), "{}=", name);
            append_value(field.data, field.size);
        }
    }

    std::string message(buffer.data(), buffer.size());
    std::string_view msg_view(message);
    write_with_prefix(level, msg_view);
}
