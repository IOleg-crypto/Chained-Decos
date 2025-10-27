//
// Created by AI Assistant
//

#include "JsonMapFileManager.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <ctime>
#include <random>
#include <filesystem>

namespace fs = std::filesystem;

bool JsonMapFileManager::SaveMap(const std::vector<JsonSerializableObject>& objects,
                                const std::string& filename,
                                const MapMetadata& metadata)
{
    std::ofstream file(filename);
    if (!file.is_open())
    {
        std::cerr << "Failed to open file for writing: " << filename << std::endl;
        return false;
    }

    // Start JSON structure
    file << "{\n";
    
    // Metadata section
    file << "  \"metadata\": {\n";
    file << "    \"version\": \"" << metadata.version << "\",\n";
    file << "    \"name\": \"" << metadata.name << "\",\n";
    file << "    \"description\": \"" << metadata.description << "\",\n";
    file << "    \"author\": \"" << metadata.author << "\",\n";
    file << "    \"createdDate\": \"" << metadata.createdDate << "\",\n";
    file << "    \"modifiedDate\": \"" << GetCurrentTimestamp() << "\",\n";
    file << "    \"worldBounds\": " << Vector3ToJson(metadata.worldBounds) << ",\n";
    file << "    \"backgroundColor\": " << ColorToJson(metadata.backgroundColor) << ",\n";
    file << "    \"skyboxTexture\": \"" << metadata.skyboxTexture << "\"\n";
    file << "  },\n";
    
    // Objects section
    file << "  \"objects\": [\n";
    
    for (size_t i = 0; i < objects.size(); ++i)
    {
        const auto& obj = objects[i];
        
        file << "    {\n";
        file << "      \"id\": \"" << obj.id << "\",\n";
        file << "      \"name\": \"" << obj.name << "\",\n";
        file << "      \"type\": " << obj.type << ",\n";
        file << "      \"position\": " << Vector3ToJson(obj.position) << ",\n";
        file << "      \"scale\": " << Vector3ToJson(obj.scale) << ",\n";
        file << "      \"rotation\": " << Vector3ToJson(obj.rotation) << ",\n";
        file << "      \"color\": " << ColorToJson(obj.color) << ",\n";
        file << "      \"modelName\": \"" << obj.modelName << "\",\n";
        file << "      \"size\": " << Vector2ToJson(obj.size) << ",\n";
        file << "      \"radiusH\": " << obj.radiusH << ",\n";
        file << "      \"radiusV\": " << obj.radiusV << ",\n";
        file << "      \"radiusSphere\": " << obj.radiusSphere << ",\n";
        file << "      \"visible\": " << (obj.visible ? "true" : "false") << ",\n";
        file << "      \"layer\": \"" << obj.layer << "\",\n";
        file << "      \"tags\": \"" << obj.tags << "\"\n";
        
        if (i < objects.size() - 1)
            file << "    },\n";
        else
            file << "    }\n";
    }
    
    file << "  ]\n";
    file << "}\n";
    
    file.close();
    std::cout << "Map saved successfully to: " << filename << std::endl;
    return true;
}

