
#include <Model/Model.h>
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_set>

#include <Color/ColorParser.h>

ModelLoader::ModelLoader() : m_cache(std::make_shared<ModelCache>())
{
    // Initialize statistics
    m_stats = LoadingStats{};
    TraceLog(LOG_INFO, "Models Manager initialized (instance: %p)", this);
}

ModelLoader::~ModelLoader()
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

    TraceLog(LOG_INFO, "Enhanced Models Manager destroyed (instance: %p)", this);
}

std::optional<ModelLoader::LoadResult> ModelLoader::LoadModelsFromJson(const std::string &path)
{
    auto startTime = std::chrono::steady_clock::now();
    TraceLog(LOG_INFO, "Loading enhanced models from: %s", path.c_str());

    LoadResult result = {0, 0, 0, 0.0f};

    // Disable selective mode for regular loading
    m_selectiveMode = false;

    std::ifstream file(path);
    if (!file.is_open())
    {
        TraceLog(LOG_ERROR, "Failed to open model list JSON: %s", path.c_str());
        return std::nullopt;
    }

    json j;
    try
    {
        file >> j;
    }
    catch (const std::exception &e)
    {
        TraceLog(LOG_ERROR, "JSON parsing error: %s", e.what());
        return std::nullopt;
    }

    for (const auto &modelEntry : j)
    {
        result.totalModels++;

        // Use enhanced parsing
        if (!JsonHelper::ValidateModelEntry(modelEntry))
        {
            TraceLog(LOG_WARNING, "Invalid model entry, skipping");
            result.failedModels++;
            continue;
        }

        // Parse using new helper
        auto modelConfigResult = JsonHelper::ParseModelConfig(modelEntry);
        if (!modelConfigResult)
        {
            TraceLog(LOG_ERROR, "Error processing model entry");
            result.failedModels++;
            continue;
        }

        auto config = modelConfigResult.value();

        // Simple path handling - if path doesn't contain directory separators, assume it's in resources folder
        if (config.path.find('/') == std::string::npos && config.path.find('\\') == std::string::npos)
        {
            config.path = "../resources/" + config.path;
        }
        else if (!config.path.empty() && config.path[0] == '/')
        {
            config.path = std::string(PROJECT_ROOT_DIR) + config.path;
        }

        // Store configuration
        m_configs[config.name] = config;

        // Load model
        if (ProcessModelConfigLegacy(config))
        {
            result.loadedModels++;
            TraceLog(LOG_INFO, "Successfully loaded model: %s", config.name.c_str());
        }
        else
        {
            result.failedModels++;
        }
    }

    // Calculate loading time
    auto endTime = std::chrono::steady_clock::now();
    result.loadingTime = std::chrono::duration<float>(endTime - startTime).count();

    // Print statistics
    TraceLog(LOG_INFO, "Loading completed: %d/%d models loaded in %.2f seconds",
             result.loadedModels, result.totalModels, result.loadingTime);

    if (result.failedModels > 0)
    {
        TraceLog(LOG_WARNING, "Failed to load %d models", result.failedModels);
    }

    return result;
}

