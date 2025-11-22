#ifndef MAPOBJECTCONVERTEREDITOR_H
#define MAPOBJECTCONVERTEREDITOR_H

#include "../Object/MapObject.h"
#include "scene/resources/map/Converter/MapObjectConverter.h"
#include "Engine/MapFileManager/Json/JsonMapFileManager.h"
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

