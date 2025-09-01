//
// Created by I#Oleg
//

#include "MapLoader.h"
#include "nlohmann/json.hpp"
#include <fstream>
#include <raymath.h>


using json = nlohmann::json;

std::vector<MapLoader> LoadMap(const std::string &path)
{
    std::ifstream file(path);
    if (!FileExists(path.c_str()))
    {
        TraceLog(LOG_ERROR, "The map don`t exist at current path");
        return {};
    }
    json j;
    file >> j;
    std::vector<MapLoader> objects;
    for (const auto &obj : j["objects"])
    {
        MapLoader mo;
        mo.modelName = obj["model"];
        mo.position = {static_cast<float>(obj["position"][0]),
                       static_cast<float>(obj["position"][1]),
                       static_cast<float>(obj["position"][2])};
        mo.rotation = {static_cast<float>(obj["rotation"][0]),
                       static_cast<float>(obj["rotation"][1]),
                       static_cast<float>(obj["rotation"][2])};
        mo.scale = {static_cast<float>(obj["scale"][0]), static_cast<float>(obj["scale"][1]),
                    static_cast<float>(obj["scale"][2])};
        mo.loadedModel = LoadModel(("models/" + mo.modelName).c_str());

        // Scaling
        mo.loadedModel.transform = MatrixScale(mo.scale.x, mo.scale.y, mo.scale.z);

        // Rotating
        Matrix rotX = MatrixRotateX(DEG2RAD * mo.rotation.x);
        Matrix rotY = MatrixRotateY(DEG2RAD * mo.rotation.y);
        Matrix rotZ = MatrixRotateZ(DEG2RAD * mo.rotation.z);
        mo.loadedModel.transform = MatrixMultiply(mo.loadedModel.transform, rotX);
        mo.loadedModel.transform = MatrixMultiply(mo.loadedModel.transform, rotY);
        mo.loadedModel.transform = MatrixMultiply(mo.loadedModel.transform, rotZ);

        mo.loadedModel.transform = MatrixMultiply(
            mo.loadedModel.transform, MatrixTranslate(mo.position.x, mo.position.y, mo.position.z));

        objects.push_back(mo);
    }

    return objects;
}
