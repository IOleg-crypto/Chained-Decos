#ifndef CD_EDITOR_LOGIC_UNDO_MODIFY_OBJECT_COMMAND_H
#define CD_EDITOR_LOGIC_UNDO_MODIFY_OBJECT_COMMAND_H

#include "editor_command.h"
#include "engine/scene/resources/map/game_scene.h"
#include "engine/scene/resources/map/map_data.h"
#include <memory>

namespace CHEngine
{
class ModifyObjectCommand : public IEditorCommand
{
public:
    ModifyObjectCommand(std::shared_ptr<GameScene> scene, int objectIndex,
                        const MapObjectData &oldData, const MapObjectData &newData)
        : m_Scene(scene), m_Index(objectIndex), m_OldData(oldData), m_NewData(newData)
    {
    }

    void Execute() override
    {
        if (m_Index >= 0 && m_Index < (int)m_Scene->GetMapObjects().size())
        {
            m_Scene->GetMapObjectsMutable()[m_Index] = m_NewData;
        }
    }

    void Undo() override
    {
        if (m_Index >= 0 && m_Index < (int)m_Scene->GetMapObjects().size())
        {
            m_Scene->GetMapObjectsMutable()[m_Index] = m_OldData;
        }
    }

    std::string GetName() const override
    {
        return "Modify Object";
    }

private:
    std::shared_ptr<GameScene> m_Scene;
    int m_Index;
    MapObjectData m_OldData;
    MapObjectData m_NewData;
};
} // namespace CHEngine

#endif // CD_EDITOR_LOGIC_UNDO_MODIFY_OBJECT_COMMAND_H
