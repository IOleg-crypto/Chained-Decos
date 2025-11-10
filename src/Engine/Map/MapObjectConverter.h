//
// Created by AI Assistant
// MapObjectConverter - Unified converter between MapObject, JsonSerializableObject, and MapObjectData
//

#ifndef MAPOBJECTCONVERTER_H
#define MAPOBJECTCONVERTER_H

#include "MapData.h"
#include "../MapFileManager/JsonMapFileManager.h"
#include <string>
#include <ctime>
#include <cstdlib>
#include <cmath>

namespace MapObjectConverter
{
    MapObjectData JsonSerializableObjectToMapObjectData(const JsonSerializableObject& jsonObj);
    JsonSerializableObject MapObjectDataToJsonSerializableObject(const MapObjectData& data);
    
    MapObjectType IntToMapObjectType(int type);
    int MapObjectTypeToInt(MapObjectType type);
    
    Vector3 SanitizeVector3(const Vector3& vec);
    float SanitizeFloat(float value, float fallback = 0.0f);
}


#endif // MAPOBJECTCONVERTER_H

