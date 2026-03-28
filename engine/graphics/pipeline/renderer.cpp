#include "engine/graphics/pipeline/renderer.h"
#include "engine/core/application.h"
#include "engine/core/log.h"
#include "engine/core/profiler.h"
#include "engine/graphics/pipeline/renderer2d.h"
#include "engine/graphics/pipeline/ui_renderer.h"
#include "engine/graphics/assets/shader_asset.h"
#include "engine/core/assets/asset_manager.h"
#include "engine/graphics/assets/texture_asset.h"

#include <glad/gl.h>
#include <algorithm>
#include <vector>
#include <glm/gtc/type_ptr.hpp>

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
    loadShader("Grid",           "resources/shaders/grid.chshader");
    
    CH_CORE_INFO("[Renderer] LoadEngineResources done. {} shader(s) loaded.", shaders.GetNames().size());
}

void Renderer::InternalInit()
{
    CH_CORE_INFO("Initializing Render System (OpenGL Pure)...");
    
    if (Application::Get().GetSpecification().Headless)
    {
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
        float vertices[] = { 
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,  
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,  
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f, 
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f 
        };
        uint32_t indices[] = { 0, 1, 2, 2, 3, 0 };
        
        m_Data->FullscreenQuadVAO = VertexArray::Create();
        auto vbo = VertexBuffer::Create(vertices, sizeof(vertices));
        vbo->SetLayout({ 
            { ShaderDataType::Float3, "vertexPosition" }, 
            { ShaderDataType::Float2, "vertexTexCoord" } 
        });
        m_Data->FullscreenQuadVAO->AddVertexBuffer(vbo);
        auto ibo = IndexBuffer::Create(indices, 6);
        m_Data->FullscreenQuadVAO->SetIndexBuffer(ibo);
    }

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
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(RenderLight) * LightingData::MaxLights, m_Data->Lighting.Lights);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        m_Data->Lighting.LightsDirty = false;
    }

    // Bind lighting uniforms to the default lighting shader if it exists
    auto lightingShaderAsset = m_Data->Shaders->Exists("Lighting") ? m_Data->Shaders->Get("Lighting") : nullptr;
    if (lightingShaderAsset)
    {
        uint32_t shaderId = lightingShaderAsset->GetShader().id;
        glUseProgram(shaderId);
        
        float time = m_Data->Time;
        float diagMode = m_Data->DiagnosticMode;
        int lightCount = m_Data->LightCount;
        float ambient = m_Data->Lighting.CurrentLighting.Ambient;
        float exposure = m_Data->Lighting.CurrentLighting.Exposure;
        float gamma = m_Data->Lighting.CurrentLighting.Gamma;

        auto setVec3 = [&](const char* name, const glm::vec3& v) {
            glUniform3fv(glGetUniformLocation(shaderId, name), 1, glm::value_ptr(v));
        };
        auto setVec4 = [&](const char* name, const glm::vec4& v) {
            glUniform4fv(glGetUniformLocation(shaderId, name), 1, glm::value_ptr(v));
        };
        auto setFloat = [&](const char* name, float v) {
            glUniform1f(glGetUniformLocation(shaderId, name), v);
        };
        auto setInt = [&](const char* name, int v) {
            glUniform1i(glGetUniformLocation(shaderId, name), v);
        };

        setVec3("viewPos", camera.Position);
        setFloat("uTime", time);
        setFloat("uMode", diagMode);
        
        setVec3("lightDir", m_Data->Lighting.CurrentLighting.Direction);
        
        glm::vec4 lightColor = { m_Data->Lighting.CurrentLighting.LightColor.r / 255.0f, 
                                 m_Data->Lighting.CurrentLighting.LightColor.g / 255.0f, 
                                 m_Data->Lighting.CurrentLighting.LightColor.b / 255.0f, 
                                 m_Data->Lighting.CurrentLighting.LightColor.a / 255.0f };
        setVec4("lightColor", lightColor);
        
        setFloat("ambient", ambient);
        
        glm::vec4 skyColor = lightColor;
        skyColor.w = ambient * 0.35f; 
        setVec4("skyAmbientColor", skyColor);

        setInt("uLightCount", lightCount);
        setFloat("uExposure", exposure);
        setFloat("uGamma", gamma);
        
        ApplyFogUniforms(lightingShaderAsset->GetShader().id);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_Data->Lighting.LightSSBO);
        m_Data->CurrentShaderId = shaderId;
    }

    // --- Direct Matrix Management (Pure OpenGL style) ---
    // 1. Calculate View Transform
    m_Data->CurrentView = glm::lookAt(camera.Position, camera.Target, camera.Up);

    // 2. Calculate Projection Transform
    // We need to get width/height without Raylib GetRenderWidth()
    // For now we can assume we have them in m_Data or Application
    int width = Application::Get().GetWindow().GetWidth();
    int height = Application::Get().GetWindow().GetHeight();
    float aspect = (float)width / (float)height;
    
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
    glUseProgram(0);
}

