#ifndef SCENE_MODEL_LOADER_H
#define SCENE_MODEL_LOADER_H

#include <raylib.h>
#include <string>
#include <unordered_map>

namespace Scene
{

class ModelLoader
{
public:
    ModelLoader() = default;
    ~ModelLoader();

    // Non-copyable
    ModelLoader(const ModelLoader &) = delete;
    ModelLoader &operator=(const ModelLoader &) = delete;

    // Load model from file
    bool Load(const std::string &name, const std::string &path);

    // Get model by name (returns nullptr if not found)
    Model *Get(const std::string &name);

    // Unload specific model
    void Unload(const std::string &name);

    // Unload all models
    void UnloadAll();

    // Check if model exists
    bool Exists(const std::string &name) const;

private:
    std::unordered_map<std::string, Model> m_models;
};

} // namespace Scene

#endif // SCENE_MODEL_LOADER_H




