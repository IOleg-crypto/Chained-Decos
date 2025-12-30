#include "DeleteObjectCommand.h"
#include "scene/resources/map/GameScene.h"

namespace CHEngine
{

DeleteObjectCommand::DeleteObjectCommand(const std::shared_ptr<::GameScene> &scene, int index)
    : m_Scene(scene), m_OriginalIndex(index)
{
    if (m_Scene && index >= 0 && index < (int)m_Scene->GetMapObjects().size())
    {
        m_ObjectData = m_Scene->GetMapObjects()[index];
    }
}

void DeleteObjectCommand::Execute()
{
    if (!m_Scene || m_OriginalIndex < 0)
        return;

    auto &objects = m_Scene->GetMapObjectsMutable();
    if (m_OriginalIndex < (int)objects.size())
    {
        objects.erase(objects.begin() + m_OriginalIndex);
    }
}

void DeleteObjectCommand::Undo()
{
    if (!m_Scene || m_OriginalIndex < 0)
        return;

    auto &objects = m_Scene->GetMapObjectsMutable();
    // Re-insert at original position
    if (m_OriginalIndex <= (int)objects.size())
    {
        objects.insert(objects.begin() + m_OriginalIndex, m_ObjectData);
    }
    else
    {
        objects.push_back(m_ObjectData);
    }
}

} // namespace CHEngine
