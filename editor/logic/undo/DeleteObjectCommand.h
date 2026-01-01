#pragma once

#include "EditorCommand.h"
#include "scene/resources/map/MapData.h"
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
    DeleteObjectCommand(const std::shared_ptr<GameScene> &scene, int index);

    void Execute() override;
    void Undo() override;
    std::string GetName() const override
    {
        return "Delete Object";
    }

private:
    std::shared_ptr<GameScene> m_Scene;
    MapObjectData m_ObjectData;
    int m_OriginalIndex;
};

} // namespace CHEngine
