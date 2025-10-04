#include "DiagnosticSystem.h"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <thread>
#include <raylib.h>

DiagnosticProbe::DiagnosticProbe(const std::string& probeName, const std::string& cat,
                                std::function<float()> func)
    : name(probeName), category(cat), valueFunction(func), currentValue(0.0f),
      minValue(999999.0f), maxValue(-999999.0f), averageValue(0.0f), enabled(true) {}

void DiagnosticProbe::Update() {
    if (!enabled || !valueFunction) return;

    currentValue = valueFunction();

    // Update statistics
    minValue = std::min(minValue, currentValue);
    maxValue = std::max(maxValue, currentValue);

    // Update running average (simple moving average)
    static float alpha = 0.1f; // Smoothing factor
    averageValue = averageValue * (1.0f - alpha) + currentValue * alpha;
}

void DiagnosticProbe::Reset() {
    minValue = 999999.0f;
    maxValue = -999999.0f;
    averageValue = 0.0f;
    currentValue = 0.0f;
}

SystemSnapshot::SystemSnapshot() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    ss << "." << std::setfill('0') << std::setw(3) << ms.count();
    timestamp = ss.str();

    memoryUsage = 0;
    activeThreads = 1; // At least main thread
    cpuUsage = 0.0f;
}

DiagnosticSystem& DiagnosticSystem::GetInstance() {
    static DiagnosticSystem instance;
    return instance;
}

DiagnosticSystem::DiagnosticSystem()
    : m_memoryTracking(false), m_memoryUsage(0), m_peakMemoryUsage(0),
      m_threadTracking(false), m_activeThreads(1), m_updateInterval(1.0f) {

    m_lastUpdate = std::chrono::steady_clock::now();

    // Enable all categories by default
    m_categoryEnabled["Performance"] = true;
    m_categoryEnabled["Memory"] = true;
    m_categoryEnabled["Rendering"] = true;
    m_categoryEnabled["Audio"] = true;
    m_categoryEnabled["Physics"] = true;
    m_categoryEnabled["Input"] = true;
}

void DiagnosticSystem::RegisterProbe(const std::string& name, const std::string& category,
                                    std::function<float()> valueFunction) {
    if (m_probeIndex.find(name) != m_probeIndex.end()) {
        TraceLog(LOG_WARNING, "DiagnosticSystem::RegisterProbe() - Probe '%s' already exists", name.c_str());
        return;
    }

    DiagnosticProbe probe(name, category, valueFunction);
    m_probes.push_back(probe);
    m_probeIndex[name] = m_probes.size() - 1;

    // Enable category if not already present
    if (m_categoryEnabled.find(category) == m_categoryEnabled.end()) {
        m_categoryEnabled[category] = true;
    }

    TraceLog(LOG_INFO, "DiagnosticSystem::RegisterProbe() - Registered probe '%s' in category '%s'",
             name.c_str(), category.c_str());
}

void DiagnosticSystem::UnregisterProbe(const std::string& name) {
    auto it = m_probeIndex.find(name);
    if (it != m_probeIndex.end()) {
        size_t index = it->second;

        // Remove probe
        m_probes.erase(m_probes.begin() + index);
        m_probeIndex.erase(it);

        // Update indices for remaining probes
        for (auto& pair : m_probeIndex) {
            if (pair.second > index) {
                pair.second--;
            }
        }

        TraceLog(LOG_INFO, "DiagnosticSystem::UnregisterProbe() - Unregistered probe: %s", name.c_str());
    }
}

void DiagnosticSystem::EnableProbe(const std::string& name, bool enable) {
    auto it = m_probeIndex.find(name);
    if (it != m_probeIndex.end()) {
        m_probes[it->second].enabled = enable;
        TraceLog(LOG_INFO, "DiagnosticSystem::EnableProbe() - %s probe: %s",
                 enable ? "Enabled" : "Disabled", name.c_str());
    }
}

void DiagnosticSystem::SetCategoryEnabled(const std::string& category, bool enable) {
    m_categoryEnabled[category] = enable;

    // Enable/disable all probes in this category
    for (auto& probe : m_probes) {
        if (probe.category == category) {
            probe.enabled = enable;
        }
    }

    TraceLog(LOG_INFO, "DiagnosticSystem::SetCategoryEnabled() - %s category: %s",
             enable ? "Enabled" : "Disabled", category.c_str());
}

std::vector<std::string> DiagnosticSystem::GetCategories() const {
    std::vector<std::string> categories;
    for (const auto& pair : m_categoryEnabled) {
        categories.push_back(pair.first);
    }
    return categories;
}

