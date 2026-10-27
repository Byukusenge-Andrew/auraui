#include "mime.hpp"
#include <algorithm>
#include <unordered_map>

namespace auraui {
namespace assets {

namespace {

const std::unordered_map<std::string, std::string> MIME_TABLE = {
    // Text & Web
    {"html", "text/html; charset=utf-8"},
    {"htm", "text/html; charset=utf-8"},
    {"css", "text/css; charset=utf-8"},
    {"js", "application/javascript; charset=utf-8"},
    {"mjs", "application/javascript; charset=utf-8"},
    {"json", "application/json; charset=utf-8"},
    {"map", "application/json; charset=utf-8"},
    {"txt", "text/plain; charset=utf-8"},
    {"xml", "application/xml; charset=utf-8"},
    {"csv", "text/csv; charset=utf-8"},

    // Images
    {"png", "image/png"},
    {"jpg", "image/jpeg"},
    {"jpeg", "image/jpeg"},
    {"gif", "image/gif"},
    {"svg", "image/svg+xml"},
    {"webp", "image/webp"},
    {"ico", "image/x-icon"},
    {"avif", "image/avif"},
    {"bmp", "image/bmp"},

    // Fonts
    {"woff", "font/woff"},
    {"woff2", "font/woff2"},
    {"ttf", "font/ttf"},
    {"otf", "font/otf"},
    {"eot", "application/vnd.ms-fontobject"},

    // Audio & Video
    {"mp3", "audio/mpeg"},
    {"wav", "audio/wav"},
    {"ogg", "audio/ogg"},
    {"m4a", "audio/mp4"},
    {"mp4", "video/mp4"},
    {"webm", "video/webm"},
    {"ogv", "video/ogg"},

    // Binary / WebAssembly
    {"wasm", "application/wasm"},
    {"pdf", "application/pdf"},
    {"bin", "application/octet-stream"}
};

const std::string DEFAULT_MIME = "application/octet-stream";

} // namespace

std::string Mime::from_extension(std::string_view ext) noexcept {
    if (ext.empty()) {
        return DEFAULT_MIME;
    }

    // Strip leading dot if present
    if (ext.front() == '.') {
        ext.remove_prefix(1);
    }

    std::string lower_ext;
    lower_ext.reserve(ext.size());
    for (char c : ext) {
        lower_ext.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }

    auto it = MIME_TABLE.find(lower_ext);
    if (it != MIME_TABLE.end()) {
        return it->second;
    }
    return DEFAULT_MIME;
}

std::string Mime::from_path(std::string_view path) noexcept {
    size_t last_dot = path.rfind('.');
    size_t last_slash = path.find_last_of("/\\");

    if (last_dot == std::string_view::npos || (last_slash != std::string_view::npos && last_dot < last_slash)) {
        return DEFAULT_MIME;
    }

    return from_extension(path.substr(last_dot + 1));
}

} // namespace assets
} // namespace auraui
