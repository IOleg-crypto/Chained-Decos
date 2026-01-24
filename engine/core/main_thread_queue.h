#ifndef CH_MAIN_THREAD_QUEUE_H
#define CH_MAIN_THREAD_QUEUE_H

#include <functional>
#include <mutex>
#include <vector>

namespace CHEngine
{

// Simple main thread task queue
// Needed for Raylib and other APIs that require main thread execution
namespace MainThread
{
void Execute(std::function<void()> &&func);
void ProcessAll();
} // namespace MainThread

} // namespace CHEngine

#endif // CH_MAIN_THREAD_QUEUE_H
