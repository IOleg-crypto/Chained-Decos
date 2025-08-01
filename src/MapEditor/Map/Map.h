#ifndef MAP_H
#define MAP_H

#include <string>
#include <vector>
#include <unordered_map>
#include <raylib.h>

#include <nlohmann/json.hpp>

class Map {
public:
    Map();
    ~Map();
    void LoadMap(const std::string& mapPath);
    void SaveMap(const std::string& mapPath);
    void DrawMap();
};

#endif