#include "../platform.hpp"
#include "core/utf8.hpp"
#include "auraui/log.hpp"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <shobjidl.h>
#include <shlobj.h>
#include <shellapi.h>
#include <combaseapi.h>
#include <thread>
#include <sstream>

namespace auraui {
namespace platform {

namespace {

std::wstring to_wide(std::string_view utf8_str) {
    if (utf8_str.empty()) return L"";
    int req_len = MultiByteToWideChar(CP_UTF8, 0, utf8_str.data(), static_cast<int>(utf8_str.size()), nullptr, 0);
    if (req_len <= 0) return L"";
    std::wstring wide(req_len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8_str.data(), static_cast<int>(utf8_str.size()), &wide[0], req_len);
    return wide;
}

std::string from_wide(const wchar_t* wide_str) {
    if (!wide_str) return "";
    int req_len = WideCharToMultiByte(CP_UTF8, 0, wide_str, -1, nullptr, 0, nullptr, nullptr);
    if (req_len <= 1) return "";
    std::string utf8(req_len - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide_str, -1, &utf8[0], req_len - 1, nullptr, nullptr);
    return utf8;
}

struct ComScope {
    HRESULT hr;
    ComScope() : hr(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE)) {}
    ~ComScope() {
        if (SUCCEEDED(hr)) {
            CoUninitialize();
        }
    }
};

} // namespace

Result<std::vector<std::string>> Platform::show_open_dialog(const FileDialogOptions& options, void* parent_window) {
    ComScope com;
    HWND hwnd = static_cast<HWND>(parent_window);

    IFileOpenDialog* pFileOpen = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));
    if (FAILED(hr) || !pFileOpen) {
        return make_error(ErrorCode::internal, "Failed to initialize Windows FileOpenDialog COM component");
    }

    FILEOPENDIALOGOPTIONS dialog_opts = 0;
    pFileOpen->GetOptions(&dialog_opts);
    if (options.choose_directories) {
        dialog_opts |= FOS_PICKFOLDERS;
    }
    if (options.allow_multiple) {
        dialog_opts |= FOS_ALLOWMULTISELECT;
    }
    dialog_opts |= FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST | FOS_FILEMUSTEXIST;
    pFileOpen->SetOptions(dialog_opts);

    if (!options.title.empty()) {
        std::wstring wtitle = to_wide(options.title);
        pFileOpen->SetTitle(wtitle.c_str());
    }

    // Set filters
    std::vector<COMDLG_FILTERSPEC> specs;
    std::vector<std::wstring> names;
    std::vector<std::wstring> patterns;
    if (!options.filters.empty() && !options.choose_directories) {
        specs.reserve(options.filters.size());
        names.reserve(options.filters.size());
        patterns.reserve(options.filters.size());

        for (const auto& f : options.filters) {
            names.push_back(to_wide(f.name));
            std::wstring pat;
            for (size_t i = 0; i < f.extensions.size(); ++i) {
                if (i > 0) pat += L";";
                pat += L"*." + to_wide(f.extensions[i]);
            }
            patterns.push_back(std::move(pat));
        }

        for (size_t i = 0; i < names.size(); ++i) {
            specs.push_back({names[i].c_str(), patterns[i].c_str()});
        }
        pFileOpen->SetFileTypes(static_cast<UINT>(specs.size()), specs.data());
    }

    hr = pFileOpen->Show(hwnd);
    if (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED)) {
        pFileOpen->Release();
        return std::vector<std::string>{}; // User cancelled
    }

    if (FAILED(hr)) {
        pFileOpen->Release();
        return make_error(ErrorCode::internal, "FileOpenDialog failed with error code: " + std::to_string(hr));
    }

    std::vector<std::string> results;
    if (options.allow_multiple) {
        IShellItemArray* pItems = nullptr;
        if (SUCCEEDED(pFileOpen->GetResults(&pItems)) && pItems) {
            DWORD count = 0;
            pItems->GetCount(&count);
            for (DWORD i = 0; i < count; ++i) {
                IShellItem* pItem = nullptr;
                if (SUCCEEDED(pItems->GetItemAt(i, &pItem)) && pItem) {
                    PWSTR pszFilePath = nullptr;
                    if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath)) && pszFilePath) {
                        results.push_back(from_wide(pszFilePath));
                        CoTaskMemFree(pszFilePath);
                    }
                    pItem->Release();
                }
            }
            pItems->Release();
        }
    } else {
        IShellItem* pItem = nullptr;
        if (SUCCEEDED(pFileOpen->GetResult(&pItem)) && pItem) {
            PWSTR pszFilePath = nullptr;
            if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath)) && pszFilePath) {
                results.push_back(from_wide(pszFilePath));
                CoTaskMemFree(pszFilePath);
            }
            pItem->Release();
        }
    }

    pFileOpen->Release();
    return results;
}

