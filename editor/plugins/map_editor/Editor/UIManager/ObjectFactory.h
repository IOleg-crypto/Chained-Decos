#ifndef OBJECT_FACTORY_H
#define OBJECT_FACTORY_H

#include "../Object/MapObject.h"
#include "../ToolManager/IToolManager.h"
#include "../SceneManager/ISceneManager.h"
#include <string>

// ObjectFactory - handles creation of map objects
class ObjectFactory
{
public:
    ObjectFactory(ISceneManager* sceneManager, IToolManager* toolManager);
    ~ObjectFactory() = default;

    // Create object based on active tool and selected model name
    void CreateObject(::Tool activeTool, const std::string& selectedModelName);

private:
    ISceneManager* m_sceneManager;
    IToolManager* m_toolManager;
};

#endif // OBJECT_FACTORY_H

