#include "engine/graphics/pipeline/renderer.h"
#include "engine/core/application.h"
#include "engine/core/log.h"
#include "engine/core/profiler.h"
#include "engine/graphics/pipeline/renderer2d.h"
#include "engine/graphics/pipeline/ui_renderer.h"
#include "engine/graphics/assets/shader_asset.h"
#include "engine/core/assets/asset_manager.h"
#include "engine/graphics/assets/texture_asset.h"
#include "rlgl.h"
#include <algorithm>
#include <vector>

#include "render_command.h"

namespace CHEngine
{

Renderer* Renderer::s_Instance = nullptr;

bool Renderer::IsInitialized()
{
    return s_Instance != nullptr && s_Instance->m_Data != nullptr;
}

Renderer& Renderer::Get()
{
    CH_CORE_ASSERT(s_Instance, "Renderer not initialized!");
    return *s_Instance;
}

void Renderer::Init()
{
    if (!s_Instance)
        s_Instance = new Renderer();
    s_Instance->InternalInit();
}

void Renderer::LoadEngineResources()
{
    auto& shaders = Get().GetShaderLibrary();
    
    auto loadShader = [&](const std::string& name, const std::string& path) {
        shaders.Load(name, path);
        if (shaders.Exists(name)) {
            auto shader = shaders.Get(name);
            CH_CORE_INFO("[Renderer] Shader '{}' loaded OK (ID={}) from '{}'.", name, shader->GetShader().id, path);
        } else {
            CH_CORE_ERROR("[Renderer] FAILED to load shader '{}' from '{}'! Viewport may be black.", name, path);
        }
    };

    loadShader("Lighting",       "resources/shaders/lighting.chshader");
    loadShader("Skybox",         "resources/shaders/skybox.chshader");
    loadShader("Unlit",          "resources/shaders/unlit.chshader");
    loadShader("CubemapGen",     "resources/shaders/cubemap.chshader");
    loadShader("SkyboxCubemap",  "resources/shaders/skybox_cubemap.chshader");
    loadShader("PostProcess",    "resources/shaders/post_process.chshader");
    
    CH_CORE_INFO("[Renderer] LoadEngineResources done. {} shader(s) loaded.", shaders.GetNames().size());
}

void Renderer::InternalInit()
{
    CH_CORE_INFO("Initializing Render System (Low-Level)...");
    
    if (Application::Get().GetSpecification().Headless)
    {
        return;
    }

    RenderCommand::Initialize();

    // Initialize SSBO for lights
    m_Data->Lighting.LightSSBO =
        rlLoadShaderBuffer(sizeof(RenderLight) * LightingData::MaxLights, nullptr, RL_DYNAMIC_DRAW);
    m_Data->Lighting.LightsDirty = true;

    InitializeSkybox();
    
    // Always load engine resources after initialization
    LoadEngineResources();
}

void Renderer::Shutdown()
{
    if (s_Instance)
    {
        s_Instance->InternalShutdown();
        delete s_Instance;
        s_Instance = nullptr;
    }
}

void Renderer::InternalShutdown()
{
    CH_CORE_INFO("Shutting down Render System...");

    if (Application::Get().GetSpecification().Headless) return;

    CleanupSkybox();
    
    if (m_Data->Lighting.LightSSBO > 0)
    {
        rlUnloadShaderBuffer(m_Data->Lighting.LightSSBO);
    }
}

Renderer::Renderer()
{
    s_Instance = this;
    m_Data = std::make_unique<RendererData>();
    m_Data->Shaders = std::make_unique<ShaderLibrary>();
}

Renderer::~Renderer()
{
    InternalShutdown();
}

void Renderer::BeginScene(const Camera3D& camera)
{
    m_Data->CurrentCameraPosition = camera.position;

    // Update light SSBO once per frame
    if (m_Data->Lighting.LightsDirty)
    {
        rlUpdateShaderBuffer(m_Data->Lighting.LightSSBO, m_Data->Lighting.Lights,
                             sizeof(RenderLight) * LightingData::MaxLights, 0);
        m_Data->Lighting.LightsDirty = false;
    }

    // Bind lighting uniforms to the default lighting shader if it exists
    auto lightingShader = m_Data->Shaders->Exists("Lighting") ? m_Data->Shaders->Get("Lighting") : nullptr;
    if (lightingShader)
    {
        Shader shader = lightingShader->GetShader();
        rlEnableShader(shader.id);
        
        float time = m_Data->Time;
        float diagMode = m_Data->DiagnosticMode;
        int lightCount = m_Data->LightCount;
        float ambient = m_Data->Lighting.CurrentLighting.Ambient;
        float exposure = m_Data->Lighting.CurrentLighting.Exposure;
        float gamma = m_Data->Lighting.CurrentLighting.Gamma;

        SetShaderValue(shader, GetShaderLocation(shader, "viewPos"), &camera.position, SHADER_UNIFORM_VEC3);
        SetShaderValue(shader, GetShaderLocation(shader, "uTime"), &time, SHADER_UNIFORM_FLOAT);
        SetShaderValue(shader, GetShaderLocation(shader, "uMode"), &diagMode, SHADER_UNIFORM_FLOAT);
        SetShaderValue(shader, GetShaderLocation(shader, "lightDir"), &m_Data->Lighting.CurrentLighting.Direction, SHADER_UNIFORM_VEC3);
        
        Vector4 lightColor = ColorNormalize(m_Data->Lighting.CurrentLighting.LightColor);
        SetShaderValue(shader, GetShaderLocation(shader, "lightColor"), &lightColor, SHADER_UNIFORM_VEC4);
        
        SetShaderValue(shader, GetShaderLocation(shader, "ambient"), &ambient, SHADER_UNIFORM_FLOAT);
        
        // Pass Sky Ambient Color (Simplest IBL)
        // In a full implementation, this would be the Irradiance Map.
        // For now, we use a color that represents the sky's influence.
        Vector4 skyColor = ColorNormalize(m_Data->Lighting.CurrentLighting.LightColor);
        skyColor.w = ambient * 0.35f; // Reduced intensity for sky ambient
        SetShaderValue(shader, GetShaderLocation(shader, "skyAmbientColor"), &skyColor, SHADER_UNIFORM_VEC4);

        SetShaderValue(shader, GetShaderLocation(shader, "uLightCount"), &lightCount, SHADER_UNIFORM_INT);
        SetShaderValue(shader, GetShaderLocation(shader, "uExposure"), &exposure, SHADER_UNIFORM_FLOAT);
        SetShaderValue(shader, GetShaderLocation(shader, "uGamma"), &gamma, SHADER_UNIFORM_FLOAT);
        
        ApplyFogUniforms(shader);
        rlBindShaderBuffer(m_Data->Lighting.LightSSBO, 0);
        m_Data->CurrentShaderId = shader.id;

        // Diagnostic: log uniform values on first frame
        CH_CORE_WARN_ONCE("[Renderer] Lighting uniforms - ambient={:.3f}, lightDir=({:.2f},{:.2f},{:.2f}), lightColor=({},{},{},{}), camPos=({:.1f},{:.1f},{:.1f})",
            ambient,
            m_Data->Lighting.CurrentLighting.Direction.x,
            m_Data->Lighting.CurrentLighting.Direction.y,
            m_Data->Lighting.CurrentLighting.Direction.z,
            m_Data->Lighting.CurrentLighting.LightColor.r,
            m_Data->Lighting.CurrentLighting.LightColor.g,
            m_Data->Lighting.CurrentLighting.LightColor.b,
            m_Data->Lighting.CurrentLighting.LightColor.a,
            camera.position.x, camera.position.y, camera.position.z);
    }

    BeginMode3D(camera);

    // Store matrices for Post-Processing (Inverse View-Proj)
    m_Data->CurrentView = rlGetMatrixModelview();
    m_Data->CurrentProj = rlGetMatrixProjection();
}

void Renderer::EndScene()
{
    m_Data->CurrentShaderId = 0;
    EndMode3D();
}

void Renderer::Clear(Color color)
{
    RenderCommand::Clear(color);
}

void Renderer::SetViewport(int x, int y, int width, int height)
{
    RenderCommand::SetViewport(x, y, width, height);
}

void Renderer::DrawMesh(const Mesh& mesh, const Material& material, const Matrix& transform)
{
    ::DrawMesh(mesh, material, transform);
}

void Renderer::DrawMeshInstanced(const Mesh& mesh, const Material& material, const std::vector<Matrix>& transforms)
{
    if (transforms.empty()) return;
    ::DrawMeshInstanced(mesh, material, transforms.data(), (int)transforms.size());
}

void Renderer::DrawLine(Vector3 start, Vector3 end, Color color)
{
    ::DrawLine3D(start, end, color);
}
void Renderer::DrawMeshWire(const Mesh& mesh, Color color, const Matrix& transform)
{
    rlPushMatrix();
    rlMultMatrixf(MatrixToFloat(transform));
    rlEnableWireMode();
    // Reusing standard material for color
    Material mat = LoadMaterialDefault();
    mat.maps[MATERIAL_MAP_ALBEDO].color = color;
    ::DrawMesh(mesh, mat, MatrixIdentity());
    rlDisableWireMode();
    rlPopMatrix();
}

void Renderer::DrawGrid(int slices, float spacing)
{
    ::DrawGrid(slices, spacing);
}

void Renderer::DrawSkybox(const SkyboxSettings& settings, const Camera3D& camera)
{
    if (settings.TexturePath.empty())
        return;

    auto texture = AssetManager::Get().Get<TextureAsset>(settings.TexturePath);
    if (!texture || texture->GetState() != AssetState::Ready)
        return;

    bool isCubemap = (settings.Mode == 2);
    auto shaderAsset = isCubemap && m_Data->Shaders->Exists("SkyboxCubemap") ? 
                       m_Data->Shaders->Get("SkyboxCubemap") : 
                       m_Data->Shaders->Get("Skybox");
    Shader shader = shaderAsset->GetShader();

    // 1. Generate Cubemap (Mode 2)
    if (isCubemap)
    {
        Texture2D tex = texture->GetTexture();
        if (m_Data->Skybox.CachedCubemap.id == 0 || 
            m_Data->Skybox.CachedCubemapPath != settings.TexturePath ||
            m_Data->Skybox.SourceTextureId != tex.id)
        {
            if (m_Data->Shaders->Exists("CubemapGen"))
            {
                if (m_Data->Skybox.CachedCubemap.id != 0) UnloadTexture(m_Data->Skybox.CachedCubemap);

                auto genShader = m_Data->Shaders->Get("CubemapGen")->GetShader();
                int isHDR = texture->IsHDR() ? 1 : 0;
                int targetFmt = isHDR ? RL_PIXELFORMAT_UNCOMPRESSED_R16G16B16A16 : RL_PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
                
                m_Data->Skybox.CachedCubemap = GenTextureCubemap(genShader, tex, 1024, targetFmt);
                m_Data->Skybox.CachedCubemapPath = settings.TexturePath;
                m_Data->Skybox.SourceTextureId = tex.id;
                
                rlCubemapParameters(m_Data->Skybox.CachedCubemap.id, RL_TEXTURE_MAG_FILTER, RL_TEXTURE_FILTER_LINEAR);
                rlCubemapParameters(m_Data->Skybox.CachedCubemap.id, RL_TEXTURE_MIN_FILTER, RL_TEXTURE_FILTER_LINEAR);
                rlCubemapParameters(m_Data->Skybox.CachedCubemap.id, RL_TEXTURE_WRAP_S, RL_TEXTURE_WRAP_CLAMP);
                rlCubemapParameters(m_Data->Skybox.CachedCubemap.id, RL_TEXTURE_WRAP_T, RL_TEXTURE_WRAP_CLAMP);
                rlCubemapParameters(m_Data->Skybox.CachedCubemap.id, 0x8072, RL_TEXTURE_WRAP_CLAMP); // GL_TEXTURE_WRAP_R
            }
        }
    }

    // 2. Prepare Render State
    rlDrawRenderBatchActive();
    RenderCommand::SetDepthFunc(RendererAPI::DepthFunc::LEqual);
    rlDisableBackfaceCulling();
    rlDisableDepthMask();

    // 3. Setup Uniforms
    rlEnableShader(shader.id);
    
    int projLoc = GetShaderLocation(shader, "projection");
    int viewLoc = GetShaderLocation(shader, "view");
    if (projLoc != -1) SetShaderValueMatrix(shader, projLoc, m_Data->CurrentProj);
    if (viewLoc != -1) SetShaderValueMatrix(shader, viewLoc, m_Data->CurrentView);

    float exposure = settings.Exposure, bright = settings.Brightness, contrast = settings.Contrast;
    SetShaderValue(shader, GetShaderLocation(shader, "exposure"), &exposure, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shader, GetShaderLocation(shader, "brightness"), &bright, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shader, GetShaderLocation(shader, "contrast"), &contrast, SHADER_UNIFORM_FLOAT);
    
    int isHDR = texture->IsHDR() ? 1 : 0;
    SetShaderValue(shader, GetShaderLocation(shader, "isHDR"), &isHDR, SHADER_UNIFORM_INT);
    
    int skyboxMode = settings.Mode;
    SetShaderValue(shader, GetShaderLocation(shader, "skyboxMode"), &skyboxMode, SHADER_UNIFORM_INT);
    
    int vflipped = 1; // Default to flipping V for proper orientation
    SetShaderValue(shader, GetShaderLocation(shader, "vflipped"), &vflipped, SHADER_UNIFORM_INT);

    ApplyFogUniforms(shader);

    // 4. Bind Textures and Draw
    m_Data->Skybox.SkyboxCube.materials[0].shader = shader;
    
    if (isCubemap)
    {
        m_Data->Skybox.SkyboxMaterial.maps[MATERIAL_MAP_ALBEDO].texture.id = 0;
        int slot = 0;
        SetShaderValue(shader, GetShaderLocation(shader, "environmentMap"), &slot, SHADER_UNIFORM_INT);
        
        rlActiveTextureSlot(0);
        rlEnableTextureCubemap(m_Data->Skybox.CachedCubemap.id);
    }
    else
    {
        m_Data->Skybox.SkyboxCube.materials[0].maps[MATERIAL_MAP_ALBEDO].texture = texture->GetTexture();
    }
    
    // Draw at 0,0,0. Shader handles view rotation and depth trick
    ::DrawModel(m_Data->Skybox.SkyboxCube, Vector3{0, 0, 0}, 1.0f, WHITE);

    // 5. Restore Render State
    rlDrawRenderBatchActive();
    RenderCommand::SetDepthFunc(RendererAPI::DepthFunc::Less);
    rlEnableBackfaceCulling();
    rlEnableDepthMask();
}

void Renderer::DrawBillboard(const Camera3D& camera, Texture2D texture, Vector3 position, float size, Color tint)
{
    ::DrawBillboard(camera, texture, position, size, tint);
}

void Renderer::DrawCubeWires(const Matrix& transform, Vector3 size, Color color)
{
    rlPushMatrix();
    rlMultMatrixf(MatrixToFloat(transform));
    ::DrawCubeWires({0}, size.x, size.y, size.z, color);
    rlPopMatrix();
}

void Renderer::DrawCapsuleWires(const Matrix& transform, float radius, float height, Color color)
{
    rlPushMatrix();
    rlMultMatrixf(MatrixToFloat(transform));
    ::DrawCapsuleWires({0, -height*0.5f, 0}, {0, height*0.5f, 0}, radius, 8, 8, color);
    rlPopMatrix();
}

void Renderer::DrawSphereWires(const Matrix& transform, float radius, Color color)
{
    rlPushMatrix();
    rlMultMatrixf(MatrixToFloat(transform));
    ::DrawSphereWires({0}, radius, 16, 16, color);
    rlPopMatrix();
}

void Renderer::ApplyPostProcessing(RenderTexture2D screenTexture, const Camera3D& camera)
{
    if (m_Data->Shaders->Exists("PostProcess"))
    {
        auto shaderAsset = m_Data->Shaders->Get("PostProcess");
        Shader shader = shaderAsset->GetShader();

        BeginShaderMode(shader);
        
        // 1. Set transformation uniforms
        Matrix invViewProj = MatrixInvert(MatrixMultiply(m_Data->CurrentProj, m_Data->CurrentView));
        SetShaderValueMatrix(shader, GetShaderLocation(shader, "matInverseViewProj"), invViewProj);
        SetShaderValue(shader, GetShaderLocation(shader, "viewPos"), &camera.position, SHADER_UNIFORM_VEC3);
        
        float time = m_Data->Time;
        SetShaderValue(shader, GetShaderLocation(shader, "uTime"), &time, SHADER_UNIFORM_FLOAT);

        // 2. Set ToneMapping/Gamma uniforms
        float exposure = m_Data->Lighting.CurrentLighting.Exposure;
        float gamma = m_Data->Lighting.CurrentLighting.Gamma;
        SetShaderValue(shader, GetShaderLocation(shader, "uExposure"), &exposure, SHADER_UNIFORM_FLOAT);
        SetShaderValue(shader, GetShaderLocation(shader, "uGamma"), &gamma, SHADER_UNIFORM_FLOAT);

        // 3. Set Fog uniforms
        ApplyFogUniforms(shader);

        // 4. Bind Depth Texture to slot 1 (texture1 in shader)
        rlActiveTextureSlot(1);
        rlEnableTexture(screenTexture.depth.id);
        rlActiveTextureSlot(0);

        // 5. Draw the screen texture (texture0 is bound automatically by DrawTextureRec/BeginShaderMode)
        ::DrawTextureRec(screenTexture.texture, { 0, 0, (float)screenTexture.texture.width, (float)-screenTexture.texture.height }, { 0, 0 }, WHITE);
        
        // Unbind depth texture
        rlActiveTextureSlot(1);
        rlDisableTexture();
        rlActiveTextureSlot(0);

        EndShaderMode();
    }
    else
    {
        // Fallback: blit the HDR texture directly without any post-processing.
        // This ensures the viewport is never black due to a missing PostProcess shader.
        CH_CORE_WARN_ONCE("Renderer: PostProcess shader not found — using direct blit fallback.");
        ::DrawTextureRec(screenTexture.texture, { 0, 0, (float)screenTexture.texture.width, (float)-screenTexture.texture.height }, { 0, 0 }, WHITE);
    }
}

void Renderer::SetLight(int index, const RenderLight& light)
{
    if (index >= 0 && index < LightingData::MaxLights)
    {
        m_Data->Lighting.Lights[index] = light;
        m_Data->Lighting.LightsDirty = true;
    }
}

void Renderer::SetLightCount(int count)
{
    m_Data->LightCount = count;
}

void Renderer::ClearLights()
{
    for (int i = 0; i < LightingData::MaxLights; i++)
    {
        m_Data->Lighting.Lights[i].enabled = 0;
    }
    m_Data->LightCount = 0;
    m_Data->Lighting.LightsDirty = true;
}

void Renderer::ApplyEnvironment(const EnvironmentSettings& settings)
{
    m_Data->Lighting.CurrentLighting = settings.Lighting;
    m_Data->Lighting.CurrentFog = settings.Fog;
    m_Data->CurrentEnv = settings;
}

void Renderer::SetMainLight(const LightingSettings& settings)
{
    m_Data->Lighting.CurrentLighting = settings;
}

void Renderer::SetDiagnosticMode(float mode)
{
    m_Data->DiagnosticMode = mode;
}

void Renderer::UpdateTime(Timestep time)
{
    m_Data->Time = time;
}

void Renderer::ApplyFogUniforms(Shader shader)
{
    const auto& fog = m_Data->Lighting.CurrentFog;
    int enabled = fog.Enabled ? 1 : 0;
    int mode = (int)fog.Mode;
    Vector4 color = ColorNormalize(fog.FogColor);

    SetShaderValue(shader, GetShaderLocation(shader, "fogEnabled"), &enabled, SHADER_UNIFORM_INT);
    SetShaderValue(shader, GetShaderLocation(shader, "fogColor"), &color, SHADER_UNIFORM_VEC4);
    SetShaderValue(shader, GetShaderLocation(shader, "fogDensity"), &fog.Density, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shader, GetShaderLocation(shader, "fogStart"), &fog.Start, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shader, GetShaderLocation(shader, "fogEnd"), &fog.End, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shader, GetShaderLocation(shader, "fogMode"), &mode, SHADER_UNIFORM_INT);
}

void Renderer::InitializeSkybox()
{
    Mesh cube = GenMeshCube(1.0f, 1.0f, 1.0f);
    m_Data->Skybox.SkyboxCube = LoadModelFromMesh(cube);
    m_Data->Skybox.SkyboxMaterial = LoadMaterialDefault();
}

TextureCubemap Renderer::GenTextureCubemap(Shader shader, Texture2D panorama, int size, int format)
{
    TextureCubemap cubemap = { 0 };
    if (shader.id == 0 || panorama.id == 0) return cubemap;

    if (format == 0)
        format = RL_PIXELFORMAT_UNCOMPRESSED_R16G16B16A16;

    unsigned int last_fbo = rlGetActiveFramebuffer();
    int last_width = GetScreenWidth();
    int last_height = GetScreenHeight();

    rlDisableBackfaceCulling();
    rlDisableDepthTest();

    // Створюємо кубмапу
    cubemap.id = rlLoadTextureCubemap(0, size, format, 1);
    unsigned int fbo = rlLoadFramebuffer();
    
    if (fbo == 0)
    {
        CH_CORE_ERROR("Renderer: Failed to create FBO for cubemap generation!");
        return cubemap;
    }

    if (fbo != 0)
    {
        // ПРОЕКЦІЯ ТА ВИДИ
        Matrix proj = MatrixPerspective(90.0f * DEG2RAD, 1.0f, 0.01f, 10.0f);
        Matrix views[] = {
            MatrixLookAt({0,0,0}, { 1,0,0}, {0,-1,0}),
            MatrixLookAt({0,0,0}, {-1,0,0}, {0,-1,0}),
            MatrixLookAt({0,0,0}, {0, 1,0}, {0,0, 1}),
            MatrixLookAt({0,0,0}, {0,-1,0}, {0,0,-1}),
            MatrixLookAt({0,0,0}, {0,0, 1}, {0,-1,0}),
            MatrixLookAt({0,0,0}, {0,0,-1}, {0,-1,0})
        };

        rlEnableShader(shader.id);
        int projLoc = GetShaderLocation(shader, "projection");
        int viewLoc = GetShaderLocation(shader, "view");
        int equirectLoc = GetShaderLocation(shader, "equirectangularMap");
        
        CH_CORE_INFO("Renderer: GenTextureCubemap - Shader ID: {}, Panorama ID: {}, Dim: {}x{}, Size: {}", 
                     shader.id, panorama.id, panorama.width, panorama.height, size);
        CH_CORE_INFO("Renderer: Uniform locations - Proj: {}, View: {}, Equirect: {}", projLoc, viewLoc, equirectLoc);
 
        if (projLoc != -1) SetShaderValueMatrix(shader, projLoc, proj);
        else CH_CORE_ERROR("Renderer: 'projection' uniform not found in CubemapGen shader!");
 
        int slot = 0;
        if (equirectLoc != -1) SetShaderValue(shader, equirectLoc, &slot, SHADER_UNIFORM_INT);
        else CH_CORE_ERROR("Renderer: 'equirectangularMap' uniform not found in CubemapGen shader!");
 
        rlActiveTextureSlot(0);
        rlEnableTexture(panorama.id);

        // НЕ активуємо FBO до того, як прив'яжемо хоча б одну грань,
        // щоб уникнути помилок Incomplete Framebuffer.
        rlViewport(0, 0, size, size);
        for (int i = 0; i < 6; i++)
        {
            // Attach the cubemap face to the framebuffer
            rlFramebufferAttach(fbo, cubemap.id, RL_ATTACHMENT_COLOR_CHANNEL0, RL_ATTACHMENT_CUBEMAP_POSITIVE_X + i, 0);
            
            if (rlFramebufferComplete(fbo))
            {
                rlEnableFramebuffer(fbo);
                rlClearScreenBuffers(); 

                rlEnableShader(shader.id);
                if (projLoc != -1) SetShaderValueMatrix(shader, projLoc, proj);
                if (viewLoc != -1) SetShaderValueMatrix(shader, viewLoc, views[i]);
                
                rlActiveTextureSlot(0);
                rlEnableTexture(panorama.id);

                rlLoadDrawCube(); 
                rlDrawRenderBatchActive(); 
            }
            else
            {
                CH_CORE_ERROR("Renderer: Cubemap FBO incomplete for face {}!", i);
            }
        }

        rlDisableTexture();
        rlDisableShader();

        rlDisableFramebuffer();
        rlUnloadFramebuffer(fbo);
    }

    rlEnableBackfaceCulling();
    rlEnableDepthTest();

    rlEnableFramebuffer(last_fbo);
    rlViewport(0, 0, last_width, last_height);

    cubemap.width = size;
    cubemap.height = size;
    cubemap.mipmaps = 1;
    cubemap.format = format;

    return cubemap;
}

void Renderer::CleanupSkybox()
{
    UnloadModel(m_Data->Skybox.SkyboxCube);
    UnloadMaterial(m_Data->Skybox.SkyboxMaterial);
    if (m_Data->Skybox.CachedCubemap.id != 0)
    {
        UnloadTexture(m_Data->Skybox.CachedCubemap);
    }
}

} // namespace CHEngine
