#ifndef PERFORMANCE_MANAGER_H
#define PERFORMANCE_MANAGER_H

#include <vector>
#include <string>
#include <chrono>
#include <unordered_map>
#include <raylib.h>

struct FrameMetrics {
    float frameTime;           // Total frame time in milliseconds
    float updateTime;          // Update phase time
    float renderTime;          // Render phase time
    float collisionTime;       // Collision detection time
    float audioTime;           // Audio processing time
    int drawCalls;             // Number of draw calls
    int trianglesRendered;     // Number of triangles rendered
    size_t memoryUsage;        // Memory usage in bytes
    int activeObjects;         // Number of active game objects

    FrameMetrics() : frameTime(0), updateTime(0), renderTime(0), collisionTime(0),
                     audioTime(0), drawCalls(0), trianglesRendered(0), memoryUsage(0), activeObjects(0) {}
};

struct LODLevel {
    std::string name;
    float distance;           // Distance threshold for this LOD
    float quality;            // Quality multiplier (0.0 to 1.0)
    int maxTriangles;         // Maximum triangles for this LOD
    bool enabled;             // Whether this LOD level is enabled

    LODLevel(const std::string& lodName, float dist, float qual = 1.0f, int maxTris = 10000)
        : name(lodName), distance(dist), quality(qual), maxTriangles(maxTris), enabled(true) {}
};

class PerformanceManager {
public:
    PerformanceManager();
    ~PerformanceManager() = default;

    // Frame timing
    void StartFrame();
    void EndFrame();
    void StartPhase(const std::string& phaseName);
    void EndPhase(const std::string& phaseName);

    // Metrics collection
    void RecordDrawCall();
    void RecordTriangles(int count);
    void RecordMemoryUsage(size_t bytes);
    void RecordActiveObjects(int count);

    // Performance monitoring
    FrameMetrics GetCurrentMetrics() const { return m_currentMetrics; }
    FrameMetrics GetAverageMetrics() const { return m_averageMetrics; }
    float GetFPS() const { return m_currentFPS; }
    float GetFrameTime() const { return m_currentMetrics.frameTime; }

    // LOD management
    void AddLODLevel(const std::string& modelName, const LODLevel& lod);
    void RemoveLODLevel(const std::string& modelName, const std::string& lodName);
    LODLevel* GetLODForDistance(const std::string& modelName, float distance);
    std::vector<LODLevel> GetLODLevels(const std::string& modelName) const;

    // Frustum culling
    void SetCamera(const Camera3D& camera);
    void UpdateFrustum();
    bool IsInFrustum(const Vector3& position, float radius) const;
    bool IsInFrustum(const BoundingBox& box) const;

    // Occlusion culling
    void EnableOcclusionCulling(bool enable);
    bool IsOcclusionCullingEnabled() const { return m_occlusionCulling; }
    void SetOcclusionQuality(int quality); // 0-3, higher = better quality but slower

    // Memory management
    void SetMemoryLimit(size_t maxMemory);
    size_t GetMemoryLimit() const { return m_memoryLimit; }
    size_t GetCurrentMemoryUsage() const { return m_currentMetrics.memoryUsage; }
    void TriggerGarbageCollection();

    // Performance settings
    void SetTargetFPS(int fps);
    int GetTargetFPS() const { return m_targetFPS; }
    void EnableVSync(bool enable);
    bool IsVSyncEnabled() const { return m_vsyncEnabled; }

    // Adaptive quality
    void EnableAdaptiveQuality(bool enable);
    bool IsAdaptiveQualityEnabled() const { return m_adaptiveQuality; }
    void SetQualityScale(float scale); // 0.1 to 2.0
    float GetQualityScale() const { return m_qualityScale; }

    // Performance warnings
    void SetPerformanceWarningThreshold(float frameTimeMs);
    bool IsPerformanceWarning() const { return m_performanceWarning; }
    std::string GetPerformanceReport() const;

    // Statistics
    void ResetStatistics();
    struct PerformanceStats {
        float minFrameTime;
        float maxFrameTime;
        float avgFrameTime;
        int totalFrames;
        int slowFrames; // Frames above threshold
    };
    PerformanceStats GetStatistics() const { return m_stats; }

private:
    // Timing
    std::chrono::steady_clock::time_point m_frameStartTime;
    std::chrono::steady_clock::time_point m_phaseStartTime;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> m_phaseTimers;

    // Current metrics
    FrameMetrics m_currentMetrics;
    FrameMetrics m_averageMetrics;
    float m_currentFPS;

    // Frame history for averaging
    std::vector<FrameMetrics> m_frameHistory;
    static const int MAX_FRAME_HISTORY = 60;

    // LOD data
    std::unordered_map<std::string, std::vector<LODLevel>> m_lodLevels;

    // Frustum culling
    Camera3D m_camera;
    bool m_frustumDirty;
    struct FrustumPlane {
        Vector3 normal;
        float distance;
    };
    FrustumPlane m_frustumPlanes[6];

    // Occlusion culling
    bool m_occlusionCulling;
    int m_occlusionQuality;

    // Memory management
    size_t m_memoryLimit;
    size_t m_peakMemoryUsage;

    // Performance settings
    int m_targetFPS;
    bool m_vsyncEnabled;
    bool m_adaptiveQuality;
    float m_qualityScale;
    float m_performanceWarningThreshold;
    bool m_performanceWarning;

    // Statistics
    PerformanceStats m_stats;

    // Helper methods
    void UpdateFrustumPlanes();
    void CalculateFrustumPlanes();
    bool PointInFrustum(const Vector3& point) const;
    bool SphereInFrustum(const Vector3& center, float radius) const;
    bool BoxInFrustum(const BoundingBox& box) const;
    void UpdateAverageMetrics();
    void CheckPerformanceThresholds();
    void UpdateAdaptiveQuality();
};

#endif // PERFORMANCE_MANAGER_H