#ifndef SCENE_MANAGER_H
#define SCENE_MANAGER_H

#include "Scene.h"
#include <functional>
#include <memory>

namespace CHEngine
{

// Manages scene lifecycle and transitions
class ECSSceneManager
{
public:
    ECSSceneManager();
    ~ECSSceneManager() = default;

    // Scene loading
    void LoadScene(std::shared_ptr<Scene> scene);
    void UnloadCurrentScene();

    // Scene access
    std::shared_ptr<Scene> GetActiveScene() const
    {
        return m_ActiveScene;
    }

    // Lifecycle
    void Update(float deltaTime);
    void Render();

    // Callback setup
    void SetOnSceneLoaded(std::function<void(std::shared_ptr<Scene>)> callback)
    {
        m_OnSceneLoaded = callback;
    }
    void SetOnSceneUnloaded(std::function<void(std::shared_ptr<Scene>)> callback)
    {
        m_OnSceneUnloaded = callback;
    }

private:
    std::shared_ptr<Scene> m_ActiveScene;

    std::function<void(std::shared_ptr<Scene>)> m_OnSceneLoaded;
    std::function<void(std::shared_ptr<Scene>)> m_OnSceneUnloaded;
};

} // namespace CHEngine

#endif // SCENE_MANAGER_H
