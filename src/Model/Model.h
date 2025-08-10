//
// Created by I#Oleg
//

#ifndef MODEL_H
#define MODEL_H

#include "ModelInstance.h"

#include <Model/Animation.h>
#include <nlohmann/json.hpp>
#include <raylib.h>
#include <raymath.h>
#include <string>
#include <unordered_map>
#include <vector>

using json = nlohmann::json;

//
// Models
// Handles loading, storage, and rendering of 3D models and their instances.
// Responsible for:
//   - Loading models from JSON configuration
//   - Managing model instances (position, rotation, etc.)
//   - Rendering all instances in the scene
//
class Models
{
public:
    Models() = default;
    ~Models();

    // -------------------- Loading --------------------

    // Load all models and their metadata from a JSON file.
    // path - File path to the JSON configuration
    void LoadModelsFromJson(const std::string &path);

    // -------------------- Rendering --------------------

    // Draw all currently loaded model instances to the screen.
    void DrawAllModels() const;

    // -------------------- Accessors --------------------

    // Get a reference to the original (base) model by name.
    // name - Name of the model to retrieve
    // Returns reference to the Model object
    Model &GetModelByName(const std::string &name);

    // -------------------- Instance Management --------------------

    // Create and add a model instance to the scene.
    // instanceJson - JSON describing the instance properties
    // modelPtr - Pointer to the base model to instance
    // modelName - Name of the model
    void AddInstance(const json &instanceJson, Model *modelPtr, const std::string &modelName,
                     Animation *animation);

private:
    std::vector<ModelInstance> m_instances;                  // List of all model instances
    std::unordered_map<std::string, Model *> m_modelByName;  // Direct access to base models by name
    std::unordered_map<std::string, Animation> m_animations; // Animation data for models
    bool m_spawnInstance = true; // Whether to spawn instances automatically
};

#endif // MODEL_H
