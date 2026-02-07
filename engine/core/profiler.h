#ifndef CH_PROFILER_H
#define CH_PROFILER_H

#include <chrono>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#include <algorithm>

#include "engine/core/base.h"

namespace CHEngine
{
    struct ProfileResult
    {
        std::string Name;
        long long Start;
        long long End;
        float DurationMS;
        uint32_t ThreadID;
    };

    struct InstrumentationSession
    {
        std::string Name;
    };

    class Instrumentor
    {
    public:
        Instrumentor()
            : m_CurrentSession(nullptr), m_ProfileCount(0)
        {
        }

        void BeginSession(const std::string& name, const std::string& filepath = "results.json")
        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            m_OutputStream.open(filepath);
            WriteHeader();
            m_CurrentSession = new InstrumentationSession{ name };
        }

        void EndSession()
        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            WriteFooter();
            m_OutputStream.close();
            delete m_CurrentSession;
            m_CurrentSession = nullptr;
            m_ProfileCount = 0;
        }

        void WriteProfile(const ProfileResult& result)
        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            
            m_FrameResults.push_back(result);

            if (m_OutputStream.is_open())
            {
                if (m_ProfileCount++ > 0)
                    m_OutputStream << ",";

                std::string name = result.Name;
                std::replace(name.begin(), name.end(), '"', '\'');

                m_OutputStream << "{";
                m_OutputStream << "\"cat\":\"function\",";
                m_OutputStream << "\"dur\":" << (result.End - result.Start) << ',';
                m_OutputStream << "\"name\":\"" << name << "\",";
                m_OutputStream << "\"ph\":\"X\",";
                m_OutputStream << "\"pid\":1,";
                m_OutputStream << "\"tid\":" << result.ThreadID << ",";
                m_OutputStream << "\"ts\":" << result.Start;
                m_OutputStream << "}";

                m_OutputStream.flush();
            }
        }

        void ClearFrameResults()
        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            m_FrameResults.clear();
        }

        const std::vector<ProfileResult>& GetFrameResults() const { return m_FrameResults; }

        void WriteHeader()
        {
            m_OutputStream << "{\"otherData\": {},\"traceEvents\":[";
            m_OutputStream.flush();
        }

        void WriteFooter()
        {
            m_OutputStream << "]}";
            m_OutputStream.flush();
        }

        static Instrumentor& Get()
        {
            static Instrumentor instance;
            return instance;
        }

    private:
        InstrumentationSession* m_CurrentSession;
        std::ofstream m_OutputStream;
        int m_ProfileCount;
        std::vector<ProfileResult> m_FrameResults;
        std::mutex m_Mutex;
    };

    class InstrumentationTimer
    {
    public:
        InstrumentationTimer(const char* name)
            : m_Name(name), m_Stopped(false)
        {
            m_StartTimepoint = std::chrono::high_resolution_clock::now();
        }

        ~InstrumentationTimer()
        {
            if (!m_Stopped)
                Stop();
        }

        void Stop()
        {
            auto endTimepoint = std::chrono::high_resolution_clock::now();

            long long start = std::chrono::time_point_cast<std::chrono::microseconds>(m_StartTimepoint).time_since_epoch().count();
            long long end = std::chrono::time_point_cast<std::chrono::microseconds>(endTimepoint).time_since_epoch().count();

            float durationMS = (float)(end - start) / 1000.0f;

            uint32_t threadID = (uint32_t)std::hash<std::thread::id>{}(std::this_thread::get_id());
            Instrumentor::Get().WriteProfile({ m_Name, start, end, durationMS, threadID });

            m_Stopped = true;
        }

    private:
        const char* m_Name;
        std::chrono::time_point<std::chrono::high_resolution_clock> m_StartTimepoint;
        bool m_Stopped;
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

        static const ProfilerStats& GetStats()
        {
            return s_Stats;
        }
        static void UpdateStats(const ProfilerStats& stats);
        static void ResetFrameStats();

        static const std::vector<ProfileResult>& GetLastFrameResults()
        {
            return s_LastFrameResults;
        }

    private:
        static std::mutex s_Mutex;
        static ProfilerStats s_Stats;
        static std::vector<ProfileResult> s_LastFrameResults;
    };

} // namespace CHEngine

#define CH_PROFILE_BEGIN_SESSION(name, filepath) ::CHEngine::Instrumentor::Get().BeginSession(name, filepath)
#define CH_PROFILE_END_SESSION() ::CHEngine::Instrumentor::Get().EndSession()
#define CH_PROFILE_SCOPE(name) ::CHEngine::InstrumentationTimer timer##__LINE__(name)
#define CH_PROFILE_FUNCTION() CH_PROFILE_SCOPE(__FUNCTION__)

#endif // CH_PROFILER_H