bool JsonMapFileManager::LoadMap(std::vector<JsonSerializableObject>& objects,
                                       const std::string& filename,
                                       MapMetadata& metadata)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        std::cerr << "Failed to open file for reading: " << filename << std::endl;
        return false;
    }

    // Read entire file content
    std::string content((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
    file.close();

    // Parse JSON manually (simple implementation)
    objects.clear();

    try
    {
        // Parse metadata section
        size_t metadataStart = content.find("\"metadata\"");
        if (metadataStart != std::string::npos)
        {
            metadataStart = content.find("{", metadataStart);
            size_t metadataEnd = content.find("}", metadataStart);

            std::string metadataJson = content.substr(metadataStart, metadataEnd - metadataStart + 1);

            // Parse metadata fields
            ParseMetadataField(metadataJson, "\"version\"", metadata.version);
            ParseMetadataField(metadataJson, "\"name\"", metadata.name);
            ParseMetadataField(metadataJson, "\"description\"", metadata.description);
            ParseMetadataField(metadataJson, "\"author\"", metadata.author);
            ParseMetadataField(metadataJson, "\"createdDate\"", metadata.createdDate);
            ParseMetadataField(metadataJson, "\"modifiedDate\"", metadata.modifiedDate);
            ParseMetadataField(metadataJson, "\"skyboxTexture\"", metadata.skyboxTexture);

            // Parse world bounds
            size_t boundsStart = metadataJson.find("\"worldBounds\"");
            if (boundsStart != std::string::npos)
            {
                boundsStart = metadataJson.find("[", boundsStart);
                size_t boundsEnd = metadataJson.find("]", boundsStart);
                std::string boundsStr = metadataJson.substr(boundsStart, boundsEnd - boundsStart + 1);
                metadata.worldBounds = ParseVector3(boundsStr);
            }

            // Parse background color
            size_t colorStart = metadataJson.find("\"backgroundColor\"");
            if (colorStart != std::string::npos)
            {
                colorStart = metadataJson.find("[", colorStart);
                size_t colorEnd = metadataJson.find("]", colorStart);
                std::string colorStr = metadataJson.substr(colorStart, colorEnd - colorStart + 1);
                metadata.backgroundColor = ParseColor(colorStr);
            }
        }

        // Parse objects section
        size_t objectsStart = content.find("\"objects\"");
        if (objectsStart != std::string::npos)
        {
            objectsStart = content.find("[", objectsStart);
            size_t objectsEnd = content.find("]", objectsStart);

            if (objectsEnd == std::string::npos)
                objectsEnd = content.length() - 1;

            std::string objectsJson = content.substr(objectsStart, objectsEnd - objectsStart + 1);

            // Parse individual objects
            ParseObjectsArray(objectsJson, objects);
        }

        std::cout << "Map loaded successfully from: " << filename << std::endl;
        std::cout << "Loaded " << objects.size() << " objects" << std::endl;
        return true;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error parsing JSON: " << e.what() << std::endl;
        return false;
    }
}

bool JsonMapFileManager::ValidateMapFile(const std::string& filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
        return false;

    // Basic validation - check if file starts with '{'
    char firstChar;
    file >> firstChar;
    file.close();
    
    return firstChar == '{';
}

MapMetadata JsonMapFileManager::CreateDefaultMetadata()
{
    MapMetadata metadata;
    metadata.version = "1.0";
    metadata.name = "Untitled Map";
    metadata.displayName = "Untitled Map";
    metadata.description = "Created with ChainedDecos Map Editor";
    metadata.author = "Unknown";
    metadata.startPosition = {0.0f, 2.0f, 0.0f};
    metadata.endPosition = {0.0f, 2.0f, 0.0f};
    metadata.skyColor = SKYBLUE;
    metadata.groundColor = DARKGREEN;
    metadata.difficulty = 1.0f;
    metadata.createdDate = GetCurrentTimestamp();
    metadata.modifiedDate = GetCurrentTimestamp();
    metadata.worldBounds = {100.0f, 100.0f, 100.0f};
    metadata.backgroundColor = {50, 50, 50, 255};
    metadata.skyboxTexture = "";

    return metadata;
}

std::string JsonMapFileManager::GetMapVersion(const std::string& filename)
{
    // Simplified version extraction
    return "1.0";
}

bool JsonMapFileManager::ExportToOBJ(const std::vector<JsonSerializableObject>& objects,
                                           const std::string& filename)
{
    std::ofstream file(filename);
    if (!file.is_open())
        return false;

    file << "# Exported from ChainedDecos Map Editor\n";
    file << "# Object count: " << objects.size() << "\n\n";

    for (size_t i = 0; i < objects.size(); ++i)
    {
        const auto& obj = objects[i];
        
        file << "o " << obj.name << "\n";
        file << "v " << obj.position.x << " " << obj.position.y << " " << obj.position.z << "\n";
        file << "# Scale: " << obj.scale.x << " " << obj.scale.y << " " << obj.scale.z << "\n";
        file << "# Color: " << (int)obj.color.r << " " << (int)obj.color.g << " " << (int)obj.color.b << "\n\n";
    }

    file.close();
    return true;
}

bool JsonMapFileManager::ImportFromOBJ(const std::string& filename,
                                              std::vector<JsonSerializableObject>& objects)
{
    // Simplified OBJ import
    std::cout << "OBJ import not fully implemented yet" << std::endl;
    return false;
}

bool JsonMapFileManager::ExportGameMap(const std::vector<JsonSerializableObject>& objects,
                                              const std::string& filename,
                                              const MapMetadata& metadata)
{
    std::ofstream file(filename);
    if (!file.is_open())
    {
        std::cerr << "Failed to open file for writing: " << filename << std::endl;
        return false;
    }

    // Group objects by modelName to create models.json format
    std::map<std::string, std::vector<const JsonSerializableObject*>> modelGroups;

    for (const auto& obj : objects)
    {
        if (!obj.modelName.empty())
        {
            modelGroups[obj.modelName].push_back(&obj);
        }
    }

    // Start JSON array structure (models.json format)
    file << "[\n";

    size_t modelIndex = 0;
    const size_t totalModels = modelGroups.size();

    for (const auto& [modelName, modelObjects] : modelGroups)
    {
        if (modelObjects.empty()) continue;

        file << "  {\n";
        file << "    \"name\": \"" << modelName << "\",\n";

        // Determine the appropriate file extension and path
        std::string modelPath = GetModelPathForModel(modelName);

        file << "    \"path\": \"" << modelPath << "\",\n";
        file << "    \"spawn\": true,\n";
        file << "    \"hasCollision\": true,\n";
        file << "    \"collisionPrecision\": \"bvh_only\",\n";
        file << "    \"hasAnimations\": " << (HasAnimations(modelPath) ? "true" : "false") << ",\n";

        // Create instances array from objects
        file << "    \"instances\": [\n";

        for (size_t i = 0; i < modelObjects.size(); ++i)
        {
            const auto& obj = *modelObjects[i];

            file << "      {\n";
            file << "        \"position\": " << Vector3ToJson(obj.position) << ",\n";

            // Calculate average scale for the instance
            float avgScale = (obj.scale.x + obj.scale.y + obj.scale.z) / 3.0f;
            file << "        \"scale\": " << avgScale << ",\n";
            file << "        \"spawn\": true\n";

            if (i < modelObjects.size() - 1)
                file << "      },\n";
            else
                file << "      }\n";
        }

        file << "    ]\n";

        if (modelIndex < totalModels - 1)
            file << "  },\n";
        else
            file << "  }\n";

        modelIndex++;
    }

    file << "]\n";
    file.close();

    std::cout << "Game map exported successfully to: " << filename << std::endl;
    return true;
}

bool JsonMapFileManager::ImportGameMap(std::vector<JsonSerializableObject>& objects,
                                              const std::string& filename,
                                              MapMetadata& metadata)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        std::cerr << "Failed to open file for reading: " << filename << std::endl;
        return false;
    }

    // Read entire file content
    std::string content((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
    file.close();

    objects.clear();

    try
    {
        // Check if this is the editor format (with metadata), exported editor format (with objects), or game format (direct array)
        size_t metadataStart = content.find("\"metadata\"");
        size_t objectsStart = content.find("\"objects\"");
        size_t arrayStart = content.find("[");

        if (metadataStart != std::string::npos)
        {
            // This is the editor format with metadata - use LoadMap instead
            std::cout << "Detected editor format, using LoadMap parser" << std::endl;
            return LoadMap(objects, filename, metadata);
        }
        else if (objectsStart != std::string::npos)
        {
            // This is the exported editor format with objects array but no metadata
            std::cout << "Detected exported editor format, using LoadMap parser" << std::endl;
            return LoadMap(objects, filename, metadata);
        }
        else if (arrayStart != std::string::npos)
        {
            // This is the game format (direct array)
            std::cout << "Detected game format, using game map parser" << std::endl;

            size_t pos = arrayStart + 1;
            std::string searchStr = "{";
            size_t objectStart = content.find(searchStr, pos);

            while (objectStart != std::string::npos)
            {
                // Find the matching closing brace
                size_t objectEnd = FindMatchingBrace(content, objectStart);

                if (objectEnd != std::string::npos)
                {
                    std::string objectJson = content.substr(objectStart, objectEnd - objectStart + 1);
                    JsonSerializableObject obj;
                    ParseGameMapObject(objectJson, obj);
                    objects.push_back(obj);

                    pos = objectEnd + 1;
                    objectStart = content.find(searchStr, pos);
                }
                else
                {
                    break;
                }
            }
        }
        else
        {
            std::cerr << "Error: No valid JSON structure found" << std::endl;
            return false;
        }

        std::cout << "Game map imported successfully from: " << filename << std::endl;
        std::cout << "Imported " << objects.size() << " objects" << std::endl;
        return true;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error parsing game map JSON: " << e.what() << std::endl;
        return false;
    }
}

bool JsonMapFileManager::CreateBackup(const std::string& filename)
{
    if (!fs::exists(filename))
        return false;

    try
    {
        std::string backupFilename = filename + ".backup." + GetCurrentTimestamp();
        fs::copy_file(filename, backupFilename);
        return true;
    }
    catch (const fs::filesystem_error& e)
    {
        std::cerr << "Failed to create backup: " << e.what() << std::endl;
        return false;
    }
}

std::vector<std::string> JsonMapFileManager::GetBackupFiles(const std::string& baseFilename)
{
    std::vector<std::string> backups;
    
    try
    {
        fs::path basePath(baseFilename);
        fs::path directory = basePath.parent_path();
        std::string baseName = basePath.stem().string();
        std::string extension = basePath.extension().string();
        
        for (const auto& entry : fs::directory_iterator(directory))
        {
            if (entry.is_regular_file())
            {
                std::string filename = entry.path().filename().string();
                if (filename.find(baseName + ".backup.") == 0 && filename.find(extension) != std::string::npos)
                {
                    backups.push_back(entry.path().string());
                }
            }
        }
    }
    catch (const fs::filesystem_error& e)
    {
        std::cerr << "Failed to get backup files: " << e.what() << std::endl;
    }
    
    return backups;
}

bool JsonMapFileManager::RestoreFromBackup(const std::string& backupFilename, const std::string& targetFilename)
{
    try
    {
        if (fs::exists(targetFilename))
        {
            fs::remove(targetFilename);
        }
        fs::copy_file(backupFilename, targetFilename);
        return true;
    }
    catch (const fs::filesystem_error& e)
    {
        std::cerr << "Failed to restore from backup: " << e.what() << std::endl;
        return false;
    }
}

// JSON parsing helper functions
void JsonMapFileManager::ParseMetadataField(const std::string& json, const std::string& fieldName, std::string& target)
{
    size_t start = json.find(fieldName);
    if (start != std::string::npos)
    {
        start = json.find("\"", start + fieldName.length());
        if (start != std::string::npos)
        {
            size_t end = json.find("\"", start + 1);
            if (end != std::string::npos)
            {
                target = json.substr(start + 1, end - start - 1);
            }
        }
    }
}

Vector3 JsonMapFileManager::ParseVector3(const std::string& json)
{
    Vector3 result = {0, 0, 0};

    // Simple parsing: [x, y, z]
    size_t start = json.find("[");
    size_t end = json.find("]");

    if (start != std::string::npos && end != std::string::npos)
    {
        std::string values = json.substr(start + 1, end - start - 1);

        // Parse comma-separated values
        std::stringstream ss(values);
        std::string token;

        if (std::getline(ss, token, ','))
            result.x = std::stof(token);
        if (std::getline(ss, token, ','))
            result.y = std::stof(token);
        if (std::getline(ss, token, ','))
            result.z = std::stof(token);
    }

    return result;
}

Color JsonMapFileManager::ParseColor(const std::string& json)
{
    Color result = {255, 255, 255, 255};

    // Simple parsing: [r, g, b, a]
    size_t start = json.find("[");
    size_t end = json.find("]");

    if (start != std::string::npos && end != std::string::npos)
    {
        std::string values = json.substr(start + 1, end - start - 1);

        // Parse comma-separated values
        std::stringstream ss(values);
        std::string token;

        if (std::getline(ss, token, ','))
            result.r = static_cast<unsigned char>(std::stoi(token));
        if (std::getline(ss, token, ','))
            result.g = static_cast<unsigned char>(std::stoi(token));
        if (std::getline(ss, token, ','))
            result.b = static_cast<unsigned char>(std::stoi(token));
        if (std::getline(ss, token, ','))
            result.a = static_cast<unsigned char>(std::stoi(token));
    }

    return result;
}

void JsonMapFileManager::ParseObjectsArray(const std::string& json, std::vector<JsonSerializableObject>& objects)
{
    size_t pos = 0;
    std::string searchStr = "{";
    size_t objectStart = json.find(searchStr, pos);

    while (objectStart != std::string::npos)
    {
        // Find the matching closing brace
        size_t objectEnd = FindMatchingBrace(json, objectStart);

        if (objectEnd != std::string::npos)
        {
            std::string objectJson = json.substr(objectStart, objectEnd - objectStart + 1);
            JsonSerializableObject obj;
            ParseObject(objectJson, obj);
            objects.push_back(obj);

            pos = objectEnd + 1;
            objectStart = json.find(searchStr, pos);
        }
        else
        {
            break;
        }
    }
}

size_t JsonMapFileManager::FindMatchingBrace(const std::string& json, size_t startPos)
{
    int braceCount = 0;
    size_t pos = startPos;

    while (pos < json.length())
    {
        if (json[pos] == '{')
            braceCount++;
        else if (json[pos] == '}')
        {
            braceCount--;
            if (braceCount == 0)
                return pos;
        }
        pos++;
    }

    return std::string::npos;
}

void JsonMapFileManager::ParseObject(const std::string& json, JsonSerializableObject& obj)
{
    // Parse basic fields
    ParseMetadataField(json, "\"id\"", obj.id);
    ParseMetadataField(json, "\"name\"", obj.name);
    ParseMetadataField(json, "\"modelName\"", obj.modelName);
    ParseMetadataField(json, "\"layer\"", obj.layer);
    ParseMetadataField(json, "\"tags\"", obj.tags);

    // Parse numeric fields
    obj.type = ParseIntField(json, "\"type\"");
    obj.visible = ParseBoolField(json, "\"visible\"");
    obj.radiusH = ParseFloatField(json, "\"radiusH\"");
    obj.radiusV = ParseFloatField(json, "\"radiusV\"");
    obj.radiusSphere = ParseFloatField(json, "\"radiusSphere\"");

    // Parse vectors and colors
    size_t pos = json.find("\"position\"");
    if (pos != std::string::npos)
    {
        pos = json.find("[", pos);
        size_t end = json.find("]", pos);
        if (pos != std::string::npos && end != std::string::npos)
        {
            std::string vecStr = json.substr(pos, end - pos + 1);
            obj.position = ParseVector3(vecStr);
        }
    }

    pos = json.find("\"scale\"");
    if (pos != std::string::npos)
    {
        pos = json.find("[", pos);
        size_t end = json.find("]", pos);
        if (pos != std::string::npos && end != std::string::npos)
        {
            std::string vecStr = json.substr(pos, end - pos + 1);
            obj.scale = ParseVector3(vecStr);
        }
    }

    pos = json.find("\"rotation\"");
    if (pos != std::string::npos)
    {
        pos = json.find("[", pos);
        size_t end = json.find("]", pos);
        if (pos != std::string::npos && end != std::string::npos)
        {
            std::string vecStr = json.substr(pos, end - pos + 1);
            obj.rotation = ParseVector3(vecStr);
        }
    }

    pos = json.find("\"color\"");
    if (pos != std::string::npos)
    {
        pos = json.find("[", pos);
        size_t end = json.find("]", pos);
        if (pos != std::string::npos && end != std::string::npos)
        {
            std::string colorStr = json.substr(pos, end - pos + 1);
            obj.color = ParseColor(colorStr);
        }
    }

    pos = json.find("\"size\"");
    if (pos != std::string::npos)
    {
        pos = json.find("[", pos);
        size_t end = json.find("]", pos);
        if (pos != std::string::npos && end != std::string::npos)
        {
            std::string sizeStr = json.substr(pos, end - pos + 1);
            obj.size = ParseVector2(sizeStr);
        }
    }
}

int JsonMapFileManager::ParseIntField(const std::string& json, const std::string& fieldName)
{
    size_t start = json.find(fieldName);
    if (start != std::string::npos)
    {
        start = json.find(":", start);
        if (start != std::string::npos)
        {
            start++;
            // Skip whitespace
            while (start < json.length() && (json[start] == ' ' || json[start] == '\t'))
                start++;

            size_t end = start;
            while (end < json.length() && json[end] != ',' && json[end] != '}')
                end++;

            std::string value = json.substr(start, end - start);
            return std::stoi(value);
        }
    }
    return 0;
}

float JsonMapFileManager::ParseFloatField(const std::string& json, const std::string& fieldName)
{
    size_t start = json.find(fieldName);
    if (start != std::string::npos)
    {
        start = json.find(":", start);
        if (start != std::string::npos)
        {
            start++;
            // Skip whitespace
            while (start < json.length() && (json[start] == ' ' || json[start] == '\t'))
                start++;

            size_t end = start;
            while (end < json.length() && json[end] != ',' && json[end] != '}')
                end++;

            std::string value = json.substr(start, end - start);
            return std::stof(value);
        }
    }
    return 0.0f;
}

bool JsonMapFileManager::ParseBoolField(const std::string& json, const std::string& fieldName)
{
    size_t start = json.find(fieldName);
    if (start != std::string::npos)
    {
        start = json.find(":", start);
        if (start != std::string::npos)
        {
            start++;
            // Skip whitespace
            while (start < json.length() && (json[start] == ' ' || json[start] == '\t'))
                start++;

            if (json.substr(start, 4) == "true")
                return true;
            else if (json.substr(start, 5) == "false")
                return false;
        }
    }
    return false;
}

Vector2 JsonMapFileManager::ParseVector2(const std::string& json)
{
    Vector2 result = {0, 0};

    // Simple parsing: [x, y]
    size_t start = json.find("[");
    size_t end = json.find("]");

    if (start != std::string::npos && end != std::string::npos)
    {
        std::string values = json.substr(start + 1, end - start - 1);

        // Parse comma-separated values
        std::stringstream ss(values);
        std::string token;

        if (std::getline(ss, token, ','))
            result.x = std::stof(token);
        if (std::getline(ss, token, ','))
            result.y = std::stof(token);
    }

    return result;
}

// Helper functions
std::string JsonMapFileManager::Vector3ToJson(const Vector3& vec)
{
    std::ostringstream oss;
    oss << "[" << vec.x << ", " << vec.y << ", " << vec.z << "]";
    return oss.str();
}

std::string JsonMapFileManager::Vector2ToJson(const Vector2& vec)
{
    std::ostringstream oss;
    oss << "[" << vec.x << ", " << vec.y << "]";
    return oss.str();
}

std::string JsonMapFileManager::ColorToJson(const Color& color)
{
    std::ostringstream oss;
    oss << "[" << (int)color.r << ", " << (int)color.g << ", " << (int)color.b << ", " << (int)color.a << "]";
    return oss.str();
}

Vector3 JsonMapFileManager::JsonToVector3(const std::string& json)
{
    Vector3 result = {0, 0, 0};
    // Simplified parsing - in production, use proper JSON parsing
    return result;
}

Vector2 JsonMapFileManager::JsonToVector2(const std::string& json)
{
    Vector2 result = {0, 0};
    // Simplified parsing - in production, use proper JSON parsing
    return result;
}

Color JsonMapFileManager::JsonToColor(const std::string& json)
{
    Color result = {255, 255, 255, 255};
    // Simplified parsing - in production, use proper JSON parsing
    return result;
}

std::string JsonMapFileManager::GetCurrentTimestamp()
{
    auto now = std::time(nullptr);
    auto tm = *std::localtime(&now);
    
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y%m%d_%H%M%S");
    return oss.str();
}

std::string JsonMapFileManager::GetUniqueId()
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(1000, 9999);

    return "obj_" + std::to_string(dis(gen)) + "_" + std::to_string(std::time(nullptr));
}

