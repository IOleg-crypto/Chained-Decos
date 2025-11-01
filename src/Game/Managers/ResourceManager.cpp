#include "ResourceManager.h"
#include "Engine/Model/Model.h"
#include "Engine/Map/MapLoader.h"
#include <raylib.h>
#include <nlohmann/json.hpp>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <unordered_set>

using json = nlohmann::json;

ResourceManager::ResourceManager(ModelLoader* models)
    : m_models(models)
{
    TraceLog(LOG_INFO, "ResourceManager created");
}

std::optional<ModelLoader::LoadResult> ResourceManager::LoadGameModels()
{
    TraceLog(LOG_INFO, "ResourceManager::LoadGameModels() - Loading game models from resources directory...");
    m_models->SetCacheEnabled(true);
    m_models->SetMaxCacheSize(50);
    m_models->EnableLOD(true);
    m_models->SetSelectiveMode(false);

    MapLoader mapLoader;
    std::string resourcesDir = std::string(PROJECT_ROOT_DIR) + "/resources";
    auto models = mapLoader.LoadModelsFromDirectory(resourcesDir);

    if (models.empty())
    {
        TraceLog(LOG_WARNING, "ResourceManager::LoadGameModels() - No models found in resources directory");
        return std::nullopt;
    }

    TraceLog(LOG_INFO, "ResourceManager::LoadGameModels() - Found %d models in resources directory",
             models.size());

    ModelLoader::LoadResult result = {
        static_cast<int>(models.size()), // totalModels
        0,                               // loadedModels
        0,                               // failedModels
        0.0f                             // loadingTime
    };

    auto startTime = std::chrono::steady_clock::now();

    // Load each model found in the directory
    for (const auto &modelInfo : models)
    {
        std::string modelPath = modelInfo.path;
        TraceLog(LOG_INFO, "ResourceManager::LoadGameModels() - Loading model: %s from %s",
                 modelInfo.name.c_str(), modelPath.c_str());

        if (m_models->LoadSingleModel(modelInfo.name, modelPath, true))
        {
            result.loadedModels++;
            TraceLog(LOG_INFO, "Successfully loaded model: %s", modelInfo.name.c_str());
        }
        else
        {
            result.failedModels++;
            TraceLog(LOG_WARNING, "Failed to load model: %s", modelInfo.name.c_str());
        }
    }

    auto endTime = std::chrono::steady_clock::now();
    result.loadingTime = std::chrono::duration<float>(endTime - startTime).count();

    m_models->PrintStatistics();
    TraceLog(LOG_INFO, "ResourceManager::LoadGameModels() - Loaded %d/%d models in %.2f seconds",
             result.loadedModels, result.totalModels, result.loadingTime);

    // Validate that we have essential models
    auto availableModels = m_models->GetAvailableModels();
    bool hasPlayerModel = std::find(availableModels.begin(), availableModels.end(), "player_low") !=
                          availableModels.end();

    if (!hasPlayerModel)
    {
        TraceLog(
            LOG_WARNING,
            "ResourceManager::LoadGameModels() - Player model not found, player may not render correctly");
    }

    return result;
}

