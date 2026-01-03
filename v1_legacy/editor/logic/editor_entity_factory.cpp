#include "editor_entity_factory.h"
#include "core/log.h"
#include "editor/logic/selection_manager.h"
#include "engine/scene/core/entity.h"
#include "engine/scene/core/scene.h"
#include "engine/scene/core/scene_manager.h"
#include "engine/scene/ecs/components/render_component.h"
#include "engine/scene/ecs/components/transform_component.h"
#include <filesystem>

namespace CHEngine
{
EditorEntityFactory::EditorEntityFactory()
{
}

entt::entity EditorEntityFactory::CreateEntity(const std::string &name)
{
    auto scene = SceneManager::Get().GetActiveScene();
    if (!scene)
        return entt::null;

    auto entity = scene->CreateEntity(name);
    SelectionManager::Get().SetSelection((entt::entity)entity);
    return (entt::entity)entity;
}

void EditorEntityFactory::DeleteEntity(entt::entity entity)
{
    auto scene = SceneManager::Get().GetActiveScene();
    if (!scene || entity == entt::null)
        return;

    scene->DestroyEntity(entity);
    if (SelectionManager::Get().GetSelectedEntity() == entity)
        SelectionManager::Get().ClearSelection();
}

void EditorEntityFactory::AddModel()
{
    // NFD dialog and AddModel logic using SceneManager::Get().GetActiveScene()
}

void EditorEntityFactory::AddUIButton(const Vector2 &position)
{
    CreateEntity("Button");
}

void EditorEntityFactory::AddUIText(const Vector2 &position)
{
    CreateEntity("Text");
}

void EditorEntityFactory::OnAssetDropped(const std::string &assetPath, const Vector3 &worldPosition)
{
    auto scene = SceneManager::Get().GetActiveScene();
    if (!scene)
        return;

    std::filesystem::path path(assetPath);
    auto entity = CreateEntity(path.filename().string());
    auto &transform = scene->GetRegistry().get<TransformComponent>(entity);
    transform.position = worldPosition;
    scene->GetRegistry().emplace<RenderComponent>(entity);
}
} // namespace CHEngine