std::string JsonMapFileManager::GetObjectTypeString(int type)
{
    switch (type)
    {
        case 0: return "CUBE";
        case 1: return "SPHERE";
        case 2: return "CYLINDER";
        case 3: return "PLANE";
        case 4: return "LIGHT";
        case 5: return "MODEL";
        default: return "UNKNOWN";
    }
}

std::string JsonMapFileManager::GetModelPathForModel(const std::string& modelName)
{
    // Check for common model files in resources directory
    std::vector<std::string> possibleExtensions = {".glb", ".gltf", ".obj"};

    for (const std::string& ext : possibleExtensions)
    {
        std::string testPath = PROJECT_ROOT_DIR "/resources/" + modelName + ext;
        std::ifstream testFile(testPath);
        if (testFile.good())
        {
            testFile.close();
            return testPath;
        }
    }

    // If no file found, return .glb as default (most common)
    return "../resources/" + modelName + ".glb";
}

bool JsonMapFileManager::HasAnimations(const std::string& modelPath)
{
    // Simple heuristic: gltf and glb files often contain animations
    std::string ext = modelPath.substr(modelPath.find_last_of('.'));
    return (ext == ".gltf" || ext == ".glb");
}

void JsonMapFileManager::ParseGameMapObject(const std::string& json, JsonSerializableObject& obj)
{
    // Parse basic string fields
    ParseMetadataField(json, "\"name\"", obj.name);

    // Parse type as string first, then convert to int
    // NOTE: MapObjectType enum in MapLoader.h defines LIGHT = 4, MODEL = 5
    std::string typeStr;
    ParseMetadataField(json, "\"type\"", typeStr);
    if (typeStr == "CUBE") obj.type = 0;
    else if (typeStr == "SPHERE") obj.type = 1;
    else if (typeStr == "CYLINDER") obj.type = 2;
    else if (typeStr == "PLANE") obj.type = 3;
    else if (typeStr == "LIGHT") obj.type = 4;
    else if (typeStr == "MODEL") obj.type = 5;
    else obj.type = 0;

    // modelName is stored under "modelName" in exported objects
    ParseMetadataField(json, "\"modelName\"", obj.modelName);
    ParseMetadataField(json, "\"layer\"", obj.layer);
    ParseMetadataField(json, "\"tags\"", obj.tags);

    // Parse boolean field
    obj.visible = ParseBoolField(json, "\"visible\"");

    // Parse numeric fields
    obj.radiusSphere = ParseFloatField(json, "\"radius\"");
    obj.radiusH = ParseFloatField(json, "\"radius\"");
    obj.radiusV = ParseFloatField(json, "\"height\"");

    // Parse vectors and colors
    size_t pos = json.find("\"position\"");
    if (pos != std::string::npos)
    {
        pos = json.find("[", pos);
        size_t end = json.find("]", pos);
        if (pos != std::string::npos && end != std::string::npos)
        {
            std::string vecStr = json.substr(pos, end - pos + 1);
            obj.position = ParseVector3(vecStr);
        }
    }

    pos = json.find("\"rotation\"");
    if (pos != std::string::npos)
    {
        pos = json.find("[", pos);
        size_t end = json.find("]", pos);
        if (pos != std::string::npos && end != std::string::npos)
        {
            std::string vecStr = json.substr(pos, end - pos + 1);
            obj.rotation = ParseVector3(vecStr);
        }
    }

    pos = json.find("\"scale\"");
    if (pos != std::string::npos)
    {
        pos = json.find("[", pos);
        size_t end = json.find("]", pos);
        if (pos != std::string::npos && end != std::string::npos)
        {
            std::string vecStr = json.substr(pos, end - pos + 1);
            obj.scale = ParseVector3(vecStr);
        }
    }

    pos = json.find("\"color\"");
    if (pos != std::string::npos)
    {
        pos = json.find("[", pos);
        size_t end = json.find("]", pos);
        if (pos != std::string::npos && end != std::string::npos)
        {
            std::string colorStr = json.substr(pos, end - pos + 1);
            obj.color = ParseColor(colorStr);
        }
    }

    pos = json.find("\"size\"");
    if (pos != std::string::npos)
    {
        pos = json.find("[", pos);
        size_t end = json.find("]", pos);
        if (pos != std::string::npos && end != std::string::npos)
        {
            std::string sizeStr = json.substr(pos, end - pos + 1);
            obj.size = ParseVector2(sizeStr);
        }
    }

    // Generate unique ID if not present
    if (obj.id.empty())
    {
        obj.id = GetUniqueId();
    }
}

