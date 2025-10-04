#include "PerformanceManager.h"
#include <algorithm>
#include <numeric>
#include <raymath.h>
#include <raylib.h>

PerformanceManager::PerformanceManager()
    : m_currentFPS(60.0f), m_frustumDirty(true), m_occlusionCulling(false), m_occlusionQuality(1),
      m_memoryLimit(1024 * 1024 * 1024), m_peakMemoryUsage(0), m_targetFPS(60), m_vsyncEnabled(true),
      m_adaptiveQuality(false), m_qualityScale(1.0f), m_performanceWarningThreshold(33.0f),
      m_performanceWarning(false) {

    // Initialize frustum planes
    for (int i = 0; i < 6; i++) {
        m_frustumPlanes[i].normal = {0, 0, 0};
        m_frustumPlanes[i].distance = 0;
    }

    // Initialize statistics
    m_stats.minFrameTime = 999.0f;
    m_stats.maxFrameTime = 0.0f;
    m_stats.avgFrameTime = 16.67f; // 60 FPS
    m_stats.totalFrames = 0;
    m_stats.slowFrames = 0;
}

void PerformanceManager::StartFrame() {
    m_frameStartTime = std::chrono::steady_clock::now();
    m_currentMetrics = FrameMetrics{};
    m_performanceWarning = false;
}

void PerformanceManager::EndFrame() {
    auto frameEndTime = std::chrono::steady_clock::now();
    m_currentMetrics.frameTime = std::chrono::duration<float, std::milli>(frameEndTime - m_frameStartTime).count();

    // Update FPS
    m_currentFPS = 1000.0f / m_currentMetrics.frameTime;

    // Update frame history
    m_frameHistory.push_back(m_currentMetrics);
    if (m_frameHistory.size() > MAX_FRAME_HISTORY) {
        m_frameHistory.erase(m_frameHistory.begin());
    }

    // Update average metrics
    UpdateAverageMetrics();

    // Update statistics
    m_stats.totalFrames++;
    m_stats.minFrameTime = std::min(m_stats.minFrameTime, m_currentMetrics.frameTime);
    m_stats.maxFrameTime = std::max(m_stats.maxFrameTime, m_currentMetrics.frameTime);
    if (m_currentMetrics.frameTime > m_performanceWarningThreshold) {
        m_stats.slowFrames++;
    }

    // Check performance thresholds
    CheckPerformanceThresholds();

    // Update adaptive quality if enabled
    if (m_adaptiveQuality) {
        UpdateAdaptiveQuality();
    }

    TraceLog(LOG_DEBUG, "PerformanceManager::EndFrame() - Frame time: %.2fms, FPS: %.1f",
             m_currentMetrics.frameTime, m_currentFPS);
}

void PerformanceManager::StartPhase(const std::string& phaseName) {
    m_phaseTimers[phaseName] = std::chrono::steady_clock::now();
}

void PerformanceManager::EndPhase(const std::string& phaseName) {
    auto it = m_phaseTimers.find(phaseName);
    if (it != m_phaseTimers.end()) {
        float phaseTime = std::chrono::duration<float, std::milli>(
            std::chrono::steady_clock::now() - it->second).count();

        if (phaseName == "update") m_currentMetrics.updateTime = phaseTime;
        else if (phaseName == "render") m_currentMetrics.renderTime = phaseTime;
        else if (phaseName == "collision") m_currentMetrics.collisionTime = phaseTime;
        else if (phaseName == "audio") m_currentMetrics.audioTime = phaseTime;
    }
}

void PerformanceManager::RecordDrawCall() {
    m_currentMetrics.drawCalls++;
}

void PerformanceManager::RecordTriangles(int count) {
    m_currentMetrics.trianglesRendered += count;
}

void PerformanceManager::RecordMemoryUsage(size_t bytes) {
    m_currentMetrics.memoryUsage = bytes;
    m_peakMemoryUsage = std::max(m_peakMemoryUsage, bytes);
}

void PerformanceManager::RecordActiveObjects(int count) {
    m_currentMetrics.activeObjects = count;
}

void PerformanceManager::AddLODLevel(const std::string& modelName, const LODLevel& lod) {
    m_lodLevels[modelName].push_back(lod);

    // Sort LOD levels by distance (ascending)
    std::sort(m_lodLevels[modelName].begin(), m_lodLevels[modelName].end(),
              [](const LODLevel& a, const LODLevel& b) {
                  return a.distance < b.distance;
              });

    TraceLog(LOG_INFO, "PerformanceManager::AddLODLevel() - Added LOD '%s' for model '%s' at distance %.1f",
             lod.name.c_str(), modelName.c_str(), lod.distance);
}

