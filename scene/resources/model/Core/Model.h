//
// Created by I#Oleg
//

#ifndef MODEL_H
#define MODEL_H

#include "../Animation/Animation.h"
#include "../Cache/ModelCache.h"
#include "../Config/ModelConfig.h"
#include "../Interfaces/IModelLoader.h"
#include "../Parser/JsonParser.h"
#include "ModelInstance.h"
#include <memory>
#include <nlohmann/json.hpp>
#include <raylib.h>
#include <raymath.h>
#include <string>
#include <unordered_map>
#include <vector>

using json = nlohmann::json;

#include "../../Kernel/Interfaces/IKernelService.h"

// Model loader with caching and statistics
class ModelLoader : public IModelLoader, public IKernelService
{
public:
    // Model constants
    static constexpr int CACHE_SIZE = 20;
    static constexpr bool LOD_ENABLED = false;
    static constexpr bool CACHE_ENABLED = true;
    ModelLoader();
    ~ModelLoader();

    // IKernelService implementation
    bool Initialize() override
    {
        return true;
    }
    void Shutdown() override
    {
        UnloadAllModels();
    }
    void Update(float deltaTime) override
    {
    }
    void Render() override
    {
    }
    const char *GetName() const override
    {
        return "ModelLoader";
    }

    // ==================== CORE METHODS ====================

    // Load models from JSON config
    struct LoadResult
    {
        int totalModels;
        int loadedModels;
        int failedModels;
        float loadingTime;
    };

    std::optional<LoadResult> LoadModelsFromJson(const std::string &path);

    // Load only specific models from JSON config
    std::optional<LoadResult>
    LoadModelsFromJsonSelective(const std::string &path,
                                const std::vector<std::string> &modelNames);

    // Set selective loading mode
    void SetSelectiveMode(bool enabled);

    // Render all model instances
    void DrawAllModels() const;

    // Get model by name
    std::optional<std::reference_wrapper<Model>> GetModelByName(const std::string &name) override;

    // Add instance (legacy method)
    void AddInstance(const json &instanceJson, Model *modelPtr, const std::string &modelName,
                     Animation *animation);

    // ==================== ENHANCED METHODS ====================

    // Add instance with enhanced config
    bool AddInstanceEx(const std::string &modelName, const ModelInstanceConfig &config);

    // ==================== GAME MODEL LOADING ====================

    /**
     * @brief Load all game models from resources directory
     *
     * Scans the resources directory for all available models and loads them.
     * Configures cache and LOD settings for optimal game performance.
     *
     * @return LoadResult with statistics, or nullopt on failure
     */
    std::optional<LoadResult> LoadGameModels();

    /**
     * @brief Load specific models required for a map
     *
     * Loads only the models specified in the modelNames list.
     * More efficient than LoadGameModels() for map-specific loading.
     *
     * @param modelNames List of model names to load
     * @return LoadResult with statistics, or nullopt on failure
     */
    std::optional<LoadResult> LoadGameModelsSelective(const std::vector<std::string> &modelNames);

    /**
     * @brief Load specific models with safe fallback handling
     *
     * Similar to LoadGameModelsSelective but with enhanced error handling
     * and validation. Uses hash set for faster lookup.
     *
     * @param modelNames List of model names to load
     * @return LoadResult with statistics, or nullopt on failure
     */
    std::optional<LoadResult>
    LoadGameModelsSelectiveSafe(const std::vector<std::string> &modelNames);

    // Model management
    bool LoadSingleModel(const std::string &name, const std::string &path,
                         bool preload = true) override;
    bool UnloadModel(const std::string &name);
    void UnloadAllModels() override;
    bool ReloadModel(const std::string &name);
    // Register a raylib::Model that was already loaded elsewhere (e.g. MapLoader)
    bool RegisterLoadedModel(const std::string &name, const ::Model &model);

    // Filtering and search
    std::vector<ModelInstance *> GetInstancesByTag(const std::string &tag);
    std::vector<ModelInstance *> GetInstancesByCategory(const std::string &category);
    [[nodiscard]] std::vector<std::string> GetAvailableModels() const override;

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
    static bool IsValidVector3(const Vector3 &v);
    static bool IsValidColor(const Color &c);
    static bool IsValidMatrix(const Matrix &m);
};

#endif // MODEL_H