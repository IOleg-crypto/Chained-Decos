#include "add_object_command.h"
#include "scene/resources/map/game_scene.h"

namespace CHEngine
{

AddObjectCommand::AddObjectCommand(GameScene *scene, const MapObjectData &object)
    : m_Scene(scene), m_ObjectData(object)
{
}

void AddObjectCommand::Execute()
{
    if (!m_Scene)
        return;

    auto &objects = m_Scene->GetMapObjectsMutable();
    objects.push_back(m_ObjectData);
    m_AddedIndex = (int)objects.size() - 1;
}

void AddObjectCommand::Undo()
{
    if (!m_Scene || m_AddedIndex < 0)
        return;

    auto &objects = m_Scene->GetMapObjectsMutable();
    if (m_AddedIndex < (int)objects.size())
    {
        objects.erase(objects.begin() + m_AddedIndex);
        m_AddedIndex = -1; // Reset until re-executed
    }
}

} // namespace CHEngine