void PerformanceManager::RemoveLODLevel(const std::string& modelName, const std::string& lodName) {
    auto it = m_lodLevels.find(modelName);
    if (it != m_lodLevels.end()) {
        it->second.erase(
            std::remove_if(it->second.begin(), it->second.end(),
                          [&lodName](const LODLevel& lod) { return lod.name == lodName; }),
            it->second.end()
        );

        TraceLog(LOG_INFO, "PerformanceManager::RemoveLODLevel() - Removed LOD '%s' from model '%s'",
                 lodName.c_str(), modelName.c_str());
    }
}

LODLevel* PerformanceManager::GetLODForDistance(const std::string& modelName, float distance) {
    auto it = m_lodLevels.find(modelName);
    if (it != m_lodLevels.end()) {
        for (auto& lod : it->second) {
            if (distance <= lod.distance && lod.enabled) {
                return &lod;
            }
        }
        // Return highest quality LOD if none found
        if (!it->second.empty()) {
            return &it->second.back();
        }
    }
    return nullptr;
}

std::vector<LODLevel> PerformanceManager::GetLODLevels(const std::string& modelName) const {
    auto it = m_lodLevels.find(modelName);
    return (it != m_lodLevels.end()) ? it->second : std::vector<LODLevel>{};
}

void PerformanceManager::SetCamera(const Camera3D& camera) {
    m_camera = camera;
    m_frustumDirty = true;
}

void PerformanceManager::UpdateFrustum() {
    if (m_frustumDirty) {
        CalculateFrustumPlanes();
        m_frustumDirty = false;
    }
}

bool PerformanceManager::IsInFrustum(const Vector3& position, float radius) const {
    return SphereInFrustum(position, radius);
}

bool PerformanceManager::IsInFrustum(const BoundingBox& box) const {
    return BoxInFrustum(box);
}

void PerformanceManager::EnableOcclusionCulling(bool enable) {
    m_occlusionCulling = enable;
    TraceLog(LOG_INFO, "PerformanceManager::EnableOcclusionCulling() - %s occlusion culling",
             enable ? "Enabled" : "Disabled");
}

void PerformanceManager::SetOcclusionQuality(int quality) {
    m_occlusionQuality = std::max(0, std::min(3, quality));
    TraceLog(LOG_INFO, "PerformanceManager::SetOcclusionQuality() - Set occlusion quality to %d", quality);
}

void PerformanceManager::SetMemoryLimit(size_t maxMemory) {
    m_memoryLimit = maxMemory;
    TraceLog(LOG_INFO, "PerformanceManager::SetMemoryLimit() - Set memory limit to %.1f MB",
             maxMemory / (1024.0f * 1024.0f));
}

void PerformanceManager::TriggerGarbageCollection() {
    // Force garbage collection (implementation depends on memory management system)
    TraceLog(LOG_INFO, "PerformanceManager::TriggerGarbageCollection() - Triggered garbage collection");
}

void PerformanceManager::SetTargetFPS(int fps) {
    m_targetFPS = fps;
    SetTargetFPS(fps); // Raylib function
    TraceLog(LOG_INFO, "PerformanceManager::SetTargetFPS() - Set target FPS to %d", fps);
}

void PerformanceManager::EnableVSync(bool enable) {
    m_vsyncEnabled = enable;
    if (enable) {
        // Enable VSync through appropriate means
        TraceLog(LOG_INFO, "PerformanceManager::EnableVSync() - Enabled VSync");
    } else {
        TraceLog(LOG_INFO, "PerformanceManager::EnableVSync() - Disabled VSync");
    }
}

void PerformanceManager::EnableAdaptiveQuality(bool enable) {
    m_adaptiveQuality = enable;
    if (enable) {
        TraceLog(LOG_INFO, "PerformanceManager::EnableAdaptiveQuality() - Enabled adaptive quality");
    } else {
        m_qualityScale = 1.0f; // Reset to full quality
        TraceLog(LOG_INFO, "PerformanceManager::EnableAdaptiveQuality() - Disabled adaptive quality");
    }
}

void PerformanceManager::SetQualityScale(float scale) {
    m_qualityScale = std::max(0.1f, std::min(2.0f, scale));
    TraceLog(LOG_INFO, "PerformanceManager::SetQualityScale() - Set quality scale to %.2f", scale);
}