void DiagnosticSystem::Update() {
    auto now = std::chrono::steady_clock::now();
    float deltaTime = std::chrono::duration<float>(now - m_lastUpdate).count();

    if (deltaTime >= m_updateInterval) {
        // Update all enabled probes
        for (auto& probe : m_probes) {
            if (probe.enabled && m_categoryEnabled[probe.category]) {
                probe.Update();
            }
        }

        // Update system metrics
        if (m_memoryTracking) {
            m_memoryUsage = GetCurrentMemoryUsage();
            m_peakMemoryUsage = std::max(m_peakMemoryUsage, m_memoryUsage);
        }

        if (m_threadTracking) {
            m_activeThreads = GetActiveThreadCount();
        }

        m_lastUpdate = now;
    }
}

void DiagnosticSystem::TakeSnapshot() {
    SystemSnapshot snapshot;

    // Capture current probe values
    for (const auto& probe : m_probes) {
        if (probe.enabled) {
            snapshot.probeValues[probe.name] = probe.currentValue;
        }
    }

    // Capture system metrics
    snapshot.memoryUsage = m_memoryUsage;
    snapshot.activeThreads = m_activeThreads;
    snapshot.cpuUsage = GetCPUUsage();

    m_snapshots.push_back(snapshot);

    // Limit snapshot count
    if (m_snapshots.size() > MAX_SNAPSHOTS) {
        m_snapshots.erase(m_snapshots.begin());
    }

    TraceLog(LOG_DEBUG, "DiagnosticSystem::TakeSnapshot() - Captured system snapshot");
}

float DiagnosticSystem::GetProbeValue(const std::string& name) const {
    auto it = m_probeIndex.find(name);
    if (it != m_probeIndex.end()) {
        return m_probes[it->second].currentValue;
    }
    return 0.0f;
}

std::vector<DiagnosticProbe*> DiagnosticSystem::GetProbesInCategory(const std::string& category) {
    std::vector<DiagnosticProbe*> probes;
    for (auto& probe : m_probes) {
        if (probe.category == category && probe.enabled) {
            probes.push_back(&probe);
        }
    }
    return probes;
}

std::unordered_map<std::string, float> DiagnosticSystem::GetAllProbeValues() const {
    std::unordered_map<std::string, float> values;
    for (const auto& probe : m_probes) {
        if (probe.enabled) {
            values[probe.name] = probe.currentValue;
        }
    }
    return values;
}

void DiagnosticSystem::StartProfiling(const std::string& profileName) {
    m_profileTimers[profileName] = std::chrono::steady_clock::now();
    TraceLog(LOG_DEBUG, "DiagnosticSystem::StartProfiling() - Started profiling: %s", profileName.c_str());
}

void DiagnosticSystem::EndProfiling(const std::string& profileName) {
    auto it = m_profileTimers.find(profileName);
    if (it != m_profileTimers.end()) {
        auto endTime = std::chrono::steady_clock::now();
        float duration = std::chrono::duration<float, std::milli>(endTime - it->second).count();
        m_profileResults[profileName] = duration;

        TraceLog(LOG_DEBUG, "DiagnosticSystem::EndProfiling() - Ended profiling '%s': %.2fms",
                 profileName.c_str(), duration);
    }
}

float DiagnosticSystem::GetProfileTime(const std::string& profileName) const {
    auto it = m_profileResults.find(profileName);
    return (it != m_profileResults.end()) ? it->second : 0.0f;
}

void DiagnosticSystem::EnableMemoryTracking(bool enable) {
    m_memoryTracking = enable;
    if (enable) {
        m_memoryUsage = GetCurrentMemoryUsage();
        m_peakMemoryUsage = m_memoryUsage;
    }
    TraceLog(LOG_INFO, "DiagnosticSystem::EnableMemoryTracking() - %s memory tracking",
             enable ? "Enabled" : "Disabled");
}

void DiagnosticSystem::ForceGarbageCollection() {
    // Force garbage collection (implementation depends on memory management)
    TraceLog(LOG_INFO, "DiagnosticSystem::ForceGarbageCollection() - Forced garbage collection");
}

void DiagnosticSystem::EnableThreadTracking(bool enable) {
    m_threadTracking = enable;
    if (enable) {
        m_activeThreads = GetActiveThreadCount();
    }
    TraceLog(LOG_INFO, "DiagnosticSystem::EnableThreadTracking() - %s thread tracking",
             enable ? "Enabled" : "Disabled");
}

DiagnosticSystem::HealthCheck::HealthCheck(const std::string& checkName, std::function<bool()> check,
                                          const std::string& desc)
    : name(checkName), checkFunction(check), description(desc), lastResult(true) {}

void DiagnosticSystem::RegisterHealthCheck(const std::string& name, std::function<bool()> check,
                                          const std::string& description) {
    HealthCheck healthCheck(name, check, description);
    m_healthChecks.push_back(healthCheck);

    TraceLog(LOG_INFO, "DiagnosticSystem::RegisterHealthCheck() - Registered health check: %s", name.c_str());
}