std::optional<ModelLoader::LoadResult> ModelLoader::LoadModelsFromJsonSelective(const std::string &path, const std::vector<std::string> &modelNames)
{
    auto startTime = std::chrono::steady_clock::now();
    TraceLog(LOG_INFO, "Loading selective models from: %s (models: %d)", path.c_str(), modelNames.size());

    LoadResult result = {0, 0, 0, 0.0f};

    // Enable selective mode to prevent auto-spawning of unwanted models
    m_selectiveMode = true;

    std::ifstream file(path);
    if (!file.is_open())
    {
        TraceLog(LOG_ERROR, "Failed to open model list JSON: %s", path.c_str());
        return std::nullopt;
    }

    json j;
    try
    {
        file >> j;
    }
    catch (const std::exception &e)
    {
        TraceLog(LOG_ERROR, "JSON parsing error: %s", e.what());
        return std::nullopt;
    }

    // Create a set for faster lookup
    std::unordered_set<std::string> modelSet(modelNames.begin(), modelNames.end());

    for (const auto &modelEntry : j)
    {
        result.totalModels++;

        // Get model name from the entry
        if (!modelEntry.contains("name") || !modelEntry["name"].is_string())
        {
            TraceLog(LOG_WARNING, "Model entry missing name field, skipping");
            result.failedModels++;
            continue;
        }

        std::string modelName = modelEntry["name"].get<std::string>();

        // Check if this model is in our selective list
        if (modelSet.find(modelName) == modelSet.end())
        {
            TraceLog(LOG_INFO, "Skipping model '%s' (not in selective list)", modelName.c_str());
            continue;
        }

        // Use enhanced parsing
        if (!JsonHelper::ValidateModelEntry(modelEntry))
        {
            TraceLog(LOG_WARNING, "Invalid model entry for '%s', skipping", modelName.c_str());
            result.failedModels++;
            continue;
        }

        // Parse using new helper
        auto modelConfigResult = JsonHelper::ParseModelConfig(modelEntry);
        if (!modelConfigResult)
        {
            TraceLog(LOG_ERROR, "Error processing model entry for '%s'", modelName.c_str());
            result.failedModels++;
            continue;
        }

        auto config = modelConfigResult.value();

        // Simple path handling - if path doesn't contain directory separators, assume it's in resources folder
        if (config.path.find('/') == std::string::npos && config.path.find('\\') == std::string::npos)
        {
            config.path = "../resources/" + config.path;
        }
        else if (!config.path.empty() && config.path[0] == '/')
        {
            config.path = std::string(PROJECT_ROOT_DIR) + config.path;
        }

        // Store configuration
        m_configs[config.name] = config;

        // Load model
        if (ProcessModelConfigLegacy(config))
        {
            result.loadedModels++;
            TraceLog(LOG_INFO, "Successfully loaded selective model: %s", config.name.c_str());
        }
        else
        {
            result.failedModels++;
        }
    }

    // Calculate loading time
    auto endTime = std::chrono::steady_clock::now();
    result.loadingTime = std::chrono::duration<float>(endTime - startTime).count();

    // Print statistics
    TraceLog(LOG_INFO, "Selective loading completed: %d/%d models loaded in %.2f seconds",
             result.loadedModels, result.totalModels, result.loadingTime);

    if (result.failedModels > 0)
    {
        TraceLog(LOG_WARNING, "Failed to load %d selective models", result.failedModels);
    }

    return result;
}

// Legacy compatible method for config processing
bool ModelLoader::ProcessModelConfigLegacy(const ModelFileConfig &config)
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
    const auto pModel = new Model(loadedModel);
    m_modelByName[config.name] = pModel;

    // Load animations
    Animation newAnimation;
    if (bool animationLoaded = newAnimation.LoadAnimations(modelPath))
    {
        m_animations[config.name] = newAnimation;
    }

    // Create instances using legacy method for compatibility
    Animation *animPtr = nullptr;
    const auto itAnim = m_animations.find(config.name);
    if (itAnim != m_animations.end())
    {
        animPtr = &itAnim->second;
    }

    // Process instances only if this model should be spawned
    // For selective loading, we need to check if this model was explicitly requested
    bool shouldSpawnModel = true;

    // Check if we're in selective loading mode
    if (m_selectiveMode)
    {
        // In selective mode, only spawn essential models (like player) or models that don't auto-spawn
        shouldSpawnModel = (config.name == "player") || !config.spawn;
    }

    // Special case: Always spawn player model even if spawn is false
    if (config.name == "player")
    {
        shouldSpawnModel = true;
        TraceLog(LOG_INFO, "ModelLoader::ProcessModelConfigLegacy() - Forcing spawn of player model");
    }

    if (shouldSpawnModel)
    {
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
    }

    return true;
}

