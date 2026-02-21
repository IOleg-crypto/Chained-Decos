#include "profiler.h"
#include <mutex>
#include <thread>

namespace CHEngine
{
std::mutex Profiler::s_Mutex;
ProfilerStats Profiler::s_Stats;

std::vector<ProfileResult> Profiler::s_LastFrameResults;

void Profiler::BeginFrame()
{
    s_LastFrameResults = Instrumentor::Get().GetFrameResults();
    Instrumentor::Get().ClearFrameResults();
    ResetFrameStats();
}

void Profiler::EndFrame()
{
}

void Profiler::UpdateStats(const ProfilerStats& stats)
{
    std::lock_guard<std::mutex> lock(s_Mutex);
    s_Stats.DrawCalls += stats.DrawCalls;
    s_Stats.PolyCount += stats.PolyCount;
    s_Stats.MeshCount += stats.MeshCount;
    s_Stats.TextureCount += stats.TextureCount;

    if (stats.EntityCount > 0)
    {
        s_Stats.EntityCount = stats.EntityCount;
    }

    if (stats.ColliderCount > 0)
    {
        s_Stats.ColliderCount = stats.ColliderCount;
    }
}

void Profiler::ResetFrameStats()
{
    s_Stats.DrawCalls = 0;
    s_Stats.PolyCount = 0;
    s_Stats.MeshCount = 0;
    s_Stats.TextureCount = 0;
}

} // namespace CHEngine
