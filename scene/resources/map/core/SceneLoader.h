#ifndef SCENELOADER_H
#define SCENELOADER_H

#include "MapData.h"
#include <memory>
#include <nlohmann/json.hpp>
#include <raylib.h>
#include <string>
#include <unordered_map>
#include <vector>

#include "scene/resources/map/skybox/skybox.h"

// ============================================================================
// Data Structures
// ============================================================================

struct ModelInfo
{
    std::string name;
    std::string path;
    std::string extension;
    std::string category;    // Type of model (Player, Building, Environment, etc.)
    std::string description; // Human-readable description
    bool hasAnimations;
    bool hasCollision;
    Vector3 defaultScale;
};

// ============================================================================
// GameScene Class
// ============================================================================

class GameScene
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

    // Skybox - updated methods
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

private:
    MapMetadata m_metadata;
    std::vector<MapObjectData> m_objects;
    std::vector<UIElementData> m_uiElements;
    std::unordered_map<std::string, Model> m_loadedModels;
    std::shared_ptr<Skybox> m_skybox;
};

// ============================================================================
// SceneLoader Class
// ============================================================================

class SceneLoader
{
public:
    SceneLoader() = default;
    ~SceneLoader() = default;

    // Non-copyable, movable
    SceneLoader(const SceneLoader &) = delete;
    SceneLoader &operator=(const SceneLoader &) = delete;
    SceneLoader(SceneLoader &&) noexcept = default;
    SceneLoader &operator=(SceneLoader &&) noexcept = default;

    GameScene LoadScene(const std::string &path);
    bool SaveScene(const GameScene &map, const std::string &path);
    std::vector<ModelInfo> LoadModelsFromDirectory(const std::string &directory);
    bool SaveModelConfig(const std::vector<ModelInfo> &models, const std::string &path);

    // Load all scenes from a directory
    std::vector<GameScene> LoadAllScenesFromDirectory(const std::string &directory);

    // Get scene names from directory
    std::vector<std::string> GetSceneNamesFromDirectory(const std::string &directory);

    // Skybox operations
    void LoadSkyboxForScene(GameScene &map);

private:
    bool SaveSceneToFile(const GameScene &map, const std::string &path);
};

// ============================================================================
// Utility Functions
// ============================================================================

MapObjectData CreateMapObjectFromType(MapObjectType type, const Vector3 &position,
                                      const Vector3 &scale, const Color &color);

#endif // SCENELOADER_H
