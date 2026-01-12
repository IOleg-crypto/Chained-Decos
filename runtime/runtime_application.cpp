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
    RuntimeLayer(Ref<Scene> scene) : Layer("RuntimeLayer"), m_Scene(scene)
    {
    }

    virtual void OnUpdate(float deltaTime) override
    {
        if (m_Scene)
            m_Scene->OnUpdateRuntime(deltaTime);
    }

    virtual void OnRender() override
    {
        if (!m_Scene)
            return;

        // Runtime Camera - looking for player
        auto view = m_Scene->GetRegistry().view<PlayerComponent, TransformComponent>();
        Camera3D camera = {0};
        bool playerFound = false;

        if (view.begin() != view.end())
        {
            auto entity = *view.begin();
            auto &transform = view.get<TransformComponent>(entity);
            auto &player = view.get<PlayerComponent>(entity);

            // Mouse/Zoom control
            if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
            {
                Vector2 mouseDelta = GetMouseDelta();
                player.CameraYaw -= mouseDelta.x * player.LookSensitivity;
                player.CameraPitch -= mouseDelta.y * player.LookSensitivity;

                if (player.CameraPitch > 89.0f)
                    player.CameraPitch = 89.0f;
                if (player.CameraPitch < -10.0f)
                    player.CameraPitch = -10.0f;
            }
            player.CameraDistance -= GetMouseWheelMove() * 2.0f;
            if (player.CameraDistance < 2.0f)
                player.CameraDistance = 2.0f;
            if (player.CameraDistance > 40.0f)
                player.CameraDistance = 40.0f;

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
            playerFound = true;
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

        SceneRender::BeginScene(m_Scene.get(), camera);
        SceneRender::SubmitScene(m_Scene.get());
        SceneRender::EndScene();
    }

private:
    Ref<Scene> m_Scene;
};

RuntimeApplication::RuntimeApplication(const Application::Config &config,
                                       const std::string &projectPath)
    : Application(config)
{
    m_ActiveScene = CreateRef<Scene>();

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
                    SceneSerializer sceneSerializer(m_ActiveScene.get());
                    sceneSerializer.Deserialize(startScenePath.string());
                    CH_CORE_INFO("Runtime: Loaded project start scene: %s",
                                 startScenePath.string().c_str());
                }
                else
                {
                    // Try searching in the same directory as the project
                    std::filesystem::path fallbackScene =
                        absolutePath.parent_path() / "assets" / "scenes" / startScene;
                    if (std::filesystem::exists(fallbackScene))
                    {
                        SceneSerializer sceneSerializer(m_ActiveScene.get());
                        sceneSerializer.Deserialize(fallbackScene.string());
                        CH_CORE_INFO("Runtime: Loaded fallback project scene: %s",
                                     fallbackScene.string().c_str());
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
        else
        {
            CH_CORE_ERROR("Runtime: Project path does not exist: %s",
                          absolutePath.string().c_str());
        }
    }
    else
    {
        // Try fallback to assets/scenes/default.chscene
        std::string fallbackPath = "assets/scenes/default.chscene";
        if (std::filesystem::exists(fallbackPath))
        {
            SceneSerializer serializer(m_ActiveScene.get());
            serializer.Deserialize(fallbackPath);
            CH_CORE_INFO("Runtime: Loaded default scene: %s", fallbackPath.c_str());
        }
        else
        {
            CH_CORE_WARN("Runtime: No scene found to load.");
        }
    }

    PushLayer(new RuntimeLayer(m_ActiveScene));
}
} // namespace CHEngine