std::optional<ModelLoader::LoadResult>
ResourceManager::LoadGameModelsSelective(const std::vector<std::string> &modelNames)
{
    TraceLog(LOG_INFO, "ResourceManager::LoadGameModelsSelective() - Loading selective models: %d models",
             modelNames.size());
    m_models->SetCacheEnabled(true);
    m_models->SetMaxCacheSize(50);
    m_models->EnableLOD(false);
    m_models->SetSelectiveMode(true);

    MapLoader mapLoader;
    std::string resourcesDir = PROJECT_ROOT_DIR "/resources";
    auto allModels = mapLoader.LoadModelsFromDirectory(resourcesDir);

    if (allModels.empty())
    {
        TraceLog(LOG_WARNING,
                 "ResourceManager::LoadGameModelsSelective() - No models found in resources directory");
        return std::nullopt;
    }

    TraceLog(LOG_INFO, "ResourceManager::LoadGameModelsSelective() - Found %d models in resources directory",
             allModels.size());

    ModelLoader::LoadResult result = {
        static_cast<int>(modelNames.size()), // totalModels (only count requested models)
        0,                                   // loadedModels
        0,                                   // failedModels
        0.0f                                 // loadingTime
    };

    auto startTime = std::chrono::steady_clock::now();

    // Load only the models that are in the required list
    for (const auto &modelName : modelNames)
    {
        auto it =
            std::find_if(allModels.begin(), allModels.end(),
                         [&modelName](const ModelInfo &info) { return info.name == modelName; });

        if (it != allModels.end())
        {
            std::string modelPath = it->path;
            TraceLog(LOG_INFO,
                     "ResourceManager::LoadGameModelsSelective() - Loading required model: %s from %s",
                     modelName.c_str(), modelPath.c_str());

            if (m_models->LoadSingleModel(modelName, modelPath, true))
            {
                result.loadedModels++;
                TraceLog(LOG_INFO, "Successfully loaded model: %s", modelName.c_str());
            }
            else
            {
                result.failedModels++;
                TraceLog(LOG_WARNING, "Failed to load model: %s", modelName.c_str());
            }
        }
        else
        {
            TraceLog(LOG_WARNING,
                     "ResourceManager::LoadGameModelsSelective() - Model not found in resources: %s",
                     modelName.c_str());
            result.failedModels++;
        }
    }

    auto endTime = std::chrono::steady_clock::now();
    result.loadingTime = std::chrono::duration<float>(endTime - startTime).count();

    m_models->PrintStatistics();
    TraceLog(LOG_INFO, "ResourceManager::LoadGameModelsSelective() - Loaded %d/%d models in %.2f seconds",
             result.loadedModels, result.totalModels, result.loadingTime);

    // Validate that we have essential models
    auto availableModels = m_models->GetAvailableModels();
    bool hasPlayerModel = std::find(availableModels.begin(), availableModels.end(), "player") !=
                          availableModels.end();

    if (!hasPlayerModel)
    {
        TraceLog(LOG_WARNING, "ResourceManager::LoadGameModelsSelective() - Player model not found, player "
                              "may not render correctly");
    }

    return result;
}

std::optional<ModelLoader::LoadResult>
ResourceManager::LoadGameModelsSelectiveSafe(const std::vector<std::string> &modelNames)
{
    TraceLog(LOG_INFO, "ResourceManager::LoadGameModelsSelectiveSafe() - Loading selective models: %d models",
             modelNames.size());
    m_models->SetCacheEnabled(true);
    m_models->SetMaxCacheSize(50);
    m_models->EnableLOD(false);
    m_models->SetSelectiveMode(true);

    MapLoader mapLoader;
    std::string resourcesDir = PROJECT_ROOT_DIR "/resources";
    auto allModels = mapLoader.LoadModelsFromDirectory(resourcesDir);

    if (allModels.empty())
    {
        TraceLog(LOG_WARNING,
                 "ResourceManager::LoadGameModelsSelectiveSafe() - No models found in resources directory");
        return std::nullopt;
    }

    TraceLog(LOG_INFO,
             "ResourceManager::LoadGameModelsSelectiveSafe() - Found %d models in resources directory",
             allModels.size());

    ModelLoader::LoadResult result = {
        static_cast<int>(modelNames.size()), // totalModels (only count requested models)
        0,                                   // loadedModels
        0,                                   // failedModels
        0.0f                                 // loadingTime
    };

    auto startTime = std::chrono::steady_clock::now();

    // Load only the models that are in the required list
    std::unordered_set<std::string> modelNameSet(modelNames.begin(), modelNames.end());

    for (const auto &modelInfo : allModels)
    {
        if (modelNameSet.find(modelInfo.name) != modelNameSet.end())
        {
            TraceLog(LOG_INFO,
                     "ResourceManager::LoadGameModelsSelectiveSafe() - Loading required model: %s from %s",
                     modelInfo.name.c_str(), modelInfo.path.c_str());

            if (m_models->LoadSingleModel(modelInfo.name, modelInfo.path, true))
            {
                result.loadedModels++;
                TraceLog(LOG_INFO, "Successfully loaded model: %s", modelInfo.name.c_str());
            }
            else
            {
                result.failedModels++;
                TraceLog(LOG_WARNING, "Failed to load model: %s", modelInfo.name.c_str());
            }
        }
    }

    auto endTime = std::chrono::steady_clock::now();
    result.loadingTime = std::chrono::duration<float>(endTime - startTime).count();

    m_models->PrintStatistics();
    TraceLog(LOG_INFO, "ResourceManager::LoadGameModelsSelectiveSafe() - Loaded %d/%d models in %.2f seconds",
             result.loadedModels, result.totalModels, result.loadingTime);

    // Validate that we have essential models
    auto availableModels = m_models->GetAvailableModels();
    bool hasPlayerModel = std::find(availableModels.begin(), availableModels.end(), "player") !=
                          availableModels.end();

    if (!hasPlayerModel)
    {
        TraceLog(LOG_WARNING, "ResourceManager::LoadGameModelsSelectiveSafe() - Player model not found, "
                              "player may not render correctly");
    }

    return result;
}

