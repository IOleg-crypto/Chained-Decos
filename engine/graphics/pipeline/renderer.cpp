#include "engine/graphics/pipeline/renderer.h"
#include "engine/graphics/pipeline/geometry_generator.h"
#include "engine/graphics/pipeline/render_command.h"
#include "engine/graphics/pipeline/renderer_types.h"
#include "engine/graphics/pipeline/shader_storage.h"

#include "engine/app/application.h"
#include "engine/assets/asset_manager.h"
#include "engine/core/log.h"
#include "engine/core/service_locator.h"


#include "engine/graphics/api/buffer.h"
#include "engine/graphics/api/framebuffer.h"
#include "engine/graphics/api/storage_buffer.h"
#include "engine/graphics/api/vertex_array.h"

#include "engine/assets/types/environment_asset.h"
#include "engine/assets/types/shader_asset.h"
#include "engine/assets/types/texture_asset.h"

#include <algorithm>
#include <glm/gtc/type_ptr.hpp>
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

    RenderCommand::Initialize();

    // Initialize SSBO for lights using abstraction
    m_Data->Lighting.LightSSBO = StorageBuffer::Create(sizeof(RenderLight) * LightingData::MaxLights);
    m_Data->Lighting.LightsDirty = true;

    // Initialize Engine static resources
    if (!m_Data->FullscreenQuadVAO)
    {
        float vertices[] = {-1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f,  -1.0f, 0.0f, 1.0f, 0.0f,
                            1.0f,  1.0f,  0.0f, 1.0f, 1.0f, -1.0f, 1.0f,  0.0f, 0.0f, 1.0f};
        uint32_t indices[] = {0, 1, 2, 2, 3, 0};

        m_Data->FullscreenQuadVAO = VertexArray::Create();
        auto vbo = VertexBuffer::Create(vertices, sizeof(vertices));
        vbo->SetLayout({{ShaderDataType::Float3, "vertexPosition"}, {ShaderDataType::Float2, "vertexTexCoord"}});
        m_Data->FullscreenQuadVAO->AddVertexBuffer(vbo);
        auto ibo = IndexBuffer::Create(indices, 6);
        m_Data->FullscreenQuadVAO->SetIndexBuffer(ibo);
    }

    // Initialize UBOs
    m_Data->CameraUBO = UniformBuffer::Create(sizeof(CameraData), 0);

    InitializeResources();
    InitializeSkybox();

    LoadEngineResources();
}

void Renderer::LoadEngineResources()
{
    auto& shaders = GetShaderLibrary();

    auto loadShader = [&](const std::string& name, const std::string& path) { shaders.LoadOrGet(name, path); };

    loadShader("Lighting", "resources/shaders/lighting.chshader");
    loadShader("Unlit", "resources/shaders/unlit.chshader");

    CH_CORE_INFO("[Renderer] LoadEngineResources done. {} shader(s) loaded.", shaders.GetNames().size());
}

void Renderer::Shutdown()
{
    CH_CORE_INFO("Shutting down Render System...");

    if (m_Headless)
    {
        return;
    }

    CleanupResources();
    CleanupSkybox();

    if (m_Data->Lighting.LightSSBO)
    {
        m_Data->Lighting.LightSSBO.reset();
    }

    // Clear caches
    m_Data->InstancedVAOCache.clear();
    m_Data->InstanceBuffer.reset();
    m_Data->LineVBO.reset();
    m_Data->LineVAO.reset();
}

Renderer::Renderer()
{
    m_Data = std::make_unique<RendererData>();
    m_Data->Shaders = std::make_unique<ShaderStorage>();
}

Renderer::~Renderer()
{
}

