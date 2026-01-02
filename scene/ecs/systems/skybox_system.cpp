#include "skybox_system.h"
#include "core/log.h"
#include "scene/ecs/components/skybox_component.h"
#include "scene/resources/texture/texture_service.h"
#include <raymath.h>
#include <rlgl.h>

namespace CHEngine
{

::Shader SkyboxSystem::s_SkyboxShader = {0};
::Mesh SkyboxSystem::s_SkyboxCube = {0};
::Material SkyboxSystem::s_SkyboxMaterial = {0};
int SkyboxSystem::s_VflippedLoc = -1;
int SkyboxSystem::s_DoGammaLoc = -1;
int SkyboxSystem::s_FragGammaLoc = -1;
int SkyboxSystem::s_ExposureLoc = -1;
int SkyboxSystem::s_BrightnessLoc = -1;
int SkyboxSystem::s_ContrastLoc = -1;
bool SkyboxSystem::s_Initialized = false;

void SkyboxSystem::Init()
{
    if (s_Initialized)
        return;

    CD_CORE_INFO("Initializing SkyboxSystem...");

    // Load skybox shader
    s_SkyboxShader = LoadShader("resources/shaders/skybox.vs", "resources/shaders/skybox.fs");

    s_VflippedLoc = GetShaderLocation(s_SkyboxShader, "vflipped");
    s_DoGammaLoc = GetShaderLocation(s_SkyboxShader, "doGamma");
    s_FragGammaLoc = GetShaderLocation(s_SkyboxShader, "fragGamma");
    s_ExposureLoc = GetShaderLocation(s_SkyboxShader, "exposure");
    s_BrightnessLoc = GetShaderLocation(s_SkyboxShader, "brightness");
    s_ContrastLoc = GetShaderLocation(s_SkyboxShader, "contrast");

    // Set default values for shader
    int vflipped = 0;
    float exposure = 1.0f;
    float brightness = 0.0f;
    float contrast = 1.0f;
    SetShaderValue(s_SkyboxShader, s_VflippedLoc, &vflipped, SHADER_UNIFORM_INT);
    SetShaderValue(s_SkyboxShader, s_ExposureLoc, &exposure, SHADER_UNIFORM_FLOAT);
    SetShaderValue(s_SkyboxShader, s_BrightnessLoc, &brightness, SHADER_UNIFORM_FLOAT);
    SetShaderValue(s_SkyboxShader, s_ContrastLoc, &contrast, SHADER_UNIFORM_FLOAT);

    // Create cube mesh for skybox
    s_SkyboxCube = GenMeshCube(1.0f, 1.0f, 1.0f);
    s_SkyboxMaterial = LoadMaterialDefault();
    s_SkyboxMaterial.shader = s_SkyboxShader;

    s_Initialized = true;
    CD_CORE_INFO("SkyboxSystem initialized.");
}

void SkyboxSystem::Shutdown()
{
    if (!s_Initialized)
        return;

    CD_CORE_INFO("Shutting down SkyboxSystem...");

    UnloadShader(s_SkyboxShader);
    UnloadMesh(s_SkyboxCube);
    // Note: s_SkyboxMaterial doesn't need explicit unloading of standard parts

    s_Initialized = false;
}

void SkyboxSystem::Render(entt::registry &registry)
{
    if (!s_Initialized)
        Init();

    auto view = registry.view<SkyboxComponent>();
    if (view.empty())
        return;

    // We only support one active skybox at a time
    auto entity = view.front();
    auto &skybox = view.get<SkyboxComponent>(entity);

    if (skybox.TexturePath.empty())
        return;

    // Load texture if needed (using TextureService or similar)
    // For now, let's assume we need to load a cubemap.
    // Raylib LoadTextureCubemap loads from a single image (panorama)

    // Optimization: Cache the cubemap texture in the component or a local map
    // For simplicity, let's just make sure we have a texture.
    // In a real scenario, the SkyboxComponent would hold the Texture2D/TextureCubemap handle.

    // Update shader uniforms from component
    int doGamma = skybox.GammaEnabled ? 1 : 0;
    SetShaderValue(s_SkyboxShader, s_DoGammaLoc, &doGamma, SHADER_UNIFORM_INT);
    SetShaderValue(s_SkyboxShader, s_FragGammaLoc, &skybox.GammaValue, SHADER_UNIFORM_FLOAT);

    // Hack: For now, if we don't have a texture, we can't render anything useful.
    // This system needs to be integrated with AssetManager/TextureService correctly.

    // Disable depth mask for skybox rendering
    rlDisableDepthMask();
    rlDisableBackfaceCulling();

    // Render the cube (GenMeshCube creates a 1.0 unit cube)
    // Raylib's DrawMesh handles the draw call
    DrawMesh(s_SkyboxCube, s_SkyboxMaterial, MatrixIdentity());

    rlEnableBackfaceCulling();
    rlEnableDepthMask();
}

} // namespace CHEngine
