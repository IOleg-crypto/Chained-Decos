#pragma once

#include "editor_command.h"
#include "scene/resources/map/map_data.h"
#include <memory>

namespace CHEngine
{
class GameScene;

/**
 * @brief Command for undoing/redoing map object transformations
 */
class TransformCommand : public IEditorCommand
{
public:
    TransformCommand(GameScene *scene, int objectIndex, const MapObjectData &oldData,
                     const MapObjectData &newData);

    void Execute() override;
    void Undo() override;
    std::string GetName() const override
    {
        return "Transform Object";
    }

private:
    GameScene *m_Scene;
    int m_ObjectIndex;
    MapObjectData m_OldData;
    MapObjectData m_NewData;
};

} // namespace CHEngine