void Renderer::BeginScene(const Camera3D& camera, float nearClip, float farClip)
{
    m_Data->CurrentCameraPosition = camera.Position;

    // Update light SSBO once per frame
    if (m_Data->Lighting.LightsDirty && m_Data->Lighting.LightSSBO)
    {
        m_Data->Lighting.LightSSBO->SetData(m_Data->Lighting.Lights, sizeof(RenderLight) * LightingData::MaxLights);
        m_Data->Lighting.LightsDirty = false;
    }

    // Bind lighting uniforms to the default lighting shader if it exists
    auto lightingShaderAsset = m_Data->Shaders->Exists("Lighting") ? m_Data->Shaders->Get("Lighting") : nullptr;
    if (lightingShaderAsset && lightingShaderAsset->GetShader())
    {
        auto shader = lightingShaderAsset->GetShader();
        shader->Bind();

        float time = m_Data->Time;
        float diagMode = m_Data->DiagnosticMode;
        float exposure = m_Data->Lighting.CurrentLighting.Exposure;
        float ambient = m_Data->Lighting.CurrentLighting.Ambient;
        float lightCount = (float)m_Data->LightCount;
        float gamma = m_Data->Lighting.CurrentLighting.Gamma;

        shader->SetVec3("viewPos", camera.Position);
        shader->SetFloat("uTime", time);
        shader->SetFloat("uMode", diagMode);

        shader->SetVec3("lightDir", m_Data->Lighting.CurrentLighting.Direction);

        glm::vec4 lightColor = {m_Data->Lighting.CurrentLighting.LightColor.r / 255.0f,
                                m_Data->Lighting.CurrentLighting.LightColor.g / 255.0f,
                                m_Data->Lighting.CurrentLighting.LightColor.b / 255.0f,
                                m_Data->Lighting.CurrentLighting.LightColor.a / 255.0f};
        shader->SetVec4("lightColor", lightColor);

        shader->SetFloat("ambient", ambient);

        glm::vec4 skyColor = lightColor;
        skyColor.w = ambient * 0.35f;
        shader->SetVec4("skyAmbientColor", skyColor);

        shader->SetInt("uLightCount", (int)lightCount);
        shader->SetFloat("uExposure", exposure);
        shader->SetFloat("uGamma", gamma);

        ApplyFogUniforms(lightingShaderAsset);
        if (m_Data->Lighting.LightSSBO)
        {
            m_Data->Lighting.LightSSBO->BindBase(0);
        }
        m_Data->CurrentShaderId = shader->GetRendererID();
    }

    // --- Direct glm::mat4 Management (Pure OpenGL style) ---
    // 1. Calculate View Transform
    m_Data->CurrentView = glm::lookAt(camera.Position, camera.Target, camera.Up);

    // 2. Calculate Projection Transform
    int width = m_ViewportWidth;
    int height = m_ViewportHeight;
    float aspect = (height > 0) ? (float)width / (float)height : 1.0f;
    if (camera.Projection == 0 /* CAMERA_PERSPECTIVE */)
    {
        m_Data->CurrentProj = glm::perspective(glm::radians(camera.FovY), aspect, nearClip, farClip);
    }
    else
    {
        float top = camera.FovY / 2.0f;
        float right = top * aspect;
        m_Data->CurrentProj = glm::ortho(-right, right, -top, top, nearClip, farClip);
    }

    // Upload to UBO
    CameraData cameraData;
    cameraData.ViewProjection = m_Data->CurrentProj * m_Data->CurrentView;
    cameraData.Projection = m_Data->CurrentProj;
    cameraData.View = m_Data->CurrentView;
    m_Data->CameraUBO->SetData(&cameraData, sizeof(CameraData));
}

void Renderer::EndScene()
{
    m_Data->CurrentShaderId = 0;
}

void Renderer::Clear(const glm::vec4& color)
{
    Color chColor((unsigned char)(color.r * 255), (unsigned char)(color.g * 255), (unsigned char)(color.b * 255),
                  (unsigned char)(color.a * 255));
    RenderCommand::Clear(chColor);
}

void Renderer::SetViewport(int x, int y, int width, int height)
{
    RenderCommand::SetViewport(x, y, width, height);
}

