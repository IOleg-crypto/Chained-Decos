#ifndef MAPOBJECTCONVERTER_H
#define MAPOBJECTCONVERTER_H

#include "../core/MapData.h"
#include "../mapfilemanager/json/jsonMapFileManager.h"
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <string>

namespace MapObjectConverter
{
MapObjectData JsonSerializableObjectToMapObjectData(const JsonSerializableObject &jsonObj);
JsonSerializableObject MapObjectDataToJsonSerializableObject(const MapObjectData &data);

MapObjectType IntToMapObjectType(int type);
int MapObjectTypeToInt(MapObjectType type);

Vector3 SanitizeVector3(const Vector3 &vec);
float SanitizeFloat(float value, float fallback = 0.0f);
} // namespace MapObjectConverter

#endif // MAPOBJECTCONVERTER_H