void ModelLoader::DrawAllModels() const
{
    for (const auto &instance : m_instances)
    {
        Model *modelPtr = instance.GetModel();

        // Enhanced null/invalid model pointer validation
        if (modelPtr == nullptr)
        {
            TraceLog(LOG_WARNING, "ModelLoader::DrawAllModels() - Null model pointer for instance: %s",
                     instance.GetModelName().c_str());
            continue;
        }

        if (modelPtr->meshCount <= 0)
        {
            TraceLog(LOG_WARNING, "ModelLoader::DrawAllModels() - Empty model (meshCount: %d) for instance: %s",
                     modelPtr->meshCount, instance.GetModelName().c_str());
            continue;
        }

        // Validate position, rotation, and scale for NaN/inf
        Vector3 position = instance.GetModelPosition();
        Vector3 rotationDeg = instance.GetRotationDegrees();
        float scale = instance.GetScale();

        if (!IsValidVector3(position))
        {
            TraceLog(LOG_ERROR, "ModelLoader::DrawAllModels() - Invalid position (NaN/inf) for instance: %s (%.2f, %.2f, %.2f)",
                     instance.GetModelName().c_str(), position.x, position.y, position.z);
            continue;
        }

        if (!IsValidVector3(rotationDeg))
        {
            TraceLog(LOG_ERROR, "ModelLoader::DrawAllModels() - Invalid rotation (NaN/inf) for instance: %s (%.2f, %.2f, %.2f)",
                     instance.GetModelName().c_str(), rotationDeg.x, rotationDeg.y, rotationDeg.z);
            continue;
        }

        if (std::isnan(scale) || std::isinf(scale) || scale <= 0.0f)
        {
            TraceLog(LOG_ERROR, "ModelLoader::DrawAllModels() - Invalid scale (NaN/inf/zero/negative) for instance: %s (%.2f)",
                     instance.GetModelName().c_str(), scale);
            continue;
        }

        // Validate color - skip drawing if invalid to prevent access violations
        Color drawColor = instance.GetColor();
        if (!IsValidColor(drawColor))
        {
            TraceLog(LOG_ERROR, "ModelLoader::DrawAllModels() - Invalid color for instance: %s (r:%d g:%d b:%d a:%d), skipping draw to prevent access violation",
                     instance.GetModelName().c_str(), drawColor.r, drawColor.g, drawColor.b, drawColor.a);
            continue;
        }

        // Build transform with rotation from instance (degrees -> radians)
        Vector3 rotRad = { DEG2RAD * rotationDeg.x, DEG2RAD * rotationDeg.y, DEG2RAD * rotationDeg.z };
        Matrix rotation = MatrixRotateXYZ(rotRad);
        Matrix translation = MatrixTranslate(position.x, position.y, position.z);
        Matrix transform = MatrixMultiply(rotation, translation);

        // Validate matrix before drawing
        if (!IsValidMatrix(rotation) || !IsValidMatrix(translation) || !IsValidMatrix(transform))
        {
            TraceLog(LOG_ERROR, "ModelLoader::DrawAllModels() - Invalid transform matrix for instance: %s",
                     instance.GetModelName().c_str());
            continue;
        }

        // Draw with transform and uniform scale
        DrawModelEx(*modelPtr, position, {0,1,0}, rotationDeg.y,
                    {scale, scale, scale}, WHITE);
    }
}

