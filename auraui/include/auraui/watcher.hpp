#pragma once

#include "webui.hpp"

#include <string>
#include <thread>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <functional>

namespace auraui {

class HotReloader {
private:
    std::atomic<bool> m_running{false};
    std::thread m_thread;

    static std::filesystem::file_time_type get_max_mtime(const std::string& dir) {
        namespace fs = std::filesystem;
        fs::file_time_type max_time = (fs::file_time_type::min)();
        try {
            for (const auto& entry : fs::recursive_directory_iterator(dir)) {
                if (entry.is_regular_file()) {
                    auto t = entry.last_write_time();
                    if (t > max_time) max_time = t;
                }
            }
        } catch (...) {}
        return max_time;
    }

public:
    HotReloader() = default;

    ~HotReloader() {
        stop();
    }

    // Non-copyable
    HotReloader(const HotReloader&) = delete;
    HotReloader& operator=(const HotReloader&) = delete;

    void start(const std::string& watch_folder, std::function<void()> on_change) {
        namespace fs = std::filesystem;
        if (!fs::exists(watch_folder) || !fs::is_directory(watch_folder)) return;
        if (m_running) return;

        m_running = true;
        m_thread = std::thread([this, watch_folder, on_change]() {
            auto last_time = get_max_mtime(watch_folder);
            std::cout << "[AuraUI Dev] Hot-reload active on '" << watch_folder << "/'\n";

            while (m_running) {
                std::this_thread::sleep_for(std::chrono::milliseconds(300));
                if (!m_running) break;

                auto current_time = get_max_mtime(watch_folder);
                if (current_time > last_time) {
                    last_time = current_time;
                    std::cout << "[AuraUI Dev] File change detected! Refreshing window...\n";
                    if (on_change) {
                        try { on_change(); } catch (...) {}
                    }
                }
            }
        });
    }

    void stop() {
        if (m_running) {
            m_running = false;
            if (m_thread.joinable()) {
                m_thread.join();
            }
        }
    }

    bool is_running() const {
        return m_running.load();
    }
};

} // namespace auraui
