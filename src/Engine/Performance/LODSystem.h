#ifndef LOD_SYSTEM_H
#define LOD_SYSTEM_H

#include <vector>
#include <string>
#include <unordered_map>
#include <raylib.h>
#include "../Model/Model.h"

struct LODModel {
    std::string name;
    std::vector<Model> lodModels;
    std::vector<float> distanceThresholds;
    int currentLOD;
    Vector3 lastPosition;
    float lastDistance;

    LODModel(const std::string& modelName);
    void AddLODLevel(Model model, float distance);
    void UpdateLOD(const Vector3& cameraPosition);
    Model* GetCurrentLODModel();
    int GetCurrentLODIndex() const { return currentLOD; }
    size_t GetLODCount() const { return lodModels.size(); }
};

class LODSystem {
public:
    LODSystem();
    ~LODSystem() = default;

    // LOD management
    void RegisterModel(const std::string& modelName);
    void AddLODLevel(const std::string& modelName, Model model, float distanceThreshold);
    void RemoveLODLevel(const std::string& modelName, int lodIndex);

    // Update system
    void Update(const Vector3& cameraPosition);
    void UpdateModelLOD(const std::string& modelName, const Vector3& modelPosition);

    // Model retrieval
    Model* GetLODModel(const std::string& modelName);
    LODModel* GetLODModelData(const std::string& modelName);

    // Performance settings
    void SetGlobalLODDistances(const std::vector<float>& distances);
    void SetModelLODDistances(const std::string& modelName, const std::vector<float>& distances);
    void EnableDistanceCulling(bool enable);
    void SetMaxRenderDistance(float distance);

    // Quality settings
    void SetQualityLevel(int level); // 0-4 (Low to Ultra)
    int GetQualityLevel() const { return m_qualityLevel; }
    void EnableAutomaticLOD(bool enable);
    bool IsAutomaticLODEnabled() const { return m_automaticLOD; }

    // Statistics
    struct LODStats {
        int totalModels;
        int activeHighLOD;
        int activeMediumLOD;
        int activeLowLOD;
        float memorySaved;
        int culledObjects;
    };

    LODStats GetStatistics() const { return m_stats; }
    void ResetStatistics();

private:
    std::unordered_map<std::string, LODModel> m_lodModels;
    std::vector<float> m_globalLODDistances;
    bool m_distanceCulling;
    float m_maxRenderDistance;
    int m_qualityLevel;
    bool m_automaticLOD;

    // Statistics
    mutable LODStats m_stats;

    // Helper methods
    int CalculateLODLevel(const std::string& modelName, float distance);
    void UpdateStatistics();
    float GetDistanceMultiplier() const;
    void OptimizeLODMemory();
};

#endif // LOD_SYSTEM_H