#include "ToolManager.h"
#include "../SceneManager/SceneManager.h"
#include "../Object/MapObject.h"
#include "Engine/Collision/CollisionStructures.h"
#include <iostream>

// Cast ISceneManager to SceneManager for implementation
#define SCENE_CAST(scene) dynamic_cast<SceneManager&>(scene)

ToolManager::ToolManager()
    : m_activeTool(SELECT), m_pendingObjectCreation(false), m_currentlySelectedModelName("")
{
}

void ToolManager::SetActiveTool(Tool tool)
{
    m_activeTool = tool;

    // Set pending creation for object creation tools
    if (tool == ADD_CUBE || tool == ADD_SPHERE || tool == ADD_CYLINDER || tool == ADD_MODEL)
    {
        m_pendingObjectCreation = true;
    }
}

Tool ToolManager::GetActiveTool() const
{
    return m_activeTool;
}

bool ToolManager::ExecutePendingAction(ISceneManager& scene)
{
    if (!m_pendingObjectCreation)
    {
        return false;
    }

    CreateObjectForTool(m_activeTool, SCENE_CAST(scene));
    m_pendingObjectCreation = false;
    m_activeTool = SELECT; // Reset to select tool after creation

    return true;
}

void ToolManager::SetSelectedModel(const std::string& modelName)
{
    m_currentlySelectedModelName = modelName;
}

const std::string& ToolManager::GetSelectedModel() const
{
    return m_currentlySelectedModelName;
}

void ToolManager::HandleToolInput(bool mousePressed, const Ray& ray, ISceneManager& scene)
{
    if (mousePressed && m_activeTool == SELECT)
    {
        CollisionRay collisionRay(ray.position, ray.direction);
        SCENE_CAST(scene).PickObject(collisionRay);
    }
}

void ToolManager::CreateObjectForTool(Tool tool, ISceneManager& scene)
{
    MapObject newObj;
    std::string baseName = "New Object " + std::to_string(SCENE_CAST(scene).GetObjects().size());

    switch (tool)
    {
    case ADD_CUBE:
        newObj.SetObjectType(0); // Cube
        newObj.SetObjectName(baseName + " (Cube)");
        break;
    case ADD_SPHERE:
        newObj.SetObjectType(1); // Sphere
        newObj.SetObjectName(baseName + " (Sphere)");
        break;
    case ADD_CYLINDER:
        newObj.SetObjectType(2); // Cylinder
        newObj.SetObjectName(baseName + " (Cylinder)");
        break;
    case ADD_MODEL:
        newObj.SetObjectType(5); // Model
        newObj.SetModelAssetName(m_currentlySelectedModelName);
        newObj.SetObjectName(m_currentlySelectedModelName + " " + std::to_string(SCENE_CAST(scene).GetObjects().size()));
        break;
    default:
        std::cout << "Warning: Attempted to create object with invalid tool" << std::endl;
        return;
    }

    SCENE_CAST(scene).AddObject(newObj);
    std::cout << "Created new object with tool: " << tool << std::endl;
}