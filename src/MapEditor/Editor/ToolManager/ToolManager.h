#ifndef TOOLMANAGER_H
#define TOOLMANAGER_H

#include "IToolManager.h"
#include <string>

class ToolManager : public IToolManager {
private:
    Tool m_activeTool;
    bool m_pendingObjectCreation;
    std::string m_currentlySelectedModelName;

public:
    ToolManager();
    ~ToolManager() override = default;

    void SetActiveTool(Tool tool) override;
    Tool GetActiveTool() const override;
    bool ExecutePendingAction(ISceneManager& scene) override;
    void SetSelectedModel(const std::string& modelName) override;
    const std::string& GetSelectedModel() const override;
    void HandleToolInput(bool mousePressed, const Ray& ray, ISceneManager& scene) override;

private:
    void CreateObjectForTool(Tool tool, ISceneManager& scene);
};

#endif // TOOLMANAGER_H