//
// Created by AI Assistant
// MapObjectConverterEditor - Editor-specific conversions using MapObjectConverter from Engine
//

#ifndef MAPOBJECTCONVERTEREDITOR_H
#define MAPOBJECTCONVERTEREDITOR_H

#include "../Object/MapObject.h"
#include "Engine/Map/MapObjectConverter.h"
#include "Engine/MapFileManager/JsonMapFileManager.h"
#include <ctime>
#include <cstdlib>

namespace MapObjectConverterEditor
{
    JsonSerializableObject MapObjectToJsonSerializableObject(const MapObject& obj);
    MapObject JsonSerializableObjectToMapObject(const JsonSerializableObject& data);
    
    MapObjectData MapObjectToMapObjectData(const MapObject& obj);
    MapObject MapObjectDataToMapObject(const MapObjectData& data);
}

#endif // MAPOBJECTCONVERTEREDITOR_H

