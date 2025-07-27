//
// Created by I#Oleg
//

#include "Model.h"

#include <iostream>

Models::~Models() {
    // Delete all models
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
    // Read all models paths from json file
    json j;
    try {
        file >> j;
    } catch (const std::exception& e) {
        TraceLog(LOG_WARNING, "JSON parsing error: %s", e.what());
        return;
    }

    for (const auto& modelPath : j) {
        std::string fullPath = GetWorkingDirectory() + modelPath.get<std::string>();
        TraceLog(LOG_INFO, "Loading model: %s", fullPath.c_str());
        AddModel(fullPath);
    }
}

void Models::AddModel(const std::string& modelPath){
    if (modelPath.empty()) {
        std::cout << "[Model]: The json don`t exist!" << std::endl;
        return;
    }
    m_models.push_back(LoadModel(modelPath.c_str())); // Add models using raylib func
}

// Draw all models
void Models::DrawAll(const float x , const float y , const float z) const {
    for (const auto& model : m_models) {
        DrawModel(model, {x, y, z}, 1.0f, RED);
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
