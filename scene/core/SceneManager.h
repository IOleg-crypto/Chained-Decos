#ifndef SCENE_MANAGER_H
#define SCENE_MANAGER_H

#include "Scene.h"
#include <functional>
#include <memory>

namespace CHEngine
{

// Manages scene lifecycle and transitions
class SceneManager
{
public:
    static void Init();
    static void Shutdown();
    static bool IsInitialized();

    // Scene loading
    static void LoadScene(std::shared_ptr<Scene> scene);
    static void LoadUIScene(std::shared_ptr<Scene> scene);
    static void UnloadCurrentScene();
    static void UnloadUIScene();

    static std::shared_ptr<Scene> GetActiveScene();
    static std::shared_ptr<Scene> GetUIScene();

    // Lifecycle
    static void Update(float deltaTime);
    static void Render();

    // Callback setup
    static void SetOnSceneLoaded(std::function<void(std::shared_ptr<Scene>)> callback);
    static void SetOnSceneUnloaded(std::function<void(std::shared_ptr<Scene>)> callback);

public:
    SceneManager();
    ~SceneManager() = default;

    void InternalLoadScene(std::shared_ptr<Scene> scene);
    void InternalLoadUIScene(std::shared_ptr<Scene> scene);
    void InternalUnloadUIScene();
    void InternalUnloadCurrentScene();

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

    std::function<void(std::shared_ptr<Scene>)> m_OnSceneLoaded;
    std::function<void(std::shared_ptr<Scene>)> m_OnSceneUnloaded;
};

} // namespace CHEngine

#endif // SCENE_MANAGER_H
