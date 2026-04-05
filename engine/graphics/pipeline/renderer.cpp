#include "engine/graphics/pipeline/renderer.h"
#include "engine/core/application.h"
#include "engine/core/log.h"
#include "engine/graphics/api/framebuffer.h"
#include "engine/graphics/pipeline/render_command.h"

#include "engine/core/assets/asset_manager.h"
#include "engine/graphics/assets/shader_asset.h"
#include "engine/graphics/assets/texture_asset.h"
#include "engine/graphics/pipeline/geometry_generator.h"

#include <algorithm>
#include <glad/gl.h>
#include <glm/gtc/type_ptr.hpp>
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
    return *s_Instance;
}

void Renderer::Init()
{
    if (!s_Instance)
    {
        s_Instance = new Renderer();
    }
    s_Instance->InternalInit();
}

void Renderer::LoadEngineResources()
{
    auto& shaders = Get().GetShaderLibrary();

    auto loadShader = [&](const std::string& name, const std::string& path) { shaders.Load(name, path); };

    loadShader("Lighting", "resources/shaders/lighting.chshader");
    loadShader("Skybox", "resources/shaders/skybox.chshader");
    loadShader("SkyboxCross", "resources/shaders/skybox_cross.chshader");
    loadShader("Unlit", "resources/shaders/unlit.chshader");
    loadShader("CubemapGen", "resources/shaders/cubemap.chshader");
    loadShader("SkyboxCubemap", "resources/shaders/skybox_cubemap.chshader");
    loadShader("PostProcess", "resources/shaders/post_process.chshader");
    loadShader("Grid", "resources/shaders/grid.chshader");
    loadShader("ColliderDebug", "resources/shaders/collider_debug.chshader");

    CH_CORE_INFO("[Renderer] LoadEngineResources done. {} shader(s) loaded.", shaders.GetNames().size());
}

void Renderer::InternalInit()
{

    if (Application::Get().GetSpecification().Headless)
    {
        CH_CORE_INFO("[Renderer] Headless mode enabled, skipping OpenGL initialization.");
        return;
    }

    RenderCommand::Initialize();

    // Initialize SSBO for lights using raw OpenGL
    glGenBuffers(1, &m_Data->Lighting.LightSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_Data->Lighting.LightSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(RenderLight) * LightingData::MaxLights, nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

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

    InitializeResources();
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

    if (Application::Get().GetSpecification().Headless)
    {
        return;
    }

    CleanupResources();
    CleanupSkybox();

    if (m_Data->Lighting.LightSSBO > 0)
    {
        glDeleteBuffers(1, &m_Data->Lighting.LightSSBO);
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

void Renderer::BeginScene(const Camera3D& camera, float nearClip, float farClip)
{
    m_Data->CurrentCameraPosition = camera.Position;

    // Update light SSBO once per frame
    if (m_Data->Lighting.LightsDirty)
    {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_Data->Lighting.LightSSBO);
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(RenderLight) * LightingData::MaxLights,
                        m_Data->Lighting.Lights);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        m_Data->Lighting.LightsDirty = false;
    }

    // Bind lighting uniforms to the default lighting shader if it exists
    auto lightingShaderAsset = m_Data->Shaders->Exists("Lighting") ? m_Data->Shaders->Get("Lighting") : nullptr;
    if (lightingShaderAsset && lightingShaderAsset->GetShader())
    {
        lightingShaderAsset->GetShader()->Bind();

        float time = m_Data->Time;
        float diagMode = m_Data->DiagnosticMode;
        int lightCount = m_Data->LightCount;
        float ambient = m_Data->Lighting.CurrentLighting.Ambient;
        float exposure = m_Data->Lighting.CurrentLighting.Exposure;
        float gamma = m_Data->Lighting.CurrentLighting.Gamma;

        lightingShaderAsset->GetShader()->SetVec3("viewPos", camera.Position);
        lightingShaderAsset->GetShader()->SetFloat("uTime", time);
        lightingShaderAsset->GetShader()->SetFloat("uMode", diagMode);

        lightingShaderAsset->GetShader()->SetVec3("lightDir", m_Data->Lighting.CurrentLighting.Direction);

        glm::vec4 lightColor = {m_Data->Lighting.CurrentLighting.LightColor.r / 255.0f,
                                m_Data->Lighting.CurrentLighting.LightColor.g / 255.0f,
                                m_Data->Lighting.CurrentLighting.LightColor.b / 255.0f,
                                m_Data->Lighting.CurrentLighting.LightColor.a / 255.0f};
        lightingShaderAsset->GetShader()->SetVec4("lightColor", lightColor);

        lightingShaderAsset->GetShader()->SetFloat("ambient", ambient);

        glm::vec4 skyColor = lightColor;
        skyColor.w = ambient * 0.35f;
        lightingShaderAsset->GetShader()->SetVec4("skyAmbientColor", skyColor);

        lightingShaderAsset->GetShader()->SetInt("uLightCount", lightCount);
        lightingShaderAsset->GetShader()->SetFloat("uExposure", exposure);
        lightingShaderAsset->GetShader()->SetFloat("uGamma", gamma);

        ApplyFogUniforms(lightingShaderAsset);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_Data->Lighting.LightSSBO);
        m_Data->CurrentShaderId = lightingShaderAsset->GetShader()->GetRendererID();
    }

    // --- Direct glm::mat4 Management (Pure OpenGL style) ---
    // 1. Calculate View Transform
    m_Data->CurrentView = glm::lookAt(camera.Position, camera.Target, camera.Up);

    // 2. Calculate Projection Transform
    int width = Application::Get().GetWindow().GetWidth();
    int height = Application::Get().GetWindow().GetHeight();
    float aspect = (height > 0) ? (float)width / (float)height : 1.0f;

    if (camera.Projection == 0 /* CAMERA_PERSPECTIVE */)
    {
        m_Data->CurrentProj = glm::perspective(glm::radians(camera.Fovy), aspect, nearClip, farClip);
    }
    else
    {
        float top = camera.Fovy / 2.0f;
        float right = top * aspect;
        m_Data->CurrentProj = glm::ortho(-right, right, -top, top, nearClip, farClip);
    }
}

