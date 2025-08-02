//
// Created by I#Oleg
//

#ifndef MAPFILEMANAGER_H
#define MAPFILEMANAGER_H

#include <string>
#include <vector>
#include <raylib.h>

// Simple object structure for saving/loading
struct SerializableObject {
    Vector3 position;
    Vector3 scale;
    Vector3 rotation;
    Color color;
    std::string name;
    int type;
};

// Simple file manager for map operations
class MapFileManager {
public:
    // Basic file operations
    static bool SaveMap(const std::vector<SerializableObject>& objects, const std::string& filename);
    static bool LoadMap(std::vector<SerializableObject>& objects, const std::string& filename);
};

#endif // MAPFILEMANAGER_H 