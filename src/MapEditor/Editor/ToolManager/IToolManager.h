#ifndef ITOOLMANAGER_H
#define ITOOLMANAGER_H

#include <string>
#include "raylib.h"
#include "../SceneManager/ISceneManager.h"

enum Tool
{
    SELECT = 0,       // Select objects
    MOVE = 1,         // Move objects
    ROTATE = 2,       // Rotate objects
    SCALE = 3,        // Scale objects
    ADD_CUBE = 4,     // Add cube primitive
    ADD_SPHERE = 5,   // Add sphere primitive
    ADD_CYLINDER = 6, // Add cylinder primitive
    ADD_MODEL = 7     // Add 3D model
};

class IToolManager {
public:
    virtual ~IToolManager() = default;
    virtual void SetActiveTool(Tool tool) = 0;
    virtual Tool GetActiveTool() const = 0;
    virtual bool ExecutePendingAction(ISceneManager& scene) = 0;
    virtual void SetSelectedModel(const std::string& modelName) = 0;
    virtual const std::string& GetSelectedModel() const = 0;
    virtual void HandleToolInput(bool mousePressed, const Ray& ray, ISceneManager& scene) = 0;
    virtual void UpdateTool(const Ray& ray, ISceneManager& scene) = 0;
    virtual void EndTransform() = 0;
};

#endif // ITOOLMANAGER_H