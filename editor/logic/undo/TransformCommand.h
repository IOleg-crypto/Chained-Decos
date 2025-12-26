#pragma once

#include "EditorCommand.h"
#include "scene/resources/map/MapData.h"
#include <memory>

class GameScene;

namespace CHEngine
{

/**
 * @brief Command for undoing/redoing map object transformations
 */
class TransformCommand : public IEditorCommand
{
public:
    TransformCommand(const std::shared_ptr<::GameScene> &scene, int objectIndex,
                     const ::MapObjectData &oldData, const ::MapObjectData &newData);

    void Execute() override;
    void Undo() override;
    std::string GetName() const override
    {
        return "Transform Object";
    }

private:
    std::shared_ptr<::GameScene> m_Scene;
    int m_ObjectIndex;
    ::MapObjectData m_OldData;
    ::MapObjectData m_NewData;
};

} // namespace CHEngine