std::string ResourceManager::GetModelNameForObjectType(int objectType, const std::string &modelName)
{
    // Cast to MapObjectType enum for better readability
    MapObjectType type = static_cast<MapObjectType>(objectType);

    switch (type)
    {
    case MapObjectType::MODEL:
    {
        // For MODEL type objects, return the actual model name if provided
        if (!modelName.empty())
        {
            TraceLog(LOG_DEBUG,
                     "ResourceManager::GetModelNameForObjectType() - MODEL object requires model: %s",
                     modelName.c_str());
            return modelName;
        }
        // Fallback: try to infer model from common naming patterns
        TraceLog(LOG_WARNING,
                 "ResourceManager::GetModelNameForObjectType() - MODEL object has no modelName specified");
        return "";
    }

    case MapObjectType::LIGHT:
    {
        // Handle incorrectly exported MODEL objects as LIGHT type
        // This is a known issue with the map editor
        if (!modelName.empty())
        {
            TraceLog(LOG_DEBUG,
                     "ResourceManager::GetModelNameForObjectType() - LIGHT object (likely MODEL) requires "
                     "model: %s",
                     modelName.c_str());
            return modelName;
        }
        // LIGHT objects don't typically require 3D models for rendering
        return "";
    }

    case MapObjectType::CUBE:
    {
        // Cubes are rendered as primitives, no model needed
        // But some maps might have custom cube models
        if (!modelName.empty())
        {
            TraceLog(LOG_DEBUG,
                     "ResourceManager::GetModelNameForObjectType() - CUBE object with custom model: %s",
                     modelName.c_str());
            return modelName;
        }
        return "";
    }

    case MapObjectType::SPHERE:
    {
        // Spheres are rendered as primitives, no model needed
        // But some maps might have custom sphere models
        if (!modelName.empty())
        {
            TraceLog(LOG_DEBUG,
                     "ResourceManager::GetModelNameForObjectType() - SPHERE object with custom model: %s",
                     modelName.c_str());
            return modelName;
        }
        return "";
    }

    case MapObjectType::CYLINDER:
    {
        // Cylinders are rendered as primitives, no model needed
        // But some maps might have custom cylinder models
        if (!modelName.empty())
        {
            TraceLog(LOG_DEBUG,
                     "ResourceManager::GetModelNameForObjectType() - CYLINDER object with custom model: %s",
                     modelName.c_str());
            return modelName;
        }
        return "";
    }

    case MapObjectType::PLANE:
    {
        // Planes are rendered as primitives, no model needed
        // But some maps might have custom plane models
        if (!modelName.empty())
        {
            TraceLog(LOG_DEBUG,
                     "ResourceManager::GetModelNameForObjectType() - PLANE object with custom model: %s",
                     modelName.c_str());
            return modelName;
        }
        return "";
    }

    default:
    {
        TraceLog(LOG_WARNING, "ResourceManager::GetModelNameForObjectType() - Unknown object type: %d",
                 objectType);
        return "";
    }
    }
}

