#ifndef ITOOLMANAGER_H
#define ITOOLMANAGER_H

#include "editor/EditorTypes.h"
#include <raylib.h>
#include <string>

// #include "editor/IEditor.h" // Removed

class IToolManager
{
public:
    virtual ~IToolManager() = default;

    virtual void SetCamera(const Camera3D &camera) = 0;
    virtual void SetActiveTool(Tool tool) = 0;
    virtual Tool GetActiveTool() const = 0;

    virtual bool ExecutePendingAction(class SceneManager &sceneManager) = 0;
    virtual void SetSelectedModel(const std::string &modelName) = 0;
    virtual const std::string &GetSelectedModel() const = 0;

    virtual void HandleToolInput(bool mousePressed, const Ray &ray,
                                 class EditorContext &context) = 0;
    virtual void UpdateTool(const Ray &ray, class EditorContext &context) = 0;
    virtual void RenderGizmos(class EditorContext &context) = 0;

    virtual void Update(float deltaTime) = 0; // Added Update method
};

#endif // ITOOLMANAGER_H
