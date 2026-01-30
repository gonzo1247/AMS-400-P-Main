#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <thread>
#include <unordered_map>

namespace database
{

class CancellationToken
{
public:
    CancellationToken() : cancelled_(std::make_shared<std::atomic<bool>>(false)) {}

    void Cancel() const { cancelled_->store(true, std::memory_order_relaxed); }
    bool IsCancelled() const { return cancelled_->load(std::memory_order_relaxed); }

private:
    std::shared_ptr<std::atomic<bool>> cancelled_;
};

class AsyncExecutor
{
public:
    AsyncExecutor();
    ~AsyncExecutor();

    AsyncExecutor(const AsyncExecutor&) = delete;
    AsyncExecutor& operator=(const AsyncExecutor&) = delete;

    using Task = std::function<void()>;

    CancellationToken Submit(Task task, std::size_t maxQueueDepth = 0);

    void Stop();
    std::size_t QueueSize() const;

private:
    void Run();

    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::queue<std::pair<Task, CancellationToken>> tasks_;
    std::thread worker_;
    std::atomic<bool> running_;
};

} // namespace database

