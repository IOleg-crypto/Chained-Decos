#include "engine/graphics/pipeline/debug_renderer.h"
#include "engine/assets/asset_manager.h"
#include "engine/assets/types/shader_asset.h"
#include "engine/core/service_locator.h"
#include "engine/graphics/pipeline/geometry_generator.h"
#include "engine/graphics/api/graphics_device.h"
#include "engine/graphics/pipeline/renderer.h"
#include <glm/gtc/matrix_transform.hpp>

namespace Chained
{

void DebugRendererService::Initialize()
{
    Resources.UnitCubeModel = std::make_unique<Model>();
    Resources.UnitCubeModel->Meshes.push_back(GeometryGenerator::GenerateUnitCube());

    Resources.UnitSphereModel = std::make_unique<Model>();
    Resources.UnitSphereModel->Meshes.push_back(GeometryGenerator::GenerateSphere(1.0f, 32, 32));

    Resources.UnitCapsuleModel = std::make_unique<Model>();
    Resources.UnitCapsuleModel->Meshes.push_back(GeometryGenerator::GenerateCapsule(1.0f, 2.0f, 32, 32));

    Resources.WireCubeModel = std::make_unique<Model>();
    Resources.WireCubeModel->Meshes.push_back(GeometryGenerator::GenerateWireCube());
}

void DebugRendererService::Shutdown()
{
    GridPlaneVAO.reset();
    
    Resources.UnitCubeModel.reset();
    Resources.UnitSphereModel.reset();
    Resources.UnitCapsuleModel.reset();
    Resources.WireCubeModel.reset();

    Lines.VBO.reset();
    Lines.VAO.reset();
    Lines.Vertices.clear();
}

namespace DebugRenderer
{
void DrawLine(const glm::vec3& start, const glm::vec3& end, const glm::vec4& color)
{
    auto* dbg = ServiceLocator::Get<DebugRendererService>();
    if (!dbg) return;
    dbg->Lines.Vertices.push_back({start, color});
    dbg->Lines.Vertices.push_back({end, color});
}

void Flush()
{
    auto* dbg = ServiceLocator::Get<DebugRendererService>();
    if (!dbg || dbg->Lines.Vertices.empty()) return;

    auto& rd = ServiceLocator::Get<Renderer>()->GetData();

    auto debugShader = rd.Shaders->LoadOrGet("ColliderDebug");
    if (!debugShader || !debugShader->GetShader())
    {
        dbg->Lines.Vertices.clear();
        return;
    }

    auto shader = debugShader->GetShader();
    shader->Bind();

    glm::mat4 vp = rd.Frame.Proj * rd.Frame.View;
    shader->SetMatrix("u_ViewProj", vp);
    shader->SetMatrix("u_Transform", glm::mat4(1.0f));
    shader->SetVec4("u_Color", glm::vec4(1.0f)); // color is per-vertex

    uint32_t dataSize = (uint32_t)(dbg->Lines.Vertices.size() * sizeof(LineVertex));

    if (!dbg->Lines.VBO || dbg->Lines.VBOSize < dataSize)
    {
        dbg->Lines.VBOSize = std::max(dataSize, (uint32_t)(1024 * sizeof(LineVertex)));
        dbg->Lines.VBO = VertexBuffer::Create(dbg->Lines.VBOSize);
        dbg->Lines.VBO->SetLayout({{VertexAttributeType::Float3, "vertexPosition"}, {VertexAttributeType::Float4, "vertexColor"}});
        dbg->Lines.VAO = VertexArray::Create();
        dbg->Lines.VAO->AddVertexBuffer(dbg->Lines.VBO);
    }

    dbg->Lines.VBO->SetData(dbg->Lines.Vertices.data(), dataSize);
    GraphicsDevice::Get().DrawLines(dbg->Lines.VAO, (uint32_t)dbg->Lines.Vertices.size());
    dbg->Lines.Vertices.clear();
}

void DrawMeshWire(const Mesh& mesh, const glm::vec4& color, const glm::mat4& transform, bool useWireframe)
{
    auto& rd = ServiceLocator::Get<Renderer>()->GetData();
    auto debugShader = rd.Shaders->LoadOrGet("ColliderDebug");
    if (!debugShader || !debugShader->GetShader()) return;

    auto shader = debugShader->GetShader();
    shader->Bind();

    glm::mat4 vp = rd.Frame.Proj * rd.Frame.View;
    shader->SetMatrix("u_ViewProj", vp);
    shader->SetMatrix("u_Transform", transform);

    glm::vec4 finalColor = color;
    if (!useWireframe) finalColor.a *= 0.35f;
    shader->SetVec4("u_Color", finalColor);

    if (mesh.VAO)
    {
        auto guard = PipelineStateGuard::Capture();
        guard.WithBlend().WithPolygonMode();
        GraphicsDevice::Get().SetBlendEnabled(true);
        GraphicsDevice::Get().SetBlendFunc(GraphicsDevice::BlendFactor::SrcAlpha, GraphicsDevice::BlendFactor::OneMinusSrcAlpha);

        if (useWireframe)
            GraphicsDevice::Get().SetPolygonMode(GraphicsDevice::PolygonMode::Line);
        else
            GraphicsDevice::Get().SetPolygonMode(GraphicsDevice::PolygonMode::Fill);

        if (mesh.TriangleCount > 0)
            GraphicsDevice::Get().DrawIndexed(mesh.VAO, mesh.TriangleCount * 3);
        else if (mesh.VAO->GetIndexBuffer() != nullptr)
            GraphicsDevice::Get().DrawIndexedLines(mesh.VAO, mesh.VAO->GetIndexBuffer()->GetCount());
        else
            GraphicsDevice::Get().DrawLines(mesh.VAO, mesh.VertexCount);
    }
}

void DrawCubeWires(const glm::mat4& transform, const glm::vec3& size, const glm::vec4& color, bool useWireframe)
{
    auto* dbg = ServiceLocator::Get<DebugRendererService>();
    if (!dbg) return;
    glm::mat4 model = transform * glm::scale(glm::mat4(1.0f), size);
    if (useWireframe && dbg->Resources.WireCubeModel && !dbg->Resources.WireCubeModel->Meshes.empty())
        DrawMeshWire(dbg->Resources.WireCubeModel->Meshes[0], color, model, true);
    else if (dbg->Resources.UnitCubeModel && !dbg->Resources.UnitCubeModel->Meshes.empty())
        DrawMeshWire(dbg->Resources.UnitCubeModel->Meshes[0], color, model, useWireframe);
}

void DrawCapsuleWires(const glm::mat4& transform, float radius, float height, const glm::vec4& color, bool useWireframe)
{
    auto* dbg = ServiceLocator::Get<DebugRendererService>();
    if (!dbg || !dbg->Resources.UnitCapsuleModel) return;
    glm::mat4 model = transform * glm::scale(glm::mat4(1.0f), glm::vec3(radius, height, radius));
    for (auto& mesh : dbg->Resources.UnitCapsuleModel->Meshes)
        DrawMeshWire(mesh, color, model, useWireframe);
}

void DrawSphereWires(const glm::mat4& transform, float radius, const glm::vec4& color, bool useWireframe)
{
    auto* dbg = ServiceLocator::Get<DebugRendererService>();
    if (!dbg || !dbg->Resources.UnitSphereModel) return;
    glm::mat4 model = transform * glm::scale(glm::mat4(1.0f), glm::vec3(radius));
    for (auto& mesh : dbg->Resources.UnitSphereModel->Meshes)
        DrawMeshWire(mesh, color, model, useWireframe);
}

void DrawInfiniteGrid(const Camera3D& camera, float spacing, const glm::vec4& color)
{
    auto* dbg = ServiceLocator::Get<DebugRendererService>();
    auto& rd = ServiceLocator::Get<Renderer>()->GetData();
    if (!dbg) return;
    
    auto shaderAsset = rd.Shaders->LoadOrGet("Grid");
    if (!shaderAsset || !shaderAsset->GetShader()) return;

    auto guard = PipelineStateGuard::Capture();
    GraphicsDevice::Get().SetCullMode(GraphicsDevice::CullMode::None);
    GraphicsDevice::Get().DisableDepthTest();
    GraphicsDevice::Get().SetBlendEnabled(true);
    GraphicsDevice::Get().SetBlendFunc(GraphicsDevice::BlendFactor::SrcAlpha, GraphicsDevice::BlendFactor::OneMinusSrcAlpha);

    shaderAsset->GetShader()->Bind();

    glm::vec3 planePos = {camera.Position.x, -0.005f, camera.Position.z};
    glm::mat4 model = glm::translate(glm::mat4(1.0f), planePos);
    model = glm::scale(model, glm::vec3(15000.0f, 1.0f, 15000.0f));

    shaderAsset->GetShader()->SetMatrix("u_ViewProjection", rd.Frame.Proj * rd.Frame.View);
    shaderAsset->GetShader()->SetMatrix("u_Model", model);
    shaderAsset->GetShader()->SetVec3("u_CameraPos", camera.Position);
    shaderAsset->GetShader()->SetVec4("u_GridColor", color);
    shaderAsset->GetShader()->SetFloat("u_GridSize", spacing);
    shaderAsset->GetShader()->SetFloat("u_FadeStart", 0.0f);
    shaderAsset->GetShader()->SetFloat("u_FadeEnd", 8000.0f);

    if (!dbg->GridPlaneVAO)
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

        dbg->GridPlaneVAO = VertexArray::Create();
        dbg->GridPlaneVAO->AddVertexBuffer(vbo);
        auto ibo = IndexBuffer::Create(indices, 6);
        dbg->GridPlaneVAO->SetIndexBuffer(ibo);
    }

    GraphicsDevice::Get().DrawIndexed(dbg->GridPlaneVAO, 6);
}
} // namespace DebugRenderer
} // namespace Chained
