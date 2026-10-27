#include "auraui/auraui.hpp"
#include "platform/platform.hpp"

#include <chrono>
#include <thread>
#include <vector>
#include <future>
#include <algorithm>
#include <sstream>

using namespace auraui;

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
    AppOptions opts;
    opts.title = "AuraUI 2.0 - Native Desktop Showcase";
    opts.config_path = "showcase/aura.config.json";
    opts.asset_path = "showcase/ui";
    opts.width = 1100;
    opts.height = 750;

    App app(opts);

    // 1. C++ Benchmark Command
    app.command("run_benchmark", [](Context ctx, Reply reply) {
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
        double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

        reply.send({
            {"primes_found", total_primes},
            {"elapsed_ms", ms}
        });
    });

    // 2. System Telemetry Command
    app.command("get_system_telemetry", [](Context, Reply reply) {
        auto info = platform::Platform::get_system_info();
        double total_gb = static_cast<double>(info.total_memory_bytes) / (1024.0 * 1024.0 * 1024.0);
        double avail_gb = static_cast<double>(info.available_memory_bytes) / (1024.0 * 1024.0 * 1024.0);
        int pct = (info.total_memory_bytes > 0)
            ? static_cast<int>(100.0 * (1.0 - (static_cast<double>(info.available_memory_bytes) / info.total_memory_bytes)))
            : 0;

        reply.send({
            {"overview", {
                {"os", info.os_name + " " + info.os_version},
                {"arch", info.architecture},
                {"cpu_cores", info.cpu_count},
                {"computer_name", "Local Desktop"}
            }},
            {"memory", {
                {"total_gb", total_gb},
                {"avail_gb", avail_gb},
                {"percent_used", pct}
            }}
        });
    });

    // 3. String Processing Command
    app.command("process_text", [](Context ctx, Reply reply) {
        std::string text = ctx.get<std::string>("text", "");
        std::string rev = text;
        std::reverse(rev.begin(), rev.end());

        std::stringstream ss(text);
        std::string word;
        int words = 0;
        while (ss >> word) words++;

        reply.send({
            {"char_count", text.size()},
            {"word_count", words},
            {"reversed", rev}
        });
    });

    // 4. Run application with entry point
    return app.run("index.html");
}
