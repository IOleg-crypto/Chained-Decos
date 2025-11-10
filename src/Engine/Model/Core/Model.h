//
// Created by I#Oleg
//

#ifndef MODEL_H
#define MODEL_H

#include "../Parser/JsonParser.h"
#include "../Cache/ModelCache.h"
#include "../Config/ModelConfig.h"
#include "ModelInstance.h"

#include "../Animation/Animation.h"
#include <memory>
#include <nlohmann/json.hpp>
#include <raylib.h>
#include <raymath.h>
#include <string>
#include <unordered_map>
#include <vector>

using json = nlohmann::json;

// Model loader with caching and statistics
class ModelLoader
{
public:
    // Model constants
    static constexpr int CACHE_SIZE = 20;
    static constexpr bool LOD_ENABLED = false;
    static constexpr bool CACHE_ENABLED = true;
    ModelLoader();
    ~ModelLoader();

    // ==================== CORE METHODS ====================

    // Load models from JSON config
    struct LoadResult {
        int totalModels;
        int loadedModels;
        int failedModels;
        float loadingTime;
    };
    
    std::optional<LoadResult> LoadModelsFromJson(const std::string &path);

    // Load only specific models from JSON config
    std::optional<LoadResult> LoadModelsFromJsonSelective(const std::string &path, const std::vector<std::string> &modelNames);

    // Set selective loading mode
    void SetSelectiveMode(bool enabled);

    // Render all model instances
    void DrawAllModels() const;

    // Get model by name
    std::optional<std::reference_wrapper<Model>> GetModelByName(const std::string &name);

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
    // Register a raylib::Model that was already loaded elsewhere (e.g. MapLoader)
    bool RegisterLoadedModel(const std::string &name, const ::Model &model);

    // Filtering and search
    std::vector<ModelInstance *> GetInstancesByTag(const std::string &tag);
    std::vector<ModelInstance *> GetInstancesByCategory(const std::string &category);
    [[nodiscard]] std::vector<std::string> GetAvailableModels() const;

    // Configuration access
    [[nodiscard]] bool HasCollision(const std::string &modelName) const;

    // Statistics and monitoring
    [[nodiscard]] const LoadingStats &GetLoadingStats() const;

    void PrintStatistics() const;
    void PrintCacheInfo() const;

    // Settings
    void SetCacheEnabled(bool enabled);

    void SetMaxCacheSize(size_t maxSize) const;
    void EnableLOD(bool enabled);

    // Configuration access
    [[nodiscard]] const ModelFileConfig *GetModelConfig(const std::string &modelName) const;

    // Cleanup and optimization
    void CleanupUnusedModels() const;
    void OptimizeCache() const;
    
    // Clear all model instances (useful when loading new maps)
    void ClearInstances();

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
    bool m_selectiveMode = false;

    // ==================== PRIVATE HELPER METHODS ====================
    bool ProcessModelConfigLegacy(const ModelFileConfig &config); // Legacy compatibility
    bool ValidateModelPath(const std::string &path) const;

    // Validation helper functions for crash prevention
    static bool IsValidVector3(const Vector3& v);
    static bool IsValidColor(const Color& c);
    static bool IsValidMatrix(const Matrix& m);

};

#endif // MODEL_H