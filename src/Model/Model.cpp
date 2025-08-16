
#include <Model/Model.h>
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include <Color/ColorParser.h>

Models::Models() : m_cache(std::make_shared<ModelCache>())
{
    // Initialize statistics
    m_stats = LoadingStats{};
    TraceLog(LOG_INFO, "Models Manager initialized");
}

Models::~Models()
{
    // Legacy cleanup
    for (auto &[name, modelPtr] : m_modelByName)
    {
        if (modelPtr)
        {
            ::UnloadModel(*modelPtr);
            delete modelPtr;
        }
    }

    m_modelByName.clear();
    m_instances.clear();
    m_animations.clear();
    m_configs.clear();

    TraceLog(LOG_INFO, "Enhanced Models Manager destroyed");
}

void Models::LoadModelsFromJson(const std::string &path)
{
    auto startTime = std::chrono::steady_clock::now();
    TraceLog(LOG_INFO, "Loading enhanced models from: %s", path.c_str());

    std::ifstream file(path);
    if (!file.is_open())
    {
        TraceLog(LOG_ERROR, "Failed to open model list JSON: %s", path.c_str());
        m_stats.failedModels++;
        return;
    }

    json j;
    try
    {
        file >> j;
    }
    catch (const std::exception &e)
    {
        TraceLog(LOG_ERROR, "JSON parsing error: %s", e.what());
        m_stats.failedModels++;
        return;
    }

    for (const auto &modelEntry : j)
    {
        m_stats.totalModels++;

        // Use enhanced parsing
        if (!JsonHelper::ValidateModelEntry(modelEntry))
        {
            TraceLog(LOG_WARNING, "Invalid model entry, skipping");
            m_stats.failedModels++;
            continue;
        }

        try
        {
            // Parse using new helper
            ModelFileConfig config = JsonHelper::ParseModelConfig(modelEntry);
            config.path = PROJECT_ROOT_DIR + config.path;

            // Store configuration
            m_configs[config.name] = config;

            // Load model
            if (ProcessModelConfigLegacy(config))
            {
                m_stats.loadedModels++;
                TraceLog(LOG_INFO, "Successfully loaded model: %s", config.name.c_str());
            }
            else
            {
                m_stats.failedModels++;
            }
        }
        catch (const std::exception &e)
        {
            TraceLog(LOG_ERROR, "Error processing model entry: %s", e.what());
            m_stats.failedModels++;
        }
    }

    // Calculate loading time
    auto endTime = std::chrono::steady_clock::now();
    m_stats.loadingTime = std::chrono::duration<float>(endTime - startTime).count();

    // Print statistics
    TraceLog(LOG_INFO, "Loading completed: %d/%d models loaded in %.2f seconds",
             m_stats.loadedModels, m_stats.totalModels, m_stats.loadingTime);

    if (m_stats.failedModels > 0)
    {
        TraceLog(LOG_WARNING, "Failed to load %d models", m_stats.failedModels);
    }
}

// Legacy compatible method for config processing
bool Models::ProcessModelConfigLegacy(const ModelFileConfig &config)
{
    std::string modelPath = config.path;

    if (!ValidateModelPath(modelPath))
    {
        return false;
    }

    TraceLog(LOG_INFO, "Loading model '%s' from: %s", config.name.c_str(), modelPath.c_str());

    Model loadedModel = LoadModel(modelPath.c_str());
    if (loadedModel.meshCount == 0)
    {
        TraceLog(LOG_WARNING, "Failed to load model at path: %s", modelPath.c_str());
        return false;
    }

    // Store for legacy compatibility
    auto pModel = new Model(loadedModel);
    m_modelByName[config.name] = pModel;

    // Load animations
    Animation newAnimation;
    bool animationLoaded = newAnimation.LoadAnimations(modelPath);
    if (animationLoaded)
    {
        m_animations[config.name] = std::move(newAnimation);
    }

    // Create instances using legacy method for compatibility
    Animation *animPtr = nullptr;
    auto itAnim = m_animations.find(config.name);
    if (itAnim != m_animations.end())
    {
        animPtr = &itAnim->second;
    }

    // Process instances
    if (!config.instances.empty())
    {
        for (const auto &instanceConfig : config.instances)
        {
            if (instanceConfig.spawn)
            {
                // Convert to legacy JSON format
                json instanceJson;
                instanceJson["position"]["x"] = instanceConfig.position.x;
                instanceJson["position"]["y"] = instanceConfig.position.y;
                instanceJson["position"]["z"] = instanceConfig.position.z;
                instanceJson["scale"] = instanceConfig.scale;
                instanceJson["spawn"] = instanceConfig.spawn;

                AddInstance(instanceJson, pModel, config.name, animPtr);
                m_stats.totalInstances++;
            }
        }
    }
    else if (config.spawn)
    {
        AddInstance(json::object(), pModel, config.name, animPtr);
        m_stats.totalInstances++;
    }

    return true;
}

