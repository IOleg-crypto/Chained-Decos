#include "editor_scene_manager.h"
#include "editor_layer.h"
#include "scripting/scriptengine.h"
#include "engine/scene/scene.h"
#include "engine/scene/project.h"
#include "engine/scene/scene_serializer.h"
#include "engine/scene/scene_scripting_manager.h"
#include "engine/core/thread_pool.h"
#include "engine/core/application.h"
#include "engine/core/input.h"
#include "engine/core/key_codes.h"
#include "engine/assets/asset_manager.h"
#include "engine/core/service_locator.h"
#include "engine/core/platform.h"
#include "engine/scene/scene_events.h"

namespace CHEngine
{

EditorSceneManager::EditorSceneManager(ScriptEngine* scriptEngine)
    : m_ScriptEngine(scriptEngine)
{
}

void EditorSceneManager::NewScene()
{
    SetScene(Scene::CreateDefault()); // CreateDefault uses placeholder nullptr, we should pass engine if we want
    // Actually, let's update CreateDefault to take it if we can.
    SetScene(std::make_shared<Scene>(m_ScriptEngine));
}

void EditorSceneManager::OpenScene()
{
    std::vector<FileDialogFilter> filters = {{"Chained Scene", "chscene"}};
    auto result = CHEngine::Platform::OpenFile(filters);
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
        CH_CORE_WARN("EditorSceneManager: Transition to '{}' ignored - already loading '{}'", path.string(), m_PendingSceneOpenPath.string());
        return;
    }
    m_IsPlayModeSceneLoad = (EditorContext::GetSceneState() == SceneState::Play);
    StartSceneOpenTransition(path);
}

