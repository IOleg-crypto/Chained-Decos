#include "SceneManager.h"
#include "MapManager.h"
#include "core/Log.h"
#include "scene/ecs/ECSRegistry.h"
#include "scene/ecs/components/UIComponents.h"
#include "scene/ecs/components/UtilityComponents.h"

SceneManager::SceneManager()
    : m_mapManager(std::make_unique<MapManager>()),
      m_activeScene(std::make_unique<CHEngine::Scene>()), m_skybox(std::make_unique<Skybox>())
{
}

void SceneManager::ClearScene()
{
    m_mapManager->ClearScene();
    m_activeScene = std::make_unique<CHEngine::Scene>();
    m_currentMapPath.clear();
    m_modified = false;
}

void SceneManager::SaveScene(const std::string &path)
{
    std::string savePath = path.empty() ? m_currentMapPath : path;
    if (savePath.empty())
        return;

    m_mapManager->SaveScene(savePath);
    m_currentMapPath = savePath;
    m_modified = false;
    CD_INFO("[SceneManager] Scene saved to: %s", savePath.c_str());
}

void SceneManager::LoadScene(const std::string &path)
{
    m_mapManager->LoadScene(path);
    m_currentMapPath = path;
    m_modified = false;

    // Crucial: Synchronize UI data to ECS after loading
    RefreshUIEntities();

    CD_INFO("[SceneManager] Scene loaded from: %s", path.c_str());
}

GameScene &SceneManager::GetGameScene()
{
    return m_mapManager->GetGameScene();
}

void SceneManager::SetSkybox(const std::string &name)
{
    // Implementation for setting skybox by name
}

void SceneManager::SetSkyboxTexture(const std::string &texturePath)
{
    if (m_skybox)
    {
        m_skybox->LoadMaterialTexture(texturePath);
        GetGameScene().GetMapMetaDataMutable().skyboxTexture = texturePath;
        SetSceneModified(true);
    }
}

void SceneManager::SetSkyboxColor(Color color)
{
    m_clearColor = color;
    SetSceneModified(true);
}

void SceneManager::ApplyMetadata(const MapMetadata &metadata)
{
    GetGameScene().SetMapMetaData(metadata);
    if (!metadata.skyboxTexture.empty())
    {
        SetSkyboxTexture(metadata.skyboxTexture);
    }
    SetSceneModified(true);
}

void SceneManager::CreateDefaultObject(MapObjectType type, const std::string &modelName)
{
    // This logic usually involves Spawning through MapManager or directly into ECS
    // For now, delegate to existing map logic if possible, or implement fresh
}

void SceneManager::LoadAndSpawnModel(const std::string &path)
{
    // Spawning logic
}
void SceneManager::RemoveObject(int index)
{
    m_mapManager->RemoveObject(index);
}

void SceneManager::RefreshUIEntities()
{
    auto &registry = ::ECSRegistry::Get();

    // 1. Remove all existing UI entities
    auto view = registry.view<CHEngine::UIElementIndex>();
    registry.destroy(view.begin(), view.end());

    // 2. Clear all entities with UI components just to be safe
    auto uiGroup = registry.view<CHEngine::RectTransform>();
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
        registry.emplace<CHEngine::UIElementIndex>(entity, i);
        registry.emplace<CHEngine::NameComponent>(entity, data.name);

        CHEngine::RectTransform transform;
        transform.position = data.position;
        transform.size = data.size;
        transform.pivot = data.pivot;
        transform.anchor = (CHEngine::UIAnchor)data.anchor;
        registry.emplace<CHEngine::RectTransform>(entity, transform);

        // Add specialized components based on type
        if (data.type == "button" || data.type == "imgui_button")
        {
            CHEngine::UIButton button;
            button.normalColor = data.normalColor;
            button.hoverColor = data.hoverColor;
            button.pressedColor = data.pressedColor;
            button.borderRadius = data.borderRadius;
            button.borderWidth = data.borderWidth;
            button.borderColor = data.borderColor;
            button.actionType = data.actionType;
            button.actionTarget = data.actionTarget;
            registry.emplace<CHEngine::UIButton>(entity, button);

            CHEngine::UIText text;
            text.text = data.text;
            text.color = data.textColor;
            text.fontName = data.fontName.empty() ? "Gantari" : data.fontName;
            text.fontSize = (float)data.fontSize;
            registry.emplace<CHEngine::UIText>(entity, text);
        }
        else if (data.type == "text" || data.type == "imgui_text")
        {
            CHEngine::UIText text;
            text.text = data.text;
            text.color = data.textColor;
            text.fontName = data.fontName.empty() ? "Gantari" : data.fontName;
            text.fontSize = (float)data.fontSize;
            registry.emplace<CHEngine::UIText>(entity, text);
        }
        else if (data.type == "image")
        {
            CHEngine::UIImage image;
            image.tint = data.tint;
            image.borderRadius = data.borderRadius;
            image.borderWidth = data.borderWidth;
            image.borderColor = data.borderColor;
            image.texturePath = data.texturePath;
            registry.emplace<CHEngine::UIImage>(entity, image);
        }
    }

    CD_INFO("[SceneManager] Refreshed %d UI entities in ECS.", (int)uiElements.size());
}
