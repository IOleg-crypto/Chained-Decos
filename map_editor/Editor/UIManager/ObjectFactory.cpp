#include "ObjectFactory.h"
#include "../Editor.h"
#include "scene/resources/map/Core/MapData.h"

ObjectFactory::ObjectFactory(Editor *editor) : m_editor(editor)
{
}

void ObjectFactory::CreateObject(::Tool activeTool, const std::string &selectedModelName)
{
    MapObjectData newObj;

    // Set default properties
    newObj.position = {0.0f, 0.0f, 0.0f};
    newObj.scale = {1.0f, 1.0f, 1.0f};
    newObj.rotation = {0.0f, 0.0f, 0.0f};
    newObj.color = WHITE;

    switch (activeTool)
    {
    case 1: // ADD_CUBE (mapping to simplified Tool enum)
        newObj.type = MapObjectType::CUBE;
        break;
    case 2: // ADD_SPHERE
        newObj.type = MapObjectType::SPHERE;
        break;
    case 3: // ADD_CYLINDER
        newObj.type = MapObjectType::CYLINDER;
        break;
    case 5: // ADD_MODEL
        newObj.type = MapObjectType::MODEL;
        newObj.modelName = selectedModelName;
        break;
    case 6: // ADD_SPAWN_ZONE
        newObj.type = MapObjectType::SPAWN_ZONE;
        newObj.color = {255, 100, 100, 200}; // Semi-transparent red
        break;
    default:
        // Try to match based on tool index
        if (activeTool > 0 && activeTool < 7)
        {
            newObj.type = static_cast<MapObjectType>(activeTool - 1);
        }
        else
        {
            return; // Unknown tool, don't create object
        }
        break;
    }

    m_editor->AddObject(newObj);
}
