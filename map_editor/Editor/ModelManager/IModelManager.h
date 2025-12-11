#ifndef IMODELMANAGER_H
#define IMODELMANAGER_H

#include <string>
#include <vector>
#include "scene/resources/model/Core/Model.h"

// Interface for model management subsystem
class IModelManager {
public:
    virtual ~IModelManager() = default;

    // Model loading and management
    virtual bool LoadModel(const std::string& name, const std::string& path) = 0;
    virtual bool HasModel(const std::string& name) const = 0;
    virtual std::vector<std::string> GetAvailableModels() const = 0;

    // Access to underlying ModelLoader
    virtual ModelLoader& GetModelLoader() = 0;
    virtual const ModelLoader& GetModelLoader() const = 0;
};

#endif // IMODELMANAGER_H