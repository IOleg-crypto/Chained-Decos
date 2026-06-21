#include <string>
#include "engine/core/service_locator.h"
#include "engine/graphics/pipeline/renderer.h"
#include "engine/graphics/pipeline/render_command.h"
#include "engine/graphics/pipeline/shader_library.h"

#include "engine/assets/asset_manager.h"
#include "engine/graphics/ui/ui_renderer.h"
#include "engine/graphics/api/buffer.h"
#include "engine/graphics/pipeline/renderer3d.h"
#include "engine/graphics/pipeline/renderer2d.h"
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <string>

namespace Chained
{
    Renderer::Renderer(bool headless) : m_Headless(headless)
    {
    }

    Renderer::~Renderer()
    {
    }

    void Renderer::Initialize()
    {
        if (m_Headless)
        {
            CH_CORE_INFO("[Renderer] Headless mode enabled, skipping OpenGL initialization.");
            return;
        }

        m_Data = std::make_unique<RendererData>();
        m_Data->Shaders = std::make_unique<ShaderLibrary>();
        m_UI = std::make_unique<UIRenderer>();

        RenderCommand::Initialize();

        Renderer2D::Init();
        Renderer3D::Init();

        if (m_UI) m_UI->Initialize();

        LoadEngineResources();
    }

    void Renderer::Shutdown()
    {
        CH_CORE_INFO("Shutting down Render System...");
        
        Renderer3D::Shutdown();
        Renderer2D::Shutdown();

        m_Data.reset();
        m_UI.reset();
    }

void Renderer::Update(Timestep ts)
{
    UpdateTime(ts);
}

void Renderer::UpdateTime(Timestep ts)
{
    if (m_Data) m_Data->Time = ts;
}

void Renderer::SetDiagnosticMode(float mode)
{
    if (m_Data) m_Data->DiagnosticMode = mode;
}

void Renderer::LoadEngineResources()
{
    auto& shaders = GetShaderLibrary();

    auto loadShader = [&](const ::std::string& name, const ::std::string& path) { shaders.LoadOrGet(name, path); };

    loadShader(::std::string("Lighting"), ::std::string("resources/shaders/lighting.chshader"));
    loadShader(::std::string("Unlit"), ::std::string("resources/shaders/unlit.chshader"));

    CH_CORE_INFO("[Renderer] LoadEngineResources done. {} shader(s) loaded.", shaders.GetNames().size());
}



void Renderer::BeginScene(const Camera3D& camera, float nearClip, float farClip)
{
    m_Data->CurrentCameraPosition = camera.Position;

    // --- Direct glm::mat4 Management (Pure OpenGL style) ---
    // Calculate View Transform
    m_Data->CurrentView = camera.ViewMatrix;

    // Calculate Projection Transform
    m_Data->CurrentProj = camera.ProjectionMatrix;

    // Upload to UBO
    Chained::CameraData cameraData;
    cameraData.ViewProjection = m_Data->CurrentProj * m_Data->CurrentView;
    cameraData.Projection = m_Data->CurrentProj;
    cameraData.View = m_Data->CurrentView;

    if (m_Data->CameraUBO)
    {
        m_Data->CameraUBO->SetData(&cameraData, sizeof(Chained::CameraData));
    }
}

void Renderer::EndScene() {
    m_Data->CurrentShaderId = 0;
}

void Renderer::Clear(const glm::vec4& color) {
    if (m_Headless) return;

    Color chColor((unsigned char)(color.r * 255), (unsigned char)(color.g * 255),
                            (unsigned char)(color.b * 255), (unsigned char)(color.a * 255));
    RenderCommand::Clear(chColor);
}

void Renderer::SetViewport(int x, int y, int width, int height)
{
    m_ViewportWidth = width;
    m_ViewportHeight = height;

    if (m_Headless) return;

    RenderCommand::SetViewport(x, y, width, height);
}

ShaderLibrary& Renderer::GetShaderLibrary() { return *m_Data->Shaders; }
RendererData& Renderer::GetData() { return *m_Data; }
UIRenderer* Renderer::GetUIRenderer() { return m_UI.get(); }
bool Renderer::IsHeadless() { return m_Headless; }
uint32_t Renderer::GetViewportWidth() { return m_ViewportWidth; }
uint32_t Renderer::GetViewportHeight() { return m_ViewportHeight; }
void Renderer::SetHeadless(bool headless) { m_Headless = headless; }
void Renderer::SetViewportSize(uint32_t width, uint32_t height) { m_ViewportWidth = width; m_ViewportHeight = height; }

} // namespace Chained

