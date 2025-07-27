//
// Created by I#Oleg
//

#include "Model.h"

Models::~Models() {
    // Delete all models
    for (const auto& model : m_models) {
        UnloadModel(model);
    }
}

void Models::AddModel(const std::string& modelPath){
    m_models.push_back(LoadModel(modelPath.c_str())); // Add models using raylib func
}

// Draw all models
void Models::DrawAll(float x , float y , float z) const {
    for (const auto& model : m_models) {
        DrawModel(model, {x, y, z}, 1.0f, RED);
    }
}

Model& Models::GetModel(size_t index) {
    return m_models.at(index);
}
