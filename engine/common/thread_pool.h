#ifndef CH_THREAD_POOL_H
#define CH_THREAD_POOL_H

#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <memory>
#include <type_traits>

#include "engine/core/engine_module.h"

namespace Chained
{
class ThreadPool : public EngineModule
{
public:
    void Initialize() override
    {
    }

    void Shutdown() override
    {
        Stop();
    }

    // Enqueues a task and returns a future for the result.
    template <class F, class... Args>
    auto Enqueue(F&& f, Args&&... args) -> std::future<typename std::invoke_result<F, Args...>::type>
    {
        using ReturnType = typename std::invoke_result<F, Args...>::type;

        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));

        std::future<ReturnType> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(m_QueueMutex);

            if (m_Stop)
            {
                throw std::runtime_error("Enqueue on stopped ThreadPool");
            }

            m_Tasks.emplace([task]() { (*task)(); });
        }

        m_Condition.notify_one();
        return res;
    }

    void QueueTask(std::function<void()> task)
    {
        {
            std::unique_lock<std::mutex> lock(m_QueueMutex);
            if (m_Stop)
            {
                throw std::runtime_error("QueueTask on stopped ThreadPool");
            }

            m_Tasks.emplace(std::move(task));
        }
        m_Condition.notify_one();
    }

    void WaitIdle()
    {
        std::unique_lock<std::mutex> lock(m_QueueMutex);
        m_Condition.wait(lock, [this]() { return m_Tasks.empty(); });
    }

public:
    explicit ThreadPool(unsigned int workerCount)
        : m_Stop(false)
    {
        m_Workers.reserve(workerCount);
        for (unsigned int i = 0; i < workerCount; ++i)
        {
            m_Workers.emplace_back([this]() {
                while (true)
                {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(this->m_QueueMutex);

                        this->m_Condition.wait(lock, [this]() { return this->m_Stop || !this->m_Tasks.empty(); });

                        if (this->m_Stop && this->m_Tasks.empty())
                        {
                            return;
                        }

                        task = std::move(this->m_Tasks.front());
                        this->m_Tasks.pop();
                    }

                    task();
                }
            });
        }
    }

    ~ThreadPool()
    {
        Stop();
    }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

private:
    void Stop()
    {
        {
            std::unique_lock<std::mutex> lock(m_QueueMutex);
            if (m_Stop)
            {
                return;
            }
            m_Stop = true;
        }

        m_Condition.notify_all();

        for (std::thread& worker : m_Workers)
        {
            if (worker.joinable())
            {
                worker.join();
            }
        }
        m_Workers.clear();
    }

private:
    std::vector<std::thread> m_Workers;

    std::queue<std::function<void()>> m_Tasks;

    std::mutex m_QueueMutex;
    std::condition_variable m_Condition;
    bool m_Stop;
};

} // namespace Chained

#endif // CH_THREAD_POOL_H