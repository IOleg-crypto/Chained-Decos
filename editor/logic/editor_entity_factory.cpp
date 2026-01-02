#include "editor_entity_factory.h"
#include "core/log.h"
#include "editor/logic/selection_manager.h"
#include "editor/logic/undo/command_history.h"
#include "editor_scene_manager.h"
#include "nfd.h"
#include "scene/core/entity.h"
#include "scene/core/scene.h"
#include "scene/ecs/components/core/id_component.h"
#include "scene/ecs/components/core/tag_component.h"
#include "scene/ecs/components/render_component.h"
#include "scene/ecs/components/transform_component.h"
#include <filesystem>

namespace CHEngine
{

EditorEntityFactory::EditorEntityFactory(std::shared_ptr<Scene> scene,
                                         CommandHistory &commandHistory,
                                         SelectionManager &selectionManager,
                                         EditorSceneManager *sceneManager)
    : m_Scene(scene), m_CommandHistory(commandHistory), m_SelectionManager(selectionManager),
      m_SceneManager(sceneManager)
{
}

entt::entity EditorEntityFactory::CreateEntity(const std::string &name)
{
    if (!m_Scene)
        return entt::null;

    auto entity = m_Scene->CreateEntity(name);
    m_SelectionManager.SetSelection((entt::entity)entity);
    return (entt::entity)entity;
}

void EditorEntityFactory::DeleteEntity(entt::entity entity)
{
    if (!m_Scene || entity == entt::null)
        return;

    m_Scene->DestroyEntity(entity);
    if (m_SelectionManager.GetSelectedEntity() == entity)
        m_SelectionManager.ClearSelection();
}

void EditorEntityFactory::AddModel()
{
    nfdchar_t *outPath = nullptr;
    nfdfilteritem_t filterItem[2] = {{"3D Models", "obj,glb,gltf"}, {"All Files", "*"}};

    if (NFD_OpenDialog(&outPath, filterItem, 2, nullptr) == NFD_OKAY)
    {
        auto entity = CreateEntity(std::filesystem::path(outPath).filename().string());
        auto &render = m_Scene->GetRegistry().emplace<RenderComponent>(entity);
        // Logic to load/assign model to RenderComponent
        // render.model = ...

        NFD_FreePath(outPath);
    }
}

void EditorEntityFactory::AddUIButton(const Vector2 &position)
{
    auto entity = CreateEntity("Button");
    // Add UI components here
}

void EditorEntityFactory::AddUIText(const Vector2 &position)
{
    auto entity = CreateEntity("Text");
    // Add UI components here
}

void EditorEntityFactory::OnAssetDropped(const std::string &assetPath, const Vector3 &worldPosition)
{
    std::filesystem::path path(assetPath);
    std::string ext = path.extension().string();

    if (ext == ".obj" || ext == ".glb" || ext == ".gltf")
    {
        auto entity = CreateEntity(path.filename().string());
        auto &transform = m_Scene->GetRegistry().get<TransformComponent>(entity);
        transform.position = worldPosition;

        auto &render = m_Scene->GetRegistry().emplace<RenderComponent>(entity);
        // render.model = ...
    }
}

} // namespace CHEngine
