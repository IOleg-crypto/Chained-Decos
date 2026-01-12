#ifndef CH_THREAD_POOL_H
#define CH_THREAD_POOL_H

#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace CHEngine
{
class ThreadPool
{
public:
    ThreadPool(size_t numThreads = std::thread::hardware_concurrency());
    ~ThreadPool();

    template <typename F, typename... Args>
    auto Enqueue(F &&f, Args &&...args)
        -> std::future<typename std::invoke_result<F, Args...>::type>
    {
        using ReturnType = typename std::invoke_result<F, Args...>::type;

        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));

        std::future<ReturnType> result = task->get_future();
        {
            std::unique_lock<std::mutex> lock(m_QueueMutex);
            if (m_Stop)
                throw std::runtime_error("Enqueue on stopped ThreadPool");

            m_Tasks.emplace([task]() { (*task)(); });
        }
        m_Condition.notify_one();
        return result;
    }

    size_t GetActiveTaskCount() const
    {
        return m_ActiveTasks.load();
    }

    void WaitAll();

private:
    std::vector<std::thread> m_Workers;
    std::queue<std::function<void()>> m_Tasks;
    std::mutex m_QueueMutex;
    std::condition_variable m_Condition;
    std::atomic<bool> m_Stop{false};
    std::atomic<size_t> m_ActiveTasks{0};
};
} // namespace CHEngine

#endif // CHEngine_THREAD_POOL_H
