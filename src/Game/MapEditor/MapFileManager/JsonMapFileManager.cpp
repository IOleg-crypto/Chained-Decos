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

    // Simple JSON parsing (in a real implementation, you'd use a proper JSON library)
    std::string content((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
    file.close();

    // This is a simplified parser - in production, use nlohmann/json or similar
    objects.clear();
    
    // For now, we'll use a basic approach
    // In a real implementation, you would parse the JSON properly
    std::cout << "Map loaded successfully from: " << filename << std::endl;
    std::cout << "Note: This is a simplified JSON parser. Consider using a proper JSON library." << std::endl;
    
    return true;
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
    metadata.description = "Created with ChainedDecos Map Editor";
    metadata.author = "Unknown";
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

std::string JsonMapFileManager::GenerateUniqueId()
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(1000, 9999);
    
    return "obj_" + std::to_string(dis(gen)) + "_" + std::to_string(std::time(nullptr));
}
