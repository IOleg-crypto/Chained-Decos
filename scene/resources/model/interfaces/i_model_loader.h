#ifndef CD_SCENE_RESOURCES_MODEL_INTERFACES_I_MODEL_LOADER_H
#define CD_SCENE_RESOURCES_MODEL_INTERFACES_I_MODEL_LOADER_H

#include "raylib.h"
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace CHEngine
{
class IModelLoader
{
public:
    virtual ~IModelLoader() = default;

    virtual bool LoadSingleModel(const std::string &name, const std::string &path,
                                 bool preload = true) = 0;
    virtual void UnloadAllModels() = 0;
    virtual std::vector<std::string> GetAvailableModels() const = 0;
    virtual std::optional<std::reference_wrapper<::Model>>
    GetModelByName(const std::string &name) = 0;
};
} // namespace CHEngine

#endif // CD_SCENE_RESOURCES_MODEL_INTERFACES_I_MODEL_LOADER_H
