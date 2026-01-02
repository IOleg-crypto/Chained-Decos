#ifndef CD_EDITOR_LOGIC_EDITOR_ENTITY_FACTORY_H
#define CD_EDITOR_LOGIC_EDITOR_ENTITY_FACTORY_H

#include "core/Base.h"
#include "scene/core/scene.h"
#include <entt/entt.hpp>
#include <memory>
#include <string>

namespace CHEngine
{
class CommandHistory;
class SelectionManager;
class EditorSceneManager;

class EditorEntityFactory
{
public:
    EditorEntityFactory(std::shared_ptr<Scene> scene, CommandHistory &commandHistory,
                        SelectionManager &selectionManager, EditorSceneManager *sceneManager);
    ~EditorEntityFactory() = default;

    entt::entity CreateEntity(const std::string &name = "Empty Entity");
    void DeleteEntity(entt::entity entity);

    void AddModel(); // Now creates an ECS entity with a RenderComponent

    // UI elements are now also ECS entities
    void AddUIButton(const Vector2 &position);
    void AddUIText(const Vector2 &position);

    void OnAssetDropped(const std::string &assetPath, const Vector3 &worldPosition);

    void SetScene(std::shared_ptr<Scene> scene)
    {
        m_Scene = scene;
    }

private:
    std::shared_ptr<Scene> m_Scene;
    CommandHistory &m_CommandHistory;
    SelectionManager &m_SelectionManager;
    EditorSceneManager *m_SceneManager;
};
} // namespace CHEngine

#endif // CD_EDITOR_LOGIC_EDITOR_ENTITY_FACTORY_H
