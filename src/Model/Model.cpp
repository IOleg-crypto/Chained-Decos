//
// Created by I#Oleg
//

#include "Model.h"

#include <iostream>
#include <fstream>

Models::~Models() {
    // Delete all models
    for (const auto& model : m_models) {
        UnloadModel(model);
    }
    m_models.clear();
    m_instances.clear();
}

void Models::LoadModelsFromJson(const std::string &path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        TraceLog(LOG_WARNING, "Failed to open model list JSON: %s", path.c_str());
        return;
    }
    // Read all models paths from json file
    json j;
    try {
        file >> j;
    } catch (const std::exception& e) {
        TraceLog(LOG_WARNING, "JSON parsing error: %s", e.what());
        return;
    }
    // Adding models in game
    for (const auto& modelEntry : j) {
        std::string modelPath = GetWorkingDirectory() + modelEntry["path"].get<std::string>();
        TraceLog(LOG_INFO, "Loading model: %s", modelPath.c_str());
        // Load models (took from .json file)
        m_models.push_back(LoadModel(modelPath.c_str()));
        Model *pModel = &m_models.back();
        // Take position from file
        if (modelEntry.contains("instances")) {
            float scaleModel = 0;
            for (const auto& instance : modelEntry["instances"]) {
                Vector3 pos = {0, 0 , 0};
                if (instance.contains("position")) {
                    pos.x = instance["position"]["x"].get<float>();
                    pos.y = instance["position"]["y"].get<float>();
                    pos.z = instance["position"]["z"].get<float>();
                }

                if (instance.contains("scale")) {
                    scaleModel = instance["scale"].get<float>();
                }

                m_instances.push_back({ pos, pModel , scaleModel });
            }
        } else {
            // If no instances, add one default
            m_instances.push_back( {{0, 0, 0}, pModel , 1.0f });
        }
    }
}

void Models::AddModel(const std::string& modelPath){
    if (modelPath.empty()) {
        std::cout << "[Model]: The json don`t exist!" << std::endl;
        return;
    }
    m_models.push_back(LoadModel(modelPath.c_str())); // Add models using raylib func

}

// Draw all models with position
void Models::DrawAllModels() const {
    for (const auto& instance : m_instances) {
        DrawModel(*instance.pModel, instance.position, instance.scale, WHITE);
    }
}

Model& Models::GetModel(const size_t index) {
    if (index >= m_models.size()) {
        static Model dummyModel = {0};
        TraceLog(LOG_WARNING, "Model index %zu is out of bounds. Returning dummy model.", index);
        return dummyModel;
    }
    return m_models.at(index);
}

void Models::CheckCollision(Player &player)
{

}


