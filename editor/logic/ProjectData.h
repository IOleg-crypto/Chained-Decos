#ifndef PROJECT_DATA_H
#define PROJECT_DATA_H

#include <nlohmann/json.hpp>
#include <string>
#include <vector>


using json = nlohmann::json;

struct ProjectData
{
    std::string name;
    std::string version = "1.0";
    std::string lastScene;
    int gridSize = 50;
    bool drawWireframe = false;
    bool drawCollisions = false;

    json ToJson() const
    {
        json j;
        j["name"] = name;
        j["version"] = version;
        j["lastScene"] = lastScene;
        j["settings"]["gridSize"] = gridSize;
        j["settings"]["drawWireframe"] = drawWireframe;
        j["settings"]["drawCollisions"] = drawCollisions;
        return j;
    }

    static ProjectData FromJson(const json &j)
    {
        ProjectData data;
        data.name = j.value("name", "Untitled Project");
        data.version = j.value("version", "1.0");
        data.lastScene = j.value("lastScene", "");

        if (j.contains("settings"))
        {
            const auto &s = j["settings"];
            data.gridSize = s.value("gridSize", 50);
            data.drawWireframe = s.value("drawWireframe", false);
            data.drawCollisions = s.value("drawCollisions", false);
        }

        return data;
    }
};

#endif // PROJECT_DATA_H
