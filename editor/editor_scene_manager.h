#ifndef CH_EDITOR_SCENE_MANAGER_H
#define CH_EDITOR_SCENE_MANAGER_H

#include "editor_context.h"
#include "engine/scene/scene.h"
#include "engine/scene/scene_events.h"
#include <filesystem>
#include <future>
#include <memory>

namespace CHEngine
{

class EditorSceneManager
{
public:
    EditorSceneManager();
    ~EditorSceneManager() = default;

    void NewScene();
    void OpenScene();
    void OpenScene(const std::filesystem::path& path);
    void SaveScene();
    void SaveSceneAs();
    void AutoSave(float interval, float ts);

    void SetScene(std::shared_ptr<Scene> scene);
    void SetSceneState(SceneState state);
    std::shared_ptr<Scene> GetActiveScene() const;

    void OnUpdate(Timestep ts);
    void OnViewportResize(uint32_t width, uint32_t height);

    bool OnSceneOpened(SceneOpenedEvent& e);
    bool OnKeyPressed(KeyPressedEvent& e);

    bool IsLoading() const
    {
        return m_IsPlayModeLoading || m_IsSceneOpenLoading;
    }
    const std::string& GetLoadingStatus() const
    {
        return m_LoadingStatus;
    }

private:
    void StartSceneOpenTransition(const std::filesystem::path& path);
    void UpdateSceneOpenTransition();
    void CancelSceneOpenTransition();
    void StartPlayModeTransition();
    void UpdatePlayModeTransition();
    void CancelPlayModeTransition();

private:
    std::shared_ptr<Scene> m_EditorScene;
    std::shared_ptr<Scene> m_RuntimeScene;

    // Async state
    std::future<std::shared_ptr<Scene>> m_PlayModeCopyFuture;
    std::future<std::shared_ptr<Scene>> m_SceneOpenFuture;
    std::filesystem::path m_PendingSceneOpenPath;

    bool m_IsPlayModeLoading = false;
    bool m_IsSceneOpenLoading = false;
    bool m_PlayModeSceneReady = false;
    bool m_SceneOpenSceneReady = false;
    bool m_PlayModeStartRequested = false;
    bool m_IsPlayModeSceneLoad = false;

    std::string m_LoadingStatus = "";

    float m_AutoSaveTimer = 0.0f;
    float m_LastAutoSaveTime = 0.0f;
};

} // namespace CHEngine

#endif // CH_EDITOR_SCENE_MANAGER_H
