#pragma once

#include "unicode.hpp"
#include "nlohmann/json.hpp"

#include <string>
#include <string_view>
#include <vector>
#include <filesystem>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <commdlg.h>
#include <psapi.h>
#include <shellapi.h>
#include <shlobj.h>
#endif

namespace auraui {
namespace system {

using json = nlohmann::json;

#if defined(_WIN32)
// Forwards from unicode module for backward compatibility
using unicode::utf8_to_wide;
using unicode::wide_to_utf8;
using unicode::build_wide_filter;
#endif

inline json get_memory_info() {
#if defined(_WIN32)
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memInfo)) {
        double total_gb = (double)memInfo.ullTotalPhys / (1024.0 * 1024.0 * 1024.0);
        double avail_gb = (double)memInfo.ullAvailPhys / (1024.0 * 1024.0 * 1024.0);
        double used_gb  = total_gb - avail_gb;
        int load_pct    = (int)memInfo.dwMemoryLoad;

        return json{
            {"total_gb", total_gb},
            {"used_gb", used_gb},
            {"avail_gb", avail_gb},
            {"percent_used", load_pct}
        };
    }
#endif
    return json{{"error", "Not supported on this platform"}};
}

inline json get_system_overview() {
#if defined(_WIN32)
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);

    char computerName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD size = sizeof(computerName);
    GetComputerNameA(computerName, &size);

    return json{
        {"os", "Windows"},
        {"computer_name", std::string(computerName)},
        {"cpu_cores", (int)sysInfo.dwNumberOfProcessors},
        {"arch", (sysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64) ? "x64" : "x86"}
    };
#else
    return json{{"os", "Other"}};
#endif
}

inline std::string open_file_dialog(std::string_view filter = {}, std::string_view title = {}) {
#if defined(_WIN32)
    wchar_t filename[32768] = L"";
    OPENFILENAMEW ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;

    std::vector<wchar_t> filter_buf = unicode::build_wide_filter(filter);
    ofn.lpstrFilter = filter_buf.data();
    ofn.lpstrFile = filename;
    ofn.nMaxFile = static_cast<DWORD>(sizeof(filename) / sizeof(wchar_t));
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

    std::wstring wtitle = unicode::utf8_to_wide(title);
    if (!wtitle.empty()) {
        ofn.lpstrTitle = wtitle.c_str();
    }

    if (GetOpenFileNameW(&ofn)) {
        return unicode::wide_to_utf8(filename);
    }
#endif
    return "";
}

inline std::string save_file_dialog(std::string_view default_name = "output.txt", std::string_view filter = {}, std::string_view title = {}) {
#if defined(_WIN32)
    wchar_t filename[32768] = L"";
    std::wstring wdef = unicode::utf8_to_wide(default_name);
    if (!wdef.empty() && wdef.size() < 32768) {
        wcsncpy_s(filename, wdef.c_str(), sizeof(filename) / sizeof(wchar_t) - 1);
    }
    OPENFILENAMEW ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;

    std::vector<wchar_t> filter_buf = unicode::build_wide_filter(filter.empty() ? "All Files (*.*)\0*.*\0" : filter);
    ofn.lpstrFilter = filter_buf.data();
    ofn.lpstrFile = filename;
    ofn.nMaxFile = static_cast<DWORD>(sizeof(filename) / sizeof(wchar_t));
    ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

    std::wstring wtitle = unicode::utf8_to_wide(title);
    if (!wtitle.empty()) {
        ofn.lpstrTitle = wtitle.c_str();
    }

    if (GetSaveFileNameW(&ofn)) {
        return unicode::wide_to_utf8(filename);
    }
#endif
    return "";
}

