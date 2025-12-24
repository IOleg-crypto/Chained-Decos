#include "ProjectData.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

json ProjectData::ToJson() const
{
    json j;
    j["name"] = name;
    j["version"] = version;
    j["lastScene"] = lastScene;
    j["startScene"] = startScene;
    j["scenes"] = scenes;
    j["settings"]["gridSize"] = gridSize;
    j["settings"]["drawWireframe"] = drawWireframe;
    j["settings"]["drawCollisions"] = drawCollisions;
    return j;
}

ProjectData ProjectData::FromJson(const nlohmann::json &j)
{
    ProjectData data;
    data.name = j.value("name", "Untitled Project");
    data.version = j.value("version", "1.0");
    data.lastScene = j.value("lastScene", "");
    data.startScene = j.value("startScene", "");

    if (j.contains("scenes"))
    {
        data.scenes = j["scenes"].get<std::vector<std::string>>();
    }

    if (j.contains("settings"))
    {
        const auto &s = j["settings"];
        data.gridSize = s.value("gridSize", 50);
        data.drawWireframe = s.value("drawWireframe", false);
        data.drawCollisions = s.value("drawCollisions", false);
    }

    return data;
}
