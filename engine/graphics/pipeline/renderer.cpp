#include "engine/graphics/pipeline/renderer.h"
#include "engine/graphics/pipeline/geometry_generator.h"
#include "engine/graphics/api/graphics_device.h"
#include "engine/graphics/api/renderer_types.h"
#include "engine/graphics/pipeline/shader_storage.h"
#include "engine/assets/asset_manager.h"
#include "engine/core/log.h"
#include "engine/core/service_locator.h"
#include "engine/graphics/api/buffer.h"
#include "engine/graphics/api/storage_buffer.h"
#include "engine/graphics/api/vertex_array.h"
#include "engine/assets/types/environment_asset.h"
#include "engine/assets/types/shader_asset.h"
#include <algorithm>
#include <glm/gtc/type_ptr.hpp>
#include <variant> // Added for type-safe variant visitation
#include <vector>


namespace Chained
{

void Renderer::Initialize()
{
    if (m_Headless)
    {
        CH_CORE_INFO("[Renderer] Headless mode enabled, skipping OpenGL initialization.");
        return;
    }

    GraphicsDevice::Set(GraphicsDevice::Create());
    GraphicsDevice::Get().Initialize();

    // Initialize SSBO for lights using abstraction
    m_Data->Lighting.LightSSBO = StorageBuffer::Create(sizeof(RenderLight) * LightingData::MaxLights);
    m_Data->Lighting.LightsDirty = true;

    // Initialize Engine static resources
    if (!m_Data->Geometry.FullscreenQuadVAO)
    {
        float vertices[] = {-1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f,  -1.0f, 0.0f, 1.0f, 0.0f,
                            1.0f,  1.0f,  0.0f, 1.0f, 1.0f, -1.0f, 1.0f,  0.0f, 0.0f, 1.0f};
        uint32_t indices[] = {0, 1, 2, 2, 3, 0};

        m_Data->Geometry.FullscreenQuadVAO = VertexArray::Create();
        auto vbo = VertexBuffer::Create(vertices, sizeof(vertices));
        vbo->SetLayout({{VertexAttributeType::Float3, "vertexPosition"}, {VertexAttributeType::Float2, "vertexTexCoord"}});
        m_Data->Geometry.FullscreenQuadVAO->AddVertexBuffer(vbo);
        auto ibo = IndexBuffer::Create(indices, 6);
        m_Data->Geometry.FullscreenQuadVAO->SetIndexBuffer(ibo);
    }

    // Initialize UBOs
    m_Data->CameraUBO = UniformBuffer::Create(sizeof(CameraData), 0);

    InitializeSkybox();

    LoadEngineResources();
}

void Renderer::LoadEngineResources()
{
    auto& shaders = GetShaderLibrary();

    shaders.LoadConfig("engine/resources/config/shaders.yaml");

    // Eager load common shaders if needed, or let them lazy load
    shaders.LoadOrGet("Lighting");
    shaders.LoadOrGet("Skinned");
    shaders.LoadOrGet("Unlit");
    shaders.LoadOrGet("Billboard");

    CH_CORE_INFO("[Renderer] LoadEngineResources done. {} shader(s) loaded.", shaders.GetNames().size());
}

void Renderer::Shutdown()
{
    CH_CORE_INFO("Shutting down Render System...");

    if (m_Headless)
    {
        return;
    }

    CleanupSkybox();

    if (m_Data->Lighting.LightSSBO)
    {
        m_Data->Lighting.LightSSBO.reset();
    }
    if (m_Data->Shaders) m_Data->Shaders.reset();
    if (m_Data->GlobalUBO) m_Data->GlobalUBO.reset();

    m_Data->Geometry.FullscreenQuadVAO.reset();
    m_Data->CameraUBO.reset();
    m_Data->Geometry.BillboardVAO.reset();
    m_Data->Geometry.SpriteVAO.reset();

    m_Data->Instancing.VAOCache.clear();
    m_Data->Instancing.Buffer.reset();
    m_Data->Instancing.Buffer.reset();

    GraphicsDevice::Get().Shutdown();
}

Renderer::Renderer()
{
    m_Data = std::make_unique<RendererData>();
    m_Data->Shaders = std::make_unique<ShaderStorage>();
}

Renderer::~Renderer() = default;

void Renderer::BeginScene(const Camera3D& camera, float nearClip, float farClip)
{
    m_Data->Frame.CameraPosition = camera.Position;

    // Update light SSBO once per frame
    if (m_Data->Lighting.LightsDirty && m_Data->Lighting.LightSSBO)
    {
        m_Data->Lighting.LightSSBO->SetData(m_Data->Lighting.Lights, sizeof(RenderLight) * LightingData::MaxLights);
        m_Data->Lighting.LightsDirty = false;
    }

    // Track the default lighting shader handle for DrawMesh fallback
    auto lightingShaderAsset = m_Data->Shaders->Exists("Lighting") ? m_Data->Shaders->Get("Lighting") : nullptr;
    if (lightingShaderAsset && lightingShaderAsset->GetShader())
    {
        m_Data->Frame.CurrentShaderId = lightingShaderAsset->GetShader()->GetNativeHandle();
    }

    // --- Direct glm::mat4 Management (Pure OpenGL style) ---
    // 1. Calculate View Transform
    m_Data->Frame.View = glm::lookAt(camera.Position, camera.Target, camera.Up);

    // 2. Calculate Projection Transform
    int width = m_ViewportWidth;
    int height = m_ViewportHeight;
    float aspect = (height > 0) ? (float)width / (float)height : 1.0f;
    if (camera.Projection == ProjectionType::Perspective)
    {
        m_Data->Frame.Proj = glm::perspective(glm::radians(camera.FovDegrees), aspect, nearClip, farClip);
    }
    else
    {
        float top = camera.FovDegrees / 2.0f;
        float right = top * aspect;
        m_Data->Frame.Proj = glm::ortho(-right, right, -top, top, nearClip, farClip);
    }

    // Upload to UBO
    CameraData cameraData;
    cameraData.ViewProjection = m_Data->Frame.Proj * m_Data->Frame.View;
    cameraData.Projection = m_Data->Frame.Proj;
    cameraData.View = m_Data->Frame.View;
    m_Data->CameraUBO->SetData(&cameraData, sizeof(CameraData));
    m_Data->CameraUBO->BindBase(0);
}

void Renderer::EndScene()
{
    m_Data->Frame.CurrentShaderId = 0;
}

void Renderer::Clear(const glm::vec4& color)
{
    Color chColor((unsigned char)(color.r * 255), (unsigned char)(color.g * 255), (unsigned char)(color.b * 255),
                  (unsigned char)(color.a * 255));
    GraphicsDevice::Get().Clear(chColor);
}

void Renderer::SetViewport(int x, int y, int width, int height)
{
    GraphicsDevice::Get().SetViewport(x, y, width, height);
}

void Renderer::DrawMesh(const Mesh& mesh, const Material& material, const glm::mat4& transform)
{
    uint32_t shaderId = material.ShaderID;
    if (shaderId == 0)
    {
        shaderId = m_Data->Frame.CurrentShaderId;
    }
    if (shaderId == 0)
    {
        return;
    }

    auto shaderAsset = GetShaderLibrary().GetById(shaderId);
    if (!shaderAsset)
    {
        return;
    }

    auto shader = shaderAsset->GetShader();
    if (!shader)
    {
        return;
    }

    shader->Bind();

    // Set model matrix
    shader->SetMatrix("matModel", transform);

    glm::mat4 matNormal = glm::transpose(glm::inverse(transform));
    shader->SetMatrix("matNormal", matNormal);

    // Bind VAO and Draw
    if (mesh.VAO)
    {
        mesh.VAO->Bind();
        if (mesh.TriangleCount > 0)
        {
            GraphicsDevice::Get().DrawIndexed(mesh.VAO, mesh.TriangleCount * 3);
        }
        else
        {
            GraphicsDevice::Get().DrawArrays(mesh.VertexCount);
        }
        mesh.VAO->Unbind();
    }
}

void Renderer::DrawMeshInstanced(const Mesh& mesh, const Material& material, const std::vector<glm::mat4>& transforms)
{
    if (transforms.empty() || !mesh.VAO)
    {
        return;
    }

    uint32_t shaderId = material.ShaderID;
    if (shaderId == 0)
    {
        shaderId = m_Data->Frame.CurrentShaderId;
    }
    if (shaderId == 0)
    {
        return;
    }

    auto shaderAsset = GetShaderLibrary().GetById(shaderId);
    if (!shaderAsset)
    {
        return;
    }

    auto shader = shaderAsset->GetShader();
    if (!shader)
    {
        return;
    }

    shader->Bind();

    // Set up matrices
    glm::mat4 mvp = m_Data->Frame.Proj * m_Data->Frame.View;
    shader->SetMatrix("u_ViewProjection", mvp);
    shader->SetMatrix("u_Transform", glm::mat4(1.0f));

    // 1. Manage/Reuse Instance Buffer
    uint32_t dataSize = (uint32_t)(transforms.size() * sizeof(glm::mat4));
    if (!m_Data->Instancing.Buffer || m_Data->Instancing.Capacity < dataSize)
    {
        // Reallocate if needed (starting at 1024 instances or required size)
        m_Data->Instancing.Capacity = std::max(dataSize, (uint32_t)(1024 * sizeof(glm::mat4)));
        m_Data->Instancing.Buffer = VertexBuffer::Create(m_Data->Instancing.Capacity);
        m_Data->Instancing.Buffer->SetLayout({{VertexAttributeType::Mat4, "a_InstanceTransform", false, true}});

        // Clear VAO cache because the VBO handle changed
        m_Data->Instancing.VAOCache.clear();
    }
    m_Data->Instancing.Buffer->SetData(transforms.data(), dataSize);

    // 2. Get or Create Cached Instanced VAO
    auto& instancedVAO = m_Data->Instancing.VAOCache[mesh.VAO.get()];
    if (!instancedVAO)
    {
        instancedVAO = VertexArray::Create();
        for (const auto& vbo : mesh.VAO->GetVertexBuffers())
        {
            instancedVAO->AddVertexBuffer(vbo);
        }
        instancedVAO->AddVertexBuffer(m_Data->Instancing.Buffer);
        instancedVAO->SetIndexBuffer(mesh.VAO->GetIndexBuffer());
    }

    // 3. Bind and Draw
    instancedVAO->Bind();
    if (mesh.TriangleCount > 0)
    {
        GraphicsDevice::Get().DrawIndexedInstanced(instancedVAO, (uint32_t)transforms.size(), mesh.TriangleCount * 3);
    }
    else
    {
        GraphicsDevice::Get().DrawArraysInstanced(mesh.VertexCount, (uint32_t)transforms.size());
    }
    instancedVAO->Unbind();
}

void Renderer::DrawSkybox(uint32_t textureId, int skyboxMode, bool isHDR, float exposure, float brightness,
                          float contrast, const Camera3D& camera, bool flipped)
{
    if (textureId == 0)
    {
        return;
    }

    skyboxMode = std::clamp(skyboxMode, 0, 2);

    auto shaderAsset = (skyboxMode == 2) ? m_Data->Shaders->LoadOrGet("SkyboxCubemap")
                                         : (skyboxMode == 1 ? m_Data->Shaders->LoadOrGet("SkyboxCross")
                                                            : m_Data->Shaders->LoadOrGet("Skybox"));
    if (!shaderAsset || !shaderAsset->GetShader())
    {
        return;
    }

    // 1. Prepare Render State
    GraphicsDevice::Get().SetDepthFunc(GraphicsDevice::DepthFunc::LEqual);
    GraphicsDevice::Get().SetCullMode(GraphicsDevice::CullMode::None);
    GraphicsDevice::Get().DisableDepthMask();

    // 2. Setup Uniforms
    shaderAsset->GetShader()->Bind();

    // Always remove translation from view matrix for skybox
    glm::mat4 view = glm::mat4(glm::mat3(m_Data->Frame.View));
    shaderAsset->GetShader()->SetMatrix("u_View", view);
    shaderAsset->GetShader()->SetMatrix("u_Projection", m_Data->Frame.Proj);

    shaderAsset->GetShader()->SetFloat("u_Exposure", exposure);
    shaderAsset->GetShader()->SetFloat("u_Brightness", brightness);
    shaderAsset->GetShader()->SetFloat("u_Contrast", contrast);
    shaderAsset->GetShader()->SetInt("u_IsHDR", isHDR ? 1 : 0);
    shaderAsset->GetShader()->SetInt("u_VFlipped", flipped ? 1 : 0);

    ApplyFogUniforms(shaderAsset.get());

    // 3. Bind Textures and Draw Mesh
    if (skyboxMode == 2)
    {
        GraphicsDevice::Get().SetTexture(0, textureId, true);
        shaderAsset->GetShader()->SetInt("u_Cubemap", 0);

        if (m_Data->Skybox.SkyboxCubeModel && !m_Data->Skybox.SkyboxCubeModel->Meshes.empty())
        {
            auto& mesh = m_Data->Skybox.SkyboxCubeModel->Meshes[0];
            mesh.VAO->Bind();
            GraphicsDevice::Get().DrawIndexed(mesh.VAO, 36);
            mesh.VAO->Unbind();
        }
    }
    else if (skyboxMode == 1)
    {
        GraphicsDevice::Get().SetTexture(0, textureId);
        shaderAsset->GetShader()->SetInt("u_CrossMap", 0);

        if (m_Data->Skybox.SkyboxCubeModel && !m_Data->Skybox.SkyboxCubeModel->Meshes.empty())
        {
            auto& mesh = m_Data->Skybox.SkyboxCubeModel->Meshes[0];
            mesh.VAO->Bind();
            GraphicsDevice::Get().DrawIndexed(mesh.VAO, 36);
            mesh.VAO->Unbind();
        }
    }
    else if (skyboxMode == 0)
    {
        GraphicsDevice::Get().SetTexture(0, textureId);
        shaderAsset->GetShader()->SetInt("u_Panorama", 0);

        if (m_Data->Skybox.SkyboxSphereModel && !m_Data->Skybox.SkyboxSphereModel->Meshes.empty())
        {
            auto& mesh = m_Data->Skybox.SkyboxSphereModel->Meshes[0];
            mesh.VAO->Bind();
            GraphicsDevice::Get().DrawIndexed(mesh.VAO, mesh.TriangleCount * 3);
            mesh.VAO->Unbind();
        }
    }

    // 4. Restore Render State
    GraphicsDevice::Get().SetDepthFunc(GraphicsDevice::DepthFunc::Less);
    GraphicsDevice::Get().SetCullMode(GraphicsDevice::CullMode::Back);
    GraphicsDevice::Get().EnableDepthMask();
}

void Renderer::DrawBillboard(const Camera3D& camera, uint32_t textureId, const glm::vec3& position, float size,
                             const glm::vec4& tint)
{
    auto billboardShaderAsset = m_Data->Shaders->LoadOrGet("Billboard");
    if (!billboardShaderAsset || !billboardShaderAsset->GetShader() || textureId == 0)
    {
        return;
    }

    auto shader = billboardShaderAsset->GetShader();
    shader->Bind();

    glm::vec3 look = glm::normalize(camera.Position - position);
    glm::vec3 right = glm::cross(camera.Up, look);
    if (glm::length(right) < 0.0001f)
    {
        right = glm::vec3(1.0f, 0.0f, 0.0f);
    }
    else
    {
        right = glm::normalize(right);
    }
    glm::vec3 up = glm::normalize(glm::cross(look, right));

    glm::mat4 model = glm::mat4(1.0f);
    model[0] = glm::vec4(right * size, 0.0f);
    model[1] = glm::vec4(up * size, 0.0f);
    model[2] = glm::vec4(look * size, 0.0f);
    model[3] = glm::vec4(position, 1.0f);

    shader->SetMatrix("mvp", m_Data->Frame.Proj * m_Data->Frame.View * model);
    shader->SetVec4("colDiffuse", tint);

    GraphicsDevice::Get().SetTexture(0, textureId);
    shader->SetInt("texture0", 0);

    const bool blendWasEnabled = GraphicsDevice::Get().IsBlendEnabled();
    const bool cullWasEnabled = GraphicsDevice::Get().IsCullFaceEnabled();

    GraphicsDevice::Get().SetBlendEnabled(true);
    GraphicsDevice::Get().SetBlendFunc(GraphicsDevice::BlendFactor::SrcAlpha, GraphicsDevice::BlendFactor::OneMinusSrcAlpha);
    GraphicsDevice::Get().SetCullMode(GraphicsDevice::CullMode::None); // Using abstraction layer safe call

    if (!m_Data->Geometry.BillboardVAO)
    {
        float vertices[] = {
            // x,     y,     z,     u,    v,    nx,   ny,   nz
            -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
             0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
             0.5f,  0.5f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
            -0.5f,  0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        };
        uint32_t indices[] = {0, 1, 2, 2, 3, 0};

        auto vbo = VertexBuffer::Create(vertices, sizeof(vertices));
        vbo->SetLayout({{VertexAttributeType::Float3, "a_Position"}, {VertexAttributeType::Float2, "a_TexCoord"}, {VertexAttributeType::Float3, "a_Normal"}});

        m_Data->Geometry.BillboardVAO = VertexArray::Create();
        m_Data->Geometry.BillboardVAO->AddVertexBuffer(vbo);
        auto ibo = IndexBuffer::Create(indices, 6);
        m_Data->Geometry.BillboardVAO->SetIndexBuffer(ibo);
    }

    m_Data->Geometry.BillboardVAO->Bind();
    GraphicsDevice::Get().DrawIndexed(m_Data->Geometry.BillboardVAO, 6);
    m_Data->Geometry.BillboardVAO->Unbind();

    GraphicsDevice::Get().SetCullMode(cullWasEnabled ? GraphicsDevice::CullMode::Back : GraphicsDevice::CullMode::None);
    GraphicsDevice::Get().SetBlendEnabled(blendWasEnabled);
}

void Renderer::ApplyPostProcessing(uint32_t screenTextureId, uint32_t depthTextureId, const Camera3D& camera,
                                   ShaderAsset* overrideShader, const std::vector<ShaderUniform>& uniforms)
{
    std::shared_ptr<ShaderAsset> shaderAsset = nullptr;

    if (overrideShader)
    {
        auto handle = ServiceLocator::Get<AssetManager>()->ResolveToHandle(overrideShader->GetPath());
        shaderAsset = ServiceLocator::Get<AssetManager>()->Get<ShaderAsset>(handle);
    }
    else
    {
        shaderAsset = m_Data->Shaders->LoadOrGet("PostProcess");
    }

    if (shaderAsset && shaderAsset->GetShader())
    {
        auto shader = shaderAsset->GetShader();
        shader->Bind();

        // Safe extraction of diagnostic float values from variant
        float diagIntensity = 0.0f;
        for (const auto& u : uniforms)
        {
            if (u.Name == "uIntensity")
            {
                if (auto* fVal = std::get_if<float>(&u.Value))
                {
                    diagIntensity = *fVal;
                }
            }
        }
        if (diagIntensity > 0.001f)
        {
            CH_CORE_INFO("[RENDER DIAG] Applying shader '{}', Intensity={}", shaderAsset->GetPath(), diagIntensity);
        }

        // 1. Set System Uniforms
        glm::mat4 identity = glm::mat4(1.0f);
        shader->SetMatrix("mvp", identity);

        glm::mat4 invViewProj = glm::inverse(m_Data->Frame.Proj * m_Data->Frame.View);
        shader->SetMatrix("matInverseViewProj", invViewProj);
        shader->SetVec3("viewPos", camera.Position);

        float currentSeconds = (float)m_Data->Frame.Time.GetSeconds();
        shader->SetFloat("uTimeF", currentSeconds);
        shader->SetFloat("uTime", currentSeconds);
        shader->SetFloat("time", currentSeconds);
        shader->SetFloat("uExposure", m_Data->Lighting.CurrentLighting.Exposure);
        shader->SetFloat("uGamma", m_Data->Lighting.CurrentLighting.Gamma);

        ApplyFogUniforms(shaderAsset.get());

        // 2. Set Custom Uniforms using type-safe std::visit
        for (const auto& u : uniforms)
        {
            std::visit(
                [&](auto&& arg) {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<T, float>)
                    {
                        shader->SetFloat(u.Name, arg);
                    }
                    else if constexpr (std::is_same_v<T, glm::vec2>)
                    {
                        shader->SetVec2(u.Name, arg);
                    }
                    else if constexpr (std::is_same_v<T, glm::vec3>)
                    {
                        shader->SetVec3(u.Name, arg);
                    }
                    else if constexpr (std::is_same_v<T, glm::vec4>)
                    {
                        shader->SetVec4(u.Name, arg);
                    }
                    else if constexpr (std::is_same_v<T, Color>)
                    {
                        // Clean on-the-fly normalization for Color structures
                        glm::vec4 colorVec = {arg.r / 255.0f, arg.g / 255.0f, arg.b / 255.0f, arg.a / 255.0f};
                        shader->SetVec4(u.Name, colorVec);
                    }
                },
                u.Value);
        }

        // 3. Bind Textures
        GraphicsDevice::Get().SetTexture(0, screenTextureId);
        shader->SetInt("texture0", 0);

        GraphicsDevice::Get().SetTexture(1, depthTextureId);
        shader->SetInt("texture1", 1);

        GraphicsDevice::Get().DisableDepthTest();

        if (m_Data->Geometry.FullscreenQuadVAO)
        {
            m_Data->Geometry.FullscreenQuadVAO->Bind();
            GraphicsDevice::Get().DrawIndexed(m_Data->Geometry.FullscreenQuadVAO, 6);
            m_Data->Geometry.FullscreenQuadVAO->Unbind();
        }

        GraphicsDevice::Get().EnableDepthTest();
        GraphicsDevice::Get().SetCullMode(GraphicsDevice::CullMode::Back);
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
    m_Data->Lighting.LightCount = count;
}

void Renderer::ClearLights()
{
    for (int i = 0; i < LightingData::MaxLights; i++)
    {
        m_Data->Lighting.Lights[i].enabled = 0;
    }
    m_Data->Lighting.LightCount = 0;
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
    m_Data->Frame.DiagnosticMode = mode;
}

void Renderer::UpdateTime(Timestep time)
{
    m_Data->Frame.Time = time;
}

void Renderer::Update(Timestep ts)
{
    UpdateTime(ts);
}

void Renderer::SetLightingUniforms(ShaderAsset* shaderAsset)
{
    if (!shaderAsset || !shaderAsset->GetShader())
        return;

    const auto& lighting = m_Data->Lighting.CurrentLighting;
    auto shader = shaderAsset->GetShader();
    shader->Bind();

    glm::vec4 lightColor = {lighting.LightColor.r / 255.0f, lighting.LightColor.g / 255.0f,
                            lighting.LightColor.b / 255.0f, lighting.LightColor.a / 255.0f};
    glm::vec4 skyColor = lightColor;
    skyColor.w = lighting.Ambient * 0.35f;

    shader->SetVec3("viewPos", m_Data->Frame.CameraPosition);
    shader->SetFloat("uTime", static_cast<float>(m_Data->Frame.Time));
    shader->SetFloat("uMode", m_Data->Frame.DiagnosticMode);
    glm::vec3 lightDirNorm = glm::length(lighting.Direction) > 0.0001f
                                 ? glm::normalize(lighting.Direction)
                                 : glm::vec3(0.0f, -1.0f, 0.0f);
    shader->SetVec3("lightDir", lightDirNorm);
    shader->SetVec4("lightColor", lightColor);
    shader->SetFloat("ambient", lighting.Ambient);
    shader->SetVec4("skyAmbientColor", skyColor);
        shader->SetInt("uLightCount", m_Data->Lighting.LightCount);
    shader->SetFloat("uExposure", lighting.Exposure);
    shader->SetFloat("uGamma", lighting.Gamma);

    if (m_Data->Lighting.LightSSBO)
        m_Data->Lighting.LightSSBO->BindBase(0);

    // Shadow uniforms
    shader->SetInt("u_ShadowsEnabled", m_Data->Shadow.Enabled ? 1 : 0);
    shader->SetMatrix("u_LightSpaceMatrix", m_Data->Shadow.LightSpaceMatrix);
    shader->SetFloat("u_ShadowBias", m_Data->Shadow.Bias);
    if (m_Data->Shadow.Enabled && m_Data->Shadow.MapTextureID > 0)
    {
        GraphicsDevice::Get().SetTexture(6, m_Data->Shadow.MapTextureID);
        shader->SetInt("u_ShadowMap", 6);
    }

    ApplyFogUniforms(shaderAsset);
}

void Renderer::ApplyFogUniforms(ShaderAsset* shader)
{
    const auto& fog = m_Data->Lighting.CurrentFog;
    int enabled = fog.Enabled ? 1 : 0;
    int mode = (int)fog.Mode;
    glm::vec4 color = {fog.FogColor.r / 255.0f, fog.FogColor.g / 255.0f, fog.FogColor.b / 255.0f,
                       fog.FogColor.a / 255.0f};

    shader->GetShader()->SetInt("fogEnabled", enabled);
    shader->GetShader()->SetVec4("fogColor", color);
    shader->GetShader()->SetFloat("fogDensity", fog.Density);
    shader->GetShader()->SetFloat("fogStart", fog.Start);
    shader->GetShader()->SetFloat("fogEnd", fog.End);
    shader->GetShader()->SetInt("fogMode", mode);
    shader->GetShader()->SetFloat("fogHeightFalloff", fog.HeightFalloff);
}

void Renderer::InitializeSkybox()
{
    m_Data->Skybox.SkyboxCubeModel = std::make_unique<Model>();
    m_Data->Skybox.SkyboxCubeModel->Meshes.push_back(GeometryGenerator::GenerateUnitCube());

    m_Data->Skybox.SkyboxSphereModel = std::make_unique<Model>();
    m_Data->Skybox.SkyboxSphereModel->Meshes.push_back(GeometryGenerator::GenerateSphere(50.0f, 64, 64));
}

void Renderer::CleanupSkybox()
{
    m_Data->Skybox.SkyboxCubeModel.reset();
    m_Data->Skybox.SkyboxSphereModel.reset();
    m_Data->Skybox.CachedCubemap.reset();
}

void Renderer::CleanupResources()
{
}

void Renderer::DrawSprite(uint32_t textureId, const glm::mat4& transform, const glm::vec4& tint, bool flipX, bool flipY)
{
    if (textureId == 0)
    {
        return;
    }

    auto shaderAsset = m_Data->Shaders->LoadOrGet("Sprite");
    if (!shaderAsset || !shaderAsset->GetShader())
    {
        return;
    }

    auto shader = shaderAsset->GetShader();
    shader->Bind();

    shader->SetMatrix("mvp", m_Data->Frame.Proj * m_Data->Frame.View * transform);
    shader->SetMatrix("matModel", transform);
    shader->SetVec4("u_Tint", tint);
    shader->SetVec2("u_Flip", glm::vec2(flipX ? 1.0f : 0.0f, flipY ? 1.0f : 0.0f));

    const bool blendWasEnabled = GraphicsDevice::Get().IsBlendEnabled();
    const bool cullWasEnabled = GraphicsDevice::Get().IsCullFaceEnabled();

    GraphicsDevice::Get().SetBlendEnabled(true);
    GraphicsDevice::Get().SetBlendFunc(GraphicsDevice::BlendFactor::SrcAlpha, GraphicsDevice::BlendFactor::OneMinusSrcAlpha);
    GraphicsDevice::Get().SetCullMode(GraphicsDevice::CullMode::None);

    if (!m_Data->Geometry.SpriteVAO)
    {
        float vertices[] = {
            // x,     y,     z,     u,    v,    nx,   ny,   nz
            -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
             0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
             0.5f,  0.5f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
            -0.5f,  0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        };
        uint32_t indices[] = {0, 1, 2, 2, 3, 0};

        auto vbo = VertexBuffer::Create(vertices, sizeof(vertices));
        vbo->SetLayout({{VertexAttributeType::Float3, "a_Position"}, {VertexAttributeType::Float2, "a_TexCoord"}, {VertexAttributeType::Float3, "a_Normal"}});

        m_Data->Geometry.SpriteVAO = VertexArray::Create();
        m_Data->Geometry.SpriteVAO->AddVertexBuffer(vbo);
        auto ibo = IndexBuffer::Create(indices, 6);
        m_Data->Geometry.SpriteVAO->SetIndexBuffer(ibo);
    }

    if (m_Data->Geometry.SpriteVAO)
    {
        m_Data->Geometry.SpriteVAO->Bind();
        GraphicsDevice::Get().DrawIndexed(m_Data->Geometry.SpriteVAO, 6);
        m_Data->Geometry.SpriteVAO->Unbind();
    }

    GraphicsDevice::Get().SetCullMode(cullWasEnabled ? GraphicsDevice::CullMode::Back : GraphicsDevice::CullMode::None);
    GraphicsDevice::Get().SetBlendEnabled(blendWasEnabled);
}

} // namespace Chained