void EditorSceneManager::SaveScene()
{
    auto scene = GetActiveScene();
    if (!scene) return;

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
    std::vector<FileDialogFilter> filters = {{"Chained Scene", "chscene"}};
    auto result = CHEngine::Platform::SaveFile(filters);
    if (result)
    {
        auto scene = GetActiveScene();
        if (!scene) return;

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
        return;

    m_AutoSaveTimer = 0.0f;
    SceneSerializer serializer(scene.get());
    serializer.Serialize(scene->GetSettings().ScenePath);
    CH_TRACE("Scene auto-saved to {0}", scene->GetSettings().ScenePath);
}

void EditorSceneManager::SetScene(std::shared_ptr<Scene> scene)
{
    CancelPlayModeTransition();
    CancelSceneOpenTransition();
    m_EditorScene = scene;
    EditorContext::SetSelectedEntity({});
}

void EditorSceneManager::SetSceneState(SceneState state)
{
    if (state == SceneState::Play)
    {
        if (m_PlayModeStartRequested || m_IsPlayModeLoading || m_IsSceneOpenLoading ||
            EditorContext::GetSceneState() == SceneState::Play)
        {
            return;
        }

        if (!m_EditorScene)
        {
            CH_CORE_WARN("EditorSceneManager::SetSceneState - No editor scene available for play mode.");
            return;
        }

        m_PlayModeStartRequested = true;
        CH_CORE_INFO("Editor: Play mode requested.");
    }
    else
    {
        if (m_PlayModeStartRequested || m_IsPlayModeLoading)
        {
            CancelPlayModeTransition();
        }

        if (m_IsSceneOpenLoading)
        {
            CancelSceneOpenTransition();
        }

        if (EditorContext::GetSceneState() == SceneState::Edit)
        {
            return;
        }

        CH_CORE_INFO("Editor: Play Mode Stopped");
        if (m_RuntimeScene)
        {
            CH_CORE_INFO("Editor: Cleaning up runtime scene...");
            m_RuntimeScene->OnRuntimeStop();
            m_RuntimeScene.reset();
        }

        EditorContext::SetSceneState(SceneState::Edit);
    }
}

std::shared_ptr<Scene> EditorSceneManager::GetActiveScene() const
{
    return (EditorContext::GetSceneState() == SceneState::Play) ? m_RuntimeScene : m_EditorScene;
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

    EditorContext::GetState().IsLoading = IsLoading();
    EditorContext::GetState().LoadingStatus = m_LoadingStatus;
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
    if (path.empty()) return;
    if (m_IsSceneOpenLoading) return;

    CancelPlayModeTransition();

    std::filesystem::path scenePath = path;
    if (scenePath.is_relative() && Project::GetActive())
    {
        scenePath = Project::GetAssetPath(scenePath);
    }

    m_PendingSceneOpenPath = scenePath;
    m_SceneOpenSceneReady = false;
    m_LoadingStatus = "Loading scene...";

    try
    {
        m_SceneOpenFuture = ThreadPool::Get().Enqueue([scenePath, engine = m_ScriptEngine]() {
            auto newScene = std::make_shared<Scene>(engine);
            SceneSerializer serializer(newScene.get());
            if (!serializer.Deserialize(scenePath.string()))
            {
                return std::shared_ptr<Scene>{};
            }
            return newScene;
        });

        m_IsSceneOpenLoading = true;
        CH_CORE_INFO("Editor: Loading scene '{}' on a worker thread.", scenePath.string());
    } catch (const std::exception& e)
    {
        CH_CORE_ERROR("Editor: Failed to start scene load: {}", e.what());
        CancelSceneOpenTransition();
    }
}

void EditorSceneManager::UpdateSceneOpenTransition()
{
    if (!m_IsSceneOpenLoading) return;

    if (!m_SceneOpenSceneReady)
    {
        if (m_SceneOpenFuture.valid() &&
            m_SceneOpenFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
        {
            try
            {
                if (m_IsPlayModeSceneLoad)
                {
                    if (m_RuntimeScene)
                    {
                        CH_CORE_INFO("Editor: Stopping current runtime scene to load '{}'.", m_PendingSceneOpenPath.string());
                        m_RuntimeScene->OnRuntimeStop();
                    }

                    m_RuntimeScene = m_SceneOpenFuture.get();
                    if (!m_RuntimeScene)
                    {
                        CH_CORE_ERROR("Editor: Runtime Scene load returned null for '{}'.", m_PendingSceneOpenPath.string());
                        CancelSceneOpenTransition();
                        return;
                    }
                }
                else
                {
                    m_EditorScene = m_SceneOpenFuture.get();
                    if (!m_EditorScene)
                    {
                        CH_CORE_ERROR("Editor: Scene load returned null for '{}'.", m_PendingSceneOpenPath.string());
                        CancelSceneOpenTransition();
                        return;
                    }
                }
                m_SceneOpenSceneReady = true;
            } catch (const std::exception& e)
            {
                CH_CORE_ERROR("Editor: Scene load failed with exception: {}", e.what());
                CancelSceneOpenTransition();
                return;
            } catch (...)
            {
                CH_CORE_ERROR("Editor: Scene load failed with unknown exception.");
                CancelSceneOpenTransition();
                return;
            }
        }
    }

    if (m_SceneOpenSceneReady && (m_EditorScene || m_RuntimeScene) && !ServiceLocator::Get<AssetManager>().HasBackgroundWork())
    {
        auto targetScene = m_IsPlayModeSceneLoad ? m_RuntimeScene : m_EditorScene;

        if (Project::GetActive() && Project::GetActive()->GetEnvironment())
        {
            // Only override if the loaded scene has no environment defined
            bool hasEnvironment = targetScene->GetSettings().Environment && 
                                 (!targetScene->GetSettings().Environment->GetPath().empty() ||
                                  (!targetScene->GetSettings().Environment->GetSettings().Skybox.TexturePath.empty()));
            
            if (!hasEnvironment)
            {
                CH_CORE_INFO("Editor: Applying project environment to scene '{}'.", targetScene->GetSettings().ScenePath);
                targetScene->GetSettings().Environment = Project::GetActive()->GetEnvironment();
            }
        }

        targetScene->GetSettings().ScenePath = m_PendingSceneOpenPath.string();
        
        if (m_IsPlayModeSceneLoad)
        {
            CH_CORE_INFO("Editor: Activating new runtime scene '{}'.", m_PendingSceneOpenPath.string());
            m_RuntimeScene->OnRuntimeStart();
        }
        else
        {
            CH_CORE_INFO("Editor: Activating new editor scene '{}'.", m_PendingSceneOpenPath.string());
            SceneOpenedEvent e(m_PendingSceneOpenPath.string());
            EditorLayer::Get().SetLastScenePath(m_PendingSceneOpenPath.string());
            Application::Get().OnEvent(e);
        }

        m_IsSceneOpenLoading = false;
        m_SceneOpenSceneReady = false;
        m_SceneOpenFuture = {};

        EditorContext::SetSelectedEntity({});
        
        m_PendingSceneOpenPath.clear();
        m_LoadingStatus = "";
        m_IsPlayModeSceneLoad = false;
        CH_CORE_INFO("Editor: Scene transition complete.");
    }
    else if (m_IsSceneOpenLoading && m_SceneOpenSceneReady)
    {
        auto& am = ServiceLocator::Get<AssetManager>();
        if (am.HasBackgroundWork())
        {
            static float logTimer = 0.0f;
            logTimer += 0.016f; // approximate
            if (logTimer > 1.0f)
            {
                CH_CORE_INFO("Editor: Transition to '{}' waiting for {} assets...", m_PendingSceneOpenPath.string(), am.GetPendingFinalizeCount());
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
    try {
        m_RuntimeScene = Scene::Copy(m_EditorScene);
        if (m_RuntimeScene)
        {
            CH_CORE_INFO("Editor: Scene copy successful. Resulting pointer: {}", (void*)m_RuntimeScene.get());
            m_PlayModeSceneReady = true;
            m_IsPlayModeLoading = true; // Still mark as loading to tick UpdatePlayModeTransition once if needed
        }
        else
        {
            CH_CORE_ERROR("Editor: Failed to copy scene for play mode - result was null.");
            CancelPlayModeTransition();
        }
    } catch (const std::exception& e) {
        CH_CORE_ERROR("Editor: Exception copying scene: {}", e.what());
        CancelPlayModeTransition();
    } catch (...) {
        CH_CORE_ERROR("Editor: Unknown exception copying scene.");
        CancelPlayModeTransition();
    }
}

void EditorSceneManager::UpdatePlayModeTransition()
{
    if (!m_IsPlayModeLoading) return;

    if (m_PlayModeSceneReady && m_RuntimeScene && !ServiceLocator::Get<AssetManager>().HasBackgroundWork())
    {
        EditorContext::SetSceneState(SceneState::Play);
        m_RuntimeScene->OnRuntimeStart();

        m_IsPlayModeLoading = false;
        m_PlayModeSceneReady = false;
        m_LoadingStatus = "";
        CH_CORE_INFO("Editor: Play Mode Started");
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
    // Sync project path
    auto project = Project::GetActive();
    if (project && !e.GetPath().empty())
    {
        project->GetConfig().ActiveScenePath = std::filesystem::relative(e.GetPath(), project->GetProjectDirectory());
        EditorLayer::Get().GetProjectManager().SaveProject();

        EditorLayer::Get().GetConfig().LastScenePath = e.GetPath();
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

    bool ctrl = Input::IsKeyDown(Key::LeftControl) || Input::IsKeyDown(Key::RightControl);
    bool shift = Input::IsKeyDown(Key::LeftShift) || Input::IsKeyDown(Key::RightShift);

    auto keyCode = e.GetKeyCode();

    if (ctrl)
    {
        switch (keyCode)
        {
        case Key::N:
            NewScene();
            return true;
        case Key::O:
            OpenScene();
            return true;
        case Key::S:
            if (shift)
            {
                SaveSceneAs();
            }
            else
            {
                SaveScene();
            }
            return true;
        case Key::Z:
            EditorLayer::Get().GetCommandHistory().Undo();
            return true;
        case Key::Y:
            EditorLayer::Get().GetCommandHistory().Redo();
            return true;
        }
    }

    if (keyCode == Key::F5)
    {
        EditorLayer::Get().LaunchStandalone();
        return true;
    }

    return false;
}
} // namespace CHEngine
