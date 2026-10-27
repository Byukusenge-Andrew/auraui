#pragma once

#include <string>
#include <string_view>
#include <iostream>
#include <mutex>

namespace auraui {

enum class LogLevel {
    trace,
    debug,
    info,
    warn,
    error,
    fatal
};

class Logger {
private:
    static inline LogLevel s_level{LogLevel::info};
    static inline std::mutex s_mutex;

public:
    static void set_level(LogLevel level) noexcept {
        s_level = level;
    }

    [[nodiscard]] static LogLevel get_level() noexcept {
        return s_level;
    }

    static void log(LogLevel level, std::string_view component, std::string_view message) {
        if (level < s_level) return;

        const char* level_str = "INFO";
        const char* color_code = "\033[32m"; // green
        switch (level) {
            case LogLevel::trace: level_str = "TRACE"; color_code = "\033[90m"; break;
            case LogLevel::debug: level_str = "DEBUG"; color_code = "\033[36m"; break;
            case LogLevel::info:  level_str = "INFO "; color_code = "\033[32m"; break;
            case LogLevel::warn:  level_str = "WARN "; color_code = "\033[33m"; break;
            case LogLevel::error: level_str = "ERROR"; color_code = "\033[31m"; break;
            case LogLevel::fatal: level_str = "FATAL"; color_code = "\033[35m"; break;
        }

        std::lock_guard<std::mutex> lock(s_mutex);
        std::cout << color_code << "[" << level_str << "]\033[0m [" 
                  << component << "] " 
                  << message << "\n";
    }

    static void info(std::string_view component, std::string_view message) {
        log(LogLevel::info, component, message);
    }

    static void warn(std::string_view component, std::string_view message) {
        log(LogLevel::warn, component, message);
    }

    static void error(std::string_view component, std::string_view message) {
        log(LogLevel::error, component, message);
    }

    static void debug(std::string_view component, std::string_view message) {
        log(LogLevel::debug, component, message);
    }
};

} // namespace auraui
