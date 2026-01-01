#ifndef MAPOBJECTCONVERTER_H
#define MAPOBJECTCONVERTER_H

#include "MapData.h"
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

#endif // MAPOBJECTCONVERTER_H
