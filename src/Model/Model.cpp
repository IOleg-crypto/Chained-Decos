//
// Created by I#Oleg
//

#include "Model.h"
#include <iostream>
#include <fstream>

#include "../Color/ColorParser.h"

Models::~Models() {
    for (const auto& model : m_models) {
        UnloadModel(model);
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
        if (!modelEntry.contains("name")) {
            TraceLog(LOG_WARNING, "JSON entry without 'name' field");
            continue;
        }
        if (!modelEntry.contains("path")) {
            TraceLog(LOG_WARNING, "JSON entry without 'path' field");
            continue;
        }

        std::string modelName = GetWorkingDirectory() + modelEntry["name"].get<std::string>();
        std::string modelPath = GetWorkingDirectory() + modelEntry["path"].get<std::string>();

        TraceLog(LOG_INFO, "Loading model: %s", modelPath.c_str());

        Model loadedModel = LoadModel(modelPath.c_str());
        if (loadedModel.meshCount == 0) {
            TraceLog(LOG_WARNING, "Failed to load model at path: %s", modelPath.c_str());
            continue;
        }

        m_models.push_back(loadedModel);
        Model* pModel = &m_models.back();

        if (modelEntry.contains("instances") && modelEntry["instances"].is_array()) {
            for (const auto& instance : modelEntry["instances"]) {
                Vector3 pos = {0.0f, 0.0f, 0.0f};
                float scaleModel = 1.0f;
                Color color = WHITE;

                if (instance.contains("position")) {
                    pos.x = instance["position"].value("x", 0.0f);
                    pos.y = instance["position"].value("y", 0.0f);
                    pos.z = instance["position"].value("z", 0.0f);
                }

                if (instance.contains("scale")) {
                    scaleModel = instance["scale"].get<float>();
                }

                if (instance.contains("color")) {
                    if (instance["color"].is_string()) {
                        color = ParseColorByName(instance["color"].get<std::string>());
                    } else if (instance["color"].is_object()) {
                        const auto& colorJson = instance["color"];
                        color.r = colorJson.value("r", 255);
                        color.g = colorJson.value("g", 255);
                        color.b = colorJson.value("b", 255);
                        color.a = colorJson.value("a", 255);
                    }
                }

                m_instances.emplace_back(pos, pModel, scaleModel, modelName, color);
            }
        } else {
            m_instances.emplace_back(Vector3Zero(), pModel, 1.0f, modelName);
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

Model& Models::GetModel(size_t index) {
    static Model dummyModel = {0};
    if (index >= m_models.size()) {
        TraceLog(LOG_WARNING, "Model index %zu is out of bounds. Returning dummy model.", index);
        return dummyModel;
    }
    return m_models[index];
}





