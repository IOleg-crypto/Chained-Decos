#include "api_context.h"
#include "asset_manager.h"
#include "engine/scene/project.h"
#include <raymath.h>

namespace CHEngine
{
RendererState APIContext::s_State;

static void InitLights(RendererState &state)
{
    state.LightingShader = AssetManager::Get<ShaderAsset>("engine:shaders/lighting.chshader");
    if (state.LightingShader)
    {
        state.LightDirLoc = state.LightingShader->GetLocation("lightDir");
        state.LightColorLoc = state.LightingShader->GetLocation("lightColor");
        state.AmbientLoc = state.LightingShader->GetLocation("ambient");

        for (int i = 0; i < 8; i++)
        {
            std::string base = "lights[" + std::to_string(i) + "].";
            state.LightLocs[i].position = state.LightingShader->GetLocation(base + "position");
            state.LightLocs[i].color = state.LightingShader->GetLocation(base + "color");
            state.LightLocs[i].radius = state.LightingShader->GetLocation(base + "radius");
            state.LightLocs[i].radiance = state.LightingShader->GetLocation(base + "radiance");
            state.LightLocs[i].falloff = state.LightingShader->GetLocation(base + "falloff");
            state.LightLocs[i].enabled = state.LightingShader->GetLocation(base + "enabled");
        }
    }
}

static void InitSkybox(RendererState &state)
{
    state.SkyboxShader = AssetManager::Get<ShaderAsset>("engine:shaders/skybox.chshader");
    state.SkyboxCube = LoadModelFromMesh(GenMeshCube(1.0f, 1.0f, 1.0f));

    if (state.SkyboxShader)
    {
        auto &shader = state.SkyboxShader->GetShader();
        state.SkyboxCube.materials[0].shader = shader;
        state.SkyboxVflippedLoc = state.SkyboxShader->GetLocation("vflipped");
        state.SkyboxDoGammaLoc = state.SkyboxShader->GetLocation("doGamma");
        state.SkyboxFragGammaLoc = state.SkyboxShader->GetLocation("fragGamma");
        state.SkyboxExposureLoc = state.SkyboxShader->GetLocation("exposure");
        state.SkyboxBrightnessLoc = state.SkyboxShader->GetLocation("brightness");
        state.SkyboxContrastLoc = state.SkyboxShader->GetLocation("contrast");

        int environmentMapLoc = state.SkyboxShader->GetLocation("environmentMap");
        if (environmentMapLoc >= 0)
        {
            shader.locs[SHADER_LOC_MAP_CUBEMAP] = environmentMapLoc;
            int envMapValue[1] = {MATERIAL_MAP_CUBEMAP};
            SetShaderValue(shader, environmentMapLoc, envMapValue, SHADER_UNIFORM_INT);
        }
    }
}

static void InitPanorama(RendererState &state)
{
    state.PanoramaShader = AssetManager::Get<ShaderAsset>("engine:shaders/panorama.chshader");
    if (state.PanoramaShader)
    {
        state.PanoDoGammaLoc = state.PanoramaShader->GetLocation("doGamma");
        state.PanoFragGammaLoc = state.PanoramaShader->GetLocation("fragGamma");
        state.PanoExposureLoc = state.PanoramaShader->GetLocation("exposure");
        state.PanoBrightnessLoc = state.PanoramaShader->GetLocation("brightness");
        state.PanoContrastLoc = state.PanoramaShader->GetLocation("contrast");
    }
}

void APIContext::Init()
{
    InitLights(s_State);
    InitSkybox(s_State);
    InitPanorama(s_State);

    float ambient = 0.3f;
    if (Project::GetActive())
        ambient = Project::GetActive()->GetConfig().Render.AmbientIntensity;

    SetDirectionalLight({-1.0f, -1.0f, -1.0f}, WHITE);
    SetAmbientLight(ambient);
}

void APIContext::Shutdown()
{
    s_State.LightingShader = nullptr;
    s_State.SkyboxShader = nullptr;
    s_State.PanoramaShader = nullptr;
    UnloadModel(s_State.SkyboxCube);
}

void APIContext::SetDirectionalLight(Vector3 direction, Color color)
{
    s_State.CurrentLightDir = direction;
    s_State.CurrentLightColor = color;

    if (s_State.LightingShader)
    {
        float dir[3] = {direction.x, direction.y, direction.z};
        float col[4] = {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f};
        SetShaderValue(s_State.LightingShader->GetShader(), s_State.LightDirLoc, dir,
                       SHADER_UNIFORM_VEC3);
        SetShaderValue(s_State.LightingShader->GetShader(), s_State.LightColorLoc, col,
                       SHADER_UNIFORM_VEC4);
    }
}

void APIContext::SetAmbientLight(float intensity)
{
    s_State.CurrentAmbientIntensity = intensity;
    if (s_State.LightingShader)
    {
        SetShaderValue(s_State.LightingShader->GetShader(), s_State.AmbientLoc, &intensity,
                       SHADER_UNIFORM_FLOAT);
    }
}

void APIContext::ApplyEnvironment(const EnvironmentSettings &settings)
{
    SetDirectionalLight(settings.LightDirection, settings.LightColor);
    SetAmbientLight(settings.AmbientIntensity);
}
} // namespace CHEngine
