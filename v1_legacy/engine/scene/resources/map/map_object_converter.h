#ifndef CD_SCENE_RESOURCES_MAP_MAP_OBJECT_CONVERTER_H
#define CD_SCENE_RESOURCES_MAP_MAP_OBJECT_CONVERTER_H

#include "map_data.h"
#include "json/JsonSceneFileManager.h"
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <string>

namespace CHEngine
{
namespace MapObjectConverter
{
MapObjectData JsonSerializableObjectToMapObjectData(const JsonSerializableObject &jsonObj);
JsonSerializableObject MapObjectDataToJsonSerializableObject(const MapObjectData &data);

MapObjectType IntToMapObjectType(int type);
int MapObjectTypeToInt(MapObjectType type);

Vector3 SanitizeVector3(const Vector3 &vec);
float SanitizeFloat(float value, float fallback = 0.0f);
} // namespace MapObjectConverter
} // namespace CHEngine

#endif // CD_SCENE_RESOURCES_MAP_MAP_OBJECT_CONVERTER_H
