#ifndef GAMESCENE_H
#define GAMESCENE_H

#include "MapData.h"
#include "Skybox.h"
#include <memory>
#include <raylib.h>
#include <string>
#include <unordered_map>
#include <vector>

namespace CHEngine
{
// ============================================================================
// GameScene Class
// ============================================================================

class GameScene : public std::enable_shared_from_this<GameScene>
{
public:
    GameScene() = default;
    ~GameScene();

    // Non-copyable, movable
    GameScene(const GameScene &) = delete;
    GameScene &operator=(const GameScene &) = delete;
    GameScene(GameScene &&) noexcept = default;
    GameScene &operator=(GameScene &&) noexcept = default;

    void Cleanup();

    // Skybox
    void SetSkyBox(std::shared_ptr<Skybox> &skybox);
    Skybox *GetSkyBox() const;

    // Models
    const std::unordered_map<std::string, Model> &GetMapModels() const;
    void AddMapModels(const std::unordered_map<std::string, Model> &modelsMap);
    std::unordered_map<std::string, Model> &GetMapModelsMutable();

    // Objects
    const std::vector<MapObjectData> &GetMapObjects() const;
    void AddMapObjects(const std::vector<MapObjectData> &mapObjects);
    std::vector<MapObjectData> &GetMapObjectsMutable();

    // UI Elements
    const std::vector<UIElementData> &GetUIElements() const;
    void AddUIElements(const std::vector<UIElementData> &uiElements);
    std::vector<UIElementData> &GetUIElementsMutable();

    // Metadata
    const MapMetadata &GetMapMetaData() const;
    void SetMapMetaData(const MapMetadata &mapData);
    MapMetadata &GetMapMetaDataMutable();

    // Scene Type
    SceneType GetSceneType() const
    {
        return m_sceneType;
    }
    void SetSceneType(SceneType type)
    {
        m_sceneType = type;
    }

    // Textures
    const std::unordered_map<std::string, Texture2D> &GetMapTextures() const;
    void AddMapTextures(const std::unordered_map<std::string, Texture2D> &texturesMap);
    std::unordered_map<std::string, Texture2D> &GetMapTexturesMutable();

private:
    MapMetadata m_metadata;
    std::vector<MapObjectData> m_objects;
    std::vector<UIElementData> m_uiElements;
    std::unordered_map<std::string, Model> m_loadedModels;
    std::unordered_map<std::string, Texture2D> m_loadedTextures;
    std::shared_ptr<Skybox> m_skybox;
    SceneType m_sceneType = SceneType::Game; // Default to Game scene
};
} // namespace CHEngine

#endif // GAMESCENE_H
