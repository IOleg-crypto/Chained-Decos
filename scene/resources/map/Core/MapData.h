#ifndef MAPDATA_H
#define MAPDATA_H

#include <raylib.h>
#include <string>


// ============================================================================
// Enumerations
// ============================================================================

enum class MapObjectType : uint8_t
{
    CUBE = 0,
    SPHERE = 1,
    CYLINDER = 2,
    PLANE = 3,
    LIGHT = 4,
    MODEL = 5,
    SPAWN_ZONE = 6
};

// ============================================================================
// Data Structures
// ============================================================================

struct MapObjectData
{
    std::string name;
    MapObjectType type;
    Vector3 position;
    Vector3 rotation;
    Vector3 scale;
    Color color;
    std::string modelName; // For MODEL type objects

    // Shape-specific properties
    float radius; // For spheres
    float height; // For cylinders
    Vector2 size; // For planes

    // Collision properties
    bool isPlatform;
    bool isObstacle;
};

struct MapMetadata
{
    std::string name;
    std::string displayName;
    std::string description;
    std::string author;
    std::string version;
    Vector3 startPosition;
    Vector3 endPosition;
    Color skyColor;
    Color groundColor;
    float difficulty;

    // Additional fields for JSON export compatibility
    std::string createdDate;
    std::string modifiedDate;
    Vector3 worldBounds;
    Color backgroundColor;
    std::string skyboxTexture;
};

#endif /* MAPDATA_H */