void Models::DrawAllModels() const
{
    for (const auto &instance : m_instances)
    {
        Model *modelPtr = instance.GetModel();
        if (modelPtr != nullptr && modelPtr->meshCount > 0)
        {
            DrawModel(*modelPtr, instance.GetModelPosition(), instance.GetScale(),
                      instance.GetColor());
        }
        else
        {
            TraceLog(LOG_WARNING, "Trying to draw invalid or empty model instance");
        }
    }
}

Model &Models::GetModelByName(const std::string &name)
{
    static Model dummyModel = {0};
    const auto it = m_modelByName.find(name);
    if (it == m_modelByName.end())
    {
        TraceLog(LOG_WARNING, "Model name '%s' not found. Returning dummy model.", name.c_str());
        return dummyModel;
    }
    return *(it->second);
}

void Models::AddInstance(const json &instanceJson, Model *modelPtr, const std::string &modelName,
                         Animation *animation)

{
    if (!modelPtr)
    {
        TraceLog(LOG_WARNING, "AddInstance called with nullptr modelPtr for model '%s'",
                 modelName.c_str());
        return;
    }

    Vector3 pos = {0.0f, 0.0f, 0.0f};
    float scaleModel = 1.0f;
    Color color = WHITE;

    if (instanceJson.contains("position"))
    {
        pos.x = instanceJson["position"].value("x", 0.0f);
        pos.y = instanceJson["position"].value("y", 0.0f);
        pos.z = instanceJson["position"].value("z", 0.0f);
    }

    if (instanceJson.contains("scale"))
    {
        scaleModel = instanceJson["scale"].get<float>();
    }

    if (instanceJson.contains("color"))
    {
        if (instanceJson["color"].is_string())
        {
            color = ParseColorByName(instanceJson["color"].get<std::string>());
        }
        else if (instanceJson["color"].is_object())
        {
            const auto &colorJson = instanceJson["color"];
            color.r = colorJson.value("r", 255);
            color.g = colorJson.value("g", 255);
            color.b = colorJson.value("b", 255);
            color.a = colorJson.value("a", 255);
        }
    }

    if (animation)
        m_instances.emplace_back(pos, modelPtr, scaleModel, modelName, color, *animation);
    else
        m_instances.emplace_back(pos, modelPtr, scaleModel, modelName, color);
}

bool Models::AddInstanceEx(const std::string &modelName, const ModelInstanceConfig &config)
{
    Model *model = nullptr;

    // Try to find model in legacy storage
    auto it = m_modelByName.find(modelName);
    if (it != m_modelByName.end())
    {
        model = it->second;
    }
    else
    {
        TraceLog(LOG_WARNING, "Model '%s' not found for instance creation", modelName.c_str());
        return false;
    }

    if (!config.spawn)
    {
        return false;
    }

    // Find animation
    Animation *animPtr = nullptr;
    auto animIt = m_animations.find(modelName);
    if (animIt != m_animations.end())
    {
        animPtr = &animIt->second;
    }

    // Create instance from enhanced config
    if (animPtr)
    {
        m_instances.emplace_back(config.position, model, config.scale, modelName, config.color,
                                 *animPtr);
    }
    else
    {
        m_instances.emplace_back(config.position, model, config.scale, modelName, config.color);
    }

    m_stats.totalInstances++;
    TraceLog(LOG_INFO, "Added enhanced instance for model '%s' at (%.2f, %.2f, %.2f)",
             modelName.c_str(), config.position.x, config.position.y, config.position.z);

    return true;
}

bool Models::LoadSingleModel(const std::string &name, const std::string &path, bool preload)
{
    std::string fullPath = PROJECT_ROOT_DIR + path;

    if (!ValidateModelPath(fullPath))
    {
        return false;
    }

    TraceLog(LOG_INFO, "Loading single model '%s' from: %s", name.c_str(), fullPath.c_str());

    Model loadedModel = ::LoadModel(fullPath.c_str());
    if (loadedModel.meshCount == 0)
    {
        TraceLog(LOG_ERROR, "Failed to load model: %s", fullPath.c_str());
        return false;
    }

    // Store in legacy storage
    auto pModel = new Model(loadedModel);
    m_modelByName[name] = pModel;

    // Load animations
    Animation newAnimation;
    if (newAnimation.LoadAnimations(fullPath))
    {
        m_animations[name] = std::move(newAnimation);
    }

    TraceLog(LOG_INFO, "Successfully loaded single model: %s", name.c_str());
    return true;
}

bool Models::UnloadModel(const std::string &name)
{
    auto it = m_modelByName.find(name);
    if (it == m_modelByName.end())
    {
        TraceLog(LOG_WARNING, "Cannot unload model '%s': not found", name.c_str());
        return false;
    }

    // Remove all instances of this model
    m_instances.erase(std::remove_if(m_instances.begin(), m_instances.end(),
                                     [&name](const ModelInstance &instance)
                                     { return instance.GetModelName() == name; }),
                      m_instances.end());

    // Remove model
    if (it->second)
    {
        ::UnloadModel(*it->second);
        delete it->second;
    }
    m_modelByName.erase(it);

    // Remove animations
    m_animations.erase(name);

    // Remove configuration
    m_configs.erase(name);

    TraceLog(LOG_INFO, "Unloaded model: %s", name.c_str());
    return true;
}

