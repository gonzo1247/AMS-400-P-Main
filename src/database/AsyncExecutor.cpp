
#include "AsyncExecutor.h"

#include <utility>

#include "Logger.h"
#include "LoggerDefines.h"

namespace database
{

AsyncExecutor::AsyncExecutor() : running_(true)
{
    worker_ = std::thread(&AsyncExecutor::Run, this);
}

AsyncExecutor::~AsyncExecutor()
{
    Stop();
    if (worker_.joinable())
        worker_.join();
}

CancellationToken AsyncExecutor::Submit(Task task, std::size_t maxQueueDepth)
{
    CancellationToken token;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (!running_.load())
        {
            LOG_WARNING("AsyncExecutor is stopped; rejecting task submission.");
            return token;
        }
        if (maxQueueDepth > 0)
        {
            cv_.wait(lock, [&]() { return tasks_.size() < maxQueueDepth || !running_.load(); });
            if (!running_.load())
            {
                LOG_WARNING("AsyncExecutor is stopping; rejecting task submission.");
                return token;
            }
        }
        tasks_.emplace(std::move(task), token);
    }
    cv_.notify_one();
    return token;
}

void AsyncExecutor::Stop()
{
    bool expected = true;
    if (running_.compare_exchange_strong(expected, false))
    {
        cv_.notify_all();
    }
}

std::size_t AsyncExecutor::QueueSize() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return tasks_.size();
}

void AsyncExecutor::Run()
{
    while (true)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [&]() { return !tasks_.empty() || !running_.load(); });

        if (!running_.load() && tasks_.empty())
            break;

        auto [task, token] = std::move(tasks_.front());
        tasks_.pop();
        cv_.notify_all();
        lock.unlock();

        if (token.IsCancelled())
            continue;

        try
        {
            task();
        }
        catch (const std::exception& ex)
        {
            LOG_ERROR(std::string("Async task failed: ") + ex.what());
        }
    }
}

} // namespace database

