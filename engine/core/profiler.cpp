#include "profiler.h"
#include <mutex>
#include <thread>

namespace CHEngine
{

std::unordered_map<std::thread::id, Profiler::ThreadContext> Profiler::s_ThreadContexts;
std::vector<ProfileResult> Profiler::s_CurrentFrameScopes;
std::vector<ProfileResult> Profiler::s_LastFrameScopes;
std::mutex Profiler::s_Mutex;
ProfilerStats Profiler::s_Stats;

void Profiler::BeginFrame()
{
    ResetFrameStats();
    std::lock_guard<std::mutex> lock(s_Mutex);
    s_CurrentFrameScopes.clear();
}

void Profiler::EndFrame()
{
    std::lock_guard<std::mutex> lock(s_Mutex);
    s_LastFrameScopes = s_CurrentFrameScopes;
}

void Profiler::BeginScope(const char *name)
{
    auto threadId = std::this_thread::get_id();
    auto &context = s_ThreadContexts[threadId];

    context.Stack.push_back({name, std::chrono::high_resolution_clock::now()});
}

void Profiler::EndScope()
{
    auto threadId = std::this_thread::get_id();
    auto &context = s_ThreadContexts[threadId];

    if (context.Stack.empty())
        return;

    auto scope = context.Stack.back();
    context.Stack.pop_back();

    auto duration = std::chrono::high_resolution_clock::now() - scope.Start;
    float ms = std::chrono::duration_cast<std::chrono::microseconds>(duration).count() / 1000.0f;

    std::lock_guard<std::mutex> lock(s_Mutex);
    s_CurrentFrameScopes.push_back(
        {scope.Name, ms, (uint32_t)std::hash<std::thread::id>{}(threadId)});
}

void Profiler::UpdateStats(const ProfilerStats &stats)
{
    std::lock_guard<std::mutex> lock(s_Mutex);
    s_Stats.DrawCalls += stats.DrawCalls;
    s_Stats.PolyCount += stats.PolyCount;
    s_Stats.MeshCount += stats.MeshCount;
    s_Stats.TextureCount += stats.TextureCount;

    if (stats.EntityCount > 0)
        s_Stats.EntityCount = stats.EntityCount;

    if (stats.ColliderCount > 0)
        s_Stats.ColliderCount = stats.ColliderCount;
}

void Profiler::ResetFrameStats()
{
    s_Stats.DrawCalls = 0;
    s_Stats.PolyCount = 0;
    s_Stats.MeshCount = 0;
    s_Stats.TextureCount = 0;
}

} // namespace CHEngine
