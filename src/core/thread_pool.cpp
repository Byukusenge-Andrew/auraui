#include "thread_pool.hpp"

namespace auraui {
namespace core {

ThreadPool::ThreadPool(size_t threads, size_t max_queue_size)
    : m_max_queue_size(max_queue_size) {
    if (threads == 0) threads = 1;

    m_workers.reserve(threads);
    for (size_t i = 0; i < threads; ++i) {
        m_workers.emplace_back([this]() {
            while (true) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(this->m_mutex);
                    this->m_cv.wait(lock, [this]() {
                        return this->m_stop.load() || !this->m_tasks.empty();
                    });

                    if (this->m_stop.load() && this->m_tasks.empty()) {
                        return;
                    }

                    task = std::move(this->m_tasks.front());
                    this->m_tasks.pop();
                    this->m_full_cv.notify_one();
                }

                if (task) {
                    try {
                        task();
                    } catch (...) {}
                }
            }
        });
    }
}

ThreadPool::~ThreadPool() {
    shutdown();
}

bool ThreadPool::post(std::function<void()> task) {
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (m_stop.load() || m_tasks.size() >= m_max_queue_size) {
            return false;
        }
        m_tasks.push(std::move(task));
    }
    m_cv.notify_one();
    return true;
}

void ThreadPool::shutdown() {
    if (!m_stop.exchange(true)) {
        m_cv.notify_all();
        m_full_cv.notify_all();
        for (std::thread& worker : m_workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
        m_workers.clear();
    }
}

} // namespace core
} // namespace auraui
