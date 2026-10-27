#include "utf8.hpp"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace auraui {
namespace core {

bool Utf8::is_valid(std::string_view str) noexcept {
    const unsigned char* bytes = reinterpret_cast<const unsigned char*>(str.data());
    size_t i = 0;
    size_t len = str.size();

    while (i < len) {
        unsigned char c = bytes[i];
        if (c <= 0x7F) {
            i++;
        } else if ((c >= 0xC2) && (c <= 0xDF)) {
            // 2-byte sequence
            if (i + 1 >= len) return false;
            if ((bytes[i + 1] & 0xC0) != 0x80) return false;
            i += 2;
        } else if (c == 0xE0) {
            // 3-byte sequence (exclude overlong)
            if (i + 2 >= len) return false;
            if (bytes[i + 1] < 0xA0 || bytes[i + 1] > 0xBF) return false;
            if ((bytes[i + 2] & 0xC0) != 0x80) return false;
            i += 3;
        } else if (((c >= 0xE1) && (c <= 0xEC)) || c == 0xEE || c == 0xEF) {
            // 3-byte standard
            if (i + 2 >= len) return false;
            if ((bytes[i + 1] & 0xC0) != 0x80) return false;
            if ((bytes[i + 2] & 0xC0) != 0x80) return false;
            i += 3;
        } else if (c == 0xED) {
            // 3-byte (exclude UTF-16 surrogates 0xD800 - 0xDFFF)
            if (i + 2 >= len) return false;
            if (bytes[i + 1] < 0x80 || bytes[i + 1] > 0x9F) return false;
            if ((bytes[i + 2] & 0xC0) != 0x80) return false;
            i += 3;
        } else if (c == 0xF0) {
            // 4-byte (exclude overlong)
            if (i + 3 >= len) return false;
            if (bytes[i + 1] < 0x90 || bytes[i + 1] > 0xBF) return false;
            if ((bytes[i + 2] & 0xC0) != 0x80 || (bytes[i + 3] & 0xC0) != 0x80) return false;
            i += 4;
        } else if ((c >= 0xF1) && (c <= 0xF3)) {
            // 4-byte standard
            if (i + 3 >= len) return false;
            if ((bytes[i + 1] & 0xC0) != 0x80 || (bytes[i + 2] & 0xC0) != 0x80 || (bytes[i + 3] & 0xC0) != 0x80) return false;
            i += 4;
        } else if (c == 0xF4) {
            // 4-byte (max U+10FFFF)
            if (i + 3 >= len) return false;
            if (bytes[i + 1] < 0x80 || bytes[i + 1] > 0x8F) return false;
            if ((bytes[i + 2] & 0xC0) != 0x80 || (bytes[i + 3] & 0xC0) != 0x80) return false;
            i += 4;
        } else {
            return false;
        }
    }
    return true;
}

std::string Utf8::sanitize(std::string_view str) {
    if (is_valid(str)) {
        return std::string(str);
    }
    // Replace invalid sequences with U+FFFD (0xEF 0xBF 0xBD)
    std::string out;
    out.reserve(str.size() + 8);
    for (size_t i = 0; i < str.size(); ) {
        unsigned char c = static_cast<unsigned char>(str[i]);
        if (c <= 0x7F) {
            out.push_back(static_cast<char>(c));
            i++;
        } else {
            // For invalid sequence, append U+FFFD
            out.append("\xEF\xBF\xBD");
            i++;
        }
    }
    return out;
}

#if defined(_WIN32)
std::wstring Utf8::to_wide(std::string_view utf8) {
    if (utf8.empty()) return L"";
    int wlen = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8.data(), static_cast<int>(utf8.size()), NULL, 0);
    if (wlen <= 0) {
        // Fallback without invalid chars flag
        wlen = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), NULL, 0);
        if (wlen <= 0) return L"";
    }
    std::wstring result(wlen, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), &result[0], wlen);
    return result;
}

std::string Utf8::to_utf8(std::wstring_view wide) {
    if (wide.empty()) return "";
    int ulen = WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), NULL, 0, NULL, NULL);
    if (ulen <= 0) return "";
    std::string result(ulen, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), &result[0], ulen, NULL, NULL);
    return result;
}
#endif

} // namespace core
} // namespace auraui
