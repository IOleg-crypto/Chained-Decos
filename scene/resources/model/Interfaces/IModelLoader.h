#ifndef IMODEL_LOADER_H
#define IMODEL_LOADER_H

#include <string>
#include <vector>
#include <optional>
#include <raylib.h>

class IModelLoader
{
public:
    virtual ~IModelLoader() = default;
    
    virtual bool LoadSingleModel(const std::string& name, const std::string& path, bool forceReload = false) = 0;
    virtual std::optional<std::reference_wrapper<Model>> GetModelByName(const std::string& name) = 0;
    virtual std::vector<std::string> GetAvailableModels() const = 0;
    virtual void UnloadAllModels() = 0;
};

#endif // IMODEL_LOADER_H




