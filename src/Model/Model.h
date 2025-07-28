//
// Created by I#Oleg
//

#ifndef MODEL_H
#define MODEL_H

#include "ModelInstance.h"

#include <vector>
#include <string>
#include <raylib.h>
#include <raymath.h>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

// # ----------------------------------------------------------------------------
// # Models class represents like model loader into game
// # ----------------------------------------------------------------------------
class Models {
private:
    std::vector<Model> m_models; // reading models from .json file
    std::vector<ModelInstance> m_instances;  // contains model position
public:
    Models() = default;
    ~Models();
public:
    // This function read all paths from .json file
    void LoadModelsFromJson(const std::string &path);
    void DrawAllModels() const; // Draw all models
    Model& GetModel(size_t index); // Access to one model;
};



#endif //MODEL_H
