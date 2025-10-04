#ifndef DIAGNOSTIC_SYSTEM_H
#define DIAGNOSTIC_SYSTEM_H

#include <vector>
#include <string>
#include <unordered_map>
#include <chrono>
#include <functional>
#include <raylib.h>

struct DiagnosticProbe {
    std::string name;
    std::string category;
    std::function<float()> valueFunction;
    float currentValue;
    float minValue;
    float maxValue;
    float averageValue;
    bool enabled;

    DiagnosticProbe(const std::string& probeName, const std::string& cat,
                   std::function<float()> func);
    void Update();
    void Reset();
};

struct SystemSnapshot {
    std::string timestamp;
    std::unordered_map<std::string, float> probeValues;
    size_t memoryUsage;
    int activeThreads;
    float cpuUsage;

    SystemSnapshot();
};

class DiagnosticSystem {
public:
    static DiagnosticSystem& GetInstance();

    // Probe management
    void RegisterProbe(const std::string& name, const std::string& category,
                      std::function<float()> valueFunction);
    void UnregisterProbe(const std::string& name);
    void EnableProbe(const std::string& name, bool enable);

    // Diagnostic categories
    void SetCategoryEnabled(const std::string& category, bool enable);
    std::vector<std::string> GetCategories() const;

    // System monitoring
    void Update();
    void TakeSnapshot();
    std::vector<SystemSnapshot> GetSnapshots() const { return m_snapshots; }

    // Real-time monitoring
    float GetProbeValue(const std::string& name) const;
    std::vector<DiagnosticProbe*> GetProbesInCategory(const std::string& category);
    std::unordered_map<std::string, float> GetAllProbeValues() const;

    // Performance profiling
    void StartProfiling(const std::string& profileName);
    void EndProfiling(const std::string& profileName);
    float GetProfileTime(const std::string& profileName) const;

    // Memory diagnostics
    void EnableMemoryTracking(bool enable);
    size_t GetTrackedMemoryUsage() const { return m_memoryUsage; }
    void ForceGarbageCollection();

    // Thread diagnostics
    void EnableThreadTracking(bool enable);
    int GetActiveThreadCount() const { return m_activeThreads; }

    // System health checks
    struct HealthCheck {
        std::string name;
        std::function<bool()> checkFunction;
        std::string description;
        bool lastResult;
        std::string lastMessage;

        HealthCheck(const std::string& checkName, std::function<bool()> check,
                   const std::string& desc);
    };

    void RegisterHealthCheck(const std::string& name, std::function<bool()> check,
                            const std::string& description);
    bool RunHealthChecks();
    std::vector<HealthCheck> GetHealthChecks() const { return m_healthChecks; }

    // Reporting
    std::string GenerateDiagnosticReport() const;
    void ExportReport(const std::string& filename) const;

private:
    DiagnosticSystem();
    ~DiagnosticSystem() = default;
    DiagnosticSystem(const DiagnosticSystem&) = delete;
    DiagnosticSystem& operator=(const DiagnosticSystem&) = delete;

    // Internal methods
    void UpdateProbeStatistics();
    void CleanupOldSnapshots();
    std::string GetCurrentTimestamp();

    // Member variables
    std::vector<DiagnosticProbe> m_probes;
    std::unordered_map<std::string, size_t> m_probeIndex;
    std::unordered_map<std::string, bool> m_categoryEnabled;

    std::vector<SystemSnapshot> m_snapshots;
    static const size_t MAX_SNAPSHOTS = 100;

    // Profiling
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> m_profileTimers;
    std::unordered_map<std::string, float> m_profileResults;

    // Memory tracking
    bool m_memoryTracking;
    size_t m_memoryUsage;
    size_t m_peakMemoryUsage;

    // Thread tracking
    bool m_threadTracking;
    int m_activeThreads;

    // Health checks
    std::vector<HealthCheck> m_healthChecks;

    // Timing
    std::chrono::steady_clock::time_point m_lastUpdate;
    float m_updateInterval;
};

#endif // DIAGNOSTIC_SYSTEM_H