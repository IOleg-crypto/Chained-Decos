#include "main_thread_queue.h"

namespace CHEngine
{

namespace MainThread
{
static std::vector<std::function<void()>> s_Queue;
static std::mutex s_Mutex;

void Execute(std::function<void()> &&func)
{
    std::lock_guard<std::mutex> lock(s_Mutex);
    s_Queue.push_back(std::move(func));
}

void ProcessAll()
{
    std::vector<std::function<void()>> localQueue;
    {
        std::lock_guard<std::mutex> lock(s_Mutex);
        localQueue = std::move(s_Queue);
        s_Queue.clear();
    }

    for (auto &fn : localQueue)
    {
        if (fn)
            fn();
    }
}
} // namespace MainThread

} // namespace CHEngine
