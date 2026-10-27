#include "auraui.hpp"
#include <chrono>
#include <thread>
#include <vector>
#include <future>
#include <numeric>

#if __has_include("vfs.h")
#include "vfs.h"
#define HAS_EMBEDDED_VFS 1
#endif

// High performance C++ calculation: Prime counting benchmark
static bool is_prime(int n) {
    if (n <= 1) return false;
    if (n <= 3) return true;
    if (n % 2 == 0 || n % 3 == 0) return false;
    for (int i = 5; i * i <= n; i += 6) {
        if (n % i == 0 || n % (i + 2) == 0) return false;
    }
    return true;
}

static int count_primes_range(int start, int end) {
    int count = 0;
    for (int i = start; i <= end; ++i) {
        if (is_prime(i)) count++;
    }
    return count;
}

int main() {
    // 1. Initialize modern AuraUI window (1080 x 740)
    auraui::App app("AuraUI - Modern C++ Desktop Showcase", 1080, 740);
    app.set_min_size(880, 600);
    app.set_center();

    // 2. Bind C++ Benchmark
    app.bind("run_benchmark", [](const auraui::Context& ctx) {
        int limit = ctx.get<int>("limit", 2000000);
        int threads_count = ctx.get<int>("threads", 4);
        if (threads_count < 1) threads_count = 1;

        auto t0 = std::chrono::high_resolution_clock::now();

        std::vector<std::future<int>> futures;
        int chunk_size = limit / threads_count;

        for (int t = 0; t < threads_count; ++t) {
            int start = t * chunk_size + 1;
            int end = (t == threads_count - 1) ? limit : (t + 1) * chunk_size;
            futures.push_back(std::async(std::launch::async, count_primes_range, start, end));
        }

        int total_primes = 0;
        for (auto& f : futures) {
            total_primes += f.get();
        }

        auto t1 = std::chrono::high_resolution_clock::now();
        double elapsed_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

        return auraui::json{
            {"limit", limit},
            {"threads", threads_count},
            {"primes_found", total_primes},
            {"elapsed_ms", elapsed_ms}
        };
    });

    // 3. Bind C++ System Stats
    app.bind("get_system_telemetry", [](const auraui::Context&) {
        return auraui::json{
            {"overview", auraui::system::get_system_overview()},
            {"memory", auraui::system::get_memory_info()}
        };
    });

    // 4. Bind C++ Text processing / cryptographic hashing simulation
    app.bind("process_text", [](const auraui::Context& ctx) {
        std::string input = ctx.get<std::string>("text", "");
        
        // Count words, chars, uppercase
        int words = 0;
        std::stringstream ss(input);
        std::string word;
        while (ss >> word) words++;

        std::string reversed = input;
        std::reverse(reversed.begin(), reversed.end());

        return auraui::json{
            {"char_count", input.size()},
            {"word_count", words},
            {"reversed", reversed}
        };
    });

    // 5. Load UI (from embedded VFS in Release, or local files in Dev)
#if defined(HAS_EMBEDDED_VFS)
    app.set_vfs(aura_vfs_handler);
    app.start("index.html");
#else
    app.start("ui/index.html");
#endif

    // 6. Run native loop
    app.run();
    return 0;
}
