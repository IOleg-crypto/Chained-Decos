#include "scene_manager.h"
#include "engine/assets/asset_manager.h"
#include "engine/common/thread_pool.h"
#include "engine/core/input.h"
#include "engine/core/key_codes.h"
#include "engine/core/service_locator.h"
#include "engine/graphics/ui/widget_renderer.h"
#include "engine/platform/dialogs/dialogs.h"
#include "engine/project/project.h"
#include "engine/scene/scene.h"
#include "engine/scene/scene_events.h"
#include "engine/scene/scene_serializer.h"
#include "layer.h"
#include "scripting/scene_scripting_manager.h"

namespace Chained
{

EditorSceneManager::EditorSceneManager(CommandHistory& cmd, EditorProjectManager& proj, EditorConfig& config,
                                       ImVec2& viewportSize, EditorState& state, const SceneContext& sceneContext)
    : m_CommandHistory(cmd),
      m_ProjectManager(proj),
      m_Config(config),
      m_ViewportSize(viewportSize),
      m_EditorState(state),
      m_Context(sceneContext)
{
}

void EditorSceneManager::NewScene()
{
    SetScene(Scene::CreateDefault());
}

void EditorSceneManager::OpenScene()
{
    std::vector<DialogFilter> filters = {{".chscene", "chscene"}};
    auto result = Chained::Dialogs::OpenFile(filters);
    if (result)
    {
        OpenScene(*result);
    }
}

void EditorSceneManager::OpenScene(const std::filesystem::path& path)
{
    CH_CORE_INFO("EditorSceneManager: Transition requested to '{}'", path.string());
    if (m_IsSceneOpenLoading)
    {
        CH_CORE_WARN("EditorSceneManager: Transition to '{}' ignored - already loading '{}'", path.string(),
                     m_PendingSceneOpenPath.string());
        return;
    }

    // Query state directly from the manager (i.e. from the scene)
    m_IsPlayModeSceneLoad = GetSceneState() == SceneState::Play;
    StartSceneOpenTransition(path);
}

void EditorSceneManager::SaveScene()
{
    auto scene = GetActiveScene();
    if (!scene)
    {
        return;
    }

    if (scene->GetSettings().ScenePath.empty())
    {
        SaveSceneAs();
        return;
    }

    SceneSerializer serializer(scene.get());
    serializer.Serialize(scene->GetSettings().ScenePath);
    CH_INFO("Scene saved to {0}", scene->GetSettings().ScenePath);
}

void EditorSceneManager::SaveSceneAs()
{
    std::vector<DialogFilter> filters = {{"Chained Scene", "chscene"}};
    auto result = Chained::Dialogs::SaveFile(filters);
    if (result)
    {
        auto scene = GetActiveScene();
        if (!scene)
        {
            return;
        }

        scene->GetSettings().ScenePath = result->string();
        SceneSerializer serializer(scene.get());
        serializer.Serialize(result->string());
    }
}

void EditorSceneManager::AutoSave(float interval, float ts)
{
    auto scene = GetActiveScene();
    if (!scene || scene->GetSettings().ScenePath.empty())
    {
        return;
    }

    m_AutoSaveTimer += ts;
    if (m_AutoSaveTimer < interval)
    {
        return;
    }

    m_AutoSaveTimer = 0.0f;
    SceneSerializer serializer(scene.get());
    serializer.Serialize(scene->GetSettings().ScenePath);
    CH_TRACE("Scene auto-saved to {0}", scene->GetSettings().ScenePath);
}

void EditorSceneManager::SetScene(const std::shared_ptr<Scene>& scene)
{
    CancelPlayModeTransition();
    CancelSceneOpenTransition();

    m_EditorScene = scene;
    if (m_EditorScene)
    {
        m_EditorScene->TransitionToState(SceneState::Edit, m_Context);
    }

    m_EditorState.SelectedEntity = {};
}

SceneState EditorSceneManager::GetSceneState() const
{
    auto activeScene = GetActiveScene();
    return activeScene ? activeScene->GetSceneState() : SceneState::Edit;
}

std::shared_ptr<Scene> EditorSceneManager::GetActiveScene() const
{
    // If the runtime scene exists — we are in Play/Simulate mode
    if (m_RuntimeScene)
    {
        return m_RuntimeScene;
    }
    return m_EditorScene;
}

void EditorSceneManager::SetSceneState(SceneState state)
{
    SceneState currentState = GetSceneState();

    if (state == SceneState::Play || state == SceneState::Simulate)
    {
        if (m_PlayModeStartRequested || m_IsPlayModeLoading || m_IsSceneOpenLoading ||
            currentState == SceneState::Play || currentState == SceneState::Simulate)
        {
            return;
        }

        if (!m_EditorScene)
        {
            CH_CORE_WARN("EditorSceneManager::SetSceneState - No editor scene available for play mode.");
            return;
        }

        m_PlayModeStartRequested = true;
        m_TargetState = state; // Store target for transition
        CH_CORE_INFO("Editor: {} mode requested.", state == SceneState::Play ? "Play" : "Simulate");
    }
    else // Returning to SceneState::Edit (Stop)
    {
        if (m_PlayModeStartRequested || m_IsPlayModeLoading)
        {
            CancelPlayModeTransition();
        }

        if (m_IsSceneOpenLoading)
        {
            CancelSceneOpenTransition();
        }

        if (currentState == SceneState::Edit)
        {
            return;
        }

        CH_CORE_INFO("Editor: Play Mode Stopped");
        if (m_RuntimeScene)
        {
            CH_CORE_INFO("Editor: Cleaning up runtime scene...");
            m_RuntimeScene->OnRuntimeStop(m_Context);
            m_RuntimeScene.reset();
        }

        if (m_EditorScene)
        {
            m_EditorScene->TransitionToState(SceneState::Edit, m_Context);
        }

        m_EditorState.SelectedEntity = {};
    }
}

void EditorSceneManager::OnUpdate(Timestep ts)
{
    if (m_PlayModeStartRequested)
    {
        StartPlayModeTransition();
    }

    if (m_IsPlayModeLoading)
    {
        UpdatePlayModeTransition();
    }

    if (m_IsSceneOpenLoading)
    {
        UpdateSceneOpenTransition();
    }
}

void EditorSceneManager::OnViewportResize(uint32_t width, uint32_t height)
{
    if (m_EditorScene)
    {
        m_EditorScene->OnViewportResize(width, height);
    }

    if (m_RuntimeScene)
    {
        m_RuntimeScene->OnViewportResize(width, height);
    }
}

void EditorSceneManager::StartSceneOpenTransition(const std::filesystem::path& path)
{
    if (path.empty() || m_IsSceneOpenLoading)
    {
        return;
    }

    CancelPlayModeTransition();

    std::filesystem::path scenePath = path;
    if (scenePath.is_relative())
    {
        if (auto project = Project::GetActive())
        {
            scenePath = project->GetAssetPathForProject(scenePath);
        }
    }

    m_PendingSceneOpenPath = scenePath;
    m_SceneOpenSceneReady = false;
    m_LoadingStatus = "Loading scene...";

    try
    {
        m_SceneOpenFuture = ServiceLocator::Get<ThreadPool>()->Enqueue([this, scenePath]() -> SceneLoadResult {
            auto newScene = std::make_shared<Scene>();
            SceneSerializer serializer(newScene.get());
            if (!serializer.Deserialize(scenePath.string()))
            {
                return SceneLoadResult{nullptr, serializer.GetLastError()};
            }
            return SceneLoadResult{newScene, {}};
        });

        m_IsSceneOpenLoading = true;
        CH_CORE_INFO("Editor: Loading scene '{}' on a worker thread.", scenePath.string());
    } catch (const std::exception& e)
    {
        CH_CORE_ERROR("Editor: Failed to start scene load: {}", e.what());
        Dialogs::ShowError("Scene loading failed",
                           "Could not start loading '" + scenePath.string() + "':\n\n" + e.what());
        CancelSceneOpenTransition();
    }
}

void EditorSceneManager::UpdateSceneOpenTransition()
{
    if (!m_IsSceneOpenLoading)
    {
        return;
    }

    if (!m_SceneOpenSceneReady)
    {
        if (m_SceneOpenFuture.valid() &&
            m_SceneOpenFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
        {
            try
            {
                SceneLoadResult loadResult = m_SceneOpenFuture.get();

                if (!loadResult.Scene)
                {
                    std::string reason = loadResult.Error.empty()
                                             ? "The scene file is corrupt or is not a valid Chained scene."
                                             : loadResult.Error;
                    CH_CORE_ERROR("Editor: Scene load failed for '{}': {}", m_PendingSceneOpenPath.string(), reason);
                    Dialogs::ShowError("Scene loading failed",
                                       "Failed to load scene:\n" + m_PendingSceneOpenPath.string() + "\n\n" + reason);
                    CancelSceneOpenTransition();
                    return;
                }

                if (m_IsPlayModeSceneLoad)
                {
                    if (m_RuntimeScene)
                    {
                        CH_CORE_INFO("Editor: Stopping current runtime scene to load '{}'.",
                                     m_PendingSceneOpenPath.string());
                        m_RuntimeScene->OnRuntimeStop(m_Context);
                    }

                    m_RuntimeScene = loadResult.Scene;
                }
                else
                {
                    m_EditorScene = loadResult.Scene;
                }
                m_SceneOpenSceneReady = true;
            } catch (const std::exception& e)
            {
                CH_CORE_ERROR("Editor: Scene load failed with exception: {}", e.what());
                Dialogs::ShowError("Scene loading failed",
                                   "Failed to load scene:\n" + m_PendingSceneOpenPath.string() + "\n\n" + e.what());
                CancelSceneOpenTransition();
                return;
            } catch (...)
            {
                CH_CORE_ERROR("Editor: Scene load failed with unknown exception.");
                Dialogs::ShowError("Scene loading failed", "Failed to load scene:\n" +
                                                               m_PendingSceneOpenPath.string() +
                                                               "\n\nAn unknown error occurred.");
                CancelSceneOpenTransition();
                return;
            }
        }
    }

    if (m_SceneOpenSceneReady && (m_EditorScene || m_RuntimeScene) &&
        !ServiceLocator::Get<AssetManager>()->HasBackgroundWork())
    {
        auto targetScene = m_IsPlayModeSceneLoad ? m_RuntimeScene : m_EditorScene;

        if (auto project = Project::GetActive(); project && project->GetEnvironment())
        {
            bool hasEnvironment = targetScene->GetSettings().Environment &&
                                  (!targetScene->GetSettings().Environment->GetPath().empty() ||
                                   (!targetScene->GetSettings().Environment->GetSettings().Skybox.TexturePath.empty()));

            if (!hasEnvironment)
            {
                CH_CORE_INFO("Editor: Applying project environment to scene '{}'.",
                             targetScene->GetSettings().ScenePath);
                targetScene->GetSettings().Environment = project->GetEnvironment();
            }
        }

        targetScene->GetSettings().ScenePath = m_PendingSceneOpenPath.string();

        if (m_IsPlayModeSceneLoad)
        {
            // Bind event callback so SceneTransitionComponents can dispatch SceneChangeRequestEvent
            if (m_SceneEventCallback)
            {
                m_RuntimeScene->SetEventCallback(m_SceneEventCallback);
            }

            // Clear stale button press flags from the previous scene before starting runtime.
            if (auto* uiRenderer = ServiceLocator::TryGet<WidgetRenderer>())
            {
                uiRenderer->ResetButtonStates(m_RuntimeScene.get());
            }

            // TransitionToState already calls OnRuntimeStart() via OnStateEnter
            m_RuntimeScene->TransitionToState(SceneState::Play, m_Context);

            CH_CORE_INFO("Editor: Activating new runtime scene '{}'.", m_PendingSceneOpenPath.string());
        }
        else
        {
            m_EditorScene->TransitionToState(SceneState::Edit, m_Context);

            // Dispatch the scene-opened event
            SceneOpenedEvent e(m_PendingSceneOpenPath.string());
            OnSceneOpened(e);
        }

        m_IsSceneOpenLoading = false;
        m_SceneOpenSceneReady = false;
        m_SceneOpenFuture = {};

        m_EditorState.SelectedEntity = {};

        m_PendingSceneOpenPath.clear();
        m_LoadingStatus = "";
        m_IsPlayModeSceneLoad = false;
        CH_CORE_INFO("Editor: Scene transition complete.");
    }
    else if (m_IsSceneOpenLoading && m_SceneOpenSceneReady)
    {
        if (ServiceLocator::Get<AssetManager>()->HasBackgroundWork())
        {
            static float logTimer = 0.0f;
            logTimer += 0.016f;
            if (logTimer > 1.0f)
            {
                CH_CORE_INFO("Editor: Transition to '{}' waiting for {} assets...", m_PendingSceneOpenPath.string(),
                             ServiceLocator::Get<AssetManager>()->GetPendingFinalizeCount());
                logTimer = 0.0f;
            }
        }
    }
}

void EditorSceneManager::CancelSceneOpenTransition()
{
    m_IsSceneOpenLoading = false;
    m_SceneOpenSceneReady = false;
    m_SceneOpenFuture = {};
    m_PendingSceneOpenPath.clear();
    m_LoadingStatus = "";
}

void EditorSceneManager::StartPlayModeTransition()
{
    if (!m_PlayModeStartRequested || m_IsPlayModeLoading || !m_EditorScene)
    {
        m_PlayModeStartRequested = false;
        return;
    }

    m_PlayModeStartRequested = false;
    m_PlayModeSceneReady = false;
    m_RuntimeScene.reset();
    m_LoadingStatus = "Preparing Play Mode...";

    CH_CORE_INFO("Editor: Copying scene for play mode on main thread.");
    try
    {
        m_RuntimeScene = Scene::Copy(m_EditorScene);
        if (m_RuntimeScene)
        {
            CH_CORE_INFO("Editor: Scene copy successful. Resulting pointer: {}", (void*)m_RuntimeScene.get());
            m_PlayModeSceneReady = true;
            m_IsPlayModeLoading = true;
        }
        else
        {
            CH_CORE_ERROR("Editor: Failed to copy scene for play mode - result was null.");
            CancelPlayModeTransition();
        }
    } catch (const std::exception& e)
    {
        CH_CORE_ERROR("Editor: Exception copying scene: {}", e.what());
        CancelPlayModeTransition();
    } catch (...)
    {
        CH_CORE_ERROR("Editor: Unknown exception copying scene.");
        CancelPlayModeTransition();
    }
}

void EditorSceneManager::UpdatePlayModeTransition()
{
    if (!m_IsPlayModeLoading)
    {
        return;
    }

    if (m_PlayModeSceneReady && m_RuntimeScene)
    {
        m_RuntimeScene->OnViewportResize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);

        // Bind event callback so SceneTransitionComponents can dispatch SceneChangeRequestEvent
        if (m_SceneEventCallback)
        {
            m_RuntimeScene->SetEventCallback(m_SceneEventCallback);
        }

        // Clear stale button press flags from the previous scene before starting runtime.
        if (auto* uiRenderer = ServiceLocator::TryGet<WidgetRenderer>())
        {
            uiRenderer->ResetButtonStates(m_RuntimeScene.get());
        }

        // Configure state directly on the cloned runtime scene.
        // TransitionToState already calls OnRuntimeStart() via OnStateEnter.
        m_RuntimeScene->TransitionToState(m_TargetState, m_Context);

        m_IsPlayModeLoading = false;
        m_PlayModeSceneReady = false;
        m_LoadingStatus = "";
        CH_CORE_INFO("Editor: Play Mode Started Successfully");
    }
}

void EditorSceneManager::CancelPlayModeTransition()
{
    m_PlayModeStartRequested = false;
    m_IsPlayModeLoading = false;
    m_PlayModeSceneReady = false;
    m_RuntimeScene.reset();
    m_LoadingStatus = "";
}

bool EditorSceneManager::OnSceneOpened(SceneOpenedEvent& e)
{
    auto project = Project::GetActive();
    if (project && !e.GetPath().empty())
    {
        project->GetConfig().ActiveScenePath =
            std::filesystem::relative(e.GetPath(), project->GetProjectDirectoryForProject());
        m_ProjectManager.SaveProject();

        m_Config.LastScenePath = e.GetPath();
        EditorLayer::Get().SaveConfig();
        return true;
    }
    return false;
}

bool EditorSceneManager::OnKeyPressed(KeyPressedEvent& e)
{
    if (e.IsRepeat())
    {
        return false;
    }

    bool ctrl = Core::Input::IsKeyDown(KeyCode::LeftControl) || Core::Input::IsKeyDown(KeyCode::RightControl);
    bool shift = Core::Input::IsKeyDown(KeyCode::LeftShift) || Core::Input::IsKeyDown(KeyCode::RightShift);

    auto keyCode = e.GetKeyCode();

    if (ctrl)
    {
        switch (keyCode)
        {
        case KeyCode::N:
            if (GetSceneState() != SceneState::Play)
            {
                NewScene();
            }
            return true;
        case KeyCode::O:
            if (GetSceneState() != SceneState::Play)
            {
                OpenScene();
            }
            return true;
        case KeyCode::S:
            if (GetSceneState() != SceneState::Play)
            {
                if (shift)
                {
                    SaveSceneAs();
                }
                else
                {
                    SaveScene();
                }
            }
            return true;

        case KeyCode::Z:
            if (GetSceneState() != SceneState::Play)
            {
                m_CommandHistory.Undo();
            }
            return true;
        case KeyCode::Y:
            if (GetSceneState() != SceneState::Play)
            {
                m_CommandHistory.Redo();
            }
            return true;
        }
    }

    if (keyCode == KeyCode::F5)
    {
        m_ProjectManager.LaunchStandalone(EditorSceneManager::GetActiveScene());
        return true;
    }

    return false;
}

} // namespace Chained