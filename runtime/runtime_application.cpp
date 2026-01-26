#include "runtime_application.h"
#include "engine/core/input.h"
#include "engine/core/log.h"
#include "engine/render/asset_manager.h"
#include "engine/render/render.h"
#include "engine/scene/project.h"
#include "engine/scene/project_serializer.h"
#include "engine/scene/scene_serializer.h"
#include "engine/scene/scriptable_entity.h"
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
            scene->OnUpdateRuntime(
                deltaTime); // This now handles Physics, Scripting, Animation, etc.
    }

    virtual void OnRender() override
    {
        auto scene = Application::Get().GetActiveScene();
        if (!scene)
            return;

        // Runtime Camera - looking for player
        auto view = scene->GetRegistry().view<PlayerComponent, TransformComponent>();
        Camera3D camera = {0};

        if (view.begin() != view.end())
        {
            auto entity = *view.begin();
            auto &transform = view.get<TransformComponent>(entity);
            auto &player = view.get<PlayerComponent>(entity);

            Vector3 target = transform.Translation;
            target.y += 1.0f;

            float yawRad = player.CameraYaw * DEG2RAD;
            float pitchRad = player.CameraPitch * DEG2RAD;
            Vector3 offset = {player.CameraDistance * cosf(pitchRad) * sinf(yawRad),
                              player.CameraDistance * sinf(pitchRad),
                              player.CameraDistance * cosf(pitchRad) * cosf(yawRad)};

            camera.position = Vector3Add(target, offset);
            camera.target = target;
            camera.up = {0.0f, 1.0f, 0.0f};
            camera.fovy = 60.0f;
            camera.projection = CAMERA_PERSPECTIVE;
        }
        else
        {
            // Fallback camera
            camera.position = {10, 10, 10};
            camera.target = {0, 0, 0};
            camera.up = {0, 1, 0};
            camera.fovy = 45;
            camera.projection = CAMERA_PERSPECTIVE;
        }

        // 1. Clear Background based on scene settings
        auto mode = scene->GetBackgroundMode();
        if (mode == BackgroundMode::Color)
        {
            ClearBackground(scene->GetBackgroundColor());
        }
        else if (mode == BackgroundMode::Texture)
        {
            auto path = scene->GetBackgroundTexturePath();
            if (!path.empty())
            {
                auto tex = AssetManager::Get<TextureAsset>(path);
                if (tex && tex->IsReady())
                {
                    DrawTexturePro(
                        tex->GetTexture(),
                        {0, 0, (float)tex->GetTexture().width, (float)tex->GetTexture().height},
                        {0, 0, (float)GetScreenWidth(), (float)GetScreenHeight()}, {0, 0}, 0.0f,
                        WHITE);
                }
                else
                {
                    ClearBackground(scene->GetBackgroundColor());
                }
            }
            else
            {
                ClearBackground(scene->GetBackgroundColor());
            }
        }
        else // Environment3D
        {
            // Skybox handles background in DrawScene usually
            ClearBackground(BLACK);
        }

        Visuals::BeginScene(camera);
        scene->OnRender(camera);
        Visuals::EndScene();
    }

    virtual void OnImGuiRender() override
    {
        auto scene = Application::Get().GetActiveScene();
        if (scene)
        {
            // Setup a fullscreen transparent window for the game UI
            ImGuiViewport *viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(viewport->WorkPos);
            ImGui::SetNextWindowSize(viewport->WorkSize);
            ImGui::SetNextWindowViewport(viewport->ID);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

            ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoNav |
                                            ImGuiWindowFlags_NoBringToFrontOnFocus |
                                            ImGuiWindowFlags_NoBackground;

            ImGui::Begin("##GameUI", nullptr, window_flags);
            // Explicitly pass full screen size to ensure UI anchors work correctly
            scene->OnImGuiRender(ImGui::GetWindowPos(), ImGui::GetWindowSize(),
                                 ImGui::GetWindowViewport()->ID, false);
            ImGui::End();

            ImGui::PopStyleVar(3);
        }
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
    if (!GetStartupScene().empty())
    {
        CH_CORE_INFO("Runtime: Using programmatically set startup scene: {}", GetStartupScene());
        LoadScene(GetStartupScene());
        return;
    }

    if (!m_ProjectPath.empty())
    {
        std::filesystem::path absolutePath = std::filesystem::absolute(m_ProjectPath);
        if (std::filesystem::exists(absolutePath))
        {
            std::shared_ptr<Project> project = std::make_shared<Project>();
            ProjectSerializer serializer(project);
            if (serializer.Deserialize(absolutePath))
            {
                Project::SetActive(project);
                std::filesystem::path startScene = project->GetConfig().StartScene;
                std::filesystem::path startScenePath = project->GetAssetDirectory() / startScene;

                if (std::filesystem::exists(startScenePath))
                {
                    LoadScene(startScenePath.string());
                }
                else
                {
                    // Try searching in the same directory as the project
                    std::filesystem::path fallbackScene =
                        absolutePath.parent_path() / "assets" / "scenes" / startScene;
                    if (std::filesystem::exists(fallbackScene))
                    {
                        LoadScene(fallbackScene.string());
                    }
                    else
                    {
                        CH_CORE_WARN("Runtime: Project start scene not found: {}",
                                     startScenePath.string().c_str());
                    }
                }
            }
            else
            {
                CH_CORE_ERROR("Runtime: Failed to deserialize project: {}",
                              absolutePath.string().c_str());
            }
        }
    }
    else
    {
        // Try fallback to project.chproj in CWD
        if (std::filesystem::exists("project.chproject"))
        {
            LoadScene("assets/scenes/default.chscene"); // Placeholder
        }
        else if (std::filesystem::exists("assets/scenes/default.chscene"))
        {
            LoadScene("assets/scenes/default.chscene");
        }
    }
}
} // namespace CHEngine
