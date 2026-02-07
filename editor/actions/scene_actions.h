#ifndef CH_SCENE_ACTIONS_H
#define CH_SCENE_ACTIONS_H

#include "engine/scene/scene.h"
#include <filesystem>

namespace CHEngine
{
class SceneActions
{
public:
    static void New();
    static void Open();
    static void Open(const std::filesystem::path &path);
    static void Save();
    static void SaveAs();
    static void SetParent(Entity child, Entity parent);
};
} // namespace CHEngine

#endif // CH_SCENE_ACTIONS_H
