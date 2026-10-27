#pragma once

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>

namespace auraui {
namespace core {

class ThreadPool {
private:
    std::vector<std::thread> m_workers;
    std::queue<std::function<void()>> m_tasks;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::condition_variable m_full_cv;
    std::atomic<bool> m_stop{false};
    size_t m_max_queue_size{256};

public:
    explicit ThreadPool(size_t threads = 4, size_t max_queue_size = 256);
    ~ThreadPool();

    // Non-copyable
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    bool post(std::function<void()> task);
    void shutdown();
};

} // namespace core
} // namespace auraui