std::vector<ModelInstance *> Models::GetInstancesByTag(const std::string &tag)
{
    std::vector<ModelInstance *> result;

    for (auto &instance : m_instances)
    {
        // Currently using model name as tag (can be extended)
        if (instance.GetModelName().find(tag) != std::string::npos)
        {
            result.push_back(&instance);
        }
    }

    return result;
}

std::vector<ModelInstance *> Models::GetInstancesByCategory(const std::string &category)
{
    std::vector<ModelInstance *> result;

    for (auto &instance : m_instances)
    {
        // Find model configuration
        auto configIt = m_configs.find(instance.GetModelName());
        if (configIt != m_configs.end() && configIt->second.category == category)
        {
            result.push_back(&instance);
        }
    }

    return result;
}

std::vector<std::string> Models::GetAvailableModels() const
{
    std::vector<std::string> models;
    models.reserve(m_modelByName.size());

    for (const auto &[name, model] : m_modelByName)
    {
        models.push_back(name);
    }

    return models;
}

bool Models::HasCollision(const std::string &modelName) const
{
    auto configIt = m_configs.find(modelName);
    if (configIt != m_configs.end())
    {
        return configIt->second.hasCollision;
    }

    // Fallback: if no config found, return false
    return false;
}

void Models::PrintStatistics() const
{
    TraceLog(LOG_INFO, "=== Enhanced Model Manager Statistics ===");
    TraceLog(LOG_INFO, "Total models processed: %d", m_stats.totalModels);
    TraceLog(LOG_INFO, "Successfully loaded: %d", m_stats.loadedModels);
    TraceLog(LOG_INFO, "Failed to load: %d", m_stats.failedModels);
    TraceLog(LOG_INFO, "Total instances: %d", m_stats.totalInstances);
    TraceLog(LOG_INFO, "Loading time: %.2f seconds", m_stats.loadingTime);
    TraceLog(LOG_INFO, "Success rate: %.1f%%", m_stats.GetSuccessRate() * 100);
    TraceLog(LOG_INFO, "Cache enabled: %s", m_cacheEnabled ? "Yes" : "No");
    TraceLog(LOG_INFO, "LOD enabled: %s", m_lodEnabled ? "Yes" : "No");
}

void Models::PrintCacheInfo() const
{
    if (m_cache && m_cacheEnabled)
    {
        m_cache->PrintCacheStats();
    }
    else
    {
        TraceLog(LOG_INFO, "Cache is disabled or not available");
    }
}

void Models::SetMaxCacheSize(size_t maxSize)
{
    if (m_cache)
    {
        m_cache->SetMaxCacheSize(maxSize);
        TraceLog(LOG_INFO, "Cache max size set to: %zu", maxSize);
    }
}

void Models::CleanupUnusedModels()
{
    if (m_cache && m_cacheEnabled)
    {
        m_cache->CleanupUnusedModels();
        TraceLog(LOG_INFO, "Cleaned up unused cached models");
    }
}

void Models::OptimizeCache()
{
    if (m_cache && m_cacheEnabled)
    {
        m_cache->CleanupUnusedModels(60); // More aggressive cleanup
        TraceLog(LOG_INFO, "Cache optimized");
    }
}

bool Models::ValidateModelPath(const std::string &path) const
{
    if (path.empty())
    {
        TraceLog(LOG_ERROR, "Empty model path provided");
        return false;
    }

    // Check if file exists
    std::ifstream file(path);
    if (!file.good())
    {
        TraceLog(LOG_ERROR, "Model file not found: %s", path.c_str());
        return false;
    }

    // Check extension
    std::string ext = std::filesystem::path(path).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    std::vector<std::string> supportedExts = {".glb", ".gltf", ".obj", ".fbx", ".dae"};
    if (std::find(supportedExts.begin(), supportedExts.end(), ext) == supportedExts.end())
    {
        TraceLog(LOG_WARNING, "Potentially unsupported model format: %s", ext.c_str());
    }

    return true;
}

bool Models::ReloadModel(const std::string &name)
{
    auto configIt = m_configs.find(name);
    if (configIt == m_configs.end())
    {
        TraceLog(LOG_WARNING, "Cannot reload model '%s': configuration not found", name.c_str());
        return false;
    }

    TraceLog(LOG_INFO, "Reloading model: %s", name.c_str());

    // First unload existing model
    if (!UnloadModel(name))
    {
        TraceLog(LOG_WARNING, "Failed to unload model '%s' before reload", name.c_str());
    }

    // Reload
    return LoadSingleModel(name, configIt->second.path, true);
}

const ModelFileConfig *Models::GetModelConfig(const std::string &modelName) const
{
    auto it = m_configs.find(modelName);
    if (it != m_configs.end())
    {
        return &it->second;
    }
    return nullptr;
}