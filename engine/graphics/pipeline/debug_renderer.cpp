#include "engine/graphics/pipeline/debug_renderer.h"
#include "engine/assets/asset_manager.h"
#include "engine/assets/types/shader_asset.h"
#include "engine/graphics/pipeline/render_command.h"
#include "engine/graphics/pipeline/renderer.h"
#include "engine/graphics/pipeline/geometry_generator.h"
#include <glm/gtc/matrix_transform.hpp>

namespace Chained::DebugRenderer {

// void Renderer3D::Shutdown()
// {
//     s_3DData.InstancedVAOCache.clear();
//     s_3DData.InstanceBuffer.reset();
//     s_3DData.GridPlaneVAO.reset();
// }

// // --- Mesh Rendering ---
// void Renderer3D::DrawMesh(const Mesh& mesh, const Material& material, const glm::mat4& transform)
// {
//     Renderer::Get().DrawMesh(mesh, material, transform); // Temporary redirection
// }

// void Renderer3D::DrawMeshInstanced(const Mesh& mesh, const Material& material, const std::vector<glm::mat4>&
// transforms)
// {
//     Renderer::Get().DrawMeshInstanced(mesh, material, transforms); // Temporary redirection
// }

// void Renderer3D::DrawMeshWire(const Mesh& mesh, const glm::vec4& color, const glm::mat4& transform, bool
// useWireframe)
// {
//     Renderer::Get().DrawMeshWire(mesh, color, transform, useWireframe); // Temporary redirection
// }

// void Renderer3D::DrawSkybox(uint32_t textureId, int skyboxMode, bool isHDR, float exposure, float brightness, float
// contrast, const Camera3D& camera, bool flipped)
// {
//     Renderer::Get().DrawSkybox(textureId, skyboxMode, isHDR, exposure, brightness, contrast, camera, flipped);
// }

// void Renderer3D::DrawCubeWires(const glm::mat4& transform, const glm::vec3& size, const glm::vec4& color, bool
// useWireframe)
// {
//     Renderer::Get().DrawCubeWires(transform, size, color, useWireframe);
// }

// void Renderer3D::DrawCapsuleWires(const glm::mat4& transform, float radius, float height, const glm::vec4& color,
// bool useWireframe)
// {
//     Renderer::Get().DrawCapsuleWires(transform, radius, height, color, useWireframe);
// }

// void Renderer3D::DrawSphereWires(const glm::mat4& transform, float radius, const glm::vec4& color, bool useWireframe)
// {
//     Renderer::DrawSphereWires(transform, radius, color, useWireframe);
// }

// void Renderer3D::DrawGrid(int slices, float spacing)
// {
//     Renderer::DrawGrid(slices, spacing);
// }

// void Renderer3D::DrawInfiniteGrid(const Camera3D& camera, float spacing, const glm::vec4& color)
// {
//     Renderer::DrawInfiniteGrid(camera, spacing, color);
// }

void DrawMeshWire(const Mesh& mesh, const glm::vec4& color, const glm::mat4& transform, bool useWireframe)
{
    auto& rd = Renderer::GetData();
    auto debugShader = rd.Shaders->LoadOrGet("ColliderDebug", "engine/resources/shaders/collider_debug.chshader");
    if (!debugShader || !debugShader->GetShader())
    {
        return;
    }

    auto shader = debugShader->GetShader();
    shader->Bind();

    // Set matrices and color with correct uniform names for ColliderDebug
    glm::mat4 vp = rd.CurrentProj * rd.CurrentView;
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
    void DrawInfiniteGrid(const Camera3D& camera, float spacing, const glm::vec4& color)
    {
        auto& rd = Renderer::GetData();
        auto& shaderLibrary = Renderer::GetShaderLibrary();
        auto shaderAsset = shaderLibrary.LoadOrGet("Grid", "resources/shaders/grid.chshader");
        if (!shaderAsset || !shaderAsset->GetShader())
        {
            return;
        }

        shaderAsset->GetShader()->Bind();

        glm::mat4 mvp = rd.CurrentProj * rd.CurrentView;
        shaderAsset->GetShader()->SetMatrix("u_ViewProjection", mvp);

        // Position the plane slightly below Y=0 to prevent Z-fighting
        glm::vec3 planePos = {camera.Position.x, -0.005f, camera.Position.z};
        glm::mat4 model = glm::translate(glm::mat4(1.0f), planePos);
        shaderAsset->GetShader()->SetMatrix("matModel", model);

        shaderAsset->GetShader()->SetVec3("cameraPos", camera.Position);

        glm::vec4 col = color;
        shaderAsset->GetShader()->SetVec4("gridColor", col);
        shaderAsset->GetShader()->SetFloat("gridSize", spacing);
        shaderAsset->GetShader()->SetFloat("uTime", rd.Time);

        RenderCommand::SetBlendMode(true);
        RenderCommand::SetBlendFunc(RendererAPI::BlendFactor::SrcAlpha, RendererAPI::BlendFactor::OneMinusSrcAlpha);

        // Disable depth test for the infinite grid or it will be occluded by the floor
        RenderCommand::DisableDepthTest();

        // Use shared mesh from GeometryGenerator
        if (!rd.GridPlaneVAO)
        {
            float vertices[] = {-0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 0.5f,  -0.5f, 0.0f, 1.0f, 0.0f,
                                0.5f,  0.5f,  0.0f, 1.0f, 1.0f, -0.5f, 0.5f,  0.0f, 0.0f, 1.0f};
            uint32_t indices[] = {0, 1, 2, 2, 3, 0};

            auto vbo = VertexBuffer::Create(vertices, sizeof(vertices));
            vbo->SetLayout({{ShaderDataType::Float3, "a_Position"}, {ShaderDataType::Float2, "a_TexCoord"}});

            rd.GridPlaneVAO = VertexArray::Create();
            rd.GridPlaneVAO->AddVertexBuffer(vbo);
            auto ibo = IndexBuffer::Create(indices, 6);
            rd.GridPlaneVAO->SetIndexBuffer(ibo);
        }

        glm::mat4 scale = glm::scale(model, glm::vec3(15000.0f, 1.0f, 15000.0f));
        shaderAsset->GetShader()->SetMatrix("matModel", scale);

        rd.GridPlaneVAO->Bind();
        RenderCommand::DrawIndexed(rd.GridPlaneVAO, 6);
        rd.GridPlaneVAO->Unbind();
        RenderCommand::EnableDepthTest();
    }

    void DrawCubeWires(const glm::mat4& transform, const glm::vec3& size, const glm::vec4& color, bool useWireframe)
    {
        // We use the procedural box model from GeometryGenerator
        ProceduralParameters p;
        p.Dimensions = size;
        Model m = GeometryGenerator::GenerateProceduralModel(":cube:", p);
        if (!m.Meshes.empty())
        {
            DrawMeshWire(m.Meshes[0], color, transform, useWireframe);
        }
    }

    void DrawSphereWires(const glm::mat4& transform, float radius, const glm::vec4& color, bool useWireframe)
    {
        ProceduralParameters p;
        p.Radius = radius;
        p.Slices = 16;
        p.Stacks = 16;
        Model m = GeometryGenerator::GenerateProceduralModel(":sphere:", p);
        if (!m.Meshes.empty())
        {
            DrawMeshWire(m.Meshes[0], color, transform, useWireframe);
        }
    }

    void DrawCapsuleWires(const glm::mat4& transform, float radius, float height, const glm::vec4& color, bool useWireframe)
    {
        ProceduralParameters p;
        p.Radius = radius;
        p.Height = height;
        p.Slices = 16;
        p.Stacks = 8;
        // Correct path for capsule if it exists, otherwise cylinder as fallback
        Model m = GeometryGenerator::GenerateProceduralModel(":cylinder:", p);
        if (!m.Meshes.empty())
        {
            DrawMeshWire(m.Meshes[0], color, transform, useWireframe);
        }
    }

    void DrawGrid(int slices, float spacing)
    {
        // Not implemented (use DrawInfiniteGrid)
    }

} // namespace Chained::DebugRenderer
