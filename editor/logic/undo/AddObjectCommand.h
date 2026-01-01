#pragma once

#include "EditorCommand.h"
#include "scene/resources/map/MapData.h"
#include <memory>

namespace CHEngine
{
class GameScene;

/**
 * @brief Command for adding an object to the scene
 */
class AddObjectCommand : public IEditorCommand
{
public:
    AddObjectCommand(const std::shared_ptr<GameScene> &scene, const MapObjectData &objData);

    void Execute() override;
    void Undo() override;
    std::string GetName() const override
    {
        return "Add Object";
    }

private:
    std::shared_ptr<GameScene> m_Scene;
    MapObjectData m_ObjectData;
    int m_AddedIndex = -1;
};

} // namespace CHEngine
