#ifndef CD_SCENE_ECS_SYSTEMS_SKYBOX_SYSTEM_H
#define CD_SCENE_ECS_SYSTEMS_SKYBOX_SYSTEM_H

#include <entt/entt.hpp>
#include <raylib.h>

namespace CHEngine
{

class SkyboxSystem
{
public:
    static void Init();
    static void Shutdown();

    static void Render(entt::registry &registry);

private:
    static ::Shader s_SkyboxShader;
    static ::Mesh s_SkyboxCube;
    static ::Material s_SkyboxMaterial;

    // Shader locations
    static int s_VflippedLoc;
    static int s_DoGammaLoc;
    static int s_FragGammaLoc;
    static int s_ExposureLoc;
    static int s_BrightnessLoc;
    static int s_ContrastLoc;

    static bool s_Initialized;
};

} // namespace CHEngine

#endif // CD_SCENE_ECS_SYSTEMS_SKYBOX_SYSTEM_H