void Renderer::Clear(const glm::vec4& color)
{
    CHEngine::Color chColor((unsigned char)(color.r * 255), (unsigned char)(color.g * 255), (unsigned char)(color.b * 255), (unsigned char)(color.a * 255));
    RenderCommand::Clear(chColor);
}

void Renderer::SetViewport(int x, int y, int width, int height)
{
    RenderCommand::SetViewport(x, y, width, height);
}

void Renderer::DrawMesh(const Mesh& mesh, const Material& material, const glm::mat4& transform)
{
    // For now, we still use Raylib's Mesh and Material, but we draw them manually if possible
    // or keep the raylib call until Mesh is fully abstracted.
    // However, the user wants "clean OpenGL" NOW.
    
    uint32_t shaderId = material.ShaderID;
    if (shaderId == 0) shaderId = m_Data->CurrentShaderId;
    if (shaderId == 0) return;

    glUseProgram(shaderId);

    // Set matrices
    glUniformMatrix4fv(glGetUniformLocation(shaderId, "matModel"), 1, GL_FALSE, glm::value_ptr(transform));
    glUniformMatrix4fv(glGetUniformLocation(shaderId, "matView"), 1, GL_FALSE, glm::value_ptr(m_Data->CurrentView));
    glUniformMatrix4fv(glGetUniformLocation(shaderId, "matProjection"), 1, GL_FALSE, glm::value_ptr(m_Data->CurrentProj));
    
    glm::mat4 matNormal = glm::transpose(glm::inverse(transform));
    glUniformMatrix4fv(glGetUniformLocation(shaderId, "matNormal"), 1, GL_FALSE, glm::value_ptr(matNormal));

    glm::mat4 mvp = m_Data->CurrentProj * m_Data->CurrentView * transform;
    glUniformMatrix4fv(glGetUniformLocation(shaderId, "mvp"), 1, GL_FALSE, glm::value_ptr(mvp));

    // Bind VAO and Draw
    if (mesh.VAO)
    {
        mesh.VAO->Bind();
        if (mesh.TriangleCount > 0)
            glDrawElements(GL_TRIANGLES, mesh.TriangleCount * 3, GL_UNSIGNED_INT, 0);
        else
            glDrawArrays(GL_TRIANGLES, 0, mesh.VertexCount);
        mesh.VAO->Unbind();
    }
}

void Renderer::DrawMeshInstanced(const Mesh& mesh, const Material& material, const std::vector<glm::mat4>& transforms)
{
    if (transforms.empty()) return;

    uint32_t shaderId = material.ShaderID;
    if (shaderId == 0) shaderId = m_Data->CurrentShaderId;
    if (shaderId == 0) return;

    glUseProgram(shaderId);
    
    // Matricies are usually passed via an instance buffer, but for simplicity we keep raylib call or loop
    // Raylib's DrawMeshInstanced is quite complex internally.
    // For "clean OpenGL" we should implement instanced rendering properly.
    // But for now, let's keep it as is or loop it? Loop is slow.
    
    // Temporary: use loop to keep it "clean OpenGL" (avoiding raylib high level)
    for (const auto& transform : transforms)
    {
        DrawMesh(mesh, material, transform);
    }
}

