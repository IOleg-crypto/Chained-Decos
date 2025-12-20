#ifndef IMODELLOADER_H
#define IMODELLOADER_H

#include "raylib.h"
#include <functional>
#include <optional>
#include <string>
#include <vector>


class IModelLoader
{
public:
    virtual ~IModelLoader() = default;

    virtual bool LoadSingleModel(const std::string &name, const std::string &path,
                                 bool preload = true) = 0;
    virtual void UnloadAllModels() = 0;
    virtual std::vector<std::string> GetAvailableModels() const = 0;
    virtual std::optional<std::reference_wrapper<Model>>
    GetModelByName(const std::string &name) = 0;
};

#endif // IMODELLOADER_H
