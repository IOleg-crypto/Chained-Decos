#include "TransformCommand.h"
#include "scene/resources/map/GameScene.h"

namespace CHEngine
{

TransformCommand::TransformCommand(const std::shared_ptr<::GameScene> &scene, int objectIndex,
                                   const ::MapObjectData &oldData, const ::MapObjectData &newData)
    : m_Scene(scene), m_ObjectIndex(objectIndex), m_OldData(oldData), m_NewData(newData)
{
}

void TransformCommand::Execute()
{
    if (!m_Scene || m_ObjectIndex < 0)
        return;

    auto &objects = m_Scene->GetMapObjectsMutable();
    if (m_ObjectIndex < (int)objects.size())
    {
        objects[m_ObjectIndex].position = m_NewData.position;
        objects[m_ObjectIndex].rotation = m_NewData.rotation;
        objects[m_ObjectIndex].scale = m_NewData.scale;
    }
}

void TransformCommand::Undo()
{
    if (!m_Scene || m_ObjectIndex < 0)
        return;

    auto &objects = m_Scene->GetMapObjectsMutable();
    if (m_ObjectIndex < (int)objects.size())
    {
        objects[m_ObjectIndex].position = m_OldData.position;
        objects[m_ObjectIndex].rotation = m_OldData.rotation;
        objects[m_ObjectIndex].scale = m_OldData.scale;
    }
}

} // namespace CHEngine
