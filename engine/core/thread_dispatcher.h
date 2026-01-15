#ifndef CH_THREAD_DISPATCHER_H
#define CH_THREAD_DISPATCHER_H

#include "engine/core/base.h"
#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace CHEngine
{
class ThreadDispatcher
{
public:
    static void Init();
    static void Shutdown();

    // Dispatch work to background threads
    template <typename F, typename... Args>
    static auto DispatchAsync(F &&f, Args &&...args)
        -> std::future<typename std::invoke_result<F, Args...>::type>
    {
        using ReturnType = typename std::invoke_result<F, Args...>::type;

        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));

        std::future<ReturnType> result = task->get_future();
        {
            std::unique_lock<std::mutex> lock(s_BackgroundMutex);
            s_BackgroundTasks.emplace([task]() { (*task)(); });
        }
        s_BackgroundCondition.notify_one();
        return result;
    }

    // Dispatch work to the main thread (to be executed via ExecuteMainThreadQueue)
    static void DispatchMain(std::function<void()> func);

    static void ExecuteMainThreadQueue();
    static std::thread::id GetMainThreadId()
    {
        return s_MainThreadId;
    }
    static bool IsMainThread()
    {
        return std::this_thread::get_id() == s_MainThreadId;
    }

private:
    static void WorkerThread();

private:
    static std::vector<std::thread> s_Workers;
    static std::queue<std::function<void()>> s_BackgroundTasks;
    static std::mutex s_BackgroundMutex;
    static std::condition_variable s_BackgroundCondition;
    static bool s_Running;

    static std::queue<std::function<void()>> s_MainThreadQueue;
    static std::mutex s_MainThreadMutex;
    static std::thread::id s_MainThreadId;
};
} // namespace CHEngine

#endif // CH_THREAD_DISPATCHER_H