std::optional<std::reference_wrapper<Model>> ModelLoader::GetModelByName(const std::string &name)
{
    // 1) Exact match
    auto it = m_modelByName.find(name);
    if (it != m_modelByName.end())
    {
        return std::ref(*it->second);
    }

    // 2) Try filename stem if a full path or filename with extension was provided
    std::string candidate = name;
    // Extract filename if path-like
    const size_t slashPos = candidate.find_last_of("/\\");
    if (slashPos != std::string::npos)
    {
        candidate = candidate.substr(slashPos + 1);
    }
    // Strip extension
    const size_t dotPos = candidate.find_last_of('.');
    if (dotPos != std::string::npos)
    {
        candidate = candidate.substr(0, dotPos);
    }
    it = m_modelByName.find(candidate);
    if (it != m_modelByName.end())
    {
        return std::ref(*it->second);
    }

    // 3) Case-insensitive match (lowercase both sides)
    auto toLower = [](std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return s;
    };

    const std::string nameLower = toLower(name);
    for (auto &pair : m_modelByName)
    {
        if (toLower(pair.first) == nameLower)
        {
            return std::ref(*pair.second);
        }
    }

    const std::string candLower = toLower(candidate);
    for (auto &pair : m_modelByName)
    {
        if (toLower(pair.first) == candLower)
        {
            return std::ref(*pair.second);
        }
    }

    TraceLog(LOG_WARNING, "Model name '%s' not found (after normalization attempts).", name.c_str());
    return std::nullopt;
}

