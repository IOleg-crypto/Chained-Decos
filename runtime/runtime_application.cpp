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
            scene->OnUpdateRuntime(deltaTime);
    }

    virtual void OnRender() override
    {
        auto scene = Application::Get().GetActiveScene();
        if (!scene)
            return;

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
            camera.position = {10, 10, 10};
            camera.target = {0, 0, 0};
            camera.up = {0, 1, 0};
            camera.fovy = 45;
            camera.projection = CAMERA_PERSPECTIVE;
        }

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
        else
        {
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

    std::filesystem::path projectToLoad = m_ProjectPath;

    if (projectToLoad.empty())
    {
        for (const auto &entry : std::filesystem::directory_iterator("."))
        {
            if (entry.path().extension() == ".chproject")
            {
                projectToLoad = entry.path();
                break;
            }
        }

        if (projectToLoad.empty() && std::filesystem::exists("game"))
        {
            for (const auto &gameEntry : std::filesystem::recursive_directory_iterator("game"))
            {
                if (gameEntry.path().extension() == ".chproject")
                {
                    projectToLoad = gameEntry.path();
                    CH_CORE_INFO("Runtime: Discovered project in game folder: {}",
                                 projectToLoad.string());
                    break;
                }
            }
        }
    }

    if (!projectToLoad.empty() && std::filesystem::exists(projectToLoad))
    {
        auto project = Project::Load(projectToLoad);
        if (project)
        {
            CH_CORE_INFO("Runtime: Loaded project: {}", project->GetConfig().Name);

            std::string sceneToLoad = project->GetConfig().StartScene;
            if (sceneToLoad.empty())
                sceneToLoad = project->GetConfig().ActiveScenePath.string();

            if (!sceneToLoad.empty())
            {
                LoadScene(sceneToLoad);
            }
            else
            {
                CH_CORE_ERROR("Runtime: No scene specified in project configuration.");
            }
        }
        else
        {
            CH_CORE_ERROR("Runtime: Failed to load project file.");
        }
    }
    else
    {
        if (std::filesystem::exists("assets/scenes/default.chscene"))
        {
            LoadScene("assets/scenes/default.chscene");
        }
        else
        {
            CH_CORE_ERROR("Runtime: No project or default scene found to launch.");
        }
    }
}
} // namespace CHEngine
