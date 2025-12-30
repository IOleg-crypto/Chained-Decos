#include "MapObjectConverter.h"
#include <algorithm>
#include <string>

namespace MapObjectConverter
{
// ============================================================================
// Helper functions
// ============================================================================

MapObjectType IntToMapObjectType(int type)
{
    switch (type)
    {
    case 0:
        return MapObjectType::CUBE;
    case 1:
        return MapObjectType::SPHERE;
    case 2:
        return MapObjectType::CYLINDER;
    case 3:
        return MapObjectType::PLANE;
    case 4:
        return MapObjectType::LIGHT;
    case 5:
        return MapObjectType::MODEL;
    case 6:
        return MapObjectType::SPAWN_ZONE;
    default:
        return MapObjectType::CUBE;
    }
}

int MapObjectTypeToInt(MapObjectType type)
{
    switch (type)
    {
    case MapObjectType::CUBE:
        return 0;
    case MapObjectType::SPHERE:
        return 1;
    case MapObjectType::CYLINDER:
        return 2;
    case MapObjectType::PLANE:
        return 3;
    case MapObjectType::LIGHT:
        return 4;
    case MapObjectType::MODEL:
        return 5;
    case MapObjectType::SPAWN_ZONE:
        return 6;
    default:
        return 0;
    }
}

Vector3 SanitizeVector3(const Vector3 &vec)
{
    Vector3 result = vec;
    if (!std::isfinite(result.x))
        result.x = 0.0f;
    if (!std::isfinite(result.y))
        result.y = 0.0f;
    if (!std::isfinite(result.z))
        result.z = 0.0f;
    return result;
}

float SanitizeFloat(float value, float fallback)
{
    return std::isfinite(value) ? value : fallback;
}

// Color conversion helpers
Color StringToColor(const std::string &colorStr)
{
    // Simple color name to Color conversion
    if (colorStr == "white")
        return WHITE;
    if (colorStr == "black")
        return BLACK;
    if (colorStr == "red")
        return RED;
    if (colorStr == "green")
        return GREEN;
    if (colorStr == "blue")
        return BLUE;
    if (colorStr == "yellow")
        return YELLOW;
    if (colorStr == "orange")
        return ORANGE;
    if (colorStr == "pink")
        return PINK;
    if (colorStr == "purple")
        return PURPLE;
    if (colorStr == "brown")
        return BROWN;
    if (colorStr == "gray")
        return GRAY;
    return WHITE; // Default
}

std::string ColorToString(const Color &color)
{
    // Convert Color to string (simple version)
    if (ColorIsEqual(color, WHITE))
        return "white";
    if (ColorIsEqual(color, BLACK))
        return "black";
    if (ColorIsEqual(color, RED))
        return "red";
    if (ColorIsEqual(color, GREEN))
        return "green";
    if (ColorIsEqual(color, BLUE))
        return "blue";
    if (ColorIsEqual(color, YELLOW))
        return "yellow";
    if (ColorIsEqual(color, ORANGE))
        return "orange";
    if (ColorIsEqual(color, PINK))
        return "pink";
    if (ColorIsEqual(color, PURPLE))
        return "purple";
    if (ColorIsEqual(color, BROWN))
        return "brown";
    if (ColorIsEqual(color, GRAY))
        return "gray";
    return "white"; // Default
}

// ============================================================================
// JsonSerializableObject <-> MapObjectData conversions
// ============================================================================

MapObjectData JsonSerializableObjectToMapObjectData(const JsonSerializableObject &jsonObj)
{
    MapObjectData data;

    data.name = jsonObj.name.empty() ? "object_" + std::to_string(time(nullptr)) : jsonObj.name;
    data.type = IntToMapObjectType(jsonObj.type);
    data.position = SanitizeVector3(jsonObj.position);
    data.rotation = SanitizeVector3(jsonObj.rotation);
    data.scale = Vector3{jsonObj.scale, jsonObj.scale, jsonObj.scale};

    // Ensure scale is not zero
    if (data.scale.x == 0.0f && data.scale.y == 0.0f && data.scale.z == 0.0f)
    {
        data.scale = {1.0f, 1.0f, 1.0f};
    }

    data.color = StringToColor(jsonObj.color);
    data.modelName = jsonObj.modelName;

    // Set shape-specific properties based on object type
    switch (data.type)
    {
    case MapObjectType::SPHERE:
        data.radius = SanitizeFloat(jsonObj.radiusSphere, 1.0f);
        data.height = 0.0f;
        data.size = {0.0f, 0.0f};
        break;
    case MapObjectType::CYLINDER:
        data.radius = SanitizeFloat(jsonObj.radiusH, 1.0f);
        data.height = SanitizeFloat(jsonObj.radiusV, 1.0f);
        data.size = {0.0f, 0.0f};
        break;
    case MapObjectType::PLANE:
        data.radius = 0.0f;
        data.height = 0.0f;
        data.size.x = SanitizeFloat(jsonObj.size.x, 0.0f);
        data.size.y = SanitizeFloat(jsonObj.size.y, 0.0f);
        break;
    case MapObjectType::LIGHT:
        data.radius = 0.0f;
        data.height = 0.0f;
        data.size = {0.0f, 0.0f};
        break;
    case MapObjectType::MODEL:
        data.radius = 0.0f;
        data.height = SanitizeFloat(jsonObj.radiusV, data.scale.y);
        data.size = {0.0f, 0.0f};
        break;
    case MapObjectType::SPAWN_ZONE:
        data.radius = 0.0f;
        data.height = 0.0f;
        data.size = {0.0f, 0.0f};
        break;
    case MapObjectType::CUBE:
    default:
        data.radius = 0.0f;
        data.height = data.scale.y;
        data.size = {0.0f, 0.0f};
        break;
    }

    // Default collision properties
    data.isPlatform = true;
    data.isObstacle = false;

    // Material properties
    data.texturePath = jsonObj.texturePath;
    data.tiling = jsonObj.tiling;

    return data;
}

JsonSerializableObject MapObjectDataToJsonSerializableObject(const MapObjectData &data)
{
    JsonSerializableObject jsonObj;

    jsonObj.name = data.name;
    jsonObj.type = MapObjectTypeToInt(data.type);
    jsonObj.position = data.position;
    jsonObj.rotation = data.rotation;
    jsonObj.scale = (data.scale.x + data.scale.y + data.scale.z) / 3.0f; // Average scale
    jsonObj.color = ColorToString(data.color);
    jsonObj.modelName = data.modelName;
    jsonObj.visible = true;
    jsonObj.layer = "default";
    jsonObj.tags = "exported";

    // Generate unique ID
    jsonObj.id =
        "obj_" + std::to_string(rand() % 9000 + 1000) + "_" + std::to_string(time(nullptr));

    // Set shape-specific properties based on object type
    switch (data.type)
    {
    case MapObjectType::SPHERE:
        jsonObj.radiusSphere = data.radius;
        jsonObj.radiusH = 0.0f;
        jsonObj.radiusV = 0.0f;
        jsonObj.size = {0.0f, 0.0f};
        break;
    case MapObjectType::CYLINDER:
        jsonObj.radiusSphere = 0.0f;
        jsonObj.radiusH = data.radius;
        jsonObj.radiusV = data.height;
        jsonObj.size = {0.0f, 0.0f};
        break;
    case MapObjectType::PLANE:
        jsonObj.radiusSphere = 0.0f;
        jsonObj.radiusH = 0.0f;
        jsonObj.radiusV = 0.0f;
        jsonObj.size = data.size;
        break;
    case MapObjectType::LIGHT:
        jsonObj.radiusSphere = 0.0f;
        jsonObj.radiusH = 0.0f;
        jsonObj.radiusV = 0.0f;
        jsonObj.size = {0.0f, 0.0f};
        break;
    case MapObjectType::MODEL:
        jsonObj.radiusSphere = 0.0f;
        jsonObj.radiusH = 0.0f;
        jsonObj.radiusV = data.height;
        jsonObj.size = {0.0f, 0.0f};
        break;
    case MapObjectType::SPAWN_ZONE:
        jsonObj.radiusSphere = 0.0f;
        jsonObj.radiusH = 0.0f;
        jsonObj.radiusV = 0.0f;
        jsonObj.size = {0.0f, 0.0f};
        break;
    case MapObjectType::CUBE:
    default:
        jsonObj.radiusSphere = 0.0f;
        jsonObj.radiusH = 0.0f;
        jsonObj.radiusV = 0.0f;
        jsonObj.size = {0.0f, 0.0f};
        break;
    }

    // Material properties
    jsonObj.texturePath = data.texturePath;
    jsonObj.tiling = data.tiling;

    return jsonObj;
}
} // namespace MapObjectConverter

// Note: MapObject conversions are implemented in MapEditor/Editor/FileManager/FileManager.cpp
// because MapObject is an Editor-specific class and cannot be included in Engine code.
// These functions will be moved to use the converter when MapObject is refactored.
