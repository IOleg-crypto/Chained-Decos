#ifndef EDITOR_ENTITY_FACTORY_H
#define EDITOR_ENTITY_FACTORY_H

#include "core/Base.h"
#include "scene/core/Scene.h"
#include "scene/resources/map/MapData.h"
#include <entt/entt.hpp>
#include <memory>
#include <string>

namespace CHEngine
{
class CommandHistory;
class SelectionManager;

class EditorEntityFactory
{
public:
    EditorEntityFactory(std::shared_ptr<Scene> scene, CommandHistory &commandHistory,
                        SelectionManager &selectionManager);
    ~EditorEntityFactory() = default;

    void CreateEntity();
    void DeleteEntity(entt::entity entity);

    void AddObject(const MapObjectData &data);
    void DeleteObject(int index);
    void AddModel();
    void AddUIElement(const std::string &type, const Vector2 &viewportSize);

    void OnAssetDropped(const std::string &assetPath, const Vector3 &worldPosition);

private:
    std::shared_ptr<Scene> m_Scene;
    CommandHistory &m_CommandHistory;
    SelectionManager &m_SelectionManager;
};
} // namespace CHEngine

#endif // EDITOR_ENTITY_FACTORY_H
