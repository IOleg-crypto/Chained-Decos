//
// Created by I#Oleg
//

#ifndef MODEL_H
#define MODEL_H

#include "ModelInstance.h"

#include <raylib.h>
#include <raymath.h>
#include <string>
#include <unordered_map>
#include <vector>


#include <nlohmann/json.hpp>

using json = nlohmann::json;

// # ----------------------------------------------------------------------------
// # Models class represents like model loader into game
// # ----------------------------------------------------------------------------
class Models
{
  private:
    std::vector<ModelInstance> m_instances; // contains model instances (with model and position)
    std::unordered_map<std::string, Model *> m_modelByName; // Direct access by name
    bool m_spawnInstance = true;

  public:
    Models() = default;
    ~Models();

  public:
    // This function reads all paths from .json file
    void LoadModelsFromJson(const std::string &path);

    // Draw all model instances
    void DrawAllModels() const;

    // Access to original model (not instance)
    Model &GetModelByName(const std::string &name);

    // Create and add instance
    void AddInstance(const json &instanceJson, Model *modelPtr, const std::string &modelName);
};

#endif // MODEL_H