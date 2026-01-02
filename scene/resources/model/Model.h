//
// Created by I#Oleg
//

#ifndef CD_SCENE_RESOURCES_MODEL_MODEL_H
#define CD_SCENE_RESOURCES_MODEL_MODEL_H

#include "Animation.h"
#include "interfaces/i_model_loader.h"
#include "json_parser.h"
#include "model_cache.h"
#include "model_config.h"
#include "model_instance.h"
#include <memory>
#include <nlohmann/json.hpp>
#include <raylib.h>
#include <raymath.h>
#include <string>
#include <unordered_map>
#include <vector>


using json = nlohmann::json;

namespace CHEngine
{

// Information about a model file found on disk
struct ModelResourceInfo
{
    std::string name;
    std::string path;
};

// Model loader with caching and statistics
class ModelLoader
{
public:
    // Model loader results
    struct LoadResult
    {
        int totalModels = 0;
        int loadedModels = 0;
        int failedModels = 0;
        float loadingTime = 0.0f;
    };

public:
    // Model constants
    static constexpr int CACHE_SIZE = 20;
    static constexpr bool LOD_ENABLED = false;
    static constexpr bool CACHE_ENABLED = true;
    static void Init();
    static void Shutdown();
    static bool IsInitialized();

    static std::optional<LoadResult> LoadModelsFromJson(const std::string &path);
    static std::optional<LoadResult>
    LoadModelsFromJsonSelective(const std::string &path,
                                const std::vector<std::string> &modelNames);
    static void SetSelectiveMode(bool enabled);
    static void DrawAllModels();
    static std::optional<std::reference_wrapper<::Model>> GetModelByName(const std::string &name);
    static bool AddInstanceEx(const std::string &modelName, const ModelInstanceConfig &config);
    static std::optional<LoadResult> LoadGameModels();
    static std::optional<LoadResult>
    LoadGameModelsSelective(const std::vector<std::string> &modelNames);
    static std::optional<LoadResult>
    LoadGameModelsSelectiveSafe(const std::vector<std::string> &modelNames);

    static bool LoadSingleModel(const std::string &name, const std::string &path,
                                bool preload = true);
    static bool UnloadModel(const std::string &name);
    static void UnloadAllModels();
    static bool ReloadModel(const std::string &name);
    static bool RegisterLoadedModel(const std::string &name, const ::Model &model);

    static std::vector<ModelInstance *> GetInstancesByTag(const std::string &tag);
    static std::vector<ModelInstance *> GetInstancesByCategory(const std::string &category);
    static std::vector<std::string> GetAvailableModels();

    static bool HasCollision(const std::string &modelName);
    static const LoadingStats &GetLoadingStats();

    static void PrintStatistics();
    static void PrintCacheInfo();
    static void SetCacheEnabled(bool enabled);
    static void SetMaxCacheSize(size_t maxSize);
    static void EnableLOD(bool enabled);
    static const ModelFileConfig *GetModelConfig(const std::string &modelName);
    static void CleanupUnusedModels();
    static void OptimizeCache();
    static void ClearInstances();

public:
    ModelLoader();
    ~ModelLoader();

    bool InternalInitialize();
    void InternalShutdown();

    std::optional<LoadResult> InternalLoadModelsFromJson(const std::string &path);
    std::optional<LoadResult>
    InternalLoadModelsFromJsonSelective(const std::string &path,
                                        const std::vector<std::string> &modelNames);

    // ==================== CORE METHODS ====================

    // Add instance (legacy method)
    void AddInstance(const json &instanceJson, ::Model *modelPtr, const std::string &modelName,
                     Animation *animation);

    // ==================== GAME MODEL LOADING ====================

    /**
     * @brief Load all game models from resources directory
     *
     * Scans the resources directory for all available models and loads them.
     * Configures cache and LOD settings for optimal game performance.
     *
     * @return LoadResult with statistics, or nullopt on failure
     */
    std::optional<LoadResult> InternalLoadGameModels();
    std::optional<LoadResult>
    InternalLoadGameModelsSelective(const std::vector<std::string> &modelNames);
    std::optional<LoadResult>
    InternalLoadGameModelsSelectiveSafe(const std::vector<std::string> &modelNames);

    bool InternalLoadSingleModel(const std::string &name, const std::string &path,
                                 bool preload = true);
    bool InternalUnloadModel(const std::string &name);
    void InternalUnloadAllModels();
    bool InternalReloadModel(const std::string &name);
    bool InternalRegisterLoadedModel(const std::string &name, const ::Model &model);

    std::vector<ModelInstance *> InternalGetInstancesByTag(const std::string &tag);
    std::vector<ModelInstance *> InternalGetInstancesByCategory(const std::string &category);
    [[nodiscard]] std::vector<std::string> InternalGetAvailableModels() const;

    [[nodiscard]] bool InternalHasCollision(const std::string &modelName) const;
    [[nodiscard]] const LoadingStats &InternalGetLoadingStats() const;

    void InternalPrintStatistics() const;
    void InternalPrintCacheInfo() const;
    void InternalSetCacheEnabled(bool enabled);
    void InternalSetMaxCacheSize(size_t maxSize) const;
    void InternalEnableLOD(bool enabled);
    [[nodiscard]] const ModelFileConfig *InternalGetModelConfig(const std::string &modelName) const;
    void InternalCleanupUnusedModels() const;
    void InternalOptimizeCache() const;
    void InternalClearInstances();

    std::optional<std::reference_wrapper<::Model>> InternalGetModelByName(const std::string &name);
    void InternalDrawAllModels() const;
    bool InternalAddInstanceEx(const std::string &modelName, const ModelInstanceConfig &config);
    void InternalSetSelectiveMode(bool enabled);

private:
    // ==================== LEGACY FIELDS ====================
    std::vector<ModelInstance> m_instances;                   // All model instances
    std::unordered_map<std::string, ::Model *> m_modelByName; // Models by name
    std::unordered_map<std::string, Animation> m_animations;  // Animation data
    bool m_spawnInstance = true;                              // Auto spawn instances

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
    bool InternalProcessModelConfigLegacy(const ModelFileConfig &config); // Legacy compatibility
    bool ValidateModelPath(const std::string &path) const;

    // Validation helper functions for crash prevention
    static bool IsValidVector3(const Vector3 &v);
    static bool IsValidColor(const Color &c);
    static bool IsValidMatrix(const Matrix &m);
};

} // namespace CHEngine

#endif // CD_SCENE_RESOURCES_MODEL_MODEL_H
