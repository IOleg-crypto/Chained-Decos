#include "EditorEntityFactory.h"
#include "core/Log.h"
#include "editor/logic/SelectionManager.h"
#include "editor/logic/undo/AddObjectCommand.h"
#include "editor/logic/undo/CommandHistory.h"
#include "editor/logic/undo/DeleteObjectCommand.h"
#include "nfd.h"
#include "scene/MapManager.h"
#include "scene/ecs/components/TransformComponent.h"
#include "scene/ecs/components/core/IDComponent.h"
#include "scene/ecs/components/core/TagComponent.h"
#include <filesystem>

namespace CHEngine
{

EditorEntityFactory::EditorEntityFactory(std::shared_ptr<Scene> scene,
                                         CommandHistory &commandHistory,
                                         SelectionManager &selectionManager)
    : m_Scene(scene), m_CommandHistory(commandHistory), m_SelectionManager(selectionManager)
{
}

void EditorEntityFactory::CreateEntity()
{
    if (!m_Scene)
        return;

    auto &registry = m_Scene->GetRegistry();
    entt::entity e = registry.create();
    registry.emplace<IDComponent>(e);
    registry.emplace<TagComponent>(e, "Empty Entity");
    registry.emplace<TransformComponent>(e);

    m_SelectionManager.SetEntitySelection(e);
    CD_INFO("Created new entity: %llu", (unsigned long long)registry.get<IDComponent>(e).ID);
}

void EditorEntityFactory::DeleteEntity(entt::entity entity)
{
    if (!m_Scene || entity == entt::null)
        return;

    std::string tagStr = "Unknown";
    if (m_Scene->GetRegistry().all_of<TagComponent>(entity))
        tagStr = m_Scene->GetRegistry().get<TagComponent>(entity).Tag;

    m_Scene->DestroyEntity(entity);
    CD_INFO("Deleted entity: %s", tagStr.c_str());

    if (m_SelectionManager.GetSelectedEntity() == entity)
        m_SelectionManager.ClearSelection();
}

void EditorEntityFactory::AddObject(const MapObjectData &data)
{
    auto activeScene = MapManager::GetCurrentScene();
    if (!activeScene)
        return;

    auto cmd = std::make_unique<AddObjectCommand>(activeScene, data);
    m_CommandHistory.PushCommand(std::move(cmd));
    CD_INFO("Added legacy object: %s", data.name.c_str());
}

void EditorEntityFactory::DeleteObject(int index)
{
    auto activeScene = MapManager::GetCurrentScene();
    if (!activeScene || index < 0)
        return;

    std::string name = activeScene->GetMapObjects()[index].name;
    auto cmd = std::make_unique<DeleteObjectCommand>(activeScene, index);
    m_CommandHistory.PushCommand(std::move(cmd));
    CD_INFO("Deleted legacy object: %s", name.c_str());

    if (m_SelectionManager.GetSelectedIndex() == index)
        m_SelectionManager.SetSelection(-1, SelectionType::NONE);
}

void EditorEntityFactory::AddModel()
{
    nfdchar_t *outPath = nullptr;
    nfdfilteritem_t filterItem[2] = {{"3D Models", "obj,glb,gltf"}, {"All Files", "*"}};
    nfdresult_t result = NFD_OpenDialog(&outPath, filterItem, 2, nullptr);

    if (result == NFD_OKAY)
    {
        std::filesystem::path fullPath(outPath);
        std::string filename = fullPath.filename().string();

        auto activeScene = MapManager::GetCurrentScene();
        auto &models = activeScene->GetMapModelsMutable();
        if (models.find(filename) == models.end())
        {
            Model model = LoadModel(outPath);
            if (model.meshCount > 0)
            {
                models[filename] = model;
                CD_INFO("Loaded model: %s", filename.c_str());
            }
            else
            {
                CD_ERROR("Failed to load model: %s", outPath);
            }
        }

        MapObjectData obj;
        obj.name = filename;
        obj.type = MapObjectType::MODEL;
        obj.modelName = filename;
        obj.position = {0, 0, 0};
        obj.scale = {1, 1, 1};
        obj.color = WHITE;

        auto &objects = activeScene->GetMapObjectsMutable();
        objects.push_back(obj);
        m_SelectionManager.SetSelection((int)objects.size() - 1, SelectionType::WORLD_OBJECT);

        NFD_FreePath(outPath);
    }
}

void EditorEntityFactory::AddUIElement(const std::string &type, const Vector2 &viewportSize)
{
    auto activeScene = MapManager::GetCurrentScene();
    if (!activeScene)
        return;

    UIElementData el;
    el.name = "element_" + std::to_string(rand() % 1000);
    el.type = type;
    el.position = {viewportSize.x * 0.5f, viewportSize.y * 0.5f};
    el.size = {100, 40};
    el.anchor = 0;
    el.pivot = {0.5f, 0.5f};

    if (type == "Button")
    {
        el.text = "Button";
    }

    auto &elements = activeScene->GetUIElementsMutable();
    elements.push_back(el);
    m_SelectionManager.SetSelection((int)elements.size() - 1, SelectionType::UI_ELEMENT);
}

void EditorEntityFactory::OnAssetDropped(const std::string &assetPath, const Vector3 &worldPosition)
{
    auto activeScene = MapManager::GetCurrentScene();
    if (!activeScene)
        return;

    std::filesystem::path fullPath(assetPath);
    std::string filename = fullPath.filename().string();
    std::string ext = fullPath.extension().string();

    if (ext == ".obj" || ext == ".glb" || ext == ".gltf")
    {
        auto &models = activeScene->GetMapModelsMutable();
        if (models.find(filename) == models.end())
        {
            Model model = LoadModel(assetPath.c_str());
            if (model.meshCount > 0)
            {
                models[filename] = model;
                CD_INFO("Loaded dropped model: %s", filename.c_str());
            }
            else
            {
                CD_ERROR("Failed to load dropped model: %s", assetPath.c_str());
                return;
            }
        }

        MapObjectData obj;
        obj.name = filename;
        obj.type = MapObjectType::MODEL;
        obj.modelName = filename;
        obj.position = worldPosition;
        obj.scale = {1, 1, 1};
        obj.color = WHITE;

        AddObject(obj);
    }
}

} // namespace CHEngine
