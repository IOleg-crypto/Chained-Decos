#include "editor_layer.h"
#include "engine/core/application.h"
#include "engine/core/log.h"

namespace CHEngine
{

bool EditorLayer::OnScenePlay(ScenePlayEvent &e)
{
    CH_CORE_INFO("Scene Play Event");

    m_EditorScene = Application::Get().GetActiveScene();
    if (!m_EditorScene)
        return false;

    Ref<Scene> runtimeScene = Scene::Copy(m_EditorScene);
    if (!runtimeScene)
    {
        CH_CORE_ERROR("Failed to clone scene for play mode!");
        return false;
    }

    m_SceneState = SceneState::Play;
    Application::Get().SetActiveScene(runtimeScene);

    // Update panels context
    for (auto &panel : m_Panels)
        panel->SetContext(runtimeScene);

    runtimeScene->OnRuntimeStart();

    return false;
}

bool EditorLayer::OnSceneStop(SceneStopEvent &e)
{
    CH_CORE_INFO("Scene Stop Event");

    auto runtimeScene = Application::Get().GetActiveScene();
    if (runtimeScene)
        runtimeScene->OnRuntimeStop();

    m_SceneState = SceneState::Edit;

    if (m_EditorScene)
    {
        Application::Get().SetActiveScene(m_EditorScene);
        for (auto &panel : m_Panels)
            panel->SetContext(m_EditorScene);

        m_EditorScene = nullptr;
    }

    return false;
}

} // namespace CHEngine
