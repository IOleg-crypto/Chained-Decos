#ifndef SCENE_CLONER_H
#define SCENE_CLONER_H

#include "scene/resources/map/SceneLoader.h"
#include <string>

namespace CHEngine
{
class SceneCloner
{
public:
    static bool SaveTemp(const GameScene &scene, const std::string &tempPath)
    {
        SceneLoader loader;
        return loader.SaveScene(scene, tempPath);
    }

    static std::string GetTempPath()
    {
        // Simple temp path relative to project root
        return std::string(PROJECT_ROOT_DIR) + "/temp_runtime_scene.json";
    }
};
} // namespace CHEngine

#endif // SCENE_CLONER_H