void PerformanceManager::SetPerformanceWarningThreshold(float frameTimeMs) {
    m_performanceWarningThreshold = frameTimeMs;
    TraceLog(LOG_INFO, "PerformanceManager::SetPerformanceWarningThreshold() - Set threshold to %.1fms",
             frameTimeMs);
}

std::string PerformanceManager::GetPerformanceReport() const {
    std::string report = "Performance Report:\n";
    report += "Frame Time: " + std::to_string(m_currentMetrics.frameTime) + "ms\n";
    report += "FPS: " + std::to_string(static_cast<int>(m_currentFPS)) + "\n";
    report += "Draw Calls: " + std::to_string(m_currentMetrics.drawCalls) + "\n";
    report += "Triangles: " + std::to_string(m_currentMetrics.trianglesRendered) + "\n";
    report += "Memory: " + std::to_string(m_currentMetrics.memoryUsage / (1024.0f * 1024.0f)) + " MB\n";
    report += "Active Objects: " + std::to_string(m_currentMetrics.activeObjects) + "\n";
    report += "Quality Scale: " + std::to_string(m_qualityScale) + "\n";
    return report;
}

void PerformanceManager::ResetStatistics() {
    m_stats = PerformanceStats{};
    m_stats.minFrameTime = 999.0f;
    m_stats.avgFrameTime = 16.67f;
    m_frameHistory.clear();
    TraceLog(LOG_INFO, "PerformanceManager::ResetStatistics() - Reset performance statistics");
}

void PerformanceManager::UpdateFrustumPlanes() {
    CalculateFrustumPlanes();
}

void PerformanceManager::CalculateFrustumPlanes() {
    // Extract frustum planes from camera projection and view matrices
    Matrix proj = MatrixPerspective(m_camera.fovy * DEG2RAD, 16.0f/9.0f, 0.1f, 1000.0f);
    Matrix view = MatrixLookAt(m_camera.position, m_camera.target, m_camera.up);

    Matrix viewProj = MatrixMultiply(view, proj);

    // Extract planes from view-projection matrix
    // Near plane
    m_frustumPlanes[0].normal = {viewProj.m12, viewProj.m13, viewProj.m14};
    m_frustumPlanes[0].distance = viewProj.m15;
    m_frustumPlanes[0].normal = Vector3Normalize(m_frustumPlanes[0].normal);
    m_frustumPlanes[0].distance = viewProj.m15 / Vector3Length({viewProj.m12, viewProj.m13, viewProj.m14});

    // Far plane
    m_frustumPlanes[1].normal = {-viewProj.m12, -viewProj.m13, -viewProj.m14};
    m_frustumPlanes[1].distance = -viewProj.m15 / Vector3Length({viewProj.m12, viewProj.m13, viewProj.m14});

    // Left plane
    m_frustumPlanes[2].normal = {viewProj.m10 + viewProj.m12, viewProj.m11 + viewProj.m13, viewProj.m12 + viewProj.m14};
    m_frustumPlanes[2].distance = viewProj.m13 + viewProj.m15;
    m_frustumPlanes[2].normal = Vector3Normalize(m_frustumPlanes[2].normal);
    m_frustumPlanes[2].distance = (viewProj.m13 + viewProj.m15) / Vector3Length({viewProj.m10 + viewProj.m12, viewProj.m11 + viewProj.m13, viewProj.m12 + viewProj.m14});

    // Right plane
    m_frustumPlanes[3].normal = {-viewProj.m10 + viewProj.m12, -viewProj.m11 + viewProj.m13, -viewProj.m12 + viewProj.m14};
    m_frustumPlanes[3].distance = -viewProj.m13 + viewProj.m15;
    m_frustumPlanes[3].normal = Vector3Normalize(m_frustumPlanes[3].normal);
    m_frustumPlanes[3].distance = (-viewProj.m13 + viewProj.m15) / Vector3Length({-viewProj.m10 + viewProj.m12, -viewProj.m11 + viewProj.m13, -viewProj.m12 + viewProj.m14});

    // Top plane
    m_frustumPlanes[4].normal = {-viewProj.m10 + viewProj.m12, -viewProj.m11 + viewProj.m13, -viewProj.m12 + viewProj.m14};
    m_frustumPlanes[4].distance = -viewProj.m13 + viewProj.m15;
    m_frustumPlanes[4].normal = Vector3Normalize(m_frustumPlanes[4].normal);
    m_frustumPlanes[4].distance = (-viewProj.m13 + viewProj.m15) / Vector3Length({-viewProj.m10 + viewProj.m12, -viewProj.m11 + viewProj.m13, -viewProj.m12 + viewProj.m14});

    // Bottom plane
    m_frustumPlanes[5].normal = {viewProj.m10 + viewProj.m12, viewProj.m11 + viewProj.m13, viewProj.m12 + viewProj.m14};
    m_frustumPlanes[5].distance = viewProj.m13 + viewProj.m15;
    m_frustumPlanes[5].normal = Vector3Normalize(m_frustumPlanes[5].normal);
    m_frustumPlanes[5].distance = (viewProj.m13 + viewProj.m15) / Vector3Length({viewProj.m10 + viewProj.m12, viewProj.m11 + viewProj.m13, viewProj.m12 + viewProj.m14});
}

