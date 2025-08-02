//
// Created by I#Oleg
//

#include "MapFileManager.h"
#include <fstream>
#include <iostream>

bool MapFileManager::SaveMap(const std::vector<SerializableObject>& objects, const std::string& filename) {
    // Simple text-based save for now
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for writing: " << filename << std::endl;
        return false;
    }
    
    file << "Map File v1.0" << std::endl;
    file << "ObjectCount: " << objects.size() << std::endl;
    
    for (const auto& obj : objects) {
        file << "Object:" << std::endl;
        file << "  Name: " << obj.name << std::endl;
        file << "  Type: " << obj.type << std::endl;
        file << "  Position: " << obj.position.x << " " << obj.position.y << " " << obj.position.z << std::endl;
        file << "  Scale: " << obj.scale.x << " " << obj.scale.y << " " << obj.scale.z << std::endl;
        file << "  Rotation: " << obj.rotation.x << " " << obj.rotation.y << " " << obj.rotation.z << std::endl;
        file << "  Color: " << (int)obj.color.r << " " << (int)obj.color.g << " " << (int)obj.color.b << " " << (int)obj.color.a << std::endl;
    }
    
    file.close();
    std::cout << "Map saved successfully to: " << filename << std::endl;
    return true;
}

bool MapFileManager::LoadMap(std::vector<SerializableObject>& objects, const std::string& filename) {
    // Simple text-based load for now
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for reading: " << filename << std::endl;
        return false;
    }
    
    objects.clear();
    std::string line;
    
    // Skip header
    std::getline(file, line); // "Map File v1.0"
    std::getline(file, line); // "ObjectCount: X"
    
    while (std::getline(file, line)) {
        if (line == "Object:") {
            SerializableObject obj;
            
            // Read object properties
            std::getline(file, line); // Name
            obj.name = line.substr(line.find(": ") + 2);
            
            std::getline(file, line); // Type
            obj.type = std::stoi(line.substr(line.find(": ") + 2));
            
            std::getline(file, line); // Position
            sscanf(line.c_str(), "  Position: %f %f %f", &obj.position.x, &obj.position.y, &obj.position.z);
            
            std::getline(file, line); // Scale
            sscanf(line.c_str(), "  Scale: %f %f %f", &obj.scale.x, &obj.scale.y, &obj.scale.z);
            
            std::getline(file, line); // Rotation
            sscanf(line.c_str(), "  Rotation: %f %f %f", &obj.rotation.x, &obj.rotation.y, &obj.rotation.z);
            
            std::getline(file, line); // Color
            int r, g, b, a;
            sscanf(line.c_str(), "  Color: %d %d %d %d", &r, &g, &b, &a);
            obj.color = {(unsigned char)r, (unsigned char)g, (unsigned char)b, (unsigned char)a};
            
            objects.push_back(obj);
        }
    }
    
    file.close();
    std::cout << "Map loaded successfully from: " << filename << std::endl;
    return true;
} 