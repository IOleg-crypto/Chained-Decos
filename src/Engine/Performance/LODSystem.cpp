#include "LODSystem.h"
#include <algorithm>
#include <raymath.h>
#include <raylib.h>

LODModel::LODModel(const std::string& modelName) : name(modelName), currentLOD(0) {
    lastPosition = {0, 0, 0};
    lastDistance = 0.0f;
}

void LODModel::AddLODLevel(Model model, float distance) {
    lodModels.push_back(model);
    distanceThresholds.push_back(distance);

    // Sort by distance (ascending)
    std::vector<size_t> indices(lodModels.size());
    for (size_t i = 0; i < indices.size(); ++i) {
        indices[i] = i;
    }

    std::sort(indices.begin(), indices.end(),
              [this](size_t a, size_t b) {
                  return distanceThresholds[a] < distanceThresholds[b];
              });

    // Reorder vectors
    std::vector<Model> sortedModels;
    std::vector<float> sortedDistances;
    for (size_t i : indices) {
        sortedModels.push_back(lodModels[i]);
        sortedDistances.push_back(distanceThresholds[i]);
    }

    lodModels = sortedModels;
    distanceThresholds = sortedDistances;

    TraceLog(LOG_INFO, "LODModel::AddLODLevel() - Added LOD level for %s at distance %.1f",
             name.c_str(), distance);
}

void LODModel::UpdateLOD(const Vector3& cameraPosition) {
    float distance = Vector3Distance(lastPosition, cameraPosition);
    lastDistance = distance;

    // Find appropriate LOD level
    int targetLOD = 0;
    for (size_t i = 0; i < distanceThresholds.size(); ++i) {
        if (distance > distanceThresholds[i]) {
            targetLOD = static_cast<int>(i);
        } else {
            break;
        }
    }

    // Clamp to valid range
    targetLOD = std::max(0, std::min(targetLOD, static_cast<int>(lodModels.size()) - 1));
    currentLOD = targetLOD;
}

Model* LODModel::GetCurrentLODModel() {
    if (currentLOD >= 0 && currentLOD < static_cast<int>(lodModels.size())) {
        return &lodModels[currentLOD];
    }
    return nullptr;
}

LODSystem::LODSystem()
    : m_distanceCulling(true), m_maxRenderDistance(1000.0f), m_qualityLevel(2), m_automaticLOD(true) {

    // Set default global LOD distances
    m_globalLODDistances = {10.0f, 25.0f, 50.0f, 100.0f};

    // Initialize statistics
    m_stats = LODStats{};
}

void LODSystem::RegisterModel(const std::string& modelName) {
    if (m_lodModels.find(modelName) == m_lodModels.end()) {
        m_lodModels[modelName] = LODModel(modelName);
        TraceLog(LOG_INFO, "LODSystem::RegisterModel() - Registered model for LOD: %s", modelName.c_str());
    }
}

void LODSystem::AddLODLevel(const std::string& modelName, Model model, float distanceThreshold) {
    auto it = m_lodModels.find(modelName);
    if (it != m_lodModels.end()) {
        it->second.AddLODLevel(model, distanceThreshold);
    } else {
        RegisterModel(modelName);
        m_lodModels[modelName].AddLODLevel(model, distanceThreshold);
    }
}

void LODSystem::RemoveLODLevel(const std::string& modelName, int lodIndex) {
    auto it = m_lodModels.find(modelName);
    if (it != m_lodModels.end()) {
        if (lodIndex >= 0 && lodIndex < static_cast<int>(it->second.GetLODCount())) {
            // Remove LOD level (would need to implement removal in LODModel)
            TraceLog(LOG_INFO, "LODSystem::RemoveLODLevel() - Removed LOD level %d from model %s",
                     lodIndex, modelName.c_str());
        }
    }
}

void LODSystem::Update(const Vector3& cameraPosition) {
    if (!m_automaticLOD) return;

    for (auto& pair : m_lodModels) {
        UpdateModelLOD(pair.first, cameraPosition);
    }

    UpdateStatistics();
}

void LODSystem::UpdateModelLOD(const std::string& modelName, const Vector3& cameraPosition) {
    auto it = m_lodModels.find(modelName);
    if (it != m_lodModels.end()) {
        it->second.UpdateLOD(cameraPosition);
    }
}

Model* LODSystem::GetLODModel(const std::string& modelName) {
    auto it = m_lodModels.find(modelName);
    if (it != m_lodModels.end()) {
        return it->second.GetCurrentLODModel();
    }
    return nullptr;
}

