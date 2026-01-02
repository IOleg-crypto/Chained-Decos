#include "transform_command.h"
#include "scene/resources/map/game_scene.h"

namespace CHEngine
{

TransformCommand::TransformCommand(GameScene *scene, int objectIndex, const MapObjectData &oldData,
                                   const MapObjectData &newData)
    : m_Scene(scene), m_ObjectIndex(objectIndex), m_OldData(oldData), m_NewData(newData)
{
}

void TransformCommand::Execute()
{
    if (m_Scene && m_ObjectIndex >= 0)
    {
        auto &objects = m_Scene->GetMapObjectsMutable();
        if (m_ObjectIndex < (int)objects.size())
        {
            objects[m_ObjectIndex] = m_NewData;
        }
    }
}

void TransformCommand::Undo()
{
    if (m_Scene && m_ObjectIndex >= 0)
    {
        auto &objects = m_Scene->GetMapObjectsMutable();
        if (m_ObjectIndex < (int)objects.size())
        {
            objects[m_ObjectIndex] = m_OldData;
        }
    }
}

} // namespace CHEngine
