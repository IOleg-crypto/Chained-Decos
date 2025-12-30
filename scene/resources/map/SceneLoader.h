#ifndef SCENELOADER_H
#define SCENELOADER_H

#include "MapData.h"
#include <memory>
#include <nlohmann/json.hpp>
#include <raylib.h>
#include <string>
#include <unordered_map>
#include <vector>

#include "GameScene.h"

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

    bool SaveSceneToFile(const GameScene &map, const std::string &path);

private:
};

// ============================================================================
// Utility Functions
// ============================================================================

MapObjectData CreateMapObjectFromType(MapObjectType type, const Vector3 &position,
                                      const Vector3 &scale, const Color &color);

#endif // SCENELOADER_H
