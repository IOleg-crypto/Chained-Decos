//
// Created by I#Oleg
//

#ifndef MAPLOADER_H
#define MAPLOADER_H


#include <string>
#include <vector>
#include <raylib.h>

// # ------------------------------------------------------
// # MapLoader - class , that loading maps from json file
// # ------------------------------------------------------
struct MapLoader {
    std::string modelName;
    Vector3 position;
    Vector3 rotation;
    Vector3 scale;
    Model loadedModel;
};


// Loading all properties about map( look at MapLoader struct)
std::vector<MapLoader>LoadMap(const std::string &path);

#endif //MAPLOADER_H
