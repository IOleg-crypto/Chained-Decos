#ifndef MODIFYOBJECTCOMMAND_H
#define MODIFYOBJECTCOMMAND_H

#include "EditorCommand.h"
#include "scene/resources/map/GameScene.h"
#include "scene/resources/map/MapData.h"
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

#endif // MODIFYOBJECTCOMMAND_H
