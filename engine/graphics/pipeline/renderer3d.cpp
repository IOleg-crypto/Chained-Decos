#include "engine/graphics/pipeline/renderer3d.h"
#include "engine/graphics/pipeline/debug_renderer.h"
#include "engine/graphics/pipeline/renderer.h"
#include "engine/graphics/pipeline/geometry_generator.h"
#include "engine/graphics/pipeline/render_command.h"
#include "engine/assets/types/shader_asset.h"
#include "engine/graphics/api/buffer.h"
#include "engine/graphics/api/vertex_array.h"
#include "engine/core/log.h"
#include <glm/gtc/type_ptr.hpp>

namespace Chained {

    void Renderer3D::Init()
    {
        s_3DData.SkyboxCubeModel = std::make_unique<Model>();
        s_3DData.SkyboxCubeModel->Meshes.push_back(GeometryGenerator::GenerateUnitCube());

        s_3DData.SkyboxSphereModel = std::make_unique<Model>();
        s_3DData.SkyboxSphereModel->Meshes.push_back(GeometryGenerator::GenerateSphere(50.0f, 64, 64));

        s_3DData.UnitCubeModel = std::make_unique<Model>();
        s_3DData.UnitCubeModel->Meshes.push_back(GeometryGenerator::GenerateUnitCube());

        s_3DData.UnitSphereModel = std::make_unique<Model>();
        s_3DData.UnitSphereModel->Meshes.push_back(GeometryGenerator::GenerateSphere(1.0f, 32, 32));

        s_3DData.UnitCapsuleModel = std::make_unique<Model>();
        s_3DData.UnitCapsuleModel->Meshes.push_back(GeometryGenerator::GenerateCapsule(1.0f, 2.0f, 32, 32));

        s_3DData.WireCubeModel = std::make_unique<Model>();
        // Add wire cube mesh generation if needed
    }

    void Renderer3D::Shutdown()
    {
        s_3DData.InstancedVAOCache.clear();
        s_3DData.InstanceBuffer.reset();
        
        s_3DData.UnitCubeModel.reset();
        s_3DData.UnitSphereModel.reset();
        s_3DData.UnitCapsuleModel.reset();
        s_3DData.WireCubeModel.reset();
        s_3DData.SkyboxCubeModel.reset();
        s_3DData.SkyboxSphereModel.reset();
    }

    void Renderer3D::BeginScene(const Camera3D& camera, float nearClip, float farClip)
    {
        // View-Projection logic is handled by Renderer core now. 
        // 3D Specific BeginScene operations go here if any.
    }

    void Renderer3D::EndScene()
    {
    }

    void Renderer3D::DrawMesh(const Mesh& mesh, const Material& material, const glm::mat4& transform)
    {
        uint32_t shaderId = material.ShaderID;
        if (shaderId == 0) shaderId = Renderer::GetData().CurrentShaderId;
        if (shaderId == 0) return;

        auto shaderAsset = Renderer::GetShaderLibrary().GetById(shaderId);
        if (!shaderAsset || !shaderAsset->GetShader()) return;

        auto shader = shaderAsset->GetShader();
        shader->Bind();

        shader->SetMatrix("matModel", transform);
        glm::mat4 matNormal = glm::transpose(glm::inverse(transform));
        shader->SetMatrix("matNormal", matNormal);

        if (mesh.VAO)
        {
            mesh.VAO->Bind();
            if (mesh.TriangleCount > 0)
                RenderCommand::DrawIndexed(mesh.VAO, mesh.TriangleCount * 3);
            else
                RenderCommand::DrawArrays(mesh.VertexCount);
            mesh.VAO->Unbind();
        }
    }

    void Renderer3D::DrawMeshInstanced(const Mesh& mesh, const Material& material, const std::vector<glm::mat4>& transforms)
    {
        if (transforms.empty() || !mesh.VAO) return;

        uint32_t shaderId = material.ShaderID;
        if (shaderId == 0) shaderId = Renderer::GetData().CurrentShaderId;
        if (shaderId == 0) return;

        auto shaderAsset = Renderer::GetShaderLibrary().GetById(shaderId);
        if (!shaderAsset || !shaderAsset->GetShader()) return;

        auto shader = shaderAsset->GetShader();
        shader->Bind();

        glm::mat4 mvp = Renderer::GetData().CurrentProj * Renderer::GetData().CurrentView;
        shader->SetMatrix("u_ViewProjection", mvp);
        shader->SetMatrix("u_Transform", glm::mat4(1.0f));

        auto dataSize = (uint32_t)(transforms.size() * sizeof(glm::mat4));
        if (!s_3DData.InstanceBuffer || s_3DData.InstanceBufferCapacity < dataSize)
        {
            s_3DData.InstanceBufferCapacity = std::max(dataSize, (uint32_t)(1024 * sizeof(glm::mat4)));
            s_3DData.InstanceBuffer = VertexBuffer::Create(s_3DData.InstanceBufferCapacity);
            s_3DData.InstanceBuffer->SetLayout({{ShaderDataType::Mat4, "a_InstanceTransform", false, true}});
            s_3DData.InstancedVAOCache.clear();
        }
        s_3DData.InstanceBuffer->SetData(transforms.data(), dataSize);

        auto& instancedVAO = s_3DData.InstancedVAOCache[mesh.VAO.get()];
        if (!instancedVAO)
        {
            instancedVAO = VertexArray::Create();
            for (const auto& vbo : mesh.VAO->GetVertexBuffers())
                instancedVAO->AddVertexBuffer(vbo);
            instancedVAO->AddVertexBuffer(s_3DData.InstanceBuffer);
            instancedVAO->SetIndexBuffer(mesh.VAO->GetIndexBuffer());
        }

        instancedVAO->Bind();
        if (mesh.TriangleCount > 0)
            RenderCommand::DrawIndexedInstanced(instancedVAO, (uint32_t)transforms.size(), mesh.TriangleCount * 3);
        else
            RenderCommand::DrawArraysInstanced(mesh.VertexCount, (uint32_t)transforms.size());
        instancedVAO->Unbind();
    }