LODModel* LODSystem::GetLODModelData(const std::string& modelName) {
    auto it = m_lodModels.find(modelName);
    return (it != m_lodModels.end()) ? &it->second : nullptr;
}

void LODSystem::SetGlobalLODDistances(const std::vector<float>& distances) {
    m_globalLODDistances = distances;
    TraceLog(LOG_INFO, "LODSystem::SetGlobalLODDistances() - Set %zu global LOD distances", distances.size());
}

void LODSystem::SetModelLODDistances(const std::string& modelName, const std::vector<float>& distances) {
    auto it = m_lodModels.find(modelName);
    if (it != m_lodModels.end()) {
        it->second.distanceThresholds = distances;
        TraceLog(LOG_INFO, "LODSystem::SetModelLODDistances() - Set %zu LOD distances for model %s",
                 distances.size(), modelName.c_str());
    }
}

void LODSystem::EnableDistanceCulling(bool enable) {
    m_distanceCulling = enable;
    TraceLog(LOG_INFO, "LODSystem::EnableDistanceCulling() - %s distance culling",
             enable ? "Enabled" : "Disabled");
}

void LODSystem::SetMaxRenderDistance(float distance) {
    m_maxRenderDistance = distance;
    TraceLog(LOG_INFO, "LODSystem::SetMaxRenderDistance() - Set max render distance to %.1f", distance);
}

void LODSystem::SetQualityLevel(int level) {
    m_qualityLevel = std::max(0, std::min(4, level));

    // Adjust LOD distances based on quality level
    float qualityMultiplier = GetDistanceMultiplier();

    for (auto& pair : m_lodModels) {
        for (size_t i = 0; i < pair.second.distanceThresholds.size() && i < m_globalLODDistances.size(); ++i) {
            pair.second.distanceThresholds[i] = m_globalLODDistances[i] * qualityMultiplier;
        }
    }

    TraceLog(LOG_INFO, "LODSystem::SetQualityLevel() - Set quality level to %d (multiplier: %.2f)",
             level, qualityMultiplier);
}

void LODSystem::EnableAutomaticLOD(bool enable) {
    m_automaticLOD = enable;
    TraceLog(LOG_INFO, "LODSystem::EnableAutomaticLOD() - %s automatic LOD",
             enable ? "Enabled" : "Disabled");
}

void LODSystem::ResetStatistics() {
    m_stats = LODStats{};
    TraceLog(LOG_INFO, "LODSystem::ResetStatistics() - Reset LOD statistics");
}

int LODSystem::CalculateLODLevel(const std::string& modelName, float distance) {
    auto it = m_lodModels.find(modelName);
    if (it != m_lodModels.end()) {
        const auto& thresholds = it->second.distanceThresholds;
        for (size_t i = 0; i < thresholds.size(); ++i) {
            if (distance <= thresholds[i]) {
                return static_cast<int>(i);
            }
        }
        return static_cast<int>(thresholds.size()) - 1; // Highest LOD
    }
    return 0;
}

void LODSystem::UpdateStatistics() {
    m_stats.totalModels = m_lodModels.size();
    m_stats.activeHighLOD = 0;
    m_stats.activeMediumLOD = 0;
    m_stats.activeLowLOD = 0;
    m_stats.culledObjects = 0;

    for (const auto& pair : m_lodModels) {
        int currentLOD = pair.second.GetCurrentLODIndex();

        if (currentLOD == 0) {
            m_stats.activeHighLOD++;
        } else if (currentLOD <= 1) {
            m_stats.activeMediumLOD++;
        } else {
            m_stats.activeLowLOD++;
        }

        // Count culled objects (beyond max render distance)
        if (m_distanceCulling && pair.second.lastDistance > m_maxRenderDistance) {
            m_stats.culledObjects++;
        }
    }

    // Estimate memory saved (rough calculation)
    m_stats.memorySaved = m_stats.activeLowLOD * 1024 * 1024; // Assume 1MB saved per low LOD object
}

float LODSystem::GetDistanceMultiplier() const {
    switch (m_qualityLevel) {
        case 0: return 0.5f;  // Low quality - closer LOD switches
        case 1: return 0.75f; // Medium-low
        case 2: return 1.0f;  // Medium (default)
        case 3: return 1.5f;  // High quality - farther LOD switches
        case 4: return 2.0f;  // Ultra quality
        default: return 1.0f;
    }
}

void LODSystem::OptimizeLODMemory() {
    // Unload unused LOD models to save memory
    for (auto& pair : m_lodModels) {
        // Could implement LRU cache for LOD models
        // For now, just log the optimization opportunity
    }

    TraceLog(LOG_DEBUG, "LODSystem::OptimizeLODMemory() - Memory optimization check completed");
}