void Renderer::EndScene()
{
    m_Data->CurrentShaderId = 0;
}

void Renderer::Clear(const glm::vec4& color)
{
    CHEngine::Color chColor((unsigned char)(color.r * 255), (unsigned char)(color.g * 255),
                            (unsigned char)(color.b * 255), (unsigned char)(color.a * 255));
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

    shaderAsset->GetShader()->Bind();

    // Set matrices
    shaderAsset->GetShader()->SetMatrix("matModel", transform);
    shaderAsset->GetShader()->SetMatrix("matView", m_Data->CurrentView);
    shaderAsset->GetShader()->SetMatrix("matProjection", m_Data->CurrentProj);

    glm::mat4 matNormal = glm::transpose(glm::inverse(transform));
    shaderAsset->GetShader()->SetMatrix("matNormal", matNormal);

    glm::mat4 mvp = m_Data->CurrentProj * m_Data->CurrentView * transform;
    shaderAsset->GetShader()->SetMatrix("mvp", mvp);

    // Bind VAO and Draw
    if (mesh.VAO)
    {
        mesh.VAO->Bind();
        if (mesh.TriangleCount > 0)
        {
            glDrawElements(GL_TRIANGLES, mesh.TriangleCount * 3, GL_UNSIGNED_INT, 0);
        }
        else
        {
            glDrawArrays(GL_TRIANGLES, 0, mesh.VertexCount);
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

    shaderAsset->GetShader()->Bind();

    // Set up matrices
    glm::mat4 mvp = m_Data->CurrentProj * m_Data->CurrentView;
    shaderAsset->GetShader()->SetMatrix("u_ViewProjection", mvp);
    shaderAsset->GetShader()->SetMatrix("u_Transform", glm::mat4(1.0f));

    // Create instance buffer with all transforms
    uint32_t instanceVBO = 0;
    glGenBuffers(1, &instanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, transforms.size() * sizeof(glm::mat4), transforms.data(), GL_STATIC_DRAW);

    // Bind the mesh VAO
    mesh.VAO->Bind();
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);

    // Set up instance vertex attributes for the model matrix (4 vec4 attributes per mat4)
    size_t vec4Size = sizeof(glm::vec4);
    for (int i = 0; i < 4; i++)
    {
        glEnableVertexAttribArray(5 + i);
        glVertexAttribPointer(5 + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(i * vec4Size));
        glVertexAttribDivisor(5 + i, 1); // One matrix per instance
    }

    // Draw with instances
    if (mesh.TriangleCount > 0)
    {
        glDrawElementsInstanced(GL_TRIANGLES, mesh.TriangleCount * 3, GL_UNSIGNED_INT, 0, (GLsizei)transforms.size());
    }
    else
    {
        glDrawArraysInstanced(GL_TRIANGLES, 0, mesh.VertexCount, (GLsizei)transforms.size());
    }

    // Cleanup instance attributes
    for (int i = 0; i < 4; i++)
    {
        glVertexAttribDivisor(5 + i, 0);
        glDisableVertexAttribArray(5 + i);
    }

    mesh.VAO->Unbind();
    glDeleteBuffers(1, &instanceVBO);
}

void Renderer::DrawLine(const glm::vec3& start, const glm::vec3& end, const glm::vec4& color)
{
    auto debugShader = m_Data->Shaders->Get("ColliderDebug");
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

    // Immediate mode replacement or small buffer
    float vertices[] = {start.x, start.y, start.z, end.x, end.y, end.z};
    uint32_t vbo, vao;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STREAM_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);

    glDrawArrays(GL_LINES, 0, 2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
}

void Renderer::DrawMeshWire(const Mesh& mesh, const glm::vec4& color, const glm::mat4& transform, bool useWireframe)
{
    auto debugShader = m_Data->Shaders->Get("ColliderDebug");
    if (!debugShader || !debugShader->GetShader())
    {
        return;
    }

    if (useWireframe)
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    else
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    auto shader = debugShader->GetShader();
    shader->Bind();

    // Set matrices and color with correct uniform names for ColliderDebug
    glm::mat4 vp = m_Data->CurrentProj * m_Data->CurrentView;
    shader->SetMatrix("u_ViewProj", vp);
    shader->SetMatrix("u_Transform", transform);
    shader->SetVec4("u_Color", color);

    // Render mesh geometry
    if (mesh.VAO)
    {
        mesh.VAO->Bind();
        if (mesh.TriangleCount > 0)
        {
            glDrawElements(GL_TRIANGLES, mesh.TriangleCount * 3, GL_UNSIGNED_INT, 0);
        }
        else
        {
            glDrawArrays(GL_TRIANGLES, 0, mesh.VertexCount);
        }
        mesh.VAO->Unbind();
    }

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
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
    if (!shaders.Exists("Grid"))
    {
        return;
    }
    auto shaderAsset = shaders.Get("Grid");

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

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Use shared mesh from GeometryGenerator
    static Mesh gridPlane;
    if (!gridPlane.VAO)
    {
        gridPlane = GeometryGenerator::GenerateQuad(15000.0f);
    }

    gridPlane.VAO->Bind();
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    gridPlane.VAO->Unbind();
}

void Renderer::DrawSkybox(uint32_t textureId, int skyboxMode, bool isHDR, float exposure, float brightness,
                          float contrast, const Camera3D& camera)
{
    if (textureId == 0)
    {
        return;
    }

    skyboxMode = std::clamp(skyboxMode, 0, 2);

    auto shaderAsset = (skyboxMode == 2)
                           ? m_Data->Shaders->Get("SkyboxCubemap")
                           : (skyboxMode == 1 ? m_Data->Shaders->Get("SkyboxCross") : m_Data->Shaders->Get("Skybox"));
    if (!shaderAsset)
    {
        return;
    }

    // 1. Prepare Render State
    RenderCommand::SetDepthFunc(RendererAPI::DepthFunc::LEqual);
    RenderCommand::SetCullMode(RendererAPI::CullMode::None);
    RenderCommand::DisableDepthMask(); // Використовую твій метод

    // 2. Setup Uniforms
    shaderAsset->GetShader()->Bind();

    // Always remove translation from view matrix for skybox (Виправлено: беремо з переданої camera)
    glm::mat4 view = glm::mat4(glm::mat3(m_Data->CurrentView));
    shaderAsset->GetShader()->SetMatrix("u_View", view);
    shaderAsset->GetShader()->SetMatrix("u_Projection", m_Data->CurrentProj);

    shaderAsset->GetShader()->SetFloat("u_Exposure", exposure);
    shaderAsset->GetShader()->SetFloat("u_Brightness", brightness);
    shaderAsset->GetShader()->SetFloat("u_Contrast", contrast);

    shaderAsset->GetShader()->SetInt("u_IsHDR", isHDR ? 1 : 0);
    shaderAsset->GetShader()->SetInt("u_VFlipped", skyboxMode == 2 ? 0 : 1);

    ApplyFogUniforms(shaderAsset);

    // 3. Bind Textures and Draw Mesh
    if (skyboxMode == 2)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, textureId);
        shaderAsset->GetShader()->SetInt("u_Cubemap", 0);

        if (m_Data->Skybox.SkyboxCubeModel && !m_Data->Skybox.SkyboxCubeModel->Meshes.empty())
        {
            auto& mesh = m_Data->Skybox.SkyboxCubeModel->Meshes[0];
            mesh.VAO->Bind();
            glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
            mesh.VAO->Unbind();
        }
    }
    else if (skyboxMode == 1)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureId);
        shaderAsset->GetShader()->SetInt("u_CrossMap", 0);

        if (m_Data->Skybox.SkyboxCubeModel && !m_Data->Skybox.SkyboxCubeModel->Meshes.empty())
        {
            auto& mesh = m_Data->Skybox.SkyboxCubeModel->Meshes[0];
            mesh.VAO->Bind();
            glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
            mesh.VAO->Unbind();
        }
    }
    else
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureId);
        shaderAsset->GetShader()->SetInt("u_Panorama", 0);

        if (m_Data->Skybox.SkyboxSphereModel && !m_Data->Skybox.SkyboxSphereModel->Meshes.empty())
        {
            auto& mesh = m_Data->Skybox.SkyboxSphereModel->Meshes[0];
            mesh.VAO->Bind();
            glDrawElements(GL_TRIANGLES, mesh.TriangleCount * 3, GL_UNSIGNED_INT, 0);
            mesh.VAO->Unbind();
        }
    }

    // 4. Restore Render State (Виправлено: робимо через API рушія)
    RenderCommand::SetDepthFunc(RendererAPI::DepthFunc::Less);
    RenderCommand::SetCullMode(RendererAPI::CullMode::Back);
    RenderCommand::EnableDepthMask(); // Якщо в тебе DisableDepthMask, то має бути і Enable, або SetDepthMask(true)
}

