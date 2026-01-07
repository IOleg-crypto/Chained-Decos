#ifndef CD_SCENE_CORE_SCENE_MANAGER_H
#define CD_SCENE_CORE_SCENE_MANAGER_H

#include "scene.h"
#include <functional>
#include <memory>
#include <vector>

namespace CHEngine
{

// Manages scene lifecycle and transitions
class SceneManager
{
public:
    static void Init();
    static void Shutdown();
    static SceneManager &Get();
    static void Update(float deltaTime);
    static void Render();
    static std::shared_ptr<Scene> GetActiveScene();
    static std::shared_ptr<Scene> GetUIScene();
    static bool IsInitialized();
    static void LoadScene(std::shared_ptr<Scene> scene);
    static void UnloadCurrentScene();
    static void LoadUIScene(std::shared_ptr<Scene> scene);
    static void UnloadUIScene();

    // Scene Operations
    void NewScene();
    bool OpenScene(const std::string &path);
    void SaveScene();
    void SaveSceneAs(const std::string &path);

    // Getters & Setters
    const std::string &GetActiveScenePath() const
    {
        return m_ActiveScenePath;
    }

public:
    SceneManager();
    ~SceneManager() = default;

    void InternalLoadScene(std::shared_ptr<Scene> scene);
    void InternalLoadUIScene(std::shared_ptr<Scene> scene);
    void InternalUnloadUIScene();
    void InternalUnloadCurrentScene();

    void PerformSceneChanges();

    std::shared_ptr<Scene> InternalGetActiveScene() const
    {
        return m_ActiveScene;
    }
    std::shared_ptr<Scene> InternalGetUIScene() const
    {
        return m_UIScene;
    }

    void InternalUpdate(float deltaTime);
    void InternalRender();

    // Scene stack support
    void PushScene(std::shared_ptr<Scene> scene);
    void PopScene();
    void ClearSceneStack();
    const std::vector<std::shared_ptr<Scene>> &GetSceneStack() const
    {
        return m_SceneStack;
    }

    // Transition support
    bool IsTransitioning() const
    {
        return m_IsTransitioning;
    }
    float GetTransitionProgress() const
    {
        return m_TransitionProgress;
    }
    void SetTransitionDuration(float duration)
    {
        m_TransitionDuration = duration;
    }

    void InternalSetOnSceneLoaded(std::function<void(std::shared_ptr<Scene>)> callback)
    {
        m_OnSceneLoaded = callback;
    }
    void InternalSetOnSceneUnloaded(std::function<void(std::shared_ptr<Scene>)> callback)
    {
        m_OnSceneUnloaded = callback;
    }

private:
    std::shared_ptr<Scene> m_ActiveScene;
    std::shared_ptr<Scene> m_UIScene;
    std::string m_ActiveScenePath;

    std::shared_ptr<Scene> m_NextActiveScene;
    std::shared_ptr<Scene> m_NextUIScene;
    std::vector<std::shared_ptr<Scene>> m_SceneStack;

    std::function<void(std::shared_ptr<Scene>)> m_OnSceneLoaded;
    std::function<void(std::shared_ptr<Scene>)> m_OnSceneUnloaded;

    bool m_IsTransitioning = false;
    float m_TransitionProgress = 0.0f;
    float m_TransitionDuration = 0.5f;

    static SceneManager *s_Instance;
};

} // namespace CHEngine

#endif // CD_SCENE_CORE_SCENE_MANAGER_H
