#include "api_context.h"
#include "engine/graphics/shader_asset.h"
#include "engine/scene/project.h"
#include "rlgl.h"

namespace CHEngine
{
RendererState APIContext::s_State;

void APIContext::Init()
{
    InitShaders();
    InitSkybox();

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

void APIContext::ApplyEnvironment(const EnvironmentSettings &settings)
{
    SetDirectionalLight(settings.LightDirection, settings.LightColor);
    SetAmbientLight(settings.AmbientIntensity);
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

void APIContext::InitShaders()
{
    // For now, we manually initialize locations if shaders were loaded
    // This logic was previously in DrawCommand static helpers
    if (s_State.LightingShader)
    {
        s_State.LightDirLoc = s_State.LightingShader->GetLocation("lightDir");
        s_State.LightColorLoc = s_State.LightingShader->GetLocation("lightColor");
        s_State.AmbientLoc = s_State.LightingShader->GetLocation("ambient");
        // ... Initialize LightLocs array if needed (point lights)
    }
}

void APIContext::InitSkybox()
{
    s_State.SkyboxCube = LoadModelFromMesh(GenMeshCube(1.0f, 1.0f, 1.0f));
}
} // namespace CHEngine
