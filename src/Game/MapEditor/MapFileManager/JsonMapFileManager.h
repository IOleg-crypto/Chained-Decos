//
// Created by AI Assistant
//

#ifndef JSONMAPFILEMANAGER_H
#define JSONMAPFILEMANAGER_H

#include <string>
#include <vector>
#include <raylib.h>

// Enhanced object structure for JSON serialization
struct JsonSerializableObject
{
    Vector3 position;
    Vector3 scale;
    Vector3 rotation;
    Color color;
    std::string name;
    int type;
    std::string modelName;
    Vector2 size;           // For plane objects
    float radiusH;          // For ellipse objects
    float radiusV;          // For ellipse objects
    float radiusSphere;     // For sphere objects
    
    // Additional metadata
    std::string id;         // Unique identifier
    bool visible;           // Visibility state
    std::string layer;      // Layer/group name
    std::string tags;       // Comma-separated tags
};

// Map metadata structure
struct MapMetadata
{
    std::string version;
    std::string name;
    std::string description;
    std::string author;
    std::string createdDate;
    std::string modifiedDate;
    Vector3 worldBounds;    // World size bounds
    Color backgroundColor;
    std::string skyboxTexture;
};

// JSON-based map file manager
class JsonMapFileManager
{
public:
    // Core file operations
    static bool SaveMap(const std::vector<JsonSerializableObject>& objects, 
                       const std::string& filename, 
                       const MapMetadata& metadata = MapMetadata());
    static bool LoadMap(std::vector<JsonSerializableObject>& objects, 
                       const std::string& filename, 
                       MapMetadata& metadata);
    
    // Utility functions
    static bool ValidateMapFile(const std::string& filename);
    static MapMetadata CreateDefaultMetadata();
    static std::string GetMapVersion(const std::string& filename);
    
    // Import/Export operations
    static bool ExportToOBJ(const std::vector<JsonSerializableObject>& objects, 
                           const std::string& filename);
    static bool ImportFromOBJ(const std::string& filename, 
                             std::vector<JsonSerializableObject>& objects);
    
    // Backup operations
    static bool CreateBackup(const std::string& filename);
    static std::vector<std::string> GetBackupFiles(const std::string& baseFilename);
    static bool RestoreFromBackup(const std::string& backupFilename, const std::string& targetFilename);
    
private:
    // Internal helper functions
    static std::string Vector3ToJson(const Vector3& vec);
    static std::string Vector2ToJson(const Vector2& vec);
    static std::string ColorToJson(const Color& color);
    static Vector3 JsonToVector3(const std::string& json);
    static Vector2 JsonToVector2(const std::string& json);
    static Color JsonToColor(const std::string& json);
    static std::string GetCurrentTimestamp();
    static std::string GenerateUniqueId();
};

#endif // JSONMAPFILEMANAGER_H
