#include "GameScene.h"

namespace CHEngine
{

GameScene::~GameScene()
{
    Cleanup();
}

void GameScene::Cleanup()
{
    for (auto &pair : m_loadedModels)
    {
        UnloadModel(pair.second);
    }
    m_loadedModels.clear();

    for (auto &pair : m_loadedTextures)
    {
        UnloadTexture(pair.second);
    }
    m_loadedTextures.clear();

    m_objects.clear();
    m_uiElements.clear();
    m_skybox.reset();
}

Skybox *GameScene::GetSkyBox() const
{
    return m_skybox.get();
}

void GameScene::SetSkyBox(std::shared_ptr<Skybox> &skybox)
{
    m_skybox = skybox;
}

const std::unordered_map<std::string, Model> &GameScene::GetMapModels() const
{
    return m_loadedModels;
}

void GameScene::AddMapModels(const std::unordered_map<std::string, Model> &modelsMap)
{
    m_loadedModels.insert(modelsMap.begin(), modelsMap.end());
}

const std::vector<MapObjectData> &GameScene::GetMapObjects() const
{
    return m_objects;
}

void GameScene::AddMapObjects(const std::vector<MapObjectData> &mapObjects)
{
    m_objects.insert(m_objects.end(), mapObjects.begin(), mapObjects.end());
}

const MapMetadata &GameScene::GetMapMetaData() const
{
    return m_metadata;
}

void GameScene::SetMapMetaData(const MapMetadata &mapData)
{
    m_metadata = mapData;
}

MapMetadata &GameScene::GetMapMetaDataMutable()
{
    return m_metadata;
}

std::unordered_map<std::string, Model> &GameScene::GetMapModelsMutable()
{
    return m_loadedModels;
}

std::vector<MapObjectData> &GameScene::GetMapObjectsMutable()
{
    return m_objects;
}

const std::vector<UIElementData> &GameScene::GetUIElements() const
{
    return m_uiElements;
}

void GameScene::AddUIElements(const std::vector<UIElementData> &uiElements)
{
    m_uiElements.insert(m_uiElements.end(), uiElements.begin(), uiElements.end());
}

std::vector<UIElementData> &GameScene::GetUIElementsMutable()
{
    return m_uiElements;
}

const std::unordered_map<std::string, Texture2D> &GameScene::GetMapTextures() const
{
    return m_loadedTextures;
}

void GameScene::AddMapTextures(const std::unordered_map<std::string, Texture2D> &texturesMap)
{
    m_loadedTextures.insert(texturesMap.begin(), texturesMap.end());
}

std::unordered_map<std::string, Texture2D> &GameScene::GetMapTexturesMutable()
{
    return m_loadedTextures;
}

} // namespace CHEngine
