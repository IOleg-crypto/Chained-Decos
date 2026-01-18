#include "runtime_application.h"
#include "engine/core/input.h"
#include "engine/core/log.h"
#include "engine/renderer/render.h"
#include "engine/renderer/scene_render.h"
#include "engine/scene/project.h"
#include "engine/scene/project_serializer.h"
#include "engine/scene/scene_serializer.h"
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

        SceneRender::BeginScene(scene.get(), camera);
        SceneRender::SubmitScene(scene.get());
        SceneRender::EndScene();
    }
};

RuntimeApplication::RuntimeApplication(const Application::Config &config,
                                       const std::string &projectPath)
    : Application(config)
{
    if (!projectPath.empty())
    {
        std::filesystem::path absolutePath = std::filesystem::absolute(projectPath);
        if (std::filesystem::exists(absolutePath))
        {
            Ref<Project> project = CreateRef<Project>();
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
                        CH_CORE_WARN("Runtime: Project start scene not found: %s",
                                     startScenePath.string().c_str());
                    }
                }
            }
            else
            {
                CH_CORE_ERROR("Runtime: Failed to deserialize project: %s",
                              absolutePath.string().c_str());
            }
        }
    }
    else
    {
        // Try fallback to assets/scenes/default.chscene
        std::string fallbackPath = "assets/scenes/default.chscene";
        if (std::filesystem::exists(fallbackPath))
        {
            LoadScene(fallbackPath);
        }
    }

    PushLayer(new RuntimeLayer());
}
} // namespace CHEngine
