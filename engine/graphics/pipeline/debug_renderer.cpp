#include "engine/graphics/pipeline/debug_renderer.h"
#include "engine/assets/asset_manager.h"
#include "engine/assets/types/shader_asset.h"
#include "engine/core/service_locator.h"
#include "engine/graphics/api/buffer.h"
#include "engine/graphics/api/vertex_array.h"
#include "engine/graphics/pipeline/geometry_generator.h"
#include "engine/graphics/api/graphics_device.h"
#include "engine/graphics/pipeline/renderer.h"
#include <glm/gtc/matrix_transform.hpp>

namespace Chained
{
namespace DebugRenderer
{
void DrawLine(const glm::vec3& start, const glm::vec3& end, const glm::vec4& color)
{
    auto& rd = ServiceLocator::Get<Renderer>()->GetData();
    rd.LineVertexBuffer.push_back({start, color});
    rd.LineVertexBuffer.push_back({end, color});
}

void Flush()
{
    auto& rd = ServiceLocator::Get<Renderer>()->GetData();
    if (rd.LineVertexBuffer.empty())
    {
        return;
    }

    auto debugShader = rd.Shaders->LoadOrGet("ColliderDebug");
    if (!debugShader || !debugShader->GetShader())
    {
        rd.LineVertexBuffer.clear();
        return;
    }

    auto shader = debugShader->GetShader();
    shader->Bind();

    glm::mat4 vp = rd.CurrentProj * rd.CurrentView;
    shader->SetMatrix("u_ViewProj", vp);
    shader->SetMatrix("u_Transform", glm::mat4(1.0f));
    shader->SetVec4("u_Color", glm::vec4(1.0f)); // color is per-vertex

    uint32_t dataSize = (uint32_t)(rd.LineVertexBuffer.size() * sizeof(LineVertex));

    if (!rd.LineVBO || rd.LineVBOSize < dataSize)
    {
        rd.LineVBOSize = std::max(dataSize, (uint32_t)(1024 * sizeof(LineVertex)));
        rd.LineVBO = VertexBuffer::Create(rd.LineVBOSize);
        rd.LineVBO->SetLayout({{VertexAttributeType::Float3, "vertexPosition"}, {VertexAttributeType::Float4, "vertexColor"}});
        rd.LineVAO = VertexArray::Create();
        rd.LineVAO->AddVertexBuffer(rd.LineVBO);
    }

    rd.LineVBO->SetData(rd.LineVertexBuffer.data(), dataSize);

    GraphicsDevice::Get().DrawLines(rd.LineVAO, (uint32_t)rd.LineVertexBuffer.size());

    rd.LineVertexBuffer.clear();
}

void DrawMeshWire(const Mesh& mesh, const glm::vec4& color, const glm::mat4& transform, bool useWireframe)
{
    auto& rd = ServiceLocator::Get<Renderer>()->GetData();
    auto debugShader = rd.Shaders->LoadOrGet("ColliderDebug");
    if (!debugShader || !debugShader->GetShader())
    {
        return;
    }

    auto shader = debugShader->GetShader();
    shader->Bind();

    glm::mat4 vp = rd.CurrentProj * rd.CurrentView;
    shader->SetMatrix("u_ViewProj", vp);
    shader->SetMatrix("u_Transform", transform);

    glm::vec4 finalColor = color;
    if (!useWireframe)
    {
        finalColor.a *= 0.35f;
    }
    shader->SetVec4("u_Color", finalColor);

    if (mesh.VAO)
    {
        auto guard = PipelineStateGuard::Capture();
        guard.WithBlend().WithPolygonMode();
        GraphicsDevice::Get().SetBlendEnabled(true);
        GraphicsDevice::Get().SetBlendFunc(GraphicsDevice::BlendFactor::SrcAlpha, GraphicsDevice::BlendFactor::OneMinusSrcAlpha);

        if (useWireframe)
        {
            GraphicsDevice::Get().SetPolygonMode(GraphicsDevice::PolygonMode::Line);
        }
        else
        {
            GraphicsDevice::Get().SetPolygonMode(GraphicsDevice::PolygonMode::Fill);
        }

        if (mesh.TriangleCount > 0)
        {
            GraphicsDevice::Get().DrawIndexed(mesh.VAO, mesh.TriangleCount * 3);
        }
        else if (mesh.VAO->GetIndexBuffer() != nullptr)
        {
            GraphicsDevice::Get().DrawIndexedLines(mesh.VAO, mesh.VAO->GetIndexBuffer()->GetCount());
        }
        else
        {
            GraphicsDevice::Get().DrawLines(mesh.VAO, mesh.VertexCount);
        }
    }
}

void DrawCubeWires(const glm::mat4& transform, const glm::vec3& size, const glm::vec4& color, bool useWireframe)
{
    auto& rd = ServiceLocator::Get<Renderer>()->GetData();
    glm::mat4 model = transform * glm::scale(glm::mat4(1.0f), size);
    if (useWireframe && rd.Resources.WireCubeModel && !rd.Resources.WireCubeModel->Meshes.empty())
    {
        DrawMeshWire(rd.Resources.WireCubeModel->Meshes[0], color, model, true);
    }
    else if (rd.Resources.UnitCubeModel && !rd.Resources.UnitCubeModel->Meshes.empty())
    {
        DrawMeshWire(rd.Resources.UnitCubeModel->Meshes[0], color, model, useWireframe);
    }
}

void DrawCapsuleWires(const glm::mat4& transform, float radius, float height, const glm::vec4& color, bool useWireframe)
{
    auto& rd = ServiceLocator::Get<Renderer>()->GetData();
    glm::mat4 model = transform * glm::scale(glm::mat4(1.0f), glm::vec3(radius, height, radius));
    if (rd.Resources.UnitCapsuleModel)
    {
        for (auto& mesh : rd.Resources.UnitCapsuleModel->Meshes)
        {
            DrawMeshWire(mesh, color, model, useWireframe);
        }
    }
}

void DrawSphereWires(const glm::mat4& transform, float radius, const glm::vec4& color, bool useWireframe)
{
    auto& rd = ServiceLocator::Get<Renderer>()->GetData();
    glm::mat4 model = transform * glm::scale(glm::mat4(1.0f), glm::vec3(radius));
    if (rd.Resources.UnitSphereModel)
    {
        for (auto& mesh : rd.Resources.UnitSphereModel->Meshes)
        {
            DrawMeshWire(mesh, color, model, useWireframe);
        }
    }
}

void DrawInfiniteGrid(const Camera3D& camera, float spacing, const glm::vec4& color)
{
    auto& rd = ServiceLocator::Get<Renderer>()->GetData();
    auto shaderAsset = rd.Shaders->LoadOrGet("Grid");
    if (!shaderAsset || !shaderAsset->GetShader())
    {
        return;
    }

    auto guard = PipelineStateGuard::Capture();
    GraphicsDevice::Get().DisableDepthTest();
    GraphicsDevice::Get().SetBlendEnabled(true);
    GraphicsDevice::Get().SetBlendFunc(GraphicsDevice::BlendFactor::SrcAlpha, GraphicsDevice::BlendFactor::OneMinusSrcAlpha);

    shaderAsset->GetShader()->Bind();

    glm::vec3 planePos = {camera.Position.x, -0.005f, camera.Position.z};
    glm::mat4 model = glm::translate(glm::mat4(1.0f), planePos);
    model = glm::scale(model, glm::vec3(15000.0f, 1.0f, 15000.0f));

    shaderAsset->GetShader()->SetMatrix("u_ViewProjection", rd.CurrentProj * rd.CurrentView);
    shaderAsset->GetShader()->SetMatrix("u_Model", model);
    shaderAsset->GetShader()->SetVec3("u_CameraPos", camera.Position);
    shaderAsset->GetShader()->SetVec4("u_GridColor", color);
    shaderAsset->GetShader()->SetFloat("u_GridSize", spacing);
    shaderAsset->GetShader()->SetFloat("u_FadeStart", 100.0f);
    shaderAsset->GetShader()->SetFloat("u_FadeEnd", 8000.0f);

    if (!rd.GridPlaneVAO)
    {
        float vertices[] = {
            -0.5f, 0.0f, -0.5f,
             0.5f, 0.0f, -0.5f,
             0.5f, 0.0f,  0.5f,
            -0.5f, 0.0f,  0.5f,
        };
        uint32_t indices[] = {0, 1, 2, 2, 3, 0};

        auto vbo = VertexBuffer::Create(vertices, sizeof(vertices));
        vbo->SetLayout({{VertexAttributeType::Float3, "a_Position"}});

        rd.GridPlaneVAO = VertexArray::Create();
        rd.GridPlaneVAO->AddVertexBuffer(vbo);
        auto ibo = IndexBuffer::Create(indices, 6);
        rd.GridPlaneVAO->SetIndexBuffer(ibo);
    }

    GraphicsDevice::Get().DrawIndexed(rd.GridPlaneVAO, 6);
}
} // namespace DebugRenderer
} // namespace Chained
