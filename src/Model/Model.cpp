// Created by I#Oleg
//

#include "Model.h"
#include <iostream>
#include <fstream>

#include <Color/ColorParser.h>

Models::~Models() {
    for (const auto &modelPtr: m_modelByName | std::views::values) {
        UnloadModel(*modelPtr);
        delete modelPtr;
    }
}

void Models::LoadModelsFromJson(const std::string &path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        TraceLog(LOG_WARNING, "Failed to open model list JSON: %s", path.c_str());
        return;
    }

    json j;
    try {
        file >> j;
    } catch (const std::exception& e) {
        TraceLog(LOG_WARNING, "JSON parsing error: %s", e.what());
        return;
    }

    for (const auto& modelEntry : j) {
        if (!modelEntry.contains("name") || !modelEntry.contains("path")) {
            TraceLog(LOG_WARNING, "Model entry missing 'name' or 'path'. Skipping.");
            continue;
        }

        std::string modelName = PROJECT_ROOT_DIR + modelEntry["name"].get<std::string>();
        std::string modelPath = PROJECT_ROOT_DIR + modelEntry["path"].get<std::string>();

        TraceLog(LOG_INFO, "Loading model '%s' from: %s", modelName.c_str(), modelPath.c_str());

        Model loadedModel = LoadModel(modelPath.c_str());
        if (loadedModel.meshCount == 0) {
            TraceLog(LOG_WARNING, "Failed to load model at path: %s", modelPath.c_str());
            continue;
        }

        auto pModel = new Model(loadedModel);
        m_modelByName[modelName] = pModel;

        bool spawnInstances = modelEntry.value("spawn", true);

        if (modelEntry.contains("instances") && modelEntry["instances"].is_array()) {
            for (const auto& instance : modelEntry["instances"]) {
                bool spawnThis = instance.value("spawn", spawnInstances);
                if (!spawnThis) continue;
                AddInstance(instance, pModel, modelName);
            }
        } else if (spawnInstances) {
            AddInstance(json::object(), pModel, modelName);
        }
    }
}

void Models::DrawAllModels() const {
    for (const auto& instance : m_instances) {
        if (instance.getModel() != nullptr) {
            DrawModel(*instance.getModel(), instance.getModelPosition(), instance.getScale(), instance.getColor());
        }
    }
}


Model& Models::GetModelByName(const std::string &name) {
    static Model dummyModel = { 0 };
    const auto it = m_modelByName.find(name);
    if (it == m_modelByName.end()) {
        TraceLog(LOG_WARNING, "Model name '%s' not found. Returning dummy model.", name.c_str());
        return dummyModel;
    }
    return *(it->second);
}

void Models::AddInstance(const json &instanceJson, Model *modelPtr, const std::string &modelName) {
    Vector3 pos = {0.0f, 0.0f, 0.0f};
    float scaleModel = 1.0f;
    Color color = WHITE;

    if (instanceJson.contains("position")) {
        pos.x = instanceJson["position"].value("x", 0.0f);
        pos.y = instanceJson["position"].value("y", 0.0f);
        pos.z = instanceJson["position"].value("z", 0.0f);
    }

    if (instanceJson.contains("scale")) {
        scaleModel = instanceJson["scale"].get<float>();
    }

    if (instanceJson.contains("color")) {
        if (instanceJson["color"].is_string()) {
            color = ParseColorByName(instanceJson["color"].get<std::string>());
        } else if (instanceJson["color"].is_object()) {
            const auto& colorJson = instanceJson["color"];
            color.r = colorJson.value("r", 255);
            color.g = colorJson.value("g", 255);
            color.b = colorJson.value("b", 255);
            color.a = colorJson.value("a", 255);
        }
    }

    m_instances.emplace_back(pos, modelPtr, scaleModel, modelName, color);
}