bool PerformanceManager::PointInFrustum(const Vector3& point) const {
    for (int i = 0; i < 6; i++) {
        if (Vector3DotProduct(m_frustumPlanes[i].normal, point) + m_frustumPlanes[i].distance <= 0) {
            return false;
        }
    }
    return true;
}

bool PerformanceManager::SphereInFrustum(const Vector3& center, float radius) const {
    for (int i = 0; i < 6; i++) {
        if (Vector3DotProduct(m_frustumPlanes[i].normal, center) + m_frustumPlanes[i].distance <= -radius) {
            return false;
        }
    }
    return true;
}

bool PerformanceManager::BoxInFrustum(const BoundingBox& box) const {
    // Check all 8 corners of the box
    Vector3 corners[8] = {
        {box.min.x, box.min.y, box.min.z},
        {box.max.x, box.min.y, box.min.z},
        {box.min.x, box.max.y, box.min.z},
        {box.max.x, box.max.y, box.min.z},
        {box.min.x, box.min.y, box.max.z},
        {box.max.x, box.min.y, box.max.z},
        {box.min.x, box.max.y, box.max.z},
        {box.max.x, box.max.y, box.max.z}
    };

    for (int i = 0; i < 6; i++) {
        int out = 0;
        for (int j = 0; j < 8; j++) {
            if (Vector3DotProduct(m_frustumPlanes[i].normal, corners[j]) + m_frustumPlanes[i].distance < 0) {
                out++;
            }
        }
        if (out == 8) return false; // All points outside this plane
    }
    return true;
}

void PerformanceManager::UpdateAverageMetrics() {
    if (m_frameHistory.empty()) return;

    FrameMetrics total = {};
    for (const auto& metrics : m_frameHistory) {
        total.frameTime += metrics.frameTime;
        total.updateTime += metrics.updateTime;
        total.renderTime += metrics.renderTime;
        total.collisionTime += metrics.collisionTime;
        total.audioTime += metrics.audioTime;
        total.drawCalls += metrics.drawCalls;
        total.trianglesRendered += metrics.trianglesRendered;
        total.memoryUsage += metrics.memoryUsage;
        total.activeObjects += metrics.activeObjects;
    }

    size_t count = m_frameHistory.size();
    m_averageMetrics.frameTime = total.frameTime / count;
    m_averageMetrics.updateTime = total.updateTime / count;
    m_averageMetrics.renderTime = total.renderTime / count;
    m_averageMetrics.collisionTime = total.collisionTime / count;
    m_averageMetrics.audioTime = total.audioTime / count;
    m_averageMetrics.drawCalls = total.drawCalls / count;
    m_averageMetrics.trianglesRendered = total.trianglesRendered / count;
    m_averageMetrics.memoryUsage = total.memoryUsage / count;
    m_averageMetrics.activeObjects = total.activeObjects / count;
}

void PerformanceManager::CheckPerformanceThresholds() {
    // Check if performance is below acceptable levels
    if (m_currentMetrics.frameTime > m_performanceWarningThreshold) {
        m_performanceWarning = true;

        // Could trigger quality reduction or other optimizations
        if (m_adaptiveQuality && m_qualityScale > 0.5f) {
            m_qualityScale *= 0.9f; // Reduce quality by 10%
            TraceLog(LOG_WARNING, "PerformanceManager::CheckPerformanceThresholds() - Reduced quality due to performance");
        }
    }
}

void PerformanceManager::UpdateAdaptiveQuality() {
    // Adjust quality based on performance
    float targetFrameTime = 1000.0f / m_targetFPS;

    if (m_currentMetrics.frameTime > targetFrameTime * 1.2f) {
        // Performance is poor, reduce quality
        m_qualityScale = std::max(0.3f, m_qualityScale * 0.95f);
    } else if (m_currentMetrics.frameTime < targetFrameTime * 0.8f) {
        // Performance is good, can increase quality
        m_qualityScale = std::min(1.5f, m_qualityScale * 1.02f);
    }
}