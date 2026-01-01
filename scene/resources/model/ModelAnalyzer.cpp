#include "ModelAnalyzer.h"
#include "core/Log.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <raylib.h>

using json = nlohmann::json;

namespace CHEngine
{
std::string ModelAnalyzer::GetModelNameForObjectType(int objectType, const std::string &modelName)
{
    MapObjectType type = static_cast<MapObjectType>(objectType);

    switch (type)
    {
    case MapObjectType::MODEL:
        if (!modelName.empty())
        {
            CD_CORE_TRACE("[ModelAnalyzer] MODEL object requires model: %s", modelName.c_str());
            return modelName;
        }
        CD_CORE_WARN("[ModelAnalyzer] MODEL object has no modelName specified");
        return "";

    case MapObjectType::LIGHT:
        // Handle incorrectly exported MODEL objects as LIGHT type
        // This is a known issue with the map editor
        if (!modelName.empty())
        {
            CD_CORE_TRACE("[ModelAnalyzer] LIGHT object (likely MODEL) requires model: %s",
                          modelName.c_str());
            return modelName;
        }
        return "";

    case MapObjectType::CUBE:
    case MapObjectType::SPHERE:
    case MapObjectType::CYLINDER:
    case MapObjectType::PLANE:
        // Primitives are rendered procedurally, but may have custom models
        if (!modelName.empty())
        {
            CD_CORE_TRACE("[ModelAnalyzer] Primitive object with custom model: %s",
                          modelName.c_str());
            return modelName;
        }
        return "";

    default:
        CD_CORE_WARN("[ModelAnalyzer] Unknown object type: %d", objectType);
        return "";
    }
}

std::vector<std::string> ModelAnalyzer::GetModelsRequiredForMap(const std::string &mapIdentifier)
{
    std::vector<std::string> requiredModels;

    // Always include the player model as it's essential for gameplay
    requiredModels.emplace_back("player_low");

    // Convert map identifier to full path
    std::string mapPath = ConvertToMapPath(mapIdentifier);

    // Verify it's a JSON or CHSCENE file
    std::string extension = std::filesystem::path(mapPath).extension().string();
    if (extension != ".json" && extension != ".chscene")
    {
        CD_CORE_WARN("[ModelAnalyzer] Map file is not JSON/CHSCENE format: %s", mapPath.c_str());
        return requiredModels;
    }

    CD_CORE_INFO("[ModelAnalyzer] Analyzing map for model requirements: %s", mapPath.c_str());

    // Read map file content
    std::ifstream file(mapPath);
    if (!file.is_open())
    {
        CD_CORE_ERROR("[ModelAnalyzer] Could not open map file: %s", mapPath.c_str());
        return requiredModels;
    }

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    // Detect format and analyze accordingly
    size_t objectsStart = content.find("\"objects\"");
    size_t arrayStart = content.find("[");

    if (objectsStart != std::string::npos)
    {
        CD_CORE_INFO("[ModelAnalyzer] Detected editor format map");
        AnalyzeEditorFormat(content, requiredModels);
    }
    else if (arrayStart != std::string::npos)
    {
        CD_CORE_INFO("[ModelAnalyzer] Detected game format map");
        AnalyzeGameFormat(content, requiredModels);
    }
    else
    {
        CD_CORE_WARN("[ModelAnalyzer] No valid JSON structure found in map file");
    }

    CD_CORE_INFO("[ModelAnalyzer] Found %d required models for map",
                 static_cast<int>(requiredModels.size()));

    return requiredModels;
}

void ModelAnalyzer::AnalyzeEditorFormat(const std::string &content,
                                        std::vector<std::string> &requiredModels)
{
    json j;
    try
    {
        j = json::parse(content);
    }
    catch (const std::exception &e)
    {
        CD_CORE_ERROR("[ModelAnalyzer] Error parsing editor format JSON: %s", e.what());
        return;
    }

    if (!j.contains("objects") || !j["objects"].is_array())
    {
        CD_CORE_WARN("[ModelAnalyzer] Editor format map has no objects array");
        return;
    }

    for (const auto &object : j["objects"])
    {
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

        // Determine if this object needs a model
        std::string modelName = GetModelNameForObjectType(objectType, objectModelName);

        if (!modelName.empty())
        {
            std::string normalizedName = NormalizeModelName(modelName);
            if (AddModelIfUnique(normalizedName, requiredModels))
            {
                CD_CORE_INFO(
                    "[ModelAnalyzer] Object type %d requires model: %s (normalized from %s)",
                    objectType, normalizedName.c_str(), modelName.c_str());
            }
        }
        else if (objectType != -1)
        {
            CD_CORE_TRACE("[ModelAnalyzer] Object type %d does not require a model", objectType);
        }
    }
}

void ModelAnalyzer::AnalyzeGameFormat(const std::string &content,
                                      std::vector<std::string> &requiredModels)
{
    size_t arrayStart = content.find("[");
    if (arrayStart == std::string::npos)
    {
        return;
    }

    // Find all objects in the array
    size_t pos = arrayStart + 1;
    size_t objectStart = content.find("{", pos);

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

        if (objectEnd >= content.length())
        {
            break;
        }

        std::string objectJson = content.substr(objectStart, objectEnd - objectStart + 1);

        // Parse modelPath field
        size_t modelPathPos = objectJson.find("\"modelPath\"");
        if (modelPathPos != std::string::npos)
        {
            size_t quoteStart = objectJson.find('\"', modelPathPos + 11);
            if (quoteStart != std::string::npos)
            {
                size_t quoteEnd = objectJson.find('\"', quoteStart + 1);
                if (quoteEnd != std::string::npos)
                {
                    std::string modelName =
                        objectJson.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
                    if (!modelName.empty())
                    {
                        std::string normalizedName = NormalizeModelName(modelName);
                        if (AddModelIfUnique(normalizedName, requiredModels))
                        {
                            CD_CORE_INFO(
                                "[ModelAnalyzer] Found model requirement: %s (normalized from %s)",
                                normalizedName.c_str(), modelName.c_str());
                        }
                    }
                }
            }
        }
        // Also check for modelName field
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
                        std::string modelName =
                            objectJson.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
                        if (!modelName.empty())
                        {
                            std::string normalizedName = NormalizeModelName(modelName);
                            if (AddModelIfUnique(normalizedName, requiredModels))
                            {
                                CD_CORE_INFO("[ModelAnalyzer] Found model requirement: %s "
                                             "(normalized from %s)",
                                             normalizedName.c_str(), modelName.c_str());
                            }
                        }
                    }
                }
            }
        }

        pos = objectEnd + 1;
        objectStart = content.find("{", pos);
    }
}

