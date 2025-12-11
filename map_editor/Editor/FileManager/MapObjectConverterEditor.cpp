#include "MapObjectConverterEditor.h"
#include "../Object/MapObject.h"
#include "scene/resources/map/Converter/MapObjectConverter.h"
#include "scene/resources/map/MapFileManager/Json/JsonMapFileManager.h"
#include <cstdlib>
#include <ctime>

#include <iomanip>
#include <sstream>


namespace MapObjectConverterEditor
{
// Helper functions for color conversion
std::string ColorToHexString(Color color)
{
    int hexValue = ColorToInt(color);
    std::stringstream ss;
    ss << "#" << std::hex << std::setw(8) << std::setfill('0') << hexValue;
    return ss.str();
}

Color HexStringToColor(const std::string &hex)
{
    if (hex.empty())
        return WHITE;
    std::string cleanHex = hex;
    if (cleanHex[0] == '#')
        cleanHex = cleanHex.substr(1);

    unsigned int hexValue;
    std::stringstream ss;
    ss << std::hex << cleanHex;
    ss >> hexValue;

    return GetColor(hexValue);
}

// ============================================================================
// MapObject (Editor) <-> JsonSerializableObject (Engine)
// ============================================================================

JsonSerializableObject MapObjectToJsonSerializableObject(const MapObject &obj)
{
    JsonSerializableObject jsonObj;

    jsonObj.position = obj.GetPosition();
    jsonObj.scale = obj.GetScale().x; // Use X component for uniform scale
    jsonObj.rotation = obj.GetRotation();
    jsonObj.color = ColorToHexString(obj.GetColor());
    jsonObj.name = obj.GetObjectName();
    jsonObj.type = obj.GetObjectType();
    jsonObj.modelName = obj.GetModelAssetName();
    jsonObj.visible = true;
    jsonObj.layer = "default";
    jsonObj.tags = "exported";
    jsonObj.id =
        "obj_" + std::to_string(rand() % 9000 + 1000) + "_" + std::to_string(time(nullptr));

    // Set shape-specific properties for non-model objects
    switch (obj.GetObjectType())
    {
    case 1: // Sphere
        jsonObj.radiusSphere = obj.GetSphereRadius();
        break;
    case 2: // Cylinder
        // Use horizontalRadius and verticalRadius if available, otherwise use scale
        jsonObj.radiusH =
            (obj.GetHorizontalRadius() > 0.0f) ? obj.GetHorizontalRadius() : obj.GetScale().x;
        jsonObj.radiusV =
            (obj.GetVerticalRadius() > 0.0f) ? obj.GetVerticalRadius() : obj.GetScale().y;
        break;
    case 3: // Plane
        jsonObj.size = obj.GetPlaneSize();
        break;
    case 4: // Light
        // Light objects don't need special properties
        break;
    case 5: // Model
        // Model objects use modelName which is already set
        break;
    case 6: // Spawn Zone
        // Spawn zone is handled separately in FileManager
        break;
    default:
        break;
    }

    return jsonObj;
}

MapObject JsonSerializableObjectToMapObject(const JsonSerializableObject &data)
{
    MapObject obj;
    obj.SetObjectName(data.name);
    obj.SetPosition(data.position);
    obj.SetRotation(data.rotation);
    obj.SetScale({data.scale, data.scale, data.scale}); // Create uniform scale vector
    obj.SetColor(HexStringToColor(data.color));
    obj.SetModelAssetName(data.modelName);
    obj.SetSelected(false);
    obj.SetObjectType(data.type);

    // Shape-specific properties
    switch (data.type)
    {
    case 1: // Sphere
        obj.SetSphereRadius(data.radiusSphere);
        break;
    case 2: // Cylinder
        // Set horizontal and vertical radius from radiusH and radiusV
        obj.SetHorizontalRadius(data.radiusH);
        obj.SetVerticalRadius(data.radiusV);
        break;
    case 3: // Plane
        obj.SetPlaneSize(data.size);
        break;
    case 4: // Light
        // Light objects don't need special properties
        break;
    case 5: // Model
        // Model objects use modelName which is already set
        break;
    case 6: // Spawn Zone
        // Spawn zone is handled separately in FileManager
        break;
    default:
        break;
    }

    return obj;
}

// ============================================================================
// MapObject (Editor) <-> MapObjectData (Engine)
// ============================================================================

MapObjectData MapObjectToMapObjectData(const MapObject &obj)
{
    MapObjectData data;
    data.name = obj.GetObjectName();
    data.position = obj.GetPosition();
    data.rotation = obj.GetRotation();
    data.scale = obj.GetScale();
    data.color = obj.GetColor();
    data.modelName = obj.GetModelAssetName();
    data.type = MapObjectConverter::IntToMapObjectType(obj.GetObjectType());

    // Set shape-specific properties based on object type
    switch (obj.GetObjectType())
    {
    case 1: // Sphere
        data.radius = obj.GetSphereRadius();
        data.height = 0.0f;
        data.size = {0.0f, 0.0f};
        break;
    case 2: // Cylinder
        data.radius =
            (obj.GetHorizontalRadius() > 0.0f) ? obj.GetHorizontalRadius() : obj.GetScale().x;
        data.height = (obj.GetVerticalRadius() > 0.0f) ? obj.GetVerticalRadius() : obj.GetScale().y;
        data.size = {0.0f, 0.0f};
        break;
    case 3: // Plane
        data.radius = 0.0f;
        data.height = 0.0f;
        data.size = obj.GetPlaneSize();
        break;
    case 4: // Light
        data.radius = 0.0f;
        data.height = 0.0f;
        data.size = {0.0f, 0.0f};
        break;
    case 5: // Model
        data.radius = 0.0f;
        data.height = obj.GetScale().y;
        data.size = {0.0f, 0.0f};
        break;
    case 6: // Spawn Zone
        data.radius = 0.0f;
        data.height = 0.0f;
        data.size = {0.0f, 0.0f};
        break;
    default: // Cube and others
        data.radius = 0.0f;
        data.height = obj.GetScale().y;
        data.size = {0.0f, 0.0f};
        break;
    }

    // Default collision properties
    data.isPlatform = true;
    data.isObstacle = false;

    return data;
}

MapObject MapObjectDataToMapObject(const MapObjectData &data)
{
    MapObject obj;
    obj.SetObjectName(data.name);
    obj.SetPosition(data.position);
    obj.SetRotation(data.rotation);
    obj.SetScale(data.scale);
    obj.SetColor(data.color);
    obj.SetModelAssetName(data.modelName);
    obj.SetSelected(false);
    obj.SetObjectType(MapObjectConverter::MapObjectTypeToInt(data.type));

    // Shape-specific properties
    switch (data.type)
    {
    case MapObjectType::SPHERE:
        obj.SetSphereRadius(data.radius);
        break;
    case MapObjectType::CYLINDER:
        // Set horizontal and vertical radius from data
        obj.SetHorizontalRadius(data.radius);
        obj.SetVerticalRadius(data.height);
        break;
    case MapObjectType::PLANE:
        obj.SetPlaneSize(data.size);
        break;
    case MapObjectType::LIGHT:
        // Light objects don't need special properties
        break;
    case MapObjectType::MODEL:
        // Model objects use modelName which is already set
        break;
    case MapObjectType::SPAWN_ZONE:
        // Spawn zone is handled separately in FileManager
        break;
    case MapObjectType::CUBE:
    default:
        // Cube and other objects use scale which is already set
        break;
    }

    return obj;
}
} // namespace MapObjectConverterEditor