void Renderer::DrawBillboard(const Camera3D& camera, uint32_t textureId, const glm::vec3& position, float size,
                             const glm::vec4& tint)
{
    // Custom billboard rendering in OpenGL
    // We can use a simple quad and rotate it to face the camera
    auto unlitShader = m_Data->Shaders->Exists("Unlit") ? m_Data->Shaders->Get("Unlit") : nullptr;
    if (unlitShader && unlitShader->GetShader())
    {
        unlitShader->GetShader()->Bind();

        // Calculate billboard transform
        glm::vec3 look = glm::normalize(camera.Position - position);
        glm::vec3 right = glm::normalize(glm::cross(camera.Up, look));
        glm::vec3 up = glm::cross(look, right);

        glm::mat4 model = glm::mat4(1.0f);
        model[0] = glm::vec4(right * size, 0.0f);
        model[1] = glm::vec4(up * size, 0.0f);
        model[2] = glm::vec4(look * size, 0.0f);
        model[3] = glm::vec4(position, 1.0f);

        glm::mat4 mvp = m_Data->CurrentProj * m_Data->CurrentView * model;
        unlitShader->GetShader()->SetMatrix("u_ViewProjection", mvp);
        unlitShader->GetShader()->SetVec4("u_Color", tint);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureId);
        unlitShader->GetShader()->SetInt("u_Texture", 0);

        // Draw quad (Reuse planeVAO from DrawInfiniteGrid or create a localized one)
        static uint32_t quadVAO = 0;
        if (quadVAO == 0)
        {
            float vertices[] = {-0.5f, -0.5f, 0, 0, 0, 0.5f, -0.5f, 0, 1, 0, 0.5f, 0.5f, 0, 1, 1, -0.5f, 0.5f, 0, 0, 1};
            uint32_t vbo;
            glGenVertexArrays(1, &quadVAO);
            glGenBuffers(1, &vbo);
            glBindVertexArray(quadVAO);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), 0);
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        }

        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    }
}

