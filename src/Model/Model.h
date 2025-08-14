//
// Created by I#Oleg
//

#ifndef MODEL_H
#define MODEL_H

#include <Model/JsonHelper.h>
#include <Model/ModelCache.h>
#include <Model/ModelConfig.h>
#include <Model/ModelInstance.h>

#include <Model/Animation.h>
#include <memory>
#include <nlohmann/json.hpp>
#include <raylib.h>
#include <raymath.h>
#include <string>
#include <unordered_map>
#include <vector>

using json = nlohmann::json;

// Enhanced Models Manager with caching, statistics and better organization
// Features: model caching, loading stats, categorization, LOD support, error handling
class Models
{
public:
    // Model constants
    static constexpr int CACHE_SIZE = 20;
    static constexpr bool LOD_ENABLED = false;
    static constexpr bool CACHE_ENABLED = true;
    Models();
    ~Models();

    // ==================== CORE METHODS ====================

    // Load models from JSON config
    void LoadModelsFromJson(const std::string &path);

    // Render all model instances
    void DrawAllModels() const;

    // Get model by name
    Model &GetModelByName(const std::string &name);

    // Add instance (legacy method)
    void AddInstance(const json &instanceJson, Model *modelPtr, const std::string &modelName,
                     Animation *animation);

    // ==================== ENHANCED METHODS ====================

    // Add instance with enhanced config
    bool AddInstanceEx(const std::string &modelName, const ModelInstanceConfig &config);

    // Model management
    bool LoadSingleModel(const std::string &name, const std::string &path, bool preload = true);
    bool UnloadModel(const std::string &name);
    bool ReloadModel(const std::string &name);

    // Filtering and search
    std::vector<ModelInstance *> GetInstancesByTag(const std::string &tag);
    std::vector<ModelInstance *> GetInstancesByCategory(const std::string &category);
    std::vector<std::string> GetAvailableModels() const;

    // Configuration access
    bool HasCollision(const std::string &modelName) const;

    // Statistics and monitoring
    const LoadingStats &GetLoadingStats() const { return m_stats; }
    void PrintStatistics() const;
    void PrintCacheInfo() const;

    // Settings
    void SetCacheEnabled(bool enabled) { m_cacheEnabled = enabled; }
    void SetMaxCacheSize(size_t maxSize);
    void EnableLOD(bool enabled) { m_lodEnabled = enabled; }

    // Cleanup and optimization
    void CleanupUnusedModels();
    void OptimizeCache();

private:
    // ==================== LEGACY FIELDS ====================
    std::vector<ModelInstance> m_instances;                  // All model instances
    std::unordered_map<std::string, Model *> m_modelByName;  // Models by name
    std::unordered_map<std::string, Animation> m_animations; // Animation data
    bool m_spawnInstance = true;                             // Auto spawn instances

    // ==================== ENHANCED FIELDS ====================
    std::shared_ptr<ModelCache> m_cache;                        // Model cache
    std::unordered_map<std::string, ModelFileConfig> m_configs; // Model configs
    LoadingStats m_stats;                                       // Loading statistics

    // Settings
    bool m_cacheEnabled = true;
    bool m_lodEnabled = false;
    float m_lodDistance = 100.0f;

    // ==================== PRIVATE HELPER METHODS ====================
    bool LoadModelFromFile(const std::string &name, const std::string &path);
    void ProcessModelConfig(const ModelFileConfig &config);
    bool ProcessModelConfigLegacy(const ModelFileConfig &config); // Legacy compatibility
    void CreateInstancesFromConfig(const ModelFileConfig &config, Model *model);
    bool ValidateModelPath(const std::string &path) const;
    void UpdateStatistics();

    // Legacy helpers
    Vector3 ParseVector3FromJson(const json &j, const Vector3 &defaultValue = {0, 0, 0});
    Color ParseColorFromJson(const json &j, const Color &defaultValue = WHITE);
};

#endif // MODEL_H
