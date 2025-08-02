//
// Created by I#Oleg
//

#ifndef MAPFILEMANAGER_H
#define MAPFILEMANAGER_H

#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <raylib.h>
#include <raymath.h>

// Serializable object structure for saving/loading map objects
struct SerializableObject {
    Vector3 position;      // Object position in 3D space
    Vector3 scale;         // Object scale (size)
    Vector3 rotation;      // Object rotation in radians
    Color color;           // Object color
    std::string name;      // Object name
    int type;              // Object type: 0=cube, 1=sphere, 2=cylinder
    
    // JSON conversion methods
    nlohmann::json toJson() const;                    // Convert object to JSON
    static SerializableObject fromJson(const nlohmann::json& j); // Create object from JSON
};

// File manager class for map save/load operations
class MapFileManager {
public:
    // Main file operations
    static bool SaveMap(const std::vector<SerializableObject>& objects, const std::string& filename);  // Save map to file
    static bool LoadMap(std::vector<SerializableObject>& objects, const std::string& filename);        // Load map from file
    
    // Utility functions for data conversion
    static nlohmann::json Vector3ToJson(const Vector3& vec);    // Convert Vector3 to JSON
    static Vector3 JsonToVector3(const nlohmann::json& j);      // Convert JSON to Vector3
    static nlohmann::json ColorToJson(const Color& color);      // Convert Color to JSON
    static Color JsonToColor(const nlohmann::json& j);          // Convert JSON to Color
};

#endif // MAPFILEMANAGER_H 