void Renderer::DrawCubeWires(const glm::mat4& transform, const glm::vec3& size, const glm::vec4& color)
{
    auto shaderAsset = m_Data->Shaders->Get("ColliderDebug");
    if (!shaderAsset || !shaderAsset->GetShader()) return;

    auto shader = shaderAsset->GetShader();
    shader->Bind();
    shader->SetMatrix("u_ViewProj", m_Data->CurrentProj * m_Data->CurrentView);
    shader->SetMatrix("u_Transform", transform * glm::scale(glm::mat4(1.0f), size));
    shader->SetVec4("u_Color", color);

    if (m_Data->Resources.WireCubeModel)
    {
        for (auto& mesh : m_Data->Resources.WireCubeModel->Meshes)
        {
            mesh.VAO->Bind();
            glDrawElements(GL_LINES, mesh.VAO->GetIndexBuffer()->GetCount(), GL_UNSIGNED_INT, 0);
        }
    }
}

void Renderer::DrawCapsuleWires(const glm::mat4& transform, float radius, float height, const glm::vec4& color)
{
    // Simplified: Draw a box representing the capsule for now, until we add a proper capsule mesh
    DrawCubeWires(transform, glm::vec3(radius * 2.0f, height, radius * 2.0f), color);
}

void Renderer::DrawSphereWires(const glm::mat4& transform, float radius, const glm::vec4& color)
{
    auto shaderAsset = m_Data->Shaders->Get("ColliderDebug");
    if (!shaderAsset || !shaderAsset->GetShader()) return;

    auto shader = shaderAsset->GetShader();
    shader->Bind();
    shader->SetMatrix("u_ViewProj", m_Data->CurrentProj * m_Data->CurrentView);
    shader->SetMatrix("u_Transform", transform * glm::scale(glm::mat4(1.0f), glm::vec3(radius)));
    shader->SetVec4("u_Color", color);

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    if (m_Data->Resources.UnitSphereModel)
    {
        for (auto& mesh : m_Data->Resources.UnitSphereModel->Meshes)
        {
            mesh.VAO->Bind();
            glDrawElements(GL_TRIANGLES, mesh.VAO->GetIndexBuffer()->GetCount(), GL_UNSIGNED_INT, 0);
        }
    }
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void Renderer::ApplyPostProcessing(uint32_t screenTextureId, uint32_t depthTextureId, const Camera3D& camera)
{
    if (m_Data->Shaders->Exists("PostProcess"))
    {
        auto shaderAsset = m_Data->Shaders->Get("PostProcess");
        if (shaderAsset && shaderAsset->GetShader())
        {
            shaderAsset->GetShader()->Bind();

            // Fullscreen quad doesn't need transformation, set MVP to identity
            glm::mat4 identity = glm::mat4(1.0f);
            shaderAsset->GetShader()->SetMatrix("mvp", identity);

            glm::mat4 invViewProj = glm::inverse(m_Data->CurrentProj * m_Data->CurrentView);
            shaderAsset->GetShader()->SetMatrix("matInverseViewProj", invViewProj);
            shaderAsset->GetShader()->SetVec3("viewPos", camera.Position);

            shaderAsset->GetShader()->SetFloat("uTime", m_Data->Time);
            shaderAsset->GetShader()->SetFloat("uExposure", m_Data->Lighting.CurrentLighting.Exposure);
            shaderAsset->GetShader()->SetFloat("uGamma", m_Data->Lighting.CurrentLighting.Gamma);

            ApplyFogUniforms(shaderAsset);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, screenTextureId);
            shaderAsset->GetShader()->SetInt("texture0", 0);

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, depthTextureId);
            shaderAsset->GetShader()->SetInt("texture1", 1);
        }

        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);

        if (m_Data->FullscreenQuadVAO)
        {
            m_Data->FullscreenQuadVAO->Bind();
            RenderCommand::DrawIndexed(m_Data->FullscreenQuadVAO);
            m_Data->FullscreenQuadVAO->Unbind();
        }

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
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
    m_Data->Resources.WireCubeModel.reset();
}
} // namespace CHEngine
