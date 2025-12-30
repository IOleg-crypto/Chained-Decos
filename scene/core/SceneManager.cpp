#include "SceneManager.h"
#include "core/Log.h"

namespace CHEngine
{

ECSSceneManager::ECSSceneManager() : m_ActiveScene(nullptr)
{
    CD_CORE_INFO("[ECSSceneManager] Initialized");
}

void ECSSceneManager::LoadScene(std::shared_ptr<Scene> scene)
{
    if (!scene)
    {
        CD_CORE_ERROR("[ECSSceneManager] Attempted to load null scene");
        return;
    }

    // Unload current scene if exists
    if (m_ActiveScene)
    {
        UnloadCurrentScene();
    }

    // Load new scene
    m_ActiveScene = scene;
    CD_CORE_INFO("[ECSSceneManager] Loaded scene: %s", scene->GetName().c_str());

    // Trigger callback
    if (m_OnSceneLoaded)
        m_OnSceneLoaded(m_ActiveScene);
}

void ECSSceneManager::UnloadCurrentScene()
{
    if (!m_ActiveScene)
        return;

    CD_CORE_INFO("[ECSSceneManager] Unloading scene: %s", m_ActiveScene->GetName().c_str());

    // Trigger callback
    if (m_OnSceneUnloaded)
        m_OnSceneUnloaded(m_ActiveScene);

    m_ActiveScene.reset();
}

void ECSSceneManager::Update(float deltaTime)
{
    if (m_ActiveScene)
    {
        m_ActiveScene->OnUpdateRuntime(deltaTime);
    }
}

void ECSSceneManager::Render()
{
    if (m_ActiveScene)
    {
        m_ActiveScene->OnRenderRuntime();
    }
}

} // namespace CHEngine