bool JsonMapFileManager::TestRoundTrip(const std::vector<JsonSerializableObject>& originalObjects,
                                              const std::string& testFilePath)
{
    std::cout << "Testing JSON export/import round-trip..." << std::endl;

    // Create test metadata
    MapMetadata metadata;
    metadata.version = "1.0";
    metadata.name = "Test Map";
    metadata.displayName = "Test Map";
    metadata.description = "Test map for round-trip validation";
    metadata.author = "Test System";
    metadata.startPosition = {0.0f, 2.0f, 0.0f};
    metadata.endPosition = {0.0f, 2.0f, 0.0f};
    metadata.skyColor = SKYBLUE;
    metadata.groundColor = DARKGREEN;
    metadata.difficulty = 1.0f;
    metadata.createdDate = GetCurrentTimestamp();
    metadata.modifiedDate = GetCurrentTimestamp();
    metadata.worldBounds = {100.0f, 100.0f, 100.0f};
    metadata.backgroundColor = {50, 50, 50, 255};
    metadata.skyboxTexture = "";

    // Export to JSON
    if (!JsonMapFileManager::ExportGameMap(originalObjects, testFilePath, metadata))
    {
        std::cout << "ERROR: Failed to export test map" << std::endl;
        return false;
    }

    std::cout << "✓ Exported " << originalObjects.size() << " objects to JSON" << std::endl;

    // Import from JSON
    std::vector<JsonSerializableObject> importedObjects;
    MapMetadata importedMetadata;

    if (!ImportGameMap(importedObjects, testFilePath, importedMetadata))
    {
        std::cout << "ERROR: Failed to import test map" << std::endl;
        return false;
    }

    std::cout << "✓ Imported " << importedObjects.size() << " objects from JSON" << std::endl;

    // Validate that we have the same number of objects
    if (originalObjects.size() != importedObjects.size())
    {
        std::cout << "ERROR: Object count mismatch! Original: " << originalObjects.size()
                  << ", Imported: " << importedObjects.size() << std::endl;
        return false;
    }

    // Validate each object
    bool allValid = true;
    for (size_t i = 0; i < originalObjects.size(); ++i)
    {
        const auto& original = originalObjects[i];
        const auto& imported = importedObjects[i];

        if (original.name != imported.name ||
            original.type != imported.type ||
            std::abs(original.position.x - imported.position.x) > 0.01f ||
            std::abs(original.position.y - imported.position.y) > 0.01f ||
            std::abs(original.position.z - imported.position.z) > 0.01f ||
            std::abs(original.scale.x - imported.scale.x) > 0.01f ||
            std::abs(original.scale.y - imported.scale.y) > 0.01f ||
            std::abs(original.scale.z - imported.scale.z) > 0.01f ||
            original.color.r != imported.color.r ||
            original.color.g != imported.color.g ||
            original.color.b != imported.color.b ||
            original.color.a != imported.color.a)
        {
            std::cout << "ERROR: Object " << i << " data mismatch!" << std::endl;
            std::cout << "  Original: " << original.name << " at (" << original.position.x << ", "
                      << original.position.y << ", " << original.position.z << ")" << std::endl;
            std::cout << "  Imported: " << imported.name << " at (" << imported.position.x << ", "
                      << imported.position.y << ", " << imported.position.z << ")" << std::endl;
            allValid = false;
        }
    }

    if (allValid)
    {
        std::cout << "✓ All objects validated successfully!" << std::endl;
        std::cout << "✓ Round-trip test PASSED!" << std::endl;
    }
    else
    {
        std::cout << "✗ Round-trip test FAILED!" << std::endl;
    }

    return allValid;
}