std::vector<std::string> ResourceManager::GetModelsRequiredForMap(const std::string &mapIdentifier)
{
    std::vector<std::string> requiredModels;

    // Always include the player model as it's essential for gameplay
    requiredModels.emplace_back("player_low");

    // Convert map name to full path if needed
    std::string mapPath = mapIdentifier;
    if (mapPath.substr(mapPath.find_last_of('.') + 1) != "json")
    {
        // If it's not a path ending in .json, assume it's a map name and construct the path
        mapPath = PROJECT_ROOT_DIR "/resources/maps/" + mapIdentifier;
        if (mapIdentifier.find(".json") == std::string::npos)
        {
            mapPath += ".json";
        }
    }

    // Check if this is a JSON file exported from map editor
    std::string extension = mapPath.substr(mapPath.find_last_of(".") + 1);
    if (extension == "json")
    {
        TraceLog(LOG_INFO,
                 "ResourceManager::GetModelsRequiredForMap() - Analyzing JSON map for model requirements: %s",
                 mapPath.c_str());

        std::ifstream file(mapPath);
        if (file.is_open())
        {
            // Read entire file content for manual parsing
            std::string content((std::istreambuf_iterator<char>(file)),
                                std::istreambuf_iterator<char>());
            file.close();

            // Check if this is the editor format or game format (direct array)
            // Editor format: {"objects": [...] ...}
            // Game format: [...]
            size_t objectsStart = content.find("\"objects\"");
            size_t arrayStart = content.find("[");

            if (objectsStart != std::string::npos)
            {
                // This is the editor format with metadata - use nlohmann/json
                json j;
                try
                {
                    j = json::parse(content);
                }
                catch (const std::exception &e)
                {
                    TraceLog(LOG_WARNING,
                             "ResourceManager::GetModelsRequiredForMap() - Error parsing map JSON: %s",
                             e.what());
                    return requiredModels;
                }

                // Look for objects with model references
                if (j.contains("objects") && j["objects"].is_array())
                {
                    for (const auto &object : j["objects"])
                    {
                        // Extract object type and model name
                        int objectType = -1;
                        std::string objectModelName = "";

                        if (object.contains("type") && object["type"].is_number_integer())
                        {
                            objectType = object["type"].get<int>();
                        }

                        if (object.contains("modelName") && object["modelName"].is_string())
                        {
                            objectModelName = object["modelName"].get<std::string>();
                        }

                        // Use our improved function to determine if this object needs a model
                        std::string modelName =
                            GetModelNameForObjectType(objectType, objectModelName);

                        // Normalize model name: remove file extension and path
                        if (!modelName.empty())
                        {
                            // Remove file extension if present
                            std::string normalizedName = std::filesystem::path(modelName).stem().string();
                            // Remove path if present, keep only filename
                            if (normalizedName.empty())
                            {
                                normalizedName = modelName;
                            }
                            
                            // Add model to requirements if found and not already in list
                            if (std::find(requiredModels.begin(), requiredModels.end(),
                                          normalizedName) == requiredModels.end())
                            {
                                requiredModels.push_back(normalizedName);
                                TraceLog(LOG_INFO,
                                         "ResourceManager::GetModelsRequiredForMap() - Object type %d "
                                         "requires model: %s (normalized from %s)",
                                         objectType, normalizedName.c_str(), modelName.c_str());
                            }
                            else
                            {
                                TraceLog(LOG_DEBUG,
                                         "ResourceManager::GetModelsRequiredForMap() - Model %s already "
                                         "in requirements list",
                                         modelName.c_str());
                            }
                        }
                        else if (objectType != -1)
                        {
                            TraceLog(LOG_DEBUG,
                                     "ResourceManager::GetModelsRequiredForMap() - Object type %d does "
                                     "not require a model",
                                     objectType);
                        }
                    }
                }
            }
            else if (arrayStart != std::string::npos)
            {
                // This is the game format (direct array) - parse manually
                TraceLog(
                    LOG_INFO,
                    "ResourceManager::GetModelsRequiredForMap() - Detected game format, parsing manually");

                // Find all objects in the array
                size_t pos = arrayStart + 1;
                std::string objectStartStr = "{";
                size_t objectStart = content.find(objectStartStr, pos);

                while (objectStart != std::string::npos)
                {
                    // Find the matching closing brace
                    size_t braceCount = 0;
                    size_t objectEnd = objectStart;

                    while (objectEnd < content.length())
                    {
                        if (content[objectEnd] == '{')
                            braceCount++;
                        else if (content[objectEnd] == '}')
                        {
                            braceCount--;
                            if (braceCount == 0)
                                break;
                        }
                        objectEnd++;
                    }

                    if (objectEnd < content.length())
                    {
                        std::string objectJson =
                            content.substr(objectStart, objectEnd - objectStart + 1);

                        // Parse modelPath field manually
                        size_t modelPathPos = objectJson.find("\"modelPath\"");
                        if (modelPathPos != std::string::npos)
                        {
                            size_t quoteStart = objectJson.find('\"', modelPathPos + 11);
                            if (quoteStart != std::string::npos)
                            {
                                size_t quoteEnd = objectJson.find('\"', quoteStart + 1);
                                if (quoteEnd != std::string::npos)
                                {
                                    std::string modelName = objectJson.substr(
                                        quoteStart + 1, quoteEnd - quoteStart - 1);
                                    if (!modelName.empty())
                                    {
                                        // Normalize model name: remove file extension and path
                                        std::string normalizedName = std::filesystem::path(modelName).stem().string();
                                        if (normalizedName.empty())
                                        {
                                            normalizedName = modelName;
                                        }
                                        
                                        // Check if this model is not already in the list
                                        if (std::find(requiredModels.begin(), requiredModels.end(),
                                                      normalizedName) == requiredModels.end())
                                        {
                                            requiredModels.push_back(normalizedName);
                                            TraceLog(LOG_INFO,
                                                     "ResourceManager::GetModelsRequiredForMap() - Found "
                                                     "model requirement: %s (normalized from %s)",
                                                     normalizedName.c_str(), modelName.c_str());
                                        }
                                    }
                                }
                            }
                        }
                        // Also check for modelName field in game format
                        else
                        {
                            size_t modelNamePos = objectJson.find("\"modelName\"");
                            if (modelNamePos != std::string::npos)
                            {
                                size_t quoteStart = objectJson.find('\"', modelNamePos + 12);
                                if (quoteStart != std::string::npos)
                                {
                                    size_t quoteEnd = objectJson.find('\"', quoteStart + 1);
                                    if (quoteEnd != std::string::npos)
                                    {
                                        std::string modelName = objectJson.substr(
                                            quoteStart + 1, quoteEnd - quoteStart - 1);
                                        if (!modelName.empty())
                                        {
                                            // Normalize model name: remove file extension and path
                                            std::string normalizedName = std::filesystem::path(modelName).stem().string();
                                            if (normalizedName.empty())
                                            {
                                                normalizedName = modelName;
                                            }
                                            
                                            // Check if this model is not already in the list
                                            if (std::find(requiredModels.begin(),
                                                          requiredModels.end(),
                                                          normalizedName) == requiredModels.end())
                                            {
                                                requiredModels.push_back(normalizedName);
                                                TraceLog(LOG_INFO,
                                                         "ResourceManager::GetModelsRequiredForMap() - Found "
                                                         "model requirement in game format: %s (normalized from %s)",
                                                         normalizedName.c_str(), modelName.c_str());
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        pos = objectEnd + 1;
                        objectStart = content.find(objectStartStr, pos);
                    }
                    else
                    {
                        break;
                    }
                }
            }
            else
            {
                TraceLog(LOG_WARNING, "ResourceManager::GetModelsRequiredForMap() - No valid JSON "
                                      "structure found in map file");
            }
        }
        else
        {
            TraceLog(LOG_WARNING, "ResourceManager::GetModelsRequiredForMap() - Could not open map file: %s",
                     mapPath.c_str());
        }
    }

    TraceLog(LOG_INFO, "ResourceManager::GetModelsRequiredForMap() - Found %d required models for map",
             requiredModels.size());
    return requiredModels;
}
