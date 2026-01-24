#ifndef CH_ASYNC_UTILS_H
#define CH_ASYNC_UTILS_H

#include <algorithm>
#include <functional>
#include <future>
#include <thread>
#include <vector>


namespace CHEngine
{

/**
 * @brief Simple parallel for utility using std::async
 */
template <typename Func> void ParallelFor(size_t count, Func &&func, size_t chunkSize = 0)
{
    if (count == 0)
        return;

    size_t threadCount = std::thread::hardware_concurrency();
    if (threadCount == 0)
        threadCount = 1;

    if (chunkSize == 0)
    {
        chunkSize = (count + threadCount - 1) / threadCount;
    }

    if (chunkSize == 0)
        chunkSize = 1;

    std::vector<std::future<void>> futures;
    for (size_t t = 0; t < threadCount; ++t)
    {
        size_t start = t * chunkSize;
        if (start >= count)
            break;
        size_t end = std::min(start + chunkSize, count);

        futures.push_back(std::async(std::launch::async,
                                     [start, end, &func]()
                                     {
                                         for (size_t i = start; i < end; ++i)
                                         {
                                             func(i);
                                         }
                                     }));
    }

    for (auto &f : futures)
    {
        f.wait();
    }
}

} // namespace CHEngine

#endif // CH_ASYNC_UTILS_H
