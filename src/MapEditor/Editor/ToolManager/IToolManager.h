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
    ADD_MODEL = 7,    // Add 3D model
    ADD_SPAWN_ZONE = 8, // Add player spawn zone
    ADD_SKYBOX = 9, // Add skybox
    ADD_SKYBOX_CUBEMAP = 10, // Add skybox cubemap
    ADD_SKYBOX_HDR = 11, // Add skybox hdr
    ADD_SKYBOX_VS = 12, // Add skybox vs
    ADD_SKYBOX_FS = 13, // Add skybox fs
    ADD_SKYBOX_CUBEMAP_VS = 14, // Add skybox cubemap vs
    ADD_SKYBOX_CUBEMAP_FS = 15 // Add skybox cubemap fs
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