Result<std::string> Platform::show_save_dialog(const FileDialogOptions& options, void* parent_window) {
    ComScope com;
    HWND hwnd = static_cast<HWND>(parent_window);

    IFileSaveDialog* pFileSave = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_FileSaveDialog, nullptr, CLSCTX_ALL, IID_IFileSaveDialog, reinterpret_cast<void**>(&pFileSave));
    if (FAILED(hr) || !pFileSave) {
        return make_error(ErrorCode::internal, "Failed to initialize Windows FileSaveDialog COM component");
    }

    FILEOPENDIALOGOPTIONS dialog_opts = 0;
    pFileSave->GetOptions(&dialog_opts);
    dialog_opts |= FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST | FOS_OVERWRITEPROMPT;
    pFileSave->SetOptions(dialog_opts);

    if (!options.title.empty()) {
        std::wstring wtitle = to_wide(options.title);
        pFileSave->SetTitle(wtitle.c_str());
    }

    hr = pFileSave->Show(hwnd);
    if (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED)) {
        pFileSave->Release();
        return std::string(""); // Cancelled
    }

    if (FAILED(hr)) {
        pFileSave->Release();
        return make_error(ErrorCode::internal, "FileSaveDialog failed with error code: " + std::to_string(hr));
    }

    std::string result_path;
    IShellItem* pItem = nullptr;
    if (SUCCEEDED(pFileSave->GetResult(&pItem)) && pItem) {
        PWSTR pszFilePath = nullptr;
        if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath)) && pszFilePath) {
            result_path = from_wide(pszFilePath);
            CoTaskMemFree(pszFilePath);
        }
        pItem->Release();
    }

    pFileSave->Release();
    return result_path;
}

Result<std::string> Platform::clipboard_read_text() {
    if (!OpenClipboard(nullptr)) {
        return make_error(ErrorCode::internal, "Could not open Windows clipboard");
    }

    HANDLE hData = GetClipboardData(CF_UNICODETEXT);
    if (!hData) {
        CloseClipboard();
        return std::string(""); // Empty or non-text content
    }

    const wchar_t* pText = static_cast<const wchar_t*>(GlobalLock(hData));
    if (!pText) {
        CloseClipboard();
        return make_error(ErrorCode::internal, "Failed to lock clipboard memory handle");
    }

    std::string result = from_wide(pText);
    GlobalUnlock(hData);
    CloseClipboard();

    return result;
}

Result<void> Platform::clipboard_write_text(std::string_view text) {
    if (!OpenClipboard(nullptr)) {
        return make_error(ErrorCode::internal, "Could not open Windows clipboard");
    }

    if (!EmptyClipboard()) {
        CloseClipboard();
        return make_error(ErrorCode::internal, "Failed to empty Windows clipboard");
    }

    std::wstring wtext = to_wide(text);
    size_t byte_count = (wtext.size() + 1) * sizeof(wchar_t);

    HGLOBAL hGlob = GlobalAlloc(GMEM_MOVEABLE, byte_count);
    if (!hGlob) {
        CloseClipboard();
        return make_error(ErrorCode::internal, "Failed to allocate memory for clipboard");
    }

    void* pDest = GlobalLock(hGlob);
    if (!pDest) {
        GlobalFree(hGlob);
        CloseClipboard();
        return make_error(ErrorCode::internal, "Failed to lock global memory for clipboard");
    }

    memcpy(pDest, wtext.c_str(), byte_count);
    GlobalUnlock(hGlob);

    if (!SetClipboardData(CF_UNICODETEXT, hGlob)) {
        GlobalFree(hGlob);
        CloseClipboard();
        return make_error(ErrorCode::internal, "Failed to set Windows clipboard data");
    }

    CloseClipboard();
    return {};
}

