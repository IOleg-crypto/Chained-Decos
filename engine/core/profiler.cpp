#ifndef GL_RENDERER
#define GL_RENDERER 0x1F01
#endif
extern "C" const unsigned char *__stdcall glGetString(unsigned int name);

#include "profiler.h"
#include "process_utils.h"
#include "rlImGui.h"
#include <rlgl.h>
#include <stdio.h>

namespace CHEngine
{

std::unordered_map<std::thread::id, Profiler::ThreadContext> Profiler::s_ThreadContexts;
std::mutex Profiler::s_ContextMutex;
std::vector<std::shared_ptr<ProfileResult>> Profiler::s_LastFrameResults;
std::vector<float> Profiler::s_FrameTimeHistory;
ProfilerStats Profiler::s_Stats;

void Profiler::Init()
{
    s_FrameTimeHistory.reserve(100);
    for (int i = 0; i < 100; i++)
        s_FrameTimeHistory.push_back(0.0f);
}

void Profiler::BeginFrame()
{
    ResetFrameStats();

    // 1. Hardware Info (Gather once)
    static bool hardwareGathered = false;
    if (!hardwareGathered)
    {
        const char *renderer = (const char *)glGetString(GL_RENDERER);
        if (renderer)
            s_Stats.GPU = renderer;
        else
        {
            static char versionBuffer[32];
            sprintf(versionBuffer, "OpenGL %d", rlGetVersion());
            s_Stats.GPU = versionBuffer;
        }

        s_Stats.CPU = ProcessUtils::GetCPUName();
        ProcessUtils::GetSystemMemoryInfo(s_Stats.TotalRAM, s_Stats.UsedRAM);
        hardwareGathered = true;
    }

    // 2. RAM Info (Gather every frame or periodically)
    static int ramTimer = 0;
    if (ramTimer++ % 60 == 0) // Once a second approx
    {
        ProcessUtils::GetSystemMemoryInfo(s_Stats.TotalRAM, s_Stats.UsedRAM);
    }
}

void Profiler::EndFrame()
{
    std::lock_guard<std::mutex> lock(s_ContextMutex);

    // We don't clear s_LastFrameResults immediately to allow it to be persistent
    // if some threads haven't finished their work yet.
    // Instead, we maintain a "stable" set of results and just update it.

    bool updated = false;
    for (auto &[id, context] : s_ThreadContexts)
    {
        auto it = context.CurrentFrame.begin();
        while (it != context.CurrentFrame.end())
        {
            if ((*it)->Duration.count() > 0)
            {
                // Find and replace old result for this thread/name if exists, or just append
                bool replaced = false;
                for (auto &oldRes : s_LastFrameResults)
                {
                    if (oldRes->ThreadID == (*it)->ThreadID && oldRes->Name == (*it)->Name)
                    {
                        oldRes = *it;
                        replaced = true;
                        break;
                    }
                }

                if (!replaced)
                    s_LastFrameResults.push_back(*it);

                // Record and maintain history for the MainThread_Frame
                if ((*it)->Name == "MainThread_Frame")
                {
                    float ms = (*it)->Duration.count() / 1000.0f;
                    if (s_FrameTimeHistory.size() >= 100)
                    {
                        for (size_t i = 1; i < s_FrameTimeHistory.size(); i++)
                            s_FrameTimeHistory[i - 1] = s_FrameTimeHistory[i];
                        s_FrameTimeHistory.back() = ms;
                    }
                    else
                    {
                        s_FrameTimeHistory.push_back(ms);
                    }
                }

                it = context.CurrentFrame.erase(it);
                updated = true;
            }
            else
            {
                ++it;
            }
        }
    }
}

void Profiler::BeginScope(const std::string &name)
{
    auto threadId = std::this_thread::get_id();

    std::lock_guard<std::mutex> lock(s_ContextMutex);
    auto &context = s_ThreadContexts[threadId];

    auto res = std::make_shared<ProfileResult>();
    res->Name = name;
    res->StartTime = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch());
    res->ThreadID = (uint32_t)std::hash<std::thread::id>{}(threadId);

    if (context.Stack.empty())
    {
        context.CurrentFrame.push_back(res);
        context.Stack.push_back(res);
    }
    else
    {
        auto parent = context.Stack.back();
        parent->Children.push_back(res);
        context.Stack.push_back(res);
    }
}

void Profiler::EndScope()
{
    auto threadId = std::this_thread::get_id();

    std::lock_guard<std::mutex> lock(s_ContextMutex);
    auto &context = s_ThreadContexts[threadId];

    if (context.Stack.empty())
        return;

    auto res = context.Stack.back();
    auto endTime = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch());
    res->Duration = endTime - res->StartTime;

    context.Stack.pop_back();
}

const std::vector<std::shared_ptr<ProfileResult>> &Profiler::GetLastFrameResults()
{
    return s_LastFrameResults;
}

void Profiler::UpdateStats(const ProfilerStats &stats)
{
    // Update non-resettable stats (Hardware) only if provided
    if (!stats.CPU.empty())
        s_Stats.CPU = stats.CPU;
    if (!stats.GPU.empty())
        s_Stats.GPU = stats.GPU;
    if (stats.TotalRAM > 0)
        s_Stats.TotalRAM = stats.TotalRAM;

    // Merge resettable stats
    if (stats.UsedRAM > 0)
        s_Stats.UsedRAM = stats.UsedRAM;

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
    // We don't reset hardware info or entity count as they are updated per frame anyway
}

} // namespace CHEngine
