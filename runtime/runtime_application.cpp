#include "runtime_application.h"
#include "engine/core/application.h"
#include "engine/core/window.h"
#include "engine/graphics/texture_asset.h"
#include "engine/scene/project.h"
#include "engine/scene/scene.h"
#include "imgui.h"
#include "raymath.h"
#include <filesystem>

namespace CHEngine
{
class RuntimeLayer : public Layer
{
public:
    RuntimeLayer() : Layer("RuntimeLayer")
    {
    }

    virtual void OnUpdate(float deltaTime) override
    {
        auto scene = Application::Get().GetActiveScene();
        if (scene)
            scene->OnUpdateRuntime(deltaTime);
    }

    virtual void OnRender() override
    {
        auto activeScene = Application::Get().GetActiveScene();
        if (!activeScene)
            return;

        auto camera = GetActiveCamera();
        activeScene->OnRender(camera);
    }

    virtual void OnImGuiRender() override
    {
        auto activeScene = Application::Get().GetActiveScene();
        if (activeScene)
        {
            ImVec2 displaySize = ImGui::GetIO().DisplaySize;
            activeScene->OnImGuiRender({0, 0}, displaySize, 0, false);
        }
        else
        {
            // Simple Splash Screen / Loading
            ImGui::SetNextWindowPos({0, 0});
            ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
            ImGui::Begin("Splash", nullptr,
                         ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs |
                             ImGuiWindowFlags_NoBackground);

            const char *text = "Chained Decos - Loading...";
            ImVec2 textSize = ImGui::CalcTextSize(text);
            ImGui::SetCursorPos({(ImGui::GetIO().DisplaySize.x - textSize.x) * 0.5f,
                                 (ImGui::GetIO().DisplaySize.y - textSize.y) * 0.5f});
            ImGui::Text("%s", text);

            ImGui::End();
        }
    }

    Camera3D GetActiveCamera()
    {
        auto activeScene = Application::Get().GetActiveScene();
        if (activeScene)
            return activeScene->GetActiveCamera();

        // Fallback
        Camera3D camera = {0};
        camera.position = {10.0f, 10.0f, 10.0f};
        camera.target = {0.0f, 0.0f, 0.0f};
        camera.up = {0.0f, 1.0f, 0.0f};
        camera.fovy = 45.0f;
        camera.projection = CAMERA_PERSPECTIVE;
        return camera;
    }
};

RuntimeApplication::RuntimeApplication(const Application::Config &config,
                                       const std::string &projectPath)
    : Application(config), m_ProjectPath(projectPath)
{
    PushLayer(new RuntimeLayer());
}

void RuntimeApplication::PostInitialize()
{
    if (!m_ProjectPath.empty())
    {
        auto project = Project::Load(m_ProjectPath);
        if (project)
        {
            // Apply Project Settings
            auto &config = project->GetConfig();

            // Note: Window size is usually set in CreateApplication, but we can sync VSync/FPS here
            if (auto &window = GetWindow())
            {
                window->SetVSync(config.Window.VSync);
            }
            SetTargetFPS(60); // Or use a config value if added to ProjectConfig

            std::string sceneToLoad = config.StartScene;
            if (sceneToLoad.empty())
                sceneToLoad = config.ActiveScenePath.string();

            if (!sceneToLoad.empty())
            {
                LoadScene(sceneToLoad);
            }
        }
    }
}

} // namespace CHEngine
