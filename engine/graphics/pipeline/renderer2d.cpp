#include "engine/graphics/pipeline/renderer2d.h"
#include "engine/graphics/pipeline/renderer.h"
#include "engine/graphics/pipeline/render_command.h"
#include "engine/assets/types/shader_asset.h"
#include "engine/graphics/api/buffer.h"
#include "engine/graphics/api/vertex_array.h"
#include "engine/core/log.h"
#include <glm/gtc/matrix_transform.hpp>
#include "engine/core/service_locator.h"

namespace Chained {

    struct LineVertex {
        glm::vec3 Position;
        glm::vec4 Color;
    };

    struct Renderer2DData
    {
        std::shared_ptr<VertexArray> FullscreenQuadVAO;
        std::shared_ptr<VertexArray> BillboardVAO;
        
        std::shared_ptr<VertexBuffer> LineVBO;
        std::shared_ptr<VertexArray> LineVAO;
    };

    static Renderer2DData s_2DData;

    void Renderer2D::Init()
    {
        // Fullscreen Quad
        float quadVertices[] = {-1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f,  -1.0f, 0.0f, 1.0f, 0.0f,
                                1.0f,  1.0f,  0.0f, 1.0f, 1.0f, -1.0f, 1.0f,  0.0f, 0.0f, 1.0f};
        uint32_t quadIndices[] = {0, 1, 2, 2, 3, 0};

        s_2DData.FullscreenQuadVAO = VertexArray::Create();
        auto qvbo = VertexBuffer::Create(quadVertices, sizeof(quadVertices));
        qvbo->SetLayout({
            {ShaderDataType::Float3, "vertexPosition"},
            {ShaderDataType::Float2, "vertexTexCoord"}
        });
        s_2DData.FullscreenQuadVAO->AddVertexBuffer(qvbo);
        auto qibo = IndexBuffer::Create(quadIndices, 6);
        s_2DData.FullscreenQuadVAO->SetIndexBuffer(qibo);

        // Billboard Quad
        float billVertices[] = {
            -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 0.5f,  -0.5f, 0.0f, 1.0f, 0.0f,
            0.5f,  0.5f,  0.0f, 1.0f, 1.0f, -0.5f, 0.5f,  0.0f, 0.0f, 1.0f,
        };
        uint32_t billIndices[] = {0, 1, 2, 2, 3, 0};

        auto bvbo = VertexBuffer::Create(billVertices, sizeof(billVertices));
        bvbo->SetLayout({{ShaderDataType::Float3, "a_Position"}, {ShaderDataType::Float2, "a_TexCoord"}});

        s_2DData.BillboardVAO = VertexArray::Create();
        s_2DData.BillboardVAO->AddVertexBuffer(bvbo);
        auto bibo = IndexBuffer::Create(billIndices, 6);
        s_2DData.BillboardVAO->SetIndexBuffer(bibo);
    }

    void Renderer2D::Shutdown()
    {
        s_2DData.FullscreenQuadVAO.reset();
        s_2DData.BillboardVAO.reset();
        s_2DData.LineVBO.reset();
        s_2DData.LineVAO.reset();
    }

    void Renderer2D::BeginScene(const Camera3D& camera)
    {
        // For now, Renderer2D uses the same UBOs as Renderer3D
    }

    void Renderer2D::EndScene()
    {
    }

    void Renderer2D::DrawSprite(uint32_t textureId, const glm::mat4& transform, const glm::vec4& tint, bool flipX, bool flipY)
    {
        auto unlitShaderAsset = ServiceLocator::Get<Renderer>()->GetShaderStorage().LoadOrGet("Unlit", "resources/shaders/unlit.chshader");
        if (!unlitShaderAsset || !unlitShaderAsset->GetShader() || textureId == 0) return;

        auto shader = unlitShaderAsset->GetShader();
        shader->Bind();

        auto& rd = ServiceLocator::Get<Renderer>()->GetData();
        shader->SetMatrix("mvp", rd.CurrentProj * rd.CurrentView * transform);
        shader->SetMatrix("matModel", transform);
        shader->SetMatrix("matNormal", glm::transpose(glm::inverse(transform)));
        shader->SetVec3("viewPos", rd.CurrentCameraPosition);
        shader->SetVec4("colDiffuse", tint);
        shader->SetInt("useTexture", 1);
        shader->SetInt("useSkinning", 0);

        RenderCommand::SetTexture(0, textureId);
        shader->SetInt("texture0", 0);

        bool blendEnabled = RenderCommand::IsBlendEnabled();
        RenderCommand::SetBlendMode(true);
        RenderCommand::SetBlendFunc(RendererAPI::BlendFactor::SrcAlpha, RendererAPI::BlendFactor::OneMinusSrcAlpha);

        if (s_2DData.BillboardVAO) // Re-use billboard quad for sprites for now
        {
            s_2DData.BillboardVAO->Bind();
            RenderCommand::DrawIndexed(s_2DData.BillboardVAO, 6);
            s_2DData.BillboardVAO->Unbind();
        }

        RenderCommand::SetBlendMode(blendEnabled);
    }