std::string ModelAnalyzer::ConvertToMapPath(const std::string &mapIdentifier)
{
    std::string mapPath = mapIdentifier;

    // Check if already a full path with extension
    size_t dotPos = mapPath.find_last_of('.');
    if (dotPos != std::string::npos && dotPos + 1 < mapPath.length())
    {
        std::string ext = mapPath.substr(dotPos + 1);
        if (ext == "json" || ext == "chscene")
        {
            return mapPath;
        }
    }

    // Construct path from map name
    std::string filename = std::filesystem::path(mapIdentifier).filename().string();
    mapPath = std::string(PROJECT_ROOT_DIR) + "/resources/maps/" + filename;

    // Prefer .chscene extension, fallback to .json
    if (mapPath.find(".chscene") == std::string::npos && mapPath.find(".json") == std::string::npos)
    {
        mapPath += ".chscene";
    }

    return mapPath;
}

std::string ModelAnalyzer::NormalizeModelName(const std::string &modelName)
{
    if (modelName.empty())
    {
        return modelName;
    }

    // Remove file extension and path, keep only filename
    std::string normalizedName = std::filesystem::path(modelName).stem().string();

    // If stem() returned empty, use original name
    if (normalizedName.empty())
    {
        normalizedName = modelName;
    }

    return normalizedName;
}

bool ModelAnalyzer::AddModelIfUnique(const std::string &modelName,
                                     std::vector<std::string> &requiredModels)
{
    if (modelName.empty())
    {
        return false;
    }

    // Check if model already in list
    if (std::find(requiredModels.begin(), requiredModels.end(), modelName) != requiredModels.end())
    {
        CD_CORE_TRACE("[ModelAnalyzer] Model %s already in requirements list", modelName.c_str());
        return false;
    }

    requiredModels.push_back(modelName);
    return true;
}
} // namespace CHEngine
