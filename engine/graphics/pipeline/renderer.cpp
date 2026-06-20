#include <string>
#include "engine/core/service_locator.h"
#include <string_view>
#include "engine/graphics/pipeline/renderer.h"
#include "engine/graphics/pipeline/geometry_generator.h"
#include "engine/graphics/pipeline/render_command.h"
#include "engine/graphics/pipeline/renderer_types.h"
#include "engine/graphics/pipeline/shader_library.h"

#include "engine/assets/asset_manager.h"
#include "engine/graphics/ui/ui_renderer.h"
#include "engine/graphics/api/buffer.h"
#include "engine/graphics/api/storage_buffer.h"
#include "engine/graphics/api/vertex_array.h"
#include "engine/graphics/pipeline/renderer3d.h"
#include "engine/graphics/pipeline/renderer2d.h"
#include "engine/assets/types/environment_asset.h"
#include "engine/assets/types/shader_asset.h"
#include "engine/assets/types/texture_asset.h"
#include <algorithm>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <string>

namespace Chained
{
    
static std::unique_ptr<Chained::RendererData> s_Data;
static bool s_Headless = false;
static uint32_t s_ViewportWidth = 1280;
static uint32_t s_ViewportHeight = 720;
static std::unique_ptr<Chained::UIRenderer> s_UI;

Renderer* Renderer::s_Instance = nullptr;


    void Renderer::Init(bool headless)
    {
        static Renderer instance;
        s_Instance = &instance;

        s_Headless = headless;
        
        if (s_Headless)
        {
            CH_CORE_INFO("[Renderer] Headless mode enabled, skipping OpenGL initialization.");
            return;
        }

        s_Data = std::make_unique<RendererData>();
        s_Data->Shaders = std::make_unique<ShaderLibrary>();
        s_UI = std::make_unique<UIRenderer>();

        RenderCommand::Initialize();

        Renderer2D::Init();
        Renderer3D::Init();

        if (s_UI) s_UI->Init(ServiceLocator::Get<AssetManager>());

        LoadEngineResources();
    }

    void Renderer::Shutdown()
    {
        CH_CORE_INFO("Shutting down Render System...");
        
        Renderer3D::Shutdown();
        Renderer2D::Shutdown();

        s_Data.reset();
        s_UI.reset();
        s_Instance = nullptr;
    }

void Renderer::Update(Timestep ts)
{
    UpdateTime(ts);
}

void Renderer::UpdateTime(Timestep ts)
{
    if (s_Data) s_Data->Time = ts;
}

void Renderer::SetDiagnosticMode(float mode)
{
    if (s_Data) s_Data->DiagnosticMode = mode;
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
    s_Data->CurrentCameraPosition = camera.Position;

    // --- Direct glm::mat4 Management (Pure OpenGL style) ---
    // 1. Calculate View Transform
    s_Data->CurrentView = glm::lookAt(camera.Position, camera.Target, camera.Up);

    // 2. Calculate Projection Transform
    int width = s_ViewportWidth;
    int height = s_ViewportHeight;
    float aspect = (height > 0) ? (float)width / (float)height : 1.0f;
    if (camera.Projection == 0 /* CAMERA_PERSPECTIVE */)
    {
        s_Data->CurrentProj = glm::perspective(glm::radians(camera.FovY), aspect, nearClip, farClip);
    }
    else
    {
        float top = camera.FovY / 2.0f;
        float right = top * aspect;
        s_Data->CurrentProj = glm::ortho(-right, right, -top, top, nearClip, farClip);
    }

    // Upload to UBO
    Chained::CameraData cameraData;
    cameraData.ViewProjection = s_Data->CurrentProj * s_Data->CurrentView;
    cameraData.Projection = s_Data->CurrentProj;
    cameraData.View = s_Data->CurrentView;

    if (s_Data->CameraUBO)
    {
        s_Data->CameraUBO->SetData(&cameraData, sizeof(Chained::CameraData));
    }
}

void Renderer::EndScene() {
    s_Data->CurrentShaderId = 0;
}

void Renderer::Clear(const glm::vec4& color) {
    if (s_Headless) return;

    Color chColor((unsigned char)(color.r * 255), (unsigned char)(color.g * 255),
                            (unsigned char)(color.b * 255), (unsigned char)(color.a * 255));
    RenderCommand::Clear(chColor);
}

void Renderer::SetViewport(int x, int y, int width, int height)
{
    s_ViewportWidth = width;
    s_ViewportHeight = height;

    if (s_Headless) return;

    RenderCommand::SetViewport(x, y, width, height);
}

ShaderLibrary& Renderer::GetShaderLibrary() { return *s_Data->Shaders; }
RendererData& Renderer::GetData() { return *s_Data; }
UIRenderer* Renderer::GetUIRenderer() { return s_UI.get(); }
bool Renderer::IsHeadless() { return s_Headless; }
uint32_t Renderer::GetViewportWidth() { return s_ViewportWidth; }
uint32_t Renderer::GetViewportHeight() { return s_ViewportHeight; }
void Renderer::SetHeadless(bool headless) { s_Headless = headless; }
void Renderer::SetViewportSize(uint32_t width, uint32_t height) { s_ViewportWidth = width; s_ViewportHeight = height; }

} // namespace Chained

