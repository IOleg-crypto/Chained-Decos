#ifndef ISCENE_MANAGER_H
#define ISCENE_MANAGER_H

#include "editor/EditorTypes.h"
#include <scene/resources/map/core/MapData.h>
#include <scene/resources/map/core/SceneLoader.h>
#include <scene/resources/map/skybox/skybox.h>
#include <string>
#include <vector>

namespace CHEngine
{
class Scene;
}

class ISceneManager
{
public:
    virtual ~ISceneManager() = default;

    // Scene Lifecycle
    virtual void ClearScene() = 0;
    virtual void SaveScene(const std::string &path = "") = 0;
    virtual void LoadScene(const std::string &path) = 0;
    virtual void RemoveObject(int index) = 0;
    virtual CHEngine::Scene *GetActiveScene() = 0;
    virtual GameScene &GetGameScene() = 0;
    virtual void RefreshUIEntities() = 0;

    // Scene State
    virtual bool IsSceneModified() const = 0;
    virtual void SetSceneModified(bool modified) = 0;
    virtual const std::string &GetCurrentMapPath() const = 0;

    // Environment/Metadata
    virtual void SetSkybox(const std::string &name) = 0;
    virtual void SetSkyboxTexture(const std::string &texturePath) = 0;
    virtual void SetSkyboxColor(Color color) = 0;
    virtual Skybox *GetSkybox() const = 0;
    virtual Color GetClearColor() const = 0;
    virtual void ApplyMetadata(const MapMetadata &metadata) = 0;

    // Spawning/Entities (High level)
    virtual void CreateDefaultObject(MapObjectType type, const std::string &modelName = "") = 0;
    virtual void LoadAndSpawnModel(const std::string &path) = 0;
};

#endif // ISCENE_MANAGER_H
