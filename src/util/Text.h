#pragma once

#include <string>
#ifdef linux
#include "iconv.h"
#endif

class TextConverter {
    public:
        static std::string UTF16ToUTF8(const std::u16string& utf16_str) {
            #ifdef linux
            iconv_t cd = iconv_open("UTF-8", "UTF-16LE");
            if (cd == (iconv_t)-1) {
                return "";
            }

            size_t in_bytes_left = utf16_str.size() * sizeof(char16_t);
            size_t out_bytes_left = in_bytes_left * 2; // Allocate enough space for UTF-8
            char* in_buf = (char*)utf16_str.data();
            char* out_buf = new char[out_bytes_left];
            char* out_ptr = out_buf;

            if (iconv(cd, &in_buf, &in_bytes_left, &out_ptr, &out_bytes_left) == (size_t)-1) {
                delete[] out_buf;
                iconv_close(cd);
                return "";
            }

            std::string result(out_buf, out_ptr - out_buf);
            delete[] out_buf;
            iconv_close(cd);
            return result;
            #endif
            #ifdef _WIN32
            return "";
            #endif
        }
        static std::u16string UTF8ToUTF16(const std::string& utf8_str) {
            #ifdef linux
            iconv_t cd = iconv_open("UTF-16LE", "UTF-8");
            if (cd == (iconv_t)-1) {
                return u"";
            }

            size_t in_bytes_left = utf8_str.size();
            size_t out_bytes_left = in_bytes_left * 2; // Allocate enough space for UTF-16
            char* in_buf = (char*)utf8_str.data();
            char* out_buf = new char[out_bytes_left];
            char* out_ptr = out_buf;

            if (iconv(cd, &in_buf, &in_bytes_left, &out_ptr, &out_bytes_left) == (size_t)-1) {
                delete[] out_buf;
                iconv_close(cd);
                return u"";
            }

            std::u16string result((char16_t*)out_buf, (out_ptr - out_buf) / sizeof(char16_t));
            delete[] out_buf;
            iconv_close(cd);
            return result;
            #endif
            #ifdef _WIN32
            return u"";
            #endif
        }
        static std::string UTF8ToUTF16LE(const std::string& utf8_str) {
            std::u16string utf16_str = UTF8ToUTF16(utf8_str);
            return std::string((char*)utf16_str.data(), utf16_str.size() * sizeof(char16_t));
        }
        static std::string UTF16LEToUTF8(const std::string& utf16le_str) {
            std::u16string utf16_str((char16_t*)utf16le_str.data(), utf16le_str.size() / sizeof(char16_t));
            return UTF16ToUTF8(utf16_str);
        }
};