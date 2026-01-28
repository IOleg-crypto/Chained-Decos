#ifndef CH_PROFILER_H
#define CH_PROFILER_H

#include "chrono"
#include "engine/core/base.h"
#include "mutex"
#include "string"
#include "thread"
#include "unordered_map"
#include "vector"


namespace CHEngine
{

struct ProfileResult
{
    std::string Name;
    float DurationMS;
    uint32_t ThreadID;
};

struct ProfilerStats
{
    // Rendering
    uint32_t DrawCalls = 0;
    uint32_t PolyCount = 0;
    uint32_t MeshCount = 0;
    uint32_t TextureCount = 0;

    // Scene
    uint32_t EntityCount = 0;
    uint32_t ColliderCount = 0;
};

class Profiler
{
public:
    static void BeginFrame();
    static void EndFrame();

    static void BeginScope(const char *name);
    static void EndScope();

    static const ProfilerStats &GetStats()
    {
        return s_Stats;
    }
    static void UpdateStats(const ProfilerStats &stats);
    static void ResetFrameStats();

    static const std::vector<ProfileResult> &GetLastFrameScopes()
    {
        return s_LastFrameScopes;
    }

private:
    struct ThreadContext
    {
        struct ScopeInfo
        {
            const char *Name;
            std::chrono::high_resolution_clock::time_point Start;
        };
        std::vector<ScopeInfo> Stack;
    };

    static std::unordered_map<std::thread::id, ThreadContext> s_ThreadContexts;
    static std::vector<ProfileResult> s_CurrentFrameScopes;
    static std::vector<ProfileResult> s_LastFrameScopes;
    static std::mutex s_Mutex;
    static ProfilerStats s_Stats;
};

class ProfileTimer
{
public:
    ProfileTimer(const char *name)
    {
        Profiler::BeginScope(name);
    }
    ~ProfileTimer()
    {
        Profiler::EndScope();
    }
};

#define CH_PROFILE_SCOPE(name) ProfileTimer timer##__LINE__(name)
#define CH_PROFILE_FUNCTION() CH_PROFILE_SCOPE(__FUNCTION__)

} // namespace CHEngine

#endif // CH_PROFILER_H
