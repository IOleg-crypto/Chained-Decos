#ifndef MODELMANAGER_H
#define MODELMANAGER_H

#include "../../../scene/resources/model/Core/Model.h"
#include "IModelManager.h"
#include <memory>
#include <string>
#include <vector>


class ModelManager : public IModelManager
{
private:
    std::shared_ptr<ModelLoader> m_modelLoader;

public:
    explicit ModelManager(std::shared_ptr<ModelLoader> modelLoader);
    ~ModelManager() override = default;

    // Model loading and management
    bool LoadModel(const std::string &name, const std::string &path) override;
    bool HasModel(const std::string &name) const override;
    std::vector<std::string> GetAvailableModels() const override;

    // Access to underlying ModelLoader
    ModelLoader &GetModelLoader() override;
    const ModelLoader &GetModelLoader() const override;
};

#endif // MODELMANAGER_H