bool DiagnosticSystem::RunHealthChecks() {
    bool allHealthy = true;

    for (auto& check : m_healthChecks) {
        try {
            check.lastResult = check.checkFunction();
            if (!check.lastResult) {
                allHealthy = false;
                check.lastMessage = "Health check failed: " + check.name;
                TraceLog(LOG_WARNING, "DiagnosticSystem::RunHealthChecks() - Health check failed: %s",
                         check.name.c_str());
            }
        } catch (const std::exception& e) {
            check.lastResult = false;
            check.lastMessage = "Health check exception: " + std::string(e.what());
            allHealthy = false;
            TraceLog(LOG_ERROR, "DiagnosticSystem::RunHealthChecks() - Health check exception in %s: %s",
                     check.name.c_str(), e.what());
        }
    }

    return allHealthy;
}

std::string DiagnosticSystem::GenerateDiagnosticReport() const {
    std::stringstream report;
    report << "=== Diagnostic Report ===\n";
    report << "Generated: " << GetCurrentTimestamp() << "\n\n";

    // System overview
    report << "System Overview:\n";
    report << "Memory Usage: " << (m_memoryUsage / (1024.0f * 1024.0f)) << " MB\n";
    report << "Peak Memory: " << (m_peakMemoryUsage / (1024.0f * 1024.0f)) << " MB\n";
    report << "Active Threads: " << m_activeThreads << "\n";
    report << "Active Probes: " << m_probes.size() << "\n\n";

    // Probe values by category
    for (const auto& category : m_categoryEnabled) {
        if (!category.second) continue;

        report << category.first << " Diagnostics:\n";
        auto probes = GetProbesInCategory(category.first);

        for (const auto* probe : probes) {
            report << "  " << probe->name << ": " << std::fixed << std::setprecision(2)
                   << probe->currentValue;
            if (probe->name.find("fps") != std::string::npos) report << " FPS";
            else if (probe->name.find("time") != std::string::npos) report << " ms";
            else if (probe->name.find("memory") != std::string::npos) report << " MB";
            else report << " units";
            report << "\n";
        }
        report << "\n";
    }

    // Health check results
    if (!m_healthChecks.empty()) {
        report << "Health Checks:\n";
        for (const auto& check : m_healthChecks) {
            report << "  " << check.name << ": " << (check.lastResult ? "PASS" : "FAIL");
            if (!check.lastMessage.empty()) {
                report << " (" << check.lastMessage << ")";
            }
            report << "\n";
        }
        report << "\n";
    }

    // Profiling results
    if (!m_profileResults.empty()) {
        report << "Profiling Results:\n";
        for (const auto& pair : m_profileResults) {
            report << "  " << pair.first << ": " << std::fixed << std::setprecision(2)
                   << pair.second << " ms\n";
        }
        report << "\n";
    }

    return report.str();
}

void DiagnosticSystem::ExportReport(const std::string& filename) const {
    std::ofstream file(filename);
    if (file.is_open()) {
        file << GenerateDiagnosticReport();
        file.close();
        TraceLog(LOG_INFO, "DiagnosticSystem::ExportReport() - Exported diagnostic report to %s", filename.c_str());
    } else {
        TraceLog(LOG_ERROR, "DiagnosticSystem::ExportReport() - Failed to open file: %s", filename.c_str());
    }
}

void DiagnosticSystem::UpdateProbeStatistics() {
    for (auto& probe : m_probes) {
        if (probe.enabled) {
            probe.Update();
        }
    }
}

void DiagnosticSystem::CleanupOldSnapshots() {
    if (m_snapshots.size() > MAX_SNAPSHOTS) {
        size_t toRemove = m_snapshots.size() - MAX_SNAPSHOTS;
        m_snapshots.erase(m_snapshots.begin(), m_snapshots.begin() + toRemove);
    }
}

std::string DiagnosticSystem::GetCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    ss << "." << std::setfill('0') << std::setw(3) << ms.count();

    return ss.str();
}

// Platform-specific implementations (simplified)
size_t DiagnosticSystem::GetCurrentMemoryUsage() {
    // In a real implementation, this would use platform-specific APIs
    // For now, return a simulated value
    return 50 * 1024 * 1024; // 50 MB
}

int DiagnosticSystem::GetActiveThreadCount() {
    // In a real implementation, this would query the thread pool
    // For now, return a simulated value
    return std::thread::hardware_concurrency();
}

float DiagnosticSystem::GetCPUUsage() {
    // In a real implementation, this would use platform-specific APIs
    // For now, return a simulated value
    return 25.0f; // 25% CPU usage
}