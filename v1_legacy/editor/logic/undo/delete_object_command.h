#pragma once

#include "editor_command.h"
#include "engine/scene/resources/map/map_data.h"
#include <memory>

namespace CHEngine
{
class GameScene;
}

namespace CHEngine
{

/**
 * @brief Command for deleting an object from the scene
 */
class DeleteObjectCommand : public IEditorCommand
{
public:
    DeleteObjectCommand(GameScene *scene, int index);

    void Execute() override;
    void Undo() override;
    std::string GetName() const override
    {
        return "Delete Object";
    }

private:
    GameScene *m_Scene;
    MapObjectData m_ObjectData;
    int m_OriginalIndex;
};

} // namespace CHEngine