bool JsonMapFileManager::TestModelsFormatExportImport()
{
    std::cout << "Testing models.json format export/import cycle..." << std::endl;

    // Create test objects that represent a simple scene
    std::vector<JsonSerializableObject> testObjects;

    // Create a tavern object
    JsonSerializableObject tavernObj;
    tavernObj.id = "test_tavern_1";
    tavernObj.name = "Castle";
    tavernObj.type = 4; // MODEL type
    tavernObj.position = {62.1f, -1.5f, -11.7f};
    tavernObj.scale = {1.0f, 1.0f, 0.9f};
    tavernObj.rotation = {0.0f, 0.0f, 0.0f};
    tavernObj.color = {255, 255, 255, 255};
    tavernObj.modelName = "TaverGLTF";
    tavernObj.visible = true;
    tavernObj.layer = "default";
    tavernObj.tags = "";
    testObjects.push_back(tavernObj);

    // Create a player object
    JsonSerializableObject playerObj;
    playerObj.id = "test_player_1";
    playerObj.name = "Player";
    playerObj.type = 4; // MODEL type
    playerObj.position = {0.0f, 2.0f, 0.0f};
    playerObj.scale = {0.01f, 0.01f, 0.01f};
    playerObj.rotation = {0.0f, 0.0f, 0.0f};
    playerObj.color = {255, 255, 255, 255};
    playerObj.modelName = "player";
    playerObj.visible = true;
    playerObj.layer = "default";
    playerObj.tags = "";
    testObjects.push_back(playerObj);

    // Create test metadata
    MapMetadata metadata;
    metadata.version = "1.0";
    metadata.name = "Test Models Format Map";
    metadata.displayName = "Test Models Format Map";
    metadata.description = "Test map for models.json format validation";
    metadata.author = "Test System";
    metadata.createdDate = GetCurrentTimestamp();
    metadata.modifiedDate = GetCurrentTimestamp();
    metadata.worldBounds = {100.0f, 100.0f, 100.0f};
    metadata.backgroundColor = {50, 50, 50, 255};

    // Export to models.json format
    std::string testFilePath = "test_models_format.json";
    if (!ExportGameMap(testObjects, testFilePath, metadata))
    {
        std::cout << "ERROR: Failed to export test map to models.json format" << std::endl;
        return false;
    }

    std::cout << "✓ Exported " << testObjects.size() << " objects to models.json format" << std::endl;

    // Import back from models.json format
    std::vector<JsonSerializableObject> importedObjects;
    MapMetadata importedMetadata;

    if (!ImportGameMap(importedObjects, testFilePath, importedMetadata))
    {
        std::cout << "ERROR: Failed to import test map from models.json format" << std::endl;
        return false;
    }

    std::cout << "✓ Imported " << importedObjects.size() << " objects from models.json format" << std::endl;

    // Validate that we have the same number of objects
    if (testObjects.size() != importedObjects.size())
    {
        std::cout << "ERROR: Object count mismatch! Original: " << testObjects.size()
                  << ", Imported: " << importedObjects.size() << std::endl;
        return false;
    }

    // Validate each object - check that model names and positions are preserved
    bool allValid = true;
    for (size_t i = 0; i < testObjects.size(); ++i)
    {
        const auto& original = testObjects[i];
        const auto& imported = importedObjects[i];

        if (original.modelName != imported.modelName)
        {
            std::cout << "ERROR: Object " << i << " model name mismatch!" << std::endl;
            std::cout << "  Original: " << original.modelName << std::endl;
            std::cout << "  Imported: " << imported.modelName << std::endl;
            allValid = false;
        }

        if (std::abs(original.position.x - imported.position.x) > 0.1f ||
            std::abs(original.position.y - imported.position.y) > 0.1f ||
            std::abs(original.position.z - imported.position.z) > 0.1f)
        {
            std::cout << "ERROR: Object " << i << " position mismatch!" << std::endl;
            std::cout << "  Original: (" << original.position.x << ", "
                      << original.position.y << ", " << original.position.z << ")" << std::endl;
            std::cout << "  Imported: (" << imported.position.x << ", "
                      << imported.position.y << ", " << imported.position.z << ")" << std::endl;
            allValid = false;
        }
    }

    if (allValid)
    {
        std::cout << "✓ All objects validated successfully!" << std::endl;
        std::cout << "✓ Models.json format export/import test PASSED!" << std::endl;
    }
    else
    {
        std::cout << "✗ Models.json format export/import test FAILED!" << std::endl;
    }

    // Clean up test file
    std::remove(testFilePath.c_str());

    return allValid;
}
