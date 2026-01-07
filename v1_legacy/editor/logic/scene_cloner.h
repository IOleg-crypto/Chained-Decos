#ifndef CD_EDITOR_LOGIC_SCENE_CLONER_H
#define CD_EDITOR_LOGIC_SCENE_CLONER_H

#include "engine/scene/core/scene.h"
#include "engine/scene/core/scene_serializer.h"
#include <memory>
#include <string>

namespace CHEngine
{
class SceneCloner
{
public:
    static bool SaveTemp(std::shared_ptr<Scene> scene, const std::string &tempPath)
    {
        if (!scene)
            return false;
        ECSSceneSerializer serializer(scene);
        serializer.Serialize(tempPath);
        return true;
    }

    static std::string GetTempPath()
    {
        return std::string(PROJECT_ROOT_DIR) + "/temp_runtime_scene.chscene";
    }
};
} // namespace CHEngine

#endif // CD_EDITOR_LOGIC_SCENE_CLONER_H