Result<void> Platform::shell_open_external(std::string_view uri) {
    std::wstring wuri = to_wide(uri);
    HINSTANCE hInst = ShellExecuteW(nullptr, L"open", wuri.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    if (reinterpret_cast<INT_PTR>(hInst) <= 32) {
        return make_error(ErrorCode::internal, "ShellExecuteW failed to open external URI");
    }
    return {};
}

SystemInfo Platform::get_system_info() {
    SystemInfo info;
    info.os_name = "Windows";

    // Architecture
    SYSTEM_INFO sys_info;
    GetNativeSystemInfo(&sys_info);
    switch (sys_info.wProcessorArchitecture) {
        case PROCESSOR_ARCHITECTURE_AMD64: info.architecture = "x86_64"; break;
        case PROCESSOR_ARCHITECTURE_ARM64: info.architecture = "arm64"; break;
        case PROCESSOR_ARCHITECTURE_INTEL: info.architecture = "x86"; break;
        default: info.architecture = "unknown"; break;
    }

    info.cpu_count = std::thread::hardware_concurrency();

    // Memory
    MEMORYSTATUSEX mem_status;
    mem_status.dwLength = sizeof(mem_status);
    if (GlobalMemoryStatusEx(&mem_status)) {
        info.total_memory_bytes = mem_status.ullTotalPhys;
        info.available_memory_bytes = mem_status.ullAvailPhys;
    }

    // Windows Version
    typedef LONG(WINAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);
    HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
    if (hNtdll) {
        auto fnRtlGetVersion = reinterpret_cast<RtlGetVersionPtr>(GetProcAddress(hNtdll, "RtlGetVersion"));
        if (fnRtlGetVersion) {
            RTL_OSVERSIONINFOW rovi;
            ZeroMemory(&rovi, sizeof(rovi));
            rovi.dwOSVersionInfoSize = sizeof(rovi);
            if (fnRtlGetVersion(&rovi) == 0) {
                std::ostringstream oss;
                oss << rovi.dwMajorVersion << "." << rovi.dwMinorVersion << " (Build " << rovi.dwBuildNumber << ")";
                info.os_version = oss.str();
            }
        }
    }
    if (info.os_version.empty()) {
        info.os_version = "Windows 10/11";
    }

    return info;
}

std::filesystem::path Platform::app_data_dir(const std::string& app_id) {
    PWSTR pszPath = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &pszPath)) && pszPath) {
        std::filesystem::path base = pszPath;
        CoTaskMemFree(pszPath);
        if (!app_id.empty()) {
            return base / app_id;
        }
        return base;
    }
    return std::filesystem::temp_directory_path();
}

std::filesystem::path Platform::cache_dir(const std::string& app_id) {
    PWSTR pszPath = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &pszPath)) && pszPath) {
        std::filesystem::path base = pszPath;
        CoTaskMemFree(pszPath);
        if (!app_id.empty()) {
            return base / app_id / "Cache";
        }
        return base / "Cache";
    }
    return std::filesystem::temp_directory_path();
}

std::filesystem::path Platform::temp_dir() {
    return std::filesystem::temp_directory_path();
}

std::filesystem::path Platform::home_dir() {
    PWSTR pszPath = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Profile, 0, nullptr, &pszPath)) && pszPath) {
        std::filesystem::path base = pszPath;
        CoTaskMemFree(pszPath);
        return base;
    }
    return "C:\\";
}

} // namespace platform
} // namespace auraui

#endif // _WIN32