inline std::string open_folder_dialog(std::string_view title = "Select Folder") {
#if defined(_WIN32)
    wchar_t path[32768] = L"";
    std::wstring wtitle = unicode::utf8_to_wide(title);
    BROWSEINFOW bi = { 0 };
    bi.lpszTitle = wtitle.c_str();
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
    if (pidl != 0) {
        if (SHGetPathFromIDListW(pidl, path)) {
            CoTaskMemFree(pidl);
            return unicode::wide_to_utf8(path);
        }
        CoTaskMemFree(pidl);
    }
#endif
    return "";
}

inline bool set_clipboard_text(const std::string& text) {
#if defined(_WIN32)
    if (!OpenClipboard(NULL)) return false;
    EmptyClipboard();

    int wlen = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, NULL, 0);
    if (wlen <= 0) {
        CloseClipboard();
        return false;
    }

    HGLOBAL hGlob = GlobalAlloc(GMEM_MOVEABLE, wlen * sizeof(wchar_t));
    if (!hGlob) { CloseClipboard(); return false; }

    wchar_t* pMem = (wchar_t*)GlobalLock(hGlob);
    if (!pMem) { GlobalFree(hGlob); CloseClipboard(); return false; }

    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, pMem, wlen);
    GlobalUnlock(hGlob);

    SetClipboardData(CF_UNICODETEXT, hGlob);
    CloseClipboard();
    return true;
#else
    return false;
#endif
}

inline std::string get_clipboard_text() {
#if defined(_WIN32)
    if (!OpenClipboard(NULL)) return "";
    HANDLE hData = GetClipboardData(CF_UNICODETEXT);
    if (!hData) {
        hData = GetClipboardData(CF_TEXT);
        if (!hData) { CloseClipboard(); return ""; }
        char* pszText = (char*)GlobalLock(hData);
        if (!pszText) { CloseClipboard(); return ""; }
        std::string result(pszText);
        GlobalUnlock(hData);
        CloseClipboard();
        return result;
    }

    wchar_t* pszWText = (wchar_t*)GlobalLock(hData);
    if (!pszWText) { CloseClipboard(); return ""; }

    int utf8_len = WideCharToMultiByte(CP_UTF8, 0, pszWText, -1, NULL, 0, NULL, NULL);
    std::string result;
    if (utf8_len > 0) {
        result.resize(utf8_len - 1);
        WideCharToMultiByte(CP_UTF8, 0, pszWText, -1, &result[0], utf8_len, NULL, NULL);
    }

    GlobalUnlock(hData);
    CloseClipboard();
    return result;
#else
    return "";
#endif
}

// Security: Prevent Arbitrary Command Execution via ShellExecuteW allowlist
inline bool open_external(const std::string& target) {
#if defined(_WIN32)
    if (target.empty() || target.size() > 2048) return false;

    // Reject control characters or newlines
    for (char c : target) {
        if (static_cast<unsigned char>(c) < 32) return false;
    }

    // 1. Strict allowlist for web & mail protocols
    if (target.rfind("https://", 0) == 0 || 
        target.rfind("http://", 0) == 0 || 
        target.rfind("mailto:", 0) == 0) 
    {
        std::wstring wtarget = unicode::utf8_to_wide(target);
        HINSTANCE res = ShellExecuteW(NULL, L"open", wtarget.c_str(), NULL, NULL, SW_SHOWNORMAL);
        return ((INT_PTR)res > 32);
    }

    // 2. Reject remote network UNC paths (\\attacker.com\share)
    if (target.rfind("\\\\", 0) == 0 || target.rfind("//", 0) == 0) {
        return false;
    }

    // 3. For local filesystem targets: allow ONLY canonical existing directories opened in Explorer
    namespace fs = std::filesystem;
    std::error_code ec;
    fs::path p = fs::canonical(fs::u8path(target), ec);
    if (!ec && fs::is_directory(p, ec)) {
        HINSTANCE res = ShellExecuteW(NULL, L"explore", p.c_str(), NULL, NULL, SW_SHOWNORMAL);
        return ((INT_PTR)res > 32);
    }

    // All local executable/document files rejected to prevent RCE
    return false;
#else
    return false;
#endif
}

} // namespace system
} // namespace auraui
