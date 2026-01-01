#include "EditorSceneManager.h"
#include "EditorMapManager.h"
#include "core/Log.h"
#include "scene/ecs/ECSRegistry.h"
#include "scene/ecs/components/PhysicsData.h"
#include "scene/ecs/components/RenderComponent.h"
#include "scene/ecs/components/ScriptingComponents.h"
#include "scene/ecs/components/TransformComponent.h"
#include "scene/ecs/components/UIComponents.h"
#include "scene/ecs/components/UtilityComponents.h"

namespace CHEngine
{

EditorSceneManager::EditorSceneManager()
    : m_mapManager(std::make_unique<EditorMapManager>()),
      m_activeScene(std::make_unique<Scene>("Game")), m_uiScene(std::make_unique<Scene>("UI")),
      m_skybox(std::make_unique<Skybox>())
{
}

void EditorSceneManager::ClearScene()
{
    m_mapManager->ClearScene();
    m_activeScene = std::make_unique<Scene>("Game");
    m_uiScene = std::make_unique<Scene>("UI");
    m_currentMapPath.clear();
    m_modified = false;
}

void EditorSceneManager::SaveScene(const std::string &path)
{
    std::string savePath = path.empty() ? m_currentMapPath : path;
    if (savePath.empty())
        return;

    m_mapManager->SaveScene(savePath);
    m_currentMapPath = savePath;
    m_modified = false;
    CD_CORE_INFO("[EditorSceneManager] Scene saved to: %s", savePath.c_str());
}

void EditorSceneManager::LoadScene(const std::string &path)
{
    m_mapManager->LoadScene(path);
    m_currentMapPath = path;
    m_modified = false;

    // Crucial: Synchronize UI data to ECS after loading
    RefreshUIEntities();
    RefreshMapEntities();

    CD_CORE_INFO("[EditorSceneManager] Scene loaded from: %s", path.c_str());
}

GameScene &EditorSceneManager::GetGameScene()
{
    return m_mapManager->GetGameScene();
}

void EditorSceneManager::SetSkybox(const std::string &name)
{
    // Implementation for setting skybox by name
}

void EditorSceneManager::SetSkyboxTexture(const std::string &texturePath)
{
    if (m_skybox)
    {
        m_skybox->LoadMaterialTexture(texturePath);
        GetGameScene().GetMapMetaDataMutable().skyboxTexture = texturePath;
        SetSceneModified(true);
    }
}

void EditorSceneManager::SetSkyboxColor(Color color)
{
    m_clearColor = color;
    SetSceneModified(true);
}

void EditorSceneManager::ApplyMetadata(const MapMetadata &metadata)
{
    GetGameScene().SetMapMetaData(metadata);
    if (!metadata.skyboxTexture.empty())
    {
        SetSkyboxTexture(metadata.skyboxTexture);
    }
    SetSceneModified(true);
}

void EditorSceneManager::CreateDefaultObject(MapObjectType type, const std::string &modelName)
{
    // This logic usually involves Spawning through MapManager or directly into ECS
}

void EditorSceneManager::LoadAndSpawnModel(const std::string &path)
{
    // Spawning logic
}
void EditorSceneManager::RemoveObject(int index)
{
    m_mapManager->RemoveObject(index);
}

void EditorSceneManager::RefreshUIEntities()
{
    auto &registry = m_uiScene->GetRegistry();

    // 1. Remove all existing UI entities
    auto view = registry.view<UIElementIndex>();
    registry.destroy(view.begin(), view.end());

    // 2. Clear all entities with UI components just to be safe
    auto uiGroup = registry.view<RectTransform>();
    registry.destroy(uiGroup.begin(), uiGroup.end());

    // 3. Recreate entities from GameScene data
    auto &uiElements = GetGameScene().GetUIElements();
    for (int i = 0; i < (int)uiElements.size(); i++)
    {
        const auto &data = uiElements[i];
        if (!data.isActive)
            continue;

        auto entity = registry.create();

        // Always add Index and Transform
        registry.emplace<UIElementIndex>(entity, i);
        registry.emplace<NameComponent>(entity, data.name);

        RectTransform transform;
        transform.position = data.position;
        transform.size = data.size;
        transform.pivot = data.pivot;
        transform.anchor = (UIAnchor)data.anchor;
        registry.emplace<RectTransform>(entity, transform);

        // Add specialized components based on type
        if (data.type == "button")
        {
            UIButton button;
            button.normalColor = data.normalColor;
            button.hoverColor = data.hoverColor;
            button.pressedColor = data.pressedColor;
            button.borderRadius = data.borderRadius;
            button.borderWidth = data.borderWidth;
            button.borderColor = data.borderColor;
            button.actionType = data.actionType;
            button.actionTarget = data.actionTarget;
            button.eventId = data.eventId;
            registry.emplace<UIButton>(entity, button);

            if (!data.texturePath.empty())
            {
                UIImage image;
                image.texturePath = data.texturePath;
                image.tint = data.tint;
                image.borderRadius = data.borderRadius;
                image.borderWidth = data.borderWidth;
                image.borderColor = data.borderColor;
                registry.emplace<UIImage>(entity, image);
            }

            UIText text;
            text.text = data.text;
            text.color = data.textColor;
            text.fontName = data.fontName.empty() ? "Gantari" : data.fontName;
            text.fontSize = (float)data.fontSize;
            text.spacing = data.spacing;
            registry.emplace<UIText>(entity, text);
        }
        else if (data.type == "imgui_button")
        {
            ImGuiComponent imgui;
            imgui.label = data.text;
            imgui.eventId = data.eventId;
            imgui.isButton = true;
            registry.emplace<ImGuiComponent>(entity, imgui);
        }
        else if (data.type == "text")
        {
            UIText text;
            text.text = data.text;
            text.color = data.textColor;
            text.fontName = data.fontName.empty() ? "Gantari" : data.fontName;
            text.fontSize = (float)data.fontSize;
            text.spacing = data.spacing;
            registry.emplace<UIText>(entity, text);
        }
        else if (data.type == "imgui_text")
        {
            ImGuiComponent imgui;
            imgui.label = data.text;
            imgui.isButton = false;
            registry.emplace<ImGuiComponent>(entity, imgui);
        }
        else if (data.type == "image")
        {
            UIImage image;
            image.tint = data.tint;
            image.borderRadius = data.borderRadius;
            image.borderWidth = data.borderWidth;
            image.borderColor = data.borderColor;
            image.texturePath = data.texturePath;
            registry.emplace<UIImage>(entity, image);
        }

        // Add Scripting if present
        if (!data.scriptPath.empty())
        {
            registry.emplace<CSharpScriptComponent>(entity, data.scriptPath, false);
        }
    }

    CD_CORE_INFO("[EditorSceneManager] Refreshed %d UI entities in ECS.", (int)uiElements.size());
}

void EditorSceneManager::RefreshMapEntities()
{
    auto &registry = m_activeScene->GetRegistry();

    // 1. Remove all existing map entities (those created by this system)
    auto mapEntities = registry.view<MapObjectIndex>();
    registry.destroy(mapEntities.begin(), mapEntities.end());

    // 2. Recreate entities from GameScene data
    auto &mapObjects = GetGameScene().GetMapObjects();
    for (int i = 0; i < (int)mapObjects.size(); i++)
    {
        const auto &data = mapObjects[i];
        if (!data.scriptPath.empty())
        {
            auto entity = registry.create();
            registry.emplace<MapObjectIndex>(entity, i);
            registry.emplace<NameComponent>(entity, data.name);

            // Match legacy MapObjectData to ECS components
            registry.emplace<TransformComponent>(entity, data.position, data.rotation, data.scale);

            // Add ScriptComponent (using scriptPath as className for compatibility)
            registry.emplace<CSharpScriptComponent>(entity, data.scriptPath, false);

            CD_CORE_INFO("[EditorSceneManager] Created ECS Entity for Map Object[%d]: %s", i,
                         data.name.c_str());
        }
    }
}

void EditorSceneManager::SyncEntitiesToMap()
{
    auto &registry = m_activeScene->GetRegistry();
    auto &mapObjects = GetGameScene().GetMapObjectsMutable();

    // Sync 3D Map Objects
    auto view = registry.view<MapObjectIndex, TransformComponent>();
    for (auto entity : view)
    {
        auto &idxComp = view.get<MapObjectIndex>(entity);
        auto &transform = view.get<TransformComponent>(entity);

        if (idxComp.index >= 0 && idxComp.index < (int)mapObjects.size())
        {
            auto &data = mapObjects[idxComp.index];
            data.position = transform.position;
            data.rotation = transform.rotation;
            data.scale = transform.scale;
        }
    }

    // Sync UI Elements
    auto &uiElements = GetGameScene().GetUIElementsMutable();
    auto uiView = m_uiScene->GetRegistry().view<UIElementIndex, RectTransform>();
    for (auto entity : uiView)
    {
        auto &idxComp = uiView.get<UIElementIndex>(entity);
        auto &transform = uiView.get<RectTransform>(entity);

        if (idxComp.index >= 0 && idxComp.index < (int)uiElements.size())
        {
            auto &data = uiElements[idxComp.index];
            data.position = transform.position;
            data.size = transform.size;
        }
    }
}

} // namespace CHEngine
