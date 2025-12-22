#include "JsonSceneFileManager.h"
#include <fstream>

json JsonSerializableObject::ToJson() const
{
    json j;
    j["name"] = name;
    j["modelName"] = modelName;
    j["position"] = {position.x, position.y, position.z};
    j["rotation"] = {rotation.x, rotation.y, rotation.z};
    j["scale"] = scale;
    j["type"] = type;

    // UI Element Specifics
    if (type == 1) // UI_MENU - though this is MapObjectType not SceneType..
    {
        // Wait, JsonSerializableObject is for MapObjectData, not UIElementData.
        // I need to check if UIElementData is serialized here or elsewhere.
        // Looking at the code for JsonSceneFileManager, it seems it only handles `objects` array
        // which are `JsonSerializableObject`. I need to find where UIElementData is serialized.
    }
    return j;
}

JsonSerializableObject JsonSerializableObject::FromJson(const json &j)
{
    JsonSerializableObject obj;
    obj.name = j.value("name", "");
    obj.modelName = j.value("modelName", "");

    if (j.contains("position") && j["position"].is_array())
    {
        obj.position = {j["position"][0].get<float>(), j["position"][1].get<float>(),
                        j["position"][2].get<float>()};
    }

    if (j.contains("rotation") && j["rotation"].is_array())
    {
        obj.rotation = {j["rotation"][0].get<float>(), j["rotation"][1].get<float>(),
                        j["rotation"][2].get<float>()};
    }

    obj.scale = j.value("scale", 1.0f);
    obj.type = j.value("type", 0);

    return obj;
}

bool JsonSceneFileManager::LoadScene(const std::string &filepath,
                                     std::vector<JsonSerializableObject> &objects)
{
    std::ifstream file(filepath);
    if (!file.is_open())
        return false;

    try
    {
        json j;
        file >> j;

        objects.clear();
        for (const auto &item : j["objects"])
        {
            objects.push_back(JsonSerializableObject::FromJson(item));
        }

        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool JsonSceneFileManager::SaveScene(const std::string &filepath,
                                     const std::vector<JsonSerializableObject> &objects)
{
    std::ofstream file(filepath);
    if (!file.is_open())
        return false;

    try
    {
        json j;
        j["objects"] = json::array();

        for (const auto &obj : objects)
        {
            j["objects"].push_back(obj.ToJson());
        }

        file << j.dump(4);
        return true;
    }
    catch (...)
    {
        return false;
    }
}