void Renderer::DrawLine(const glm::vec3& start, const glm::vec3& end, const glm::vec4& color)
{
    // Native OpenGL line drawing (Fixed function or simple shader)
    // For now we use the simple unlit shader if available
    auto unlitShader = m_Data->Shaders->Exists("Unlit") ? m_Data->Shaders->Get("Unlit") : nullptr;
    if (unlitShader)
    {
        uint32_t shaderId = unlitShader->GetShader().id;
        glUseProgram(shaderId);
        
        glm::mat4 mvp = m_Data->CurrentProj * m_Data->CurrentView;
        glUniformMatrix4fv(glGetUniformLocation(shaderId, "u_ViewProjection"), 1, GL_FALSE, glm::value_ptr(mvp));
        glUniform4fv(glGetUniformLocation(shaderId, "u_Color"), 1, glm::value_ptr(color));
        
        // Immediate mode replacement or small buffer
        float vertices[] = { start.x, start.y, start.z, end.x, end.y, end.z };
        uint32_t vbo, vao;
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STREAM_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
        
        glDrawArrays(GL_LINES, 0, 2);
        
        glDeleteBuffers(1, &vbo);
        glDeleteVertexArrays(1, &vao);
    }
}

void Renderer::DrawMeshWire(const Mesh& mesh, const glm::vec4& color, const glm::mat4& transform)
{
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    
    // Create a temporary unlit material for wireframe
    Material mat; 
    // We don't have a full Material abstraction yet that easily takes a color, 
    // but the original code had maps[0].color = color.
    // However, our new Material struct might be different.
    
    DrawMesh(mesh, mat, transform);
    
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void Renderer::DrawGrid(int slices, float spacing)
{
    // Draw grid manually using DrawLine
    float halfSize = (slices * spacing) / 2.0f;
    glm::vec4 color = { 0.5f, 0.5f, 0.5f, 1.0f };
    
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
    if (!shaders.Exists("Grid")) return;
    auto shader = shaders.Get("Grid")->GetShader();

    uint32_t shaderId = shader.id;
    glUseProgram(shaderId);

    glm::mat4 mvp = m_Data->CurrentProj * m_Data->CurrentView;
    glUniformMatrix4fv(glGetUniformLocation(shaderId, "u_ViewProjection"), 1, GL_FALSE, glm::value_ptr(mvp));
    
    // Position the plane slightly below Y=0 to prevent Z-fighting
    glm::vec3 planePos = { camera.Position.x, -0.005f, camera.Position.z };
    glm::mat4 model = glm::translate(glm::mat4(1.0f), planePos);
    glUniformMatrix4fv(glGetUniformLocation(shaderId, "matModel"), 1, GL_FALSE, glm::value_ptr(model));
    
    glUniform3fv(glGetUniformLocation(shaderId, "cameraPos"), 1, &camera.Position.x);
    
    glm::vec4 col = color;
    glUniform4fv(glGetUniformLocation(shaderId, "gridColor"), 1, glm::value_ptr(col));
    glUniform1f(glGetUniformLocation(shaderId, "gridSize"), spacing);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Draw the plane mesh (static or shared mesh)
    // We assume we have a quad/plane VAO ready
    static uint32_t planeVAO = 0;
    if (planeVAO == 0)
    {
        float gridPlaneSize = 15000.0f; 
        float vertices[] = {
            -gridPlaneSize, 0.0f, -gridPlaneSize,
             gridPlaneSize, 0.0f, -gridPlaneSize,
             gridPlaneSize, 0.0f,  gridPlaneSize,
            -gridPlaneSize, 0.0f,  gridPlaneSize
        };
        uint32_t vbo;
        glGenVertexArrays(1, &planeVAO);
        glGenBuffers(1, &vbo);
        glBindVertexArray(planeVAO);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
    }
    
    glBindVertexArray(planeVAO);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glBindVertexArray(0);
}

void Renderer::DrawSkybox(const SkyboxSettings& settings, const Camera3D& camera)
{
    if (settings.TexturePath.empty())
        return;

    auto texture = AssetManager::Get().Get<TextureAsset>(settings.TexturePath);
    if (!texture || texture->GetState() != AssetState::Ready)
        return;

    bool isCubemap = (settings.Mode == 2);
    auto shaderAsset = isCubemap ? 
                       m_Data->Shaders->Get("SkyboxCubemap") : 
                       m_Data->Shaders->Get("Skybox");
    if (!shaderAsset) return;
    uint32_t shaderId = shaderAsset->GetShader().id;

    // 1. Generate Cubemap (Mode 2)
    if (isCubemap)
    {
        uint32_t texId = texture->GetTexture().id;
        if (m_Data->Skybox.CachedCubemapId == 0 || 
            m_Data->Skybox.CachedCubemapPath != settings.TexturePath ||
            m_Data->Skybox.SourceTextureId != texId)
        {
            auto genShaderAsset = m_Data->Shaders->Get("CubemapGen");
            if (genShaderAsset)
            {
                if (m_Data->Skybox.CachedCubemapId != 0) glDeleteTextures(1, &m_Data->Skybox.CachedCubemapId);
                m_Data->Skybox.CachedCubemapId = GenTextureCubemap(genShaderAsset->GetShader().id, texture->GetTexture().id, 1024);
                m_Data->Skybox.CachedCubemapPath = settings.TexturePath;
                m_Data->Skybox.SourceTextureId = texId;
            }
        }
    }

    // 2. Prepare Render State
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_CULL_FACE);
    glDepthMask(GL_FALSE);

    // 3. Setup Uniforms
    glUseProgram(shaderId);
    
    // Always remove translation from view matrix for skybox
    glm::mat4 view = glm::mat4(glm::mat3(m_Data->CurrentView));
    glUniformMatrix4fv(glGetUniformLocation(shaderId, "u_View"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderId, "u_Projection"), 1, GL_FALSE, glm::value_ptr(m_Data->CurrentProj));

    glUniform1f(glGetUniformLocation(shaderId, "u_Exposure"), settings.Exposure);
    glUniform1f(glGetUniformLocation(shaderId, "u_Brightness"), settings.Brightness);
    glUniform1f(glGetUniformLocation(shaderId, "u_Contrast"), settings.Contrast);
    
    int isHDR = texture->IsHDR() ? 1 : 0;
    glUniform1i(glGetUniformLocation(shaderId, "u_IsHDR"), isHDR);
    glUniform1i(glGetUniformLocation(shaderId, "u_VFlipped"), isCubemap ? 0 : 1);

    ApplyFogUniforms(shaderId);

    // 4. Bind Textures and Draw Mesh
    if (isCubemap)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_Data->Skybox.CachedCubemapId);
        glUniform1i(glGetUniformLocation(shaderId, "u_Cubemap"), 0);

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
        glBindTexture(GL_TEXTURE_2D, texture->GetTexture().id);
        glUniform1i(glGetUniformLocation(shaderId, "u_Panorama"), 0);

        if (m_Data->Skybox.SkyboxSphereModel && !m_Data->Skybox.SkyboxSphereModel->Meshes.empty())
        {
            auto& mesh = m_Data->Skybox.SkyboxSphereModel->Meshes[0];
            mesh.VAO->Bind();
            glDrawElements(GL_TRIANGLES, mesh.TriangleCount * 3, GL_UNSIGNED_INT, 0);
            mesh.VAO->Unbind();
        }
    }

    // 5. Restore Render State
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glDepthMask(GL_TRUE);
}

