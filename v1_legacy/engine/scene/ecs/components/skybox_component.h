#ifndef CD_SCENE_ECS_COMPONENTS_SKYBOX_COMPONENT_H
#define CD_SCENE_ECS_COMPONENTS_SKYBOX_COMPONENT_H

#include <string>

namespace CHEngine
{

/**
 * SkyboxComponent - Defines the environment map for a scene.
 * Usually attached to a "Global Settings" entity.
 */
struct SkyboxComponent
{
    std::string TexturePath;
    float GammaValue = 2.2f;
    bool GammaEnabled = false;

    SkyboxComponent() = default;
    SkyboxComponent(const std::string &path) : TexturePath(path)
    {
    }
};

} // namespace CHEngine

#endif // CD_SCENE_ECS_COMPONENTS_SKYBOX_COMPONENT_H