void Renderer::DrawMesh(const Mesh& mesh, const Material& material, const glm::mat4& transform)
{
    uint32_t shaderId = material.ShaderID;
    if (shaderId == 0)
    {
        shaderId = m_Data->CurrentShaderId;
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
            RenderCommand::DrawIndexed(mesh.VAO, mesh.TriangleCount * 3);
        }
        else
        {
            RenderCommand::DrawArrays(mesh.VertexCount);
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
        shaderId = m_Data->CurrentShaderId;
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
    glm::mat4 mvp = m_Data->CurrentProj * m_Data->CurrentView;
    shader->SetMatrix("u_ViewProjection", mvp);
    shader->SetMatrix("u_Transform", glm::mat4(1.0f));

    // 1. Manage/Reuse Instance Buffer
    uint32_t dataSize = (uint32_t)(transforms.size() * sizeof(glm::mat4));
    if (!m_Data->InstanceBuffer || m_Data->InstanceBufferCapacity < dataSize)
    {
        // Reallocate if needed (starting at 1024 instances or required size)
        m_Data->InstanceBufferCapacity = std::max(dataSize, (uint32_t)(1024 * sizeof(glm::mat4)));
        m_Data->InstanceBuffer = VertexBuffer::Create(m_Data->InstanceBufferCapacity);
        m_Data->InstanceBuffer->SetLayout({{ShaderDataType::Mat4, "a_InstanceTransform", false, true}});

        // Clear VAO cache because the VBO handle changed
        m_Data->InstancedVAOCache.clear();
    }
    m_Data->InstanceBuffer->SetData(transforms.data(), dataSize);

    // 2. Get or Create Cached Instanced VAO
    auto& instancedVAO = m_Data->InstancedVAOCache[mesh.VAO.get()];
    if (!instancedVAO)
    {
        instancedVAO = VertexArray::Create();
        for (const auto& vbo : mesh.VAO->GetVertexBuffers())
        {
            instancedVAO->AddVertexBuffer(vbo);
        }
        instancedVAO->AddVertexBuffer(m_Data->InstanceBuffer);
        instancedVAO->SetIndexBuffer(mesh.VAO->GetIndexBuffer());
    }

    // 3. Bind and Draw
    instancedVAO->Bind();
    if (mesh.TriangleCount > 0)
    {
        RenderCommand::DrawIndexedInstanced(instancedVAO, (uint32_t)transforms.size(), mesh.TriangleCount * 3);
    }
    else
    {
        RenderCommand::DrawArraysInstanced(mesh.VertexCount, (uint32_t)transforms.size());
    }
    instancedVAO->Unbind();
}

void Renderer::DrawLine(const glm::vec3& start, const glm::vec3& end, const glm::vec4& color)
{
    auto debugShader = m_Data->Shaders->LoadOrGet("ColliderDebug", "resources/shaders/collider_debug.chshader");
    if (!debugShader || !debugShader->GetShader())
    {
        return;
    }

    auto shader = debugShader->GetShader();
    shader->Bind();

    glm::mat4 vp = m_Data->CurrentProj * m_Data->CurrentView;
    shader->SetMatrix("u_ViewProj", vp);
    shader->SetMatrix("u_Transform", glm::mat4(1.0f));
    shader->SetVec4("u_Color", color);

    // Use a persistent buffer with abstraction
    float vertices[] = {start.x, start.y, start.z, end.x, end.y, end.z};

    if (!m_Data->LineVBO)
    {
        m_Data->LineVBO = VertexBuffer::Create(sizeof(vertices));
        m_Data->LineVBO->SetLayout({{ShaderDataType::Float3, "vertexPosition"}});
    }
    m_Data->LineVBO->SetData(vertices, sizeof(vertices));

    if (!m_Data->LineVAO)
    {
        m_Data->LineVAO = VertexArray::Create();
        m_Data->LineVAO->AddVertexBuffer(m_Data->LineVBO);
    }

    m_Data->LineVAO->Bind();
    RenderCommand::DrawLines(m_Data->LineVAO, 2);
    m_Data->LineVAO->Unbind();
}

void Renderer::DrawMeshWire(const Mesh& mesh, const glm::vec4& color, const glm::mat4& transform, bool useWireframe)
{
    auto debugShader = m_Data->Shaders->LoadOrGet("ColliderDebug", "resources/shaders/collider_debug.chshader");
    if (!debugShader || !debugShader->GetShader())
    {
        return;
    }

    auto shader = debugShader->GetShader();
    shader->Bind();

    // Set matrices and color with correct uniform names for ColliderDebug
    glm::mat4 vp = m_Data->CurrentProj * m_Data->CurrentView;
    shader->SetMatrix("u_ViewProj", vp);
    shader->SetMatrix("u_Transform", transform);

    // Apply transparency for solid mode
    glm::vec4 finalColor = color;
    if (!useWireframe)
    {
        finalColor.a *= 0.35f; // More subtle transparency
    }
    shader->SetVec4("u_Color", finalColor);

    if (useWireframe)
    {
        RenderCommand::SetPolygonMode(RendererAPI::PolygonMode::Line);
    }
    else
    {
        RenderCommand::SetPolygonMode(RendererAPI::PolygonMode::Fill);
    }

    // Render mesh geometry
    if (mesh.VAO)
    {
        RenderCommand::SetBlendMode(true);
        RenderCommand::SetBlendFunc(RendererAPI::BlendFactor::SrcAlpha, RendererAPI::BlendFactor::OneMinusSrcAlpha);

        // Depth Test is now disabled globally for debug overlays in SceneRenderer::RenderDebug.
        // We do not enable it here.

        // DrawMesh helper now handles Binding inside DrawIndexed/DrawArrays
        mesh.VAO->Bind();
        if (mesh.TriangleCount > 0)
        {
            RenderCommand::DrawIndexed(mesh.VAO, mesh.TriangleCount * 3);
        }
        else if (mesh.VAO->GetIndexBuffer() != nullptr)
        {
            RenderCommand::DrawIndexedLines(mesh.VAO, mesh.VAO->GetIndexBuffer()->GetCount());
        }
        else
        {
            RenderCommand::DrawLines(mesh.VAO, mesh.VertexCount);
        }
        mesh.VAO->Unbind();

        RenderCommand::SetBlendMode(false);
    }

    RenderCommand::SetPolygonMode(RendererAPI::PolygonMode::Fill);
}

void Renderer::DrawGrid(int slices, float spacing)
{
    // Draw grid manually using DrawLine
    float halfSize = (slices * spacing) / 2.0f;
    glm::vec4 color = {0.5f, 0.5f, 0.5f, 1.0f};

    for (int i = 0; i <= slices; i++)
    {
        float pos = -halfSize + (i * spacing);
        DrawLine({pos, 0, -halfSize}, {pos, 0, halfSize}, color);
        DrawLine({-halfSize, 0, pos}, {halfSize, 0, pos}, color);
    }
}

void Renderer::DrawInfiniteGrid(const Camera3D& camera, float spacing, const glm::vec4& color)
{
    auto& shaders = GetShaderLibrary();
    auto shaderAsset = shaders.LoadOrGet("Grid", "resources/shaders/grid.chshader");
    if (!shaderAsset || !shaderAsset->GetShader())
    {
        return;
    }

    shaderAsset->GetShader()->Bind();

    glm::mat4 mvp = m_Data->CurrentProj * m_Data->CurrentView;
    shaderAsset->GetShader()->SetMatrix("u_ViewProjection", mvp);

    // Position the plane slightly below Y=0 to prevent Z-fighting
    glm::vec3 planePos = {camera.Position.x, -0.005f, camera.Position.z};
    glm::mat4 model = glm::translate(glm::mat4(1.0f), planePos);
    shaderAsset->GetShader()->SetMatrix("matModel", model);

    shaderAsset->GetShader()->SetVec3("cameraPos", camera.Position);

    glm::vec4 col = color;
    shaderAsset->GetShader()->SetVec4("gridColor", col);
    shaderAsset->GetShader()->SetFloat("gridSize", spacing);
    shaderAsset->GetShader()->SetFloat("uTime", m_Data->Time);

    RenderCommand::SetBlendMode(true);
    RenderCommand::SetBlendFunc(RendererAPI::BlendFactor::SrcAlpha, RendererAPI::BlendFactor::OneMinusSrcAlpha);

    // Disable depth test for the infinite grid or it will be occluded by the floor
    RenderCommand::DisableDepthTest();

    // Use shared mesh from GeometryGenerator
    if (!m_Data->GridPlaneVAO)
    {
        float vertices[] = {-0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 0.5f,  -0.5f, 0.0f, 1.0f, 0.0f,
                            0.5f,  0.5f,  0.0f, 1.0f, 1.0f, -0.5f, 0.5f,  0.0f, 0.0f, 1.0f};
        uint32_t indices[] = {0, 1, 2, 2, 3, 0};

        auto vbo = VertexBuffer::Create(vertices, sizeof(vertices));
        vbo->SetLayout({{ShaderDataType::Float3, "a_Position"}, {ShaderDataType::Float2, "a_TexCoord"}});

        m_Data->GridPlaneVAO = VertexArray::Create();
        m_Data->GridPlaneVAO->AddVertexBuffer(vbo);
        auto ibo = IndexBuffer::Create(indices, 6);
        m_Data->GridPlaneVAO->SetIndexBuffer(ibo);

        // Scale the grid plane vbo if needed, or just use a large transform.
        // The original used GenerateQuad(15000.0f), so we scale here.
    }

    glm::mat4 scale = glm::scale(model, glm::vec3(15000.0f, 1.0f, 15000.0f));
    shaderAsset->GetShader()->SetMatrix("matModel", scale);

    m_Data->GridPlaneVAO->Bind();
    RenderCommand::DrawIndexed(m_Data->GridPlaneVAO, 6);
    m_Data->GridPlaneVAO->Unbind();
    RenderCommand::EnableDepthTest();
}

void Renderer::DrawSkybox(uint32_t textureId, int skyboxMode, bool isHDR, float exposure, float brightness,
                          float contrast, const Camera3D& camera, bool flipped)
{
    if (textureId == 0)
    {
        return;
    }

    skyboxMode = std::clamp(skyboxMode, 0, 2);

    auto shaderAsset =
        (skyboxMode == 2)
            ? m_Data->Shaders->LoadOrGet("SkyboxCubemap", "resources/shaders/skybox_cubemap.chshader")
            : (skyboxMode == 1
                   ? m_Data->Shaders->LoadOrGet("SkyboxCross", "resources/shaders/skybox_cross.chshader")
                   : m_Data->Shaders->LoadOrGet("Skybox", "resources/shaders/skybox.chshader"));
    if (!shaderAsset || !shaderAsset->GetShader())
    {
        return;
    }

    // 1. Prepare Render State
    RenderCommand::SetDepthFunc(RendererAPI::DepthFunc::LEqual);
    RenderCommand::SetCullMode(RendererAPI::CullMode::None);
    RenderCommand::DisableDepthMask();

    // 2. Setup Uniforms
    shaderAsset->GetShader()->Bind();

    // Always remove translation from view matrix for skybox
    glm::mat4 view = glm::mat4(glm::mat3(m_Data->CurrentView));
    shaderAsset->GetShader()->SetMatrix("u_View", view);
    shaderAsset->GetShader()->SetMatrix("u_Projection", m_Data->CurrentProj);

    shaderAsset->GetShader()->SetFloat("u_Exposure", exposure);
    shaderAsset->GetShader()->SetFloat("u_Brightness", brightness);
    shaderAsset->GetShader()->SetFloat("u_Contrast", contrast);
    shaderAsset->GetShader()->SetInt("u_IsHDR", isHDR ? 1 : 0);
    shaderAsset->GetShader()->SetInt("u_VFlipped", flipped ? 1 : 0);

    ApplyFogUniforms(shaderAsset);

    // 3. Bind Textures and Draw Mesh
    if (skyboxMode == 2)
    {
        RenderCommand::SetTexture(0, textureId, true);
        shaderAsset->GetShader()->SetInt("u_Cubemap", 0);

        if (m_Data->Skybox.SkyboxCubeModel && !m_Data->Skybox.SkyboxCubeModel->Meshes.empty())
        {
            auto& mesh = m_Data->Skybox.SkyboxCubeModel->Meshes[0];
            mesh.VAO->Bind();
            RenderCommand::DrawIndexed(mesh.VAO, 36);
            mesh.VAO->Unbind();
        }
    }
    else if (skyboxMode == 1)
    {
        RenderCommand::SetTexture(0, textureId);
        shaderAsset->GetShader()->SetInt("u_CrossMap", 0);

        if (m_Data->Skybox.SkyboxCubeModel && !m_Data->Skybox.SkyboxCubeModel->Meshes.empty())
        {
            auto& mesh = m_Data->Skybox.SkyboxCubeModel->Meshes[0];
            mesh.VAO->Bind();
            RenderCommand::DrawIndexed(mesh.VAO, 36);
            mesh.VAO->Unbind();
        }
    }
    if (skyboxMode == 0)
    {
        RenderCommand::SetTexture(0, textureId);
        shaderAsset->GetShader()->SetInt("u_Panorama", 0);

        if (m_Data->Skybox.SkyboxSphereModel && !m_Data->Skybox.SkyboxSphereModel->Meshes.empty())
        {
            auto& mesh = m_Data->Skybox.SkyboxSphereModel->Meshes[0];
            mesh.VAO->Bind();
            RenderCommand::DrawIndexed(mesh.VAO, mesh.TriangleCount * 3);
            mesh.VAO->Unbind();
        }
    }

    // 4. Restore Render State
    RenderCommand::SetDepthFunc(RendererAPI::DepthFunc::Less);
    RenderCommand::SetCullMode(RendererAPI::CullMode::Back);
    RenderCommand::EnableDepthMask();
}

void Renderer::DrawBillboard(const Camera3D& camera, uint32_t textureId, const glm::vec3& position, float size,
                             const glm::vec4& tint)
{
    auto unlitShaderAsset = m_Data->Shaders->LoadOrGet("Unlit", "resources/shaders/unlit.chshader");
    if (!unlitShaderAsset || !unlitShaderAsset->GetShader() || textureId == 0)
    {
        return;
    }

    auto shader = unlitShaderAsset->GetShader();
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

    shader->SetMatrix("mvp", m_Data->CurrentProj * m_Data->CurrentView * model);
    shader->SetMatrix("matModel", model);
    shader->SetMatrix("matNormal", glm::transpose(glm::inverse(model)));
    shader->SetVec3("viewPos", camera.Position);
    shader->SetVec4("colDiffuse", tint);
    shader->SetVec4("colEmissive", glm::vec4(0.0f));
    shader->SetInt("useTexture", 1);
    shader->SetInt("useEmissiveTexture", 0);
    shader->SetFloat("emissiveIntensity", 0.0f);
    shader->SetInt("useSkinning", 0);

    RenderCommand::SetTexture(0, textureId);
    shader->SetInt("texture0", 0);

    const bool blendWasEnabled = RenderCommand::IsBlendEnabled();
    const bool cullWasEnabled = RenderCommand::IsCullFaceEnabled();

    RenderCommand::SetBlendMode(true);
    RenderCommand::SetBlendFunc(RendererAPI::BlendFactor::SrcAlpha, RendererAPI::BlendFactor::OneMinusSrcAlpha);
    RenderCommand::SetCullMode(RendererAPI::CullMode::None);

    RenderCommand::SetBlendFunc(RendererAPI::BlendFactor::SrcAlpha, RendererAPI::BlendFactor::OneMinusSrcAlpha);
    RenderCommand::SetCullMode(RendererAPI::CullMode::None);

    if (!m_Data->BillboardVAO)
    {
        float vertices[] = {
            -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 0.5f,  -0.5f, 0.0f, 1.0f, 0.0f,
            0.5f,  0.5f,  0.0f, 1.0f, 1.0f, -0.5f, 0.5f,  0.0f, 0.0f, 1.0f,
        };
        uint32_t indices[] = {0, 1, 2, 2, 3, 0};

        auto vbo = VertexBuffer::Create(vertices, sizeof(vertices));
        vbo->SetLayout({{ShaderDataType::Float3, "a_Position"}, {ShaderDataType::Float2, "a_TexCoord"}});

        m_Data->BillboardVAO = VertexArray::Create();
        m_Data->BillboardVAO->AddVertexBuffer(vbo);
        auto ibo = IndexBuffer::Create(indices, 6);
        m_Data->BillboardVAO->SetIndexBuffer(ibo);
    }

    m_Data->BillboardVAO->Bind();
    RenderCommand::DrawIndexed(m_Data->BillboardVAO, 6);
    m_Data->BillboardVAO->Unbind();

    RenderCommand::SetCullMode(cullWasEnabled ? RendererAPI::CullMode::Back : RendererAPI::CullMode::None);
    RenderCommand::SetBlendMode(blendWasEnabled);
}

void Renderer::DrawCubeWires(const glm::mat4& transform, const glm::vec3& size, const glm::vec4& color,
                             bool useWireframe)
{
    glm::mat4 model = transform * glm::scale(glm::mat4(1.0f), size);
    if (useWireframe && m_Data->Resources.WireCubeModel && !m_Data->Resources.WireCubeModel->Meshes.empty())
    {
        DrawMeshWire(m_Data->Resources.WireCubeModel->Meshes[0], color, model, true);
    }
    else if (m_Data->Resources.UnitCubeModel && !m_Data->Resources.UnitCubeModel->Meshes.empty())
    {
        DrawMeshWire(m_Data->Resources.UnitCubeModel->Meshes[0], color, model, useWireframe);
    }
}

void Renderer::DrawCapsuleWires(const glm::mat4& transform, float radius, float height, const glm::vec4& color,
                                bool useWireframe)
{
    glm::mat4 model = transform * glm::scale(glm::mat4(1.0f), glm::vec3(radius, height, radius));
    if (m_Data->Resources.UnitCapsuleModel)
    {
        for (auto& mesh : m_Data->Resources.UnitCapsuleModel->Meshes)
        {
            DrawMeshWire(mesh, color, model, useWireframe);
        }
    }
}

void Renderer::DrawSphereWires(const glm::mat4& transform, float radius, const glm::vec4& color, bool useWireframe)
{
    glm::mat4 model = transform * glm::scale(glm::mat4(1.0f), glm::vec3(radius));
    if (m_Data->Resources.UnitSphereModel)
    {
        for (auto& mesh : m_Data->Resources.UnitSphereModel->Meshes)
        {
            DrawMeshWire(mesh, color, model, useWireframe);
        }
    }
}

void Renderer::ApplyPostProcessing(uint32_t screenTextureId, uint32_t depthTextureId, const Camera3D& camera,
                                   ShaderAsset* overrideShader, const std::vector<ShaderUniform>& uniforms)
{
    auto shaderAsset = overrideShader;
    if (!shaderAsset)
    {
        shaderAsset = m_Data->Shaders->LoadOrGet("PostProcess", "resources/shaders/post_process.chshader").get();
    }

    if (shaderAsset && shaderAsset->GetShader())
    {
        auto shader = shaderAsset->GetShader();
        shader->Bind();

        float diagIntensity = 0.0f;
        for (const auto& u : uniforms)
        {
            if (u.Name == "uIntensity")
            {
                diagIntensity = u.Value[0];
            }
        }
        if (diagIntensity > 0.001f)
        {
            CH_CORE_INFO("[RENDER DIAG] Applying shader '{}', Intensity={}", shaderAsset->GetPath(), diagIntensity);
        }

        // 1. Set System Uniforms
        glm::mat4 identity = glm::mat4(1.0f);
        shader->SetMatrix("mvp", identity);

        glm::mat4 invViewProj = glm::inverse(m_Data->CurrentProj * m_Data->CurrentView);
        shader->SetMatrix("matInverseViewProj", invViewProj);
        shader->SetVec3("viewPos", camera.Position);

        shader->SetFloat("uTimeF", (float)m_Data->Time.GetSeconds());
        shader->SetFloat("uTime", (float)m_Data->Time.GetSeconds()); // system uniform
        shader->SetFloat("time", (float)m_Data->Time.GetSeconds());  // alias for custom shaders
        shader->SetFloat("uExposure", m_Data->Lighting.CurrentLighting.Exposure);
        shader->SetFloat("uGamma", m_Data->Lighting.CurrentLighting.Gamma);

        auto handle =
            ServiceLocator::Get<AssetManager>()->ResolveToHandle(shaderAsset->GetPath(), ShaderAsset::GetStaticType());
        ApplyFogUniforms(ServiceLocator::Get<AssetManager>()->GetAsset<ShaderAsset>(handle));

        // 2. Set Custom Uniforms (if any)
        for (const auto& u : uniforms)
        {
            switch (u.Type)
            {
            case 0:
                shader->SetFloat(u.Name, u.Value[0]);
                break;
            case 1:
                shader->SetVec2(u.Name, *(glm::vec2*)u.Value);
                break;
            case 2:
                shader->SetVec3(u.Name, *(glm::vec3*)u.Value);
                break;
            case 3:
                shader->SetVec4(u.Name, *(glm::vec4*)u.Value);
                break;
            case 4:
                shader->SetVec4(u.Name, *(glm::vec4*)u.Value);
                break; // Color is Vec4
            }
        }

        // 3. Bind Textures
        RenderCommand::SetTexture(0, screenTextureId);
        shader->SetInt("texture0", 0);

        RenderCommand::SetTexture(1, depthTextureId);
        shader->SetInt("texture1", 1);

        RenderCommand::DisableDepthTest();

        if (m_Data->FullscreenQuadVAO)
        {
            m_Data->FullscreenQuadVAO->Bind();
            RenderCommand::DrawIndexed(m_Data->FullscreenQuadVAO, 6);
            m_Data->FullscreenQuadVAO->Unbind();
        }

        RenderCommand::SetBlendMode(false);
        RenderCommand::EnableDepthTest();
        RenderCommand::SetCullMode(RendererAPI::CullMode::Back);
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

void Renderer::ApplyEnvironment(const EnvironmentParameters& settings)
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

void Renderer::Update(Timestep ts)
{
    UpdateTime(ts);
}

void Renderer::ApplyFogUniforms(const std::shared_ptr<ShaderAsset>& shader)
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
}

void Renderer::InitializeSkybox()
{
    m_Data->Skybox.SkyboxCubeModel = std::make_unique<Model>();
    m_Data->Skybox.SkyboxCubeModel->Meshes.push_back(GeometryGenerator::GenerateUnitCube());

    m_Data->Skybox.SkyboxSphereModel = std::make_unique<Model>();
    m_Data->Skybox.SkyboxSphereModel->Meshes.push_back(GeometryGenerator::GenerateSphere(50.0f, 64, 64));
}

void Renderer::InitializeResources()
{
    m_Data->Resources.UnitCubeModel = std::make_unique<Model>();
    m_Data->Resources.UnitCubeModel->Meshes.push_back(GeometryGenerator::GenerateUnitCube());

    m_Data->Resources.UnitSphereModel = std::make_unique<Model>();
    m_Data->Resources.UnitSphereModel->Meshes.push_back(GeometryGenerator::GenerateSphere(1.0f, 32, 32));

    m_Data->Resources.UnitCapsuleModel = std::make_unique<Model>();
    m_Data->Resources.UnitCapsuleModel->Meshes.push_back(GeometryGenerator::GenerateCapsule(1.0f, 2.0f, 32, 32));

    m_Data->Resources.WireCubeModel = std::make_unique<Model>();
    m_Data->Resources.WireCubeModel->Meshes.push_back(GeometryGenerator::GenerateWireCube());
}

void Renderer::CleanupSkybox()
{
    m_Data->Skybox.SkyboxCubeModel.reset();
    m_Data->Skybox.SkyboxSphereModel.reset();

    m_Data->Skybox.CachedCubemap.reset();
}

void Renderer::CleanupResources()
{
    m_Data->Resources.UnitCubeModel.reset();
    m_Data->Resources.UnitSphereModel.reset();
    m_Data->Resources.UnitCapsuleModel.reset();
    m_Data->Resources.WireCubeModel.reset();
}

void Renderer::DrawSprite(uint32_t textureId, const glm::mat4& transform, const glm::vec4& tint, bool flipX, bool flipY)
{
    if (textureId == 0)
    {
        return;
    }

    auto shaderAsset = m_Data->Shaders->LoadOrGet("Sprite", "resources/shaders/sprite.chshader");
    if (!shaderAsset || !shaderAsset->GetShader())
    {
        return;
    }

    auto shader = shaderAsset->GetShader();
    shader->Bind();

    shader->SetMatrix("mvp", m_Data->CurrentProj * m_Data->CurrentView * transform);
    shader->SetMatrix("matModel", transform);
    shader->SetVec4("u_Tint", tint);
    shader->SetVec2("u_Flip", glm::vec2(flipX ? 1.0f : 0.0f, flipY ? 1.0f : 0.0f));

    const bool blendWasEnabled = RenderCommand::IsBlendEnabled();
    const bool cullWasEnabled = RenderCommand::IsCullFaceEnabled();

    RenderCommand::SetBlendMode(true);
    RenderCommand::SetBlendFunc(RendererAPI::BlendFactor::SrcAlpha, RendererAPI::BlendFactor::OneMinusSrcAlpha);
    RenderCommand::SetCullMode(RendererAPI::CullMode::None);

    if (!m_Data->SpriteVAO)
    {
        float vertices[] = {
            -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 0.5f,  -0.5f, 0.0f, 1.0f, 0.0f,
            0.5f,  0.5f,  0.0f, 1.0f, 1.0f, -0.5f, 0.5f,  0.0f, 0.0f, 1.0f,
        };
        uint32_t indices[] = {0, 1, 2, 2, 3, 0};

        auto vbo = VertexBuffer::Create(vertices, sizeof(vertices));
        vbo->SetLayout({{ShaderDataType::Float3, "a_Position"}, {ShaderDataType::Float2, "a_TexCoord"}});

        m_Data->SpriteVAO = VertexArray::Create();
        m_Data->SpriteVAO->AddVertexBuffer(vbo);
        auto ibo = IndexBuffer::Create(indices, 6);
        m_Data->SpriteVAO->SetIndexBuffer(ibo);
    }

    if (m_Data->SpriteVAO)
    {
        m_Data->SpriteVAO->Bind();
        RenderCommand::DrawIndexed(m_Data->SpriteVAO, 6);
        m_Data->SpriteVAO->Unbind();
    }

    RenderCommand::SetCullMode(cullWasEnabled ? RendererAPI::CullMode::Back : RendererAPI::CullMode::None);
    RenderCommand::SetBlendMode(blendWasEnabled);
}

} // namespace Chained