void Renderer::DrawBillboard(const Camera3D& camera, uint32_t textureId, const glm::vec3& position, float size, const glm::vec4& tint)
{
    // Custom billboard rendering in OpenGL
    // We can use a simple quad and rotate it to face the camera
    auto unlitShader = m_Data->Shaders->Exists("Unlit") ? m_Data->Shaders->Get("Unlit") : nullptr;
    if (unlitShader)
    {
        uint32_t shaderId = unlitShader->GetShader().id;
        glUseProgram(shaderId);
        
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
        glUniformMatrix4fv(glGetUniformLocation(shaderId, "u_ViewProjection"), 1, GL_FALSE, glm::value_ptr(mvp));
        glUniform4fv(glGetUniformLocation(shaderId, "u_Color"), 1, glm::value_ptr(tint));
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureId);
        glUniform1i(glGetUniformLocation(shaderId, "u_Texture"), 0);
        
        // Draw quad (Reuse planeVAO from DrawInfiniteGrid or create a localized one)
        static uint32_t quadVAO = 0;
        if (quadVAO == 0)
        {
            float vertices[] = { -0.5f, -0.5f, 0, 0, 0,  0.5f, -0.5f, 0, 1, 0,  0.5f, 0.5f, 0, 1, 1, -0.5f, 0.5f, 0, 0, 1 };
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
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    // Draw cube wires using DrawLine or a shared cube mesh
    // For simplicity, we just use 12 DrawLine calls
    glm::vec3 h = size * 0.5f;
    glm::vec3 p[8] = {
        glm::vec3(transform * glm::vec4(-h.x, -h.y, -h.z, 1)), glm::vec3(transform * glm::vec4(h.x, -h.y, -h.z, 1)),
        glm::vec3(transform * glm::vec4(h.x, h.y, -h.z, 1)),   glm::vec3(transform * glm::vec4(-h.x, h.y, -h.z, 1)),
        glm::vec3(transform * glm::vec4(-h.x, -h.y, h.z, 1)),  glm::vec3(transform * glm::vec4(h.x, -h.y, h.z, 1)),
        glm::vec3(transform * glm::vec4(h.x, h.y, h.z, 1)),    glm::vec3(transform * glm::vec4(-h.x, h.y, h.z, 1))
    };
    for(int i=0; i<4; i++) {
        DrawLine(p[i], p[(i+1)%4], color);
        DrawLine(p[i+4], p[((i+1)%4)+4], color);
        DrawLine(p[i], p[i+4], color);
    }
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void Renderer::DrawCapsuleWires(const glm::mat4& transform, float radius, float height, const glm::vec4& color)
{
    // Simplified: just draw a cylinder-like box for now or implement proper wires
    DrawCubeWires(transform, {radius*2, height, radius*2}, color);
}

void Renderer::DrawSphereWires(const glm::mat4& transform, float radius, const glm::vec4& color)
{
    // Simplified: just draw 3 circles
    DrawCubeWires(transform, {radius*2, radius*2, radius*2}, color);
}

void Renderer::ApplyPostProcessing(uint32_t screenTextureId, uint32_t depthTextureId, const Camera3D& camera)
{
    if (m_Data->Shaders->Exists("PostProcess"))
    {
        auto shaderAsset = m_Data->Shaders->Get("PostProcess");
        uint32_t shaderId = shaderAsset->GetShader().id;

        glUseProgram(shaderId);
        
        // Fullscreen quad doesn't need transformation, set MVP to identity
        glm::mat4 identity = glm::mat4(1.0f);
        glUniformMatrix4fv(glGetUniformLocation(shaderId, "mvp"), 1, GL_FALSE, glm::value_ptr(identity));

        glm::mat4 invViewProj = glm::inverse(m_Data->CurrentProj * m_Data->CurrentView);
        glUniformMatrix4fv(glGetUniformLocation(shaderId, "matInverseViewProj"), 1, GL_FALSE, glm::value_ptr(invViewProj));
        glUniform3fv(glGetUniformLocation(shaderId, "viewPos"), 1, glm::value_ptr(camera.Position));
        
        glUniform1f(glGetUniformLocation(shaderId, "uTime"), m_Data->Time);
        glUniform1f(glGetUniformLocation(shaderId, "uExposure"), m_Data->Lighting.CurrentLighting.Exposure);
        glUniform1f(glGetUniformLocation(shaderId, "uGamma"), m_Data->Lighting.CurrentLighting.Gamma);

        ApplyFogUniforms(shaderId);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, screenTextureId);
        glUniform1i(glGetUniformLocation(shaderId, "texture0"), 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthTextureId);
        glUniform1i(glGetUniformLocation(shaderId, "texture1"), 1);

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

void Renderer::ApplyFogUniforms(uint32_t shaderId)
{
    const auto& fog = m_Data->Lighting.CurrentFog;
    int enabled = fog.Enabled ? 1 : 0;
    int mode = (int)fog.Mode;
    glm::vec4 color = { fog.FogColor.r / 255.0f, fog.FogColor.g / 255.0f, fog.FogColor.b / 255.0f, fog.FogColor.a / 255.0f };

    glUniform1i(glGetUniformLocation(shaderId, "fogEnabled"), enabled);
    glUniform4fv(glGetUniformLocation(shaderId, "fogColor"), 1, glm::value_ptr(color));
    glUniform1f(glGetUniformLocation(shaderId, "fogDensity"), fog.Density);
    glUniform1f(glGetUniformLocation(shaderId, "fogStart"), fog.Start);
    glUniform1f(glGetUniformLocation(shaderId, "fogEnd"), fog.End);
    glUniform1i(glGetUniformLocation(shaderId, "fogMode"), mode);
}

void Renderer::InitializeSkybox()
{
    // 1. Cube Generation
    m_Data->Skybox.SkyboxCubeModel = std::make_unique<Model>();
    float s = 1.0f;
    float cubeVertices[] = {
        -s, -s,  s,  s, -s,  s,  s,  s,  s, -s,  s,  s, // Front
        -s, -s, -s, -s,  s, -s,  s,  s, -s,  s, -s, -s, // Back
        -s,  s, -s, -s,  s,  s,  s,  s,  s,  s,  s, -s, // Top
        -s, -s, -s,  s, -s, -s,  s, -s,  s, -s, -s,  s, // Bottom
         s, -s, -s,  s,  s, -s,  s,  s,  s,  s, -s,  s, // Right
        -s, -s, -s, -s, -s,  s, -s,  s,  s, -s,  s, -s  // Left
    };
    uint32_t cubeIndices[] = {
        0,1,2,  2,3,0,   4,5,6,  6,7,4,   8,9,10, 10,11,8,
        12,13,14, 14,15,12, 16,17,18, 18,19,16, 20,21,22, 22,23,20
    };
    Mesh cubeMesh;
    cubeMesh.VertexCount = 24;
    cubeMesh.TriangleCount = 12;
    cubeMesh.VAO = VertexArray::Create();
    auto cubeVbo = VertexBuffer::Create(cubeVertices, sizeof(cubeVertices));
    cubeVbo->SetLayout({ { ShaderDataType::Float3, "a_Position" } });
    cubeMesh.VAO->AddVertexBuffer(cubeVbo);
    cubeMesh.VAO->SetIndexBuffer(IndexBuffer::Create(cubeIndices, 36));
    m_Data->Skybox.SkyboxCubeModel->Meshes.push_back(cubeMesh);

    // 2. Sphere Generation (for Panoramas)
    m_Data->Skybox.SkyboxSphereModel = std::make_unique<Model>();
    std::vector<float> sphereVertices;
    std::vector<uint32_t> sphereIndices;
    const unsigned int X_SEGMENTS = 64;
    const unsigned int Y_SEGMENTS = 64;
    for (unsigned int x = 0; x <= X_SEGMENTS; ++x) {
        for (unsigned int y = 0; y <= Y_SEGMENTS; ++y) {
            float xSegment = (float)x / (float)X_SEGMENTS;
            float ySegment = (float)y / (float)Y_SEGMENTS;
            float xPos = std::cos(xSegment * 2.0f * glm::pi<float>()) * std::sin(ySegment * glm::pi<float>());
            float yPos = std::cos(ySegment * glm::pi<float>());
            float zPos = std::sin(xSegment * 2.0f * glm::pi<float>()) * std::sin(ySegment * glm::pi<float>());
            sphereVertices.push_back(xPos * 50.0f); // Large enough to not be clipped
            sphereVertices.push_back(yPos * 50.0f);
            sphereVertices.push_back(zPos * 50.0f);
        }
    }
    for (unsigned int y = 0; y < Y_SEGMENTS; ++y) {
        if (y % 2 == 0) {
            for (unsigned int x = 0; x <= X_SEGMENTS; ++x) {
                sphereIndices.push_back(y * (X_SEGMENTS + 1) + x);
                sphereIndices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
            }
        } else {
            for (int x = X_SEGMENTS; x >= 0; --x) {
                sphereIndices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                sphereIndices.push_back(y * (X_SEGMENTS + 1) + x);
            }
        }
    }
    Mesh sphereMesh;
    sphereMesh.VertexCount = (uint32_t)sphereVertices.size() / 3;
    sphereMesh.TriangleCount = (uint32_t)sphereIndices.size() - 2; // Approximated for Triangle Strip
    sphereMesh.VAO = VertexArray::Create();
    auto sphereVbo = VertexBuffer::Create(sphereVertices.data(), (uint32_t)sphereVertices.size() * sizeof(float));
    sphereVbo->SetLayout({ { ShaderDataType::Float3, "a_Position" } });
    sphereMesh.VAO->AddVertexBuffer(sphereVbo);
    sphereMesh.VAO->SetIndexBuffer(IndexBuffer::Create(sphereIndices.data(), (uint32_t)sphereIndices.size()));
    // Actually, use Triangle Strip for sphere? No, let's keep it simple with Triangles if possible
    // Wait, the index generation above is for Triangle Strip. 
    // Let's rewrite for standard Triangles to avoid Renderer complexity.
    sphereIndices.clear();
    for (unsigned int y = 0; y < Y_SEGMENTS; ++y) {
        for (unsigned int x = 0; x < X_SEGMENTS; ++x) {
            sphereIndices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
            sphereIndices.push_back(y * (X_SEGMENTS + 1) + x);
            sphereIndices.push_back(y * (X_SEGMENTS + 1) + x + 1);
            sphereIndices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
            sphereIndices.push_back(y * (X_SEGMENTS + 1) + x + 1);
            sphereIndices.push_back((y + 1) * (X_SEGMENTS + 1) + x + 1);
        }
    }
    sphereMesh.TriangleCount = (uint32_t)sphereIndices.size() / 3;
    sphereMesh.VAO->SetIndexBuffer(IndexBuffer::Create(sphereIndices.data(), (uint32_t)sphereIndices.size()));
    m_Data->Skybox.SkyboxSphereModel->Meshes.push_back(sphereMesh);
}

uint32_t Renderer::GenTextureCubemap(uint32_t shaderId, uint32_t panoramaId, int size)
{
    uint32_t cubemap;
    glGenTextures(1, &cubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap);
    for (unsigned int i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA16F, size, size, 0, GL_RGBA, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    // Save current state to restore later
    GLint lastFBO;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &lastFBO);
    GLint lastViewport[4];
    glGetIntegerv(GL_VIEWPORT, lastViewport);

    uint32_t captureFBO;
    glGenFramebuffers(1, &captureFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    
    glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    glm::mat4 captureViews[] = {
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
    };

    glUseProgram(shaderId);
    glUniformMatrix4fv(glGetUniformLocation(shaderId, "projection"), 1, GL_FALSE, glm::value_ptr(captureProjection));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, panoramaId);
    glUniform1i(glGetUniformLocation(shaderId, "equirectangularMap"), 0);

    glViewport(0, 0, size, size);
    
    // Disable depth testing since FBO has no depth attachment, and disable culling to see inside the cube
    bool lastCullFace = glIsEnabled(GL_CULL_FACE);
    bool lastDepthTest = glIsEnabled(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);

    for (unsigned int i = 0; i < 6; ++i)
    {
        glUniformMatrix4fv(glGetUniformLocation(shaderId, "view"), 1, GL_FALSE, glm::value_ptr(captureViews[i]));
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, cubemap, 0);
        glClear(GL_COLOR_BUFFER_BIT);
        
        // Draw Unit Cube
        if (m_Data->Skybox.SkyboxCubeModel && !m_Data->Skybox.SkyboxCubeModel->Meshes.empty())
        {
            auto& mesh = m_Data->Skybox.SkyboxCubeModel->Meshes[0];
            mesh.VAO->Bind();
            glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
            mesh.VAO->Unbind();
        }
    }
    
    // Restore state
    if (lastCullFace) glEnable(GL_CULL_FACE);
    if (lastDepthTest) glEnable(GL_DEPTH_TEST);

    // Restore viewport and framebuffer
    glViewport(lastViewport[0], lastViewport[1], lastViewport[2], lastViewport[3]);
    glBindFramebuffer(GL_FRAMEBUFFER, lastFBO);
    
    glDeleteFramebuffers(1, &captureFBO);

    // Reset clear color to something neutral
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    
    return cubemap;
}

void Renderer::CleanupSkybox()
{
    m_Data->Skybox.SkyboxCubeModel.reset();
    m_Data->Skybox.SkyboxSphereModel.reset();
    
    if (m_Data->Skybox.CachedCubemapId != 0)
    {
        glDeleteTextures(1, &m_Data->Skybox.CachedCubemapId);
    }
}
} // namespace CHEngine
