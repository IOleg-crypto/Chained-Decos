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

void Models::AddModel(const std::string& modelPath){
    if (modelPath.empty()) {
        std::cout << "[Model]: path to model empty!The model don`t exist!" << std::endl;
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
