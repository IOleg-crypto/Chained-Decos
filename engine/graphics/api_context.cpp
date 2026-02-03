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
    CH_CORE_INFO("APIContext: AssetManager RootPath = {}", AssetManager::GetRootPath().string());
    
    // Debug: show resolved paths
    std::string lightingPath = AssetManager::ResolvePath("engine/resources/shaders/lighting.chshader");
    std::string skyboxPath = AssetManager::ResolvePath("engine/resources/shaders/skybox.chshader");
    std::string panoramaPath = AssetManager::ResolvePath("engine/resources/shaders/panorama.chshader");
    CH_CORE_INFO("APIContext: Resolved lighting shader = {}", lightingPath);
    CH_CORE_INFO("APIContext: Resolved skybox shader = {}", skyboxPath);
    CH_CORE_INFO("APIContext: Resolved panorama shader = {}", panoramaPath);
    
    // Try to load default shaders
    s_State.LightingShader = AssetManager::Get<ShaderAsset>("engine/resources/shaders/lighting.chshader");
    s_State.SkyboxShader = AssetManager::Get<ShaderAsset>("engine/resources/shaders/skybox.chshader");
    s_State.PanoramaShader = AssetManager::Get<ShaderAsset>("engine/resources/shaders/panorama.chshader");

    if (s_State.LightingShader) {
        CH_CORE_INFO("APIContext: Lighting shader loaded (state={}).", (int)s_State.LightingShader->GetState());
    }
    else
    { 
        CH_CORE_ERROR("APIContext: FAILED to load Lighting shader!");
    }

    if (s_State.SkyboxShader) {
        CH_CORE_INFO("APIContext: Skybox shader loaded (state={}).", (int)s_State.SkyboxShader->GetState());
    }
    else
    { 
        CH_CORE_ERROR("APIContext: FAILED to load Skybox shader!");
    }

    if (s_State.PanoramaShader) {
        CH_CORE_INFO("APIContext: Panorama shader loaded (state={}).", (int)s_State.PanoramaShader->GetState());
    }
    else
    { 
        CH_CORE_ERROR("APIContext: FAILED to load Panorama shader!");
    }

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
        s_State.LightingShader->SetVec3("lightDir", direction);
        s_State.LightingShader->SetColor("lightColor", color);
    }
}

void APIContext::SetAmbientLight(float intensity)
{
    s_State.CurrentAmbientIntensity = intensity;
    if (s_State.LightingShader)
    {
        s_State.LightingShader->SetFloat("ambient", intensity);
    }
}


void APIContext::InitSkybox()
{
    s_State.SkyboxCube = LoadModelFromMesh(GenMeshCube(1.0f, 1.0f, 1.0f));
}
} // namespace CHEngine
