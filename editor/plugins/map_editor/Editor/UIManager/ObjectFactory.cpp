#include "ObjectFactory.h"
#include "../Object/MapObject.h"
#include "../ToolManager/IToolManager.h"
#include "../SceneManager/ISceneManager.h"

ObjectFactory::ObjectFactory(ISceneManager* sceneManager, IToolManager* toolManager)
    : m_sceneManager(sceneManager), m_toolManager(toolManager)
{
}

void ObjectFactory::CreateObject(::Tool activeTool, const std::string& selectedModelName)
{
    MapObject newObj;
    std::string baseName = "New Object " + std::to_string(m_sceneManager->GetObjects().size());

    switch (activeTool)
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
        newObj.SetModelAssetName(selectedModelName);
        newObj.SetObjectName(selectedModelName + " " +
                             std::to_string(m_sceneManager->GetObjects().size()));
        break;
    case ADD_SPAWN_ZONE:
        newObj.SetObjectType(6); // Spawn Zone
        newObj.SetObjectName("Spawn Zone");
        newObj.SetColor({255, 100, 100, 200}); // Semi-transparent red
        break;
    default:
        return; // Unknown tool, don't create object
    }

    // Set default position, scale, and other properties
    newObj.SetPosition({0.0f, 0.0f, 0.0f});
    newObj.SetScale({1.0f, 1.0f, 1.0f});
    newObj.SetRotation({0.0f, 0.0f, 0.0f});
    newObj.SetColor({255, 255, 255, 255}); // White color

    m_sceneManager->AddObject(newObj);
}

