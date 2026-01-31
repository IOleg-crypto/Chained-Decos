#include "api_context.h"
#include "engine/graphics/shader_asset.h"
#include "engine/graphics/asset_manager.h"
#include "engine/scene/project.h"
#include "rlgl.h"

namespace CHEngine
{
RendererState APIContext::s_State;

void APIContext::Init()
{
    CH_CORE_INFO("APIContext: Initializing Renderer...");
    
    // Try to load default shaders
    s_State.LightingShader = AssetManager::Get<ShaderAsset>("engine:shaders/lighting.chshader");
    s_State.SkyboxShader = AssetManager::Get<ShaderAsset>("engine:shaders/skybox.chshader");
    s_State.PanoramaShader = AssetManager::Get<ShaderAsset>("engine:shaders/panorama.chshader");

    if (s_State.LightingShader) {
        CH_CORE_INFO("APIContext: Lighting shader loaded.");
    }
    else
    { 
        CH_CORE_ERROR("APIContext: FAILED to load Lighting shader!");
    }

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
        //CH_CORE_INFO("APIContext: Light Dir({:.2f},{:.2f},{:.2f}), Color({},{},{},{})", 
        //    direction.x, direction.y, direction.z, color.r, color.g, color.b, color.a);
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
        
        for (int i = 0; i < 8; i++)
        {
            std::string prefix = "lights[" + std::to_string(i) + "].";
            s_State.LightLocs[i].position = s_State.LightingShader->GetLocation(prefix + "position");
            s_State.LightLocs[i].color = s_State.LightingShader->GetLocation(prefix + "color");
            s_State.LightLocs[i].radius = s_State.LightingShader->GetLocation(prefix + "radius");
            s_State.LightLocs[i].radiance = s_State.LightingShader->GetLocation(prefix + "radiance");
            s_State.LightLocs[i].falloff = s_State.LightingShader->GetLocation(prefix + "falloff");
            s_State.LightLocs[i].enabled = s_State.LightingShader->GetLocation(prefix + "enabled");
        }
    }
    
    if (s_State.SkyboxShader)
    {
        s_State.SkyboxVflippedLoc = s_State.SkyboxShader->GetLocation("vflipped");
        s_State.SkyboxDoGammaLoc = s_State.SkyboxShader->GetLocation("doGamma");
        s_State.SkyboxFragGammaLoc = s_State.SkyboxShader->GetLocation("fragGamma");
        s_State.SkyboxExposureLoc = s_State.SkyboxShader->GetLocation("exposure");
        s_State.SkyboxBrightnessLoc = s_State.SkyboxShader->GetLocation("brightness");
        s_State.SkyboxContrastLoc = s_State.SkyboxShader->GetLocation("contrast");
    }

    if (s_State.PanoramaShader)
    {
        s_State.PanoDoGammaLoc = s_State.PanoramaShader->GetLocation("doGamma");
        s_State.PanoFragGammaLoc = s_State.PanoramaShader->GetLocation("fragGamma");
        s_State.PanoExposureLoc = s_State.PanoramaShader->GetLocation("exposure");
        s_State.PanoBrightnessLoc = s_State.PanoramaShader->GetLocation("brightness");
        s_State.PanoContrastLoc = s_State.PanoramaShader->GetLocation("contrast");
    }
}

void APIContext::InitSkybox()
{
    s_State.SkyboxCube = LoadModelFromMesh(GenMeshCube(1.0f, 1.0f, 1.0f));
}
} // namespace CHEngine
