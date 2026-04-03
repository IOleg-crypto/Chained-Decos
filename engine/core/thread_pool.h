#ifndef CH_THREAD_POOL_H
#define CH_THREAD_POOL_H

#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace CHEngine
{
/**
 * @brief A modern C++20 Thread Pool for parallel task execution.
 * Uses std::jthread for automatic thread management and a thread-safe singleton pattern.
 */
class ThreadPool
{
public:
    /**
     * @brief Access the global thread pool instance.
     */
    static ThreadPool& Get()
    {
        static ThreadPool instance;
        return instance;
    }

    // Deleted constructors for singleton
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    /**
     * @brief Enqueues a task and returns a std::future for the result.
     * Useful for tasks where you need the return value later (e.g. Asset Loading).
     */
    template <class F, class... Args>
    auto Enqueue(F&& f, Args&&... args) -> std::future<typename std::invoke_result<F, Args...>::type>
    {
        using return_type = typename std::invoke_result<F, Args...>::type;

        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));

        std::future<return_type> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(m_QueueMutex);
            if (m_Stop)
                throw std::runtime_error("Enqueue on stopped ThreadPool");

            m_Tasks.emplace([task]() { (*task)(); });
        }
        m_Condition.notify_one();
        return res;
    }

    /**
     * @brief Simpler version for "fire and forget" tasks.
     */
    void QueueTask(std::function<void()> task)
    {
        {
            std::unique_lock<std::mutex> lock(m_QueueMutex);
            m_Tasks.emplace(std::move(task));
        }
        m_Condition.notify_one();
    }

private:
    ThreadPool();
    ~ThreadPool();

    void WorkerThread(std::stop_token stopToken);

private:
   std::queue<std::function<void()>> m_Tasks;
   std::mutex m_QueueMutex;
   std::condition_variable_any m_Condition;
   bool m_Stop = false;
   std::vector<std::jthread> m_Workers;
};
} // namespace CHEngine

#endif // CH_THREAD_POOL_H