    // Skybox, Wires etc...
    void Renderer3D::DrawSkybox(uint32_t textureId, int skyboxMode, bool isHDR, float exposure, float brightness, float contrast, const Camera3D& camera, bool flipped)
    {
        if (textureId == 0) return;

        int mode = (skyboxMode < 0) ? 0 : (skyboxMode > 2 ? 2 : skyboxMode);

        auto shaderAsset =
            (skyboxMode == 2)
                ? Renderer::GetShaderLibrary().LoadOrGet("SkyboxCubemap", "resources/shaders/skybox_cubemap.chshader")
                : (skyboxMode == 1
                       ? Renderer::GetShaderLibrary().LoadOrGet("SkyboxCross", "resources/shaders/skybox_cross.chshader")
                       : Renderer::GetShaderLibrary().LoadOrGet("Skybox", "resources/shaders/skybox.chshader"));
        if (!shaderAsset || !shaderAsset->GetShader()) return;

        RenderCommand::SetDepthFunc(RendererAPI::DepthFunc::LEqual);
        RenderCommand::SetCullMode(RendererAPI::CullMode::None);
        RenderCommand::DisableDepthMask();

        auto shader = shaderAsset->GetShader();
        shader->Bind();

        glm::mat4 view = glm::mat4(glm::mat3(Renderer::GetData().CurrentView));
        shader->SetMatrix("u_View", view);
        shader->SetMatrix("u_Projection", Renderer::GetData().CurrentProj);
        shader->SetFloat("u_Exposure", exposure);
        shader->SetFloat("u_Brightness", brightness);
        shader->SetFloat("u_Contrast", contrast);
        shader->SetInt("u_IsHDR", isHDR ? 1 : 0);
        shader->SetInt("u_VFlipped", flipped ? 1 : 0);

        // ApplyFogUniforms - omitted for standard Renderer3D decoupling, SceneRenderer can set uniform blocks if needed.

        if (skyboxMode == 2)
        {
            RenderCommand::SetTexture(0, textureId, true);
            shader->SetInt("u_Cubemap", 0);

            if (s_3DData.SkyboxCubeModel && !s_3DData.SkyboxCubeModel->Meshes.empty())
            {
                auto& mesh = s_3DData.SkyboxCubeModel->Meshes[0];
                mesh.VAO->Bind();
                RenderCommand::DrawIndexed(mesh.VAO, 36);
                mesh.VAO->Unbind();
            }
        }
        else if (skyboxMode == 1)
        {
            RenderCommand::SetTexture(0, textureId);
            shader->SetInt("u_CrossMap", 0);

            if (s_3DData.SkyboxCubeModel && !s_3DData.SkyboxCubeModel->Meshes.empty())
            {
                auto& mesh = s_3DData.SkyboxCubeModel->Meshes[0];
                mesh.VAO->Bind();
                RenderCommand::DrawIndexed(mesh.VAO, 36);
                mesh.VAO->Unbind();
            }
        }
        else if (skyboxMode == 0)
        {
            RenderCommand::SetTexture(0, textureId);
            shader->SetInt("u_Panorama", 0);

            if (s_3DData.SkyboxSphereModel && !s_3DData.SkyboxSphereModel->Meshes.empty())
            {
                auto& mesh = s_3DData.SkyboxSphereModel->Meshes[0];
                mesh.VAO->Bind();
                RenderCommand::DrawIndexed(mesh.VAO, mesh.TriangleCount * 3);
                mesh.VAO->Unbind();
            }
        }

        RenderCommand::SetDepthFunc(RendererAPI::DepthFunc::Less);
        RenderCommand::SetCullMode(RendererAPI::CullMode::Back);
        RenderCommand::EnableDepthMask();
    }

    void Renderer3D::DrawMeshWire(const Mesh& mesh, const glm::vec4& color, const glm::mat4& transform, bool useWireframe)
    {
        DebugRenderer::DrawMeshWire(mesh, color, transform, useWireframe);
    }

    void Renderer3D::DrawCubeWires(const glm::mat4& transform, const glm::vec3& size, const glm::vec4& color, bool useWireframe)
    {
        DebugRenderer::DrawCubeWires(transform, size, color, useWireframe);
    }

    void Renderer3D::DrawSphereWires(const glm::mat4& transform, float radius, const glm::vec4& color, bool useWireframe)
    {
        DebugRenderer::DrawSphereWires(transform, radius, color, useWireframe);
    }

    void Renderer3D::DrawCapsuleWires(const glm::mat4& transform, float radius, float height, const glm::vec4& color, bool useWireframe)
    {
        DebugRenderer::DrawCapsuleWires(transform, radius, height, color, useWireframe);
    }
} // namespace Chained
