//
// Created by I#Oleg
//

#ifndef MAPLOADER_H
#define MAPLOADER_H

#include <raylib.h>
#include <string>
#include <vector>


// # ------------------------------------------------------
// # MapLoader - class , that loading maps from json file
// # ------------------------------------------------------
struct MapLoader
{
    std::string modelName;
    Vector3 position;
    Vector3 rotation;
    Vector3 scale;
    Model loadedModel;

    // Constructor to properly initialize the Model
    MapLoader()
        : modelName(""), position{0, 0, 0}, rotation{0, 0, 0}, scale{0, 0, 0}, loadedModel{0}
    {
    }
};

// Loading all properties about map( look at MapLoader struct)
std::vector<MapLoader> LoadMap(const std::string &path);

#endif // MAPLOADER_H
