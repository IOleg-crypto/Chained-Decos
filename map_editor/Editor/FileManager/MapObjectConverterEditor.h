#ifndef MAPOBJECTCONVERTEREDITOR_H
#define MAPOBJECTCONVERTEREDITOR_H

#include "../Object/MapObject.h"
#include "scene/resources/map/Converter/MapObjectConverter.h"
#include "scene/resources/map/Core/MapData.h"
#include "scene/resources/map/MapFileManager/Json/JsonMapFileManager.h"
#include <cstdlib>
#include <ctime>

namespace MapObjectConverterEditor
{
JsonSerializableObject MapObjectToJsonSerializableObject(const MapObject &obj);
MapObject JsonSerializableObjectToMapObject(const JsonSerializableObject &data);

MapObjectData MapObjectToMapObjectData(const MapObject &obj);
MapObject MapObjectDataToMapObject(const MapObjectData &data);
} // namespace MapObjectConverterEditor

#endif // MAPOBJECTCONVERTEREDITOR_H
