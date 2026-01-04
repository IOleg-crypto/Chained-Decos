#include "runtime_application.h"
#include "engine/core/input.h"
#include "engine/renderer/renderer.h"
#include "engine/scene/scene_serializer.h"
#include <filesystem>

namespace CH
{
class RuntimeLayer : public Layer
{
public:
    RuntimeLayer(Ref<Scene> scene) : Layer("RuntimeLayer"), m_Scene(scene)
    {
    }

    virtual void OnUpdate(float deltaTime) override;

    virtual void OnRender() override;

private:
    Ref<Scene> m_Scene;
};

void RuntimeLayer::OnUpdate(float deltaTime)
{
    if (m_Scene)
        m_Scene->OnUpdateRuntime(deltaTime);
}

void RuntimeLayer::OnRender()
{
    if (m_Scene)
    {
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

        Renderer::BeginScene(camera);
        Renderer::DrawScene(m_Scene.get());
        Renderer::EndScene();
    }
}

RuntimeApplication::RuntimeApplication(const Application::Config &config) : Application(config)
{
    m_ActiveScene = CreateRef<Scene>();

    // Load default scene if exists
    std::string scenePath = "assets/scenes/default.chscene";
    if (std::filesystem::exists(scenePath))
    {
        SceneSerializer serializer(m_ActiveScene.get());
        serializer.Deserialize(scenePath);
        CH_CORE_INFO("Loaded default runtime scene: {0}", scenePath);
    }
    else
    {
        CH_CORE_WARN("No default scene found at {0}. Starting empty.", scenePath);
    }

    PushLayer(new RuntimeLayer(m_ActiveScene));
}
} // namespace CH
