#pragma once

#include <string>
#include <string_view>
#include <vector>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace auraui {
namespace unicode {

#if defined(_WIN32)
inline std::wstring utf8_to_wide(std::string_view utf8) {
    if (utf8.empty()) return L"";
    int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), NULL, 0);
    if (wlen <= 0) return L"";
    std::wstring result(wlen, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), &result[0], wlen);
    return result;
}

inline std::string wide_to_utf8(std::wstring_view wide) {
    if (wide.empty()) return "";
    int ulen = WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), NULL, 0, NULL, NULL);
    if (ulen <= 0) return "";
    std::string result(ulen, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), &result[0], ulen, NULL, NULL);
    return result;
}

inline std::vector<wchar_t> build_wide_filter(std::string_view filter) {
    std::vector<wchar_t> wfilter;
    if (filter.empty()) {
        const wchar_t def[] = L"All Files (*.*)\0*.*\0\0";
        wfilter.assign(def, def + (sizeof(def) / sizeof(wchar_t)));
    } else {
        std::wstring wstr = utf8_to_wide(filter);
        for (wchar_t& wc : wstr) {
            if (wc == L'|') wc = L'\0';
        }
        wfilter.assign(wstr.begin(), wstr.end());
        wfilter.push_back(L'\0');
        wfilter.push_back(L'\0');
    }
    return wfilter;
}
#endif

inline std::string trim_slashes(const std::string& str) {
    size_t start = 0;
    while (start < str.size() && str[start] == '/') ++start;
    size_t end = str.size();
    while (end > start && str[end - 1] == '/') --end;
    return str.substr(start, end - start);
}

} // namespace unicode
} // namespace auraui
