#ifndef CH_PROFILER_H
#define CH_PROFILER_H

#include "engine/core/base.h"
#include <chrono>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace CHEngine
{

struct ProfileResult
{
    std::string Name;
    std::chrono::microseconds StartTime;
    std::chrono::microseconds Duration;
    uint32_t ThreadID;
    uint32_t Color = 0;
    std::vector<std::shared_ptr<ProfileResult>> Children;
};

struct ProfilerStats
{
    // Hardware
    std::string CPU;
    std::string GPU;
    uint64_t TotalRAM = 0;
    uint64_t UsedRAM = 0;

    // Rendering
    uint32_t DrawCalls = 0;
    uint32_t PolyCount = 0;
    uint32_t MeshCount = 0;
    uint32_t TextureCount = 0;

    // Scene
    uint32_t EntityCount = 0;
    uint32_t ColliderCount = 0;
    std::unordered_map<int, uint32_t> ColliderTypeCounts; // Type -> Count

    // History
    std::vector<float> FrameTimeHistory;
};

class Profiler
{
public:
    static void Init();
    static void BeginFrame();
    static void EndFrame();

    static void BeginScope(const std::string &name);
    static void EndScope();

    static const std::vector<std::shared_ptr<ProfileResult>> &GetLastFrameResults();

    static const ProfilerStats &GetStats()
    {
        return s_Stats;
    }
    static void UpdateStats(const ProfilerStats &stats);
    static void ResetFrameStats();

    static const std::vector<float> &GetFrameTimeHistory()
    {
        return s_FrameTimeHistory;
    }

private:
    struct ThreadContext
    {
        std::vector<std::shared_ptr<ProfileResult>> Stack;
        std::vector<std::shared_ptr<ProfileResult>> CurrentFrame;
    };

    static std::unordered_map<std::thread::id, ThreadContext> s_ThreadContexts;
    static std::mutex s_ContextMutex;
    static std::vector<std::shared_ptr<ProfileResult>> s_LastFrameResults;
    static std::vector<float> s_FrameTimeHistory;
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
