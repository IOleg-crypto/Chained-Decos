#ifndef PROJECT_DATA_H
#define PROJECT_DATA_H

#include <nlohmann/json_fwd.hpp>
#include <string>
#include <vector>

struct ProjectData
{
    std::string name;
    std::string version = "1.0";
    std::string lastScene;
    int gridSize = 50;
    bool drawWireframe = false;
    bool drawCollisions = false;

    std::vector<std::string> scenes;
    std::string startScene;

    nlohmann::json ToJson() const;
    static ProjectData FromJson(const nlohmann::json &j);
};

#endif // PROJECT_DATA_H
