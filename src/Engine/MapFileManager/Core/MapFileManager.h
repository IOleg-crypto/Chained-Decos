//
// Created by I#Oleg
//

#ifndef MAPFILEMANAGER_H
#define MAPFILEMANAGER_H

#include <scene/resources/model/Core/Model.h>
#include <raylib.h>
#include <string>
#include <vector>


// Simple object structure for saving/loading
struct SerializableObject
{
    Vector3 position;
    Vector3 scale;
    Vector3 rotation;
    Color color;
    std::string name;
    int type;
    std::string modelName; // For model objects (type=5)
};

// Simple file manager for map operations
class MapFileManager
{
public:
    // Basic file operations
    static bool SaveMap(const std::vector<SerializableObject> &objects,
                         const std::string &filename);
    static bool LoadMap(std::vector<SerializableObject> &objects, const std::string &filename);
};

#endif // MAPFILEMANAGER_H