    void Renderer2D::DrawLine(const glm::vec3& start, const glm::vec3& end, const glm::vec4& color)
    {
        auto debugShader = ServiceLocator::Get<Renderer>()->GetShaderStorage().LoadOrGet("ColliderDebug", "resources/shaders/collider_debug.chshader");
        if (!debugShader || !debugShader->GetShader()) return;

        auto shader = debugShader->GetShader();
        shader->Bind();

        glm::mat4 vp = ServiceLocator::Get<Renderer>()->GetData().CurrentProj * ServiceLocator::Get<Renderer>()->GetData().CurrentView;
        shader->SetMatrix("u_ViewProj", vp);
        shader->SetMatrix("u_Transform", glm::mat4(1.0f));
        shader->SetVec4("u_Color", color);

        float vertices[] = {start.x, start.y, start.z, end.x, end.y, end.z};
        
        if (!s_2DData.LineVBO)
        {
            s_2DData.LineVBO = VertexBuffer::Create(sizeof(vertices));
            s_2DData.LineVBO->SetLayout({{ShaderDataType::Float3, "vertexPosition"}});
        }
        s_2DData.LineVBO->SetData(vertices, sizeof(vertices));

        if (!s_2DData.LineVAO)
        {
            s_2DData.LineVAO = VertexArray::Create();
            s_2DData.LineVAO->AddVertexBuffer(s_2DData.LineVBO);
        }

        s_2DData.LineVAO->Bind();
        RenderCommand::DrawLines(s_2DData.LineVAO, 2);
        s_2DData.LineVAO->Unbind();
    }

    void Renderer2D::DrawBillboard(const Camera3D& camera, uint32_t textureId, const glm::vec3& position, float size, const glm::vec4& tint)
    {
        auto unlitShaderAsset = ServiceLocator::Get<Renderer>()->GetShaderStorage().LoadOrGet("Unlit", "resources/shaders/unlit.chshader");
        if (!unlitShaderAsset || !unlitShaderAsset->GetShader() || textureId == 0) return;

        auto shader = unlitShaderAsset->GetShader();
        shader->Bind();

        glm::vec3 look = glm::normalize(camera.Position - position);
        glm::vec3 right = glm::cross(camera.Up, look);
        if (glm::length(right) < 0.0001f) right = glm::vec3(1.0f, 0.0f, 0.0f);
        else right = glm::normalize(right);
        glm::vec3 up = glm::normalize(glm::cross(look, right));

        glm::mat4 model = glm::mat4(1.0f);
        model[0] = glm::vec4(right * size, 0.0f);
        model[1] = glm::vec4(up * size, 0.0f);
        model[2] = glm::vec4(look * size, 0.0f);
        model[3] = glm::vec4(position, 1.0f);

        shader->SetMatrix("mvp", ServiceLocator::Get<Renderer>()->GetData().CurrentProj * ServiceLocator::Get<Renderer>()->GetData().CurrentView * model);
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

        if (s_2DData.BillboardVAO)
        {
            s_2DData.BillboardVAO->Bind();
            RenderCommand::DrawIndexed(s_2DData.BillboardVAO, 6);
            s_2DData.BillboardVAO->Unbind();
        }

        RenderCommand::SetCullMode(cullWasEnabled ? RendererAPI::CullMode::Back : RendererAPI::CullMode::None);
        RenderCommand::SetBlendMode(blendWasEnabled);
    }
}