void ModelLoader::AddInstance(const json &instanceJson, Model *modelPtr, const std::string &modelName,
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
    auto color = WHITE;

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

    // Optional rotation in degrees
    Vector3 rotationDeg = {0.0f, 0.0f, 0.0f};
    if (instanceJson.contains("rotation"))
    {
        if (instanceJson["rotation"].is_number())
        {
            // If single number provided, assume Yaw (Y-axis) rotation
            rotationDeg.y = instanceJson["rotation"].get<float>();
        }
        else if (instanceJson["rotation"].is_object())
        {
            const auto &rotJson = instanceJson["rotation"];
            rotationDeg.x = rotJson.value("x", 0.0f);
            rotationDeg.y = rotJson.value("y", 0.0f);
            rotationDeg.z = rotJson.value("z", 0.0f);
        }
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

    // Apply rotation to the last instance
    if (!m_instances.empty())
    {
        m_instances.back().SetRotationDegrees(rotationDeg);
    }
}

bool ModelLoader::AddInstanceEx(const std::string &modelName, const ModelInstanceConfig &config)
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

bool ModelLoader::LoadSingleModel(const std::string &name, const std::string &path, bool preload)
{
    std::string fullPath;

    // Simple path handling - if path doesn't contain directory separators, assume it's in resources folder
    if (path.find('/') == std::string::npos && path.find('\\') == std::string::npos)
    {
        fullPath = "../resources/" + path;
    }
    else
    {
        // If path starts with "/", treat it as relative to project root
        if (!path.empty() && path[0] == '/')
        {
            fullPath = std::string(PROJECT_ROOT_DIR) + path;
        }
        else
        {
            fullPath = path;
        }
    }

    // Fix texture paths for .gltf files
    if (fullPath.substr(fullPath.size() - 5) == ".gltf")
    {
        std::ifstream file(fullPath);
        json j;
        file >> j;
        if (j.contains("textures"))
        {
            for (auto &tex : j["textures"])
            {
                if (tex.contains("uri") && tex["uri"].is_string())
                {
                    std::string uri = tex["uri"];
                    if (uri.substr(0, 3) == "MI_")
                    {
                        tex["uri"] = "../resources/textures/" + uri;
                    }
                }
            }
        }
        // Write to temp file
        std::string tempPath = fullPath + ".temp";
        std::ofstream tempFile(tempPath);
        tempFile << j.dump(4);
        tempFile.close();
        fullPath = tempPath;
    }

    if (!ValidateModelPath(fullPath))
    {
        return false;
    }

    TraceLog(LOG_INFO, "Loading single model '%s' from: %s", name.c_str(), fullPath.c_str());

    Model loadedModel = ::LoadModel(fullPath.c_str());

    // Clean up temp file if used
    if (fullPath.find(".temp") != std::string::npos)
    {
        std::filesystem::remove(fullPath);
    }

    if (loadedModel.meshCount == 0)
    {
        TraceLog(LOG_ERROR, "Failed to load model: %s (meshCount: %d)", fullPath.c_str(), loadedModel.meshCount);

        // Try to load as different format or check if file is valid
        if (std::ifstream(fullPath).good())
        {
            TraceLog(LOG_WARNING, "Model file exists but failed to load - may be corrupted or unsupported format");
        }
        else
        {
            TraceLog(LOG_ERROR, "Model file not accessible: %s", fullPath.c_str());
        }
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

bool ModelLoader::UnloadModel(const std::string &name)
{
    auto it = m_modelByName.find(name);
    if (it == m_modelByName.end())
    {
        TraceLog(LOG_WARNING, "Cannot unload model '%s': not found", name.c_str());
        return false;
    }

    // Remove all instances of this model
    std::erase_if(m_instances,
                  [&name](const ModelInstance &instance)
                  { return instance.GetModelName() == name; });

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

std::vector<ModelInstance *> ModelLoader::GetInstancesByTag(const std::string &tag)
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

std::vector<ModelInstance *> ModelLoader::GetInstancesByCategory(const std::string &category)
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

std::vector<std::string> ModelLoader::GetAvailableModels() const
{
    std::vector<std::string> models;
    models.reserve(m_modelByName.size());

    // Use traditional iteration instead of C++20 ranges for GCC compatibility
    for (const auto &[name, modelPtr] : m_modelByName)
    {
        models.push_back(name);
    }

    return models;
}

bool ModelLoader::HasCollision(const std::string &modelName) const
{
    auto configIt = m_configs.find(modelName);
    if (configIt != m_configs.end())
    {
        return configIt->second.hasCollision;
    }

    // Fallback: if no config found, return false
    return false;
}

const LoadingStats & ModelLoader::GetLoadingStats() const { return m_stats; }

void ModelLoader::PrintStatistics() const
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

void ModelLoader::PrintCacheInfo() const
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

void ModelLoader::SetCacheEnabled(const bool enabled) { m_cacheEnabled = enabled; }

void ModelLoader::SetMaxCacheSize(const size_t maxSize) const {
    if (m_cache)
    {
        m_cache->SetMaxCacheSize(maxSize);
        TraceLog(LOG_INFO, "Cache max size set to: %zu", maxSize);
    }
}

void ModelLoader::EnableLOD(bool enabled) { m_lodEnabled = enabled; }

void ModelLoader::SetSelectiveMode(bool enabled) { m_selectiveMode = enabled; }

void ModelLoader::CleanupUnusedModels() const {
    if (m_cache && m_cacheEnabled)
    {
        m_cache->CleanupUnusedModels();
        TraceLog(LOG_INFO, "Cleaned up unused cached models");
    }
}

void ModelLoader::OptimizeCache() const {
    if (m_cache && m_cacheEnabled)
    {
        m_cache->CleanupUnusedModels(60); // More aggressive cleanup
        TraceLog(LOG_INFO, "Cache optimized");
    }
}

// Validation helper functions for crash prevention
bool ModelLoader::IsValidVector3(const Vector3& v)
{
    return !std::isnan(v.x) && !std::isnan(v.y) && !std::isnan(v.z) &&
           !std::isinf(v.x) && !std::isinf(v.y) && !std::isinf(v.z);
}

bool ModelLoader::IsValidColor(const Color& c)
{
    // Check for valid range and ensure no access violations by checking component types
    // Color components should be unsigned char (0-255), but we add safety checks
    return (c.r >= 0 && c.r <= 255) &&
           (c.g >= 0 && c.g <= 255) &&
           (c.b >= 0 && c.b <= 255) &&
           (c.a >= 0 && c.a <= 255);
}

bool ModelLoader::IsValidMatrix(const Matrix& m)
{
    // Check for NaN or infinite values in all matrix elements
    for (int i = 0; i < 16; ++i)
    {
        float val = (&m.m0)[i];
        if (std::isnan(val) || std::isinf(val))
        {
            return false;
        }
    }
    return true;
}

bool ModelLoader::ValidateModelPath(const std::string &path) const
{
    if (path.empty())
    {
        TraceLog(LOG_ERROR, "Empty model path provided");
        return false;
    }

    // Check if file exists
    if (const std::ifstream file(path); !file.good())
    {
        TraceLog(LOG_ERROR, "Model file not found: %s", path.c_str());
        return false;
    }

    // Check extension
    std::string ext = std::filesystem::path(path).extension().string();
    std::ranges::transform(ext, ext.begin(), ::tolower);

    std::vector<std::string> supportedExtensions = {".glb", ".gltf", ".obj", ".fbx", ".dae"};
    if (std::ranges::find(supportedExtensions, ext) == supportedExtensions.end())
    {
        TraceLog(LOG_WARNING, "Potentially unsupported model format: %s", ext.c_str());
    }

    return true;
}

bool ModelLoader::ReloadModel(const std::string &name)
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

const ModelFileConfig *ModelLoader::GetModelConfig(const std::string &modelName) const
{
    const auto it = m_configs.find(modelName);
    if (it != m_configs.end())
    {
        return &it->second;
    }
    return nullptr;
}

// Register a Model that was already loaded by another system (MapLoader)
bool ModelLoader::RegisterLoadedModel(const std::string &name, const ::Model &model)
{
    // If already present, skip
    if (m_modelByName.find(name) != m_modelByName.end())
    {
        TraceLog(LOG_INFO, "ModelLoader::RegisterLoadedModel() - Model '%s' already registered", name.c_str());
        return true;
    }

    // Allocate and copy the provided raylib Model
    Model *pModel = new Model(model);
    if (!pModel)
    {
        TraceLog(LOG_ERROR, "ModelLoader::RegisterLoadedModel() - Allocation failed for model '%s'", name.c_str());
        return false;
    }

    m_modelByName[name] = pModel;
    m_stats.loadedModels++;
    TraceLog(LOG_INFO, "ModelLoader::RegisterLoadedModel() - Registered model '%s' (meshCount=%d)", name.c_str(), pModel->meshCount);

    // Also register common aliases to improve matching between editor exports and runtime keys.
    try {
        std::string stem = std::filesystem::path(name).stem().string();
        std::string ext = std::filesystem::path(name).extension().string();

        // Register stem (without extension) as an alias if different
        if (!stem.empty() && stem != name && m_modelByName.find(stem) == m_modelByName.end()) {
            m_modelByName[stem] = pModel;
            TraceLog(LOG_INFO, "ModelLoader::RegisterLoadedModel() - Registered alias '%s' -> '%s'", stem.c_str(), name.c_str());
        }

        // Register lowercase variants for robustness
        auto toLower = [](std::string s) {
            std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return std::tolower(c); });
            return s;
        };

        std::string lname = toLower(name);
        if (lname != name && m_modelByName.find(lname) == m_modelByName.end()) {
            m_modelByName[lname] = pModel;
            TraceLog(LOG_INFO, "ModelLoader::RegisterLoadedModel() - Registered lowercase alias '%s'", lname.c_str());
        }

        std::string lstem = toLower(stem);
        if (!lstem.empty() && lstem != stem && m_modelByName.find(lstem) == m_modelByName.end()) {
            m_modelByName[lstem] = pModel;
            TraceLog(LOG_INFO, "ModelLoader::RegisterLoadedModel() - Registered lowercase alias '%s'", lstem.c_str());
        }
    } catch (...) {
        // aliasing should not be fatal; ignore any filesystem or transform issues
    }

    // Attempt to load animations for this model if possible (best-effort)
    try
    {
        std::string potentialPath = std::string(PROJECT_ROOT_DIR) + "/resources/" + name;
        Animation anim;
        if (anim.LoadAnimations(potentialPath))
        {
            m_animations[name] = std::move(anim);
            TraceLog(LOG_INFO, "ModelLoader::RegisterLoadedModel() - Loaded animations for '%s' (if available)", name.c_str());
        }
    }
    catch (...) { /* ignore */ }

    return true;
}