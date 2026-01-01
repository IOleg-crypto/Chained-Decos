#include "SceneManager.h"
#include "core/Log.h"

namespace CHEngine
{
static std::unique_ptr<SceneManager> s_Instance = nullptr;

void SceneManager::Init()
{
    s_Instance = std::unique_ptr<SceneManager>(new SceneManager());
}

bool SceneManager::IsInitialized()
{
    return s_Instance != nullptr;
}

void SceneManager::Shutdown()
{
    if (s_Instance)
    {
        s_Instance->InternalUnloadCurrentScene();
        s_Instance->InternalUnloadUIScene();
        s_Instance.reset();
    }
}

void SceneManager::LoadScene(std::shared_ptr<Scene> scene)
{
    s_Instance->InternalLoadScene(scene);
}
void SceneManager::LoadUIScene(std::shared_ptr<Scene> scene)
{
    s_Instance->InternalLoadUIScene(scene);
}
void SceneManager::UnloadCurrentScene()
{
    s_Instance->InternalUnloadCurrentScene();
}
void SceneManager::UnloadUIScene()
{
    s_Instance->InternalUnloadUIScene();
}
std::shared_ptr<Scene> SceneManager::GetActiveScene()
{
    return s_Instance->InternalGetActiveScene();
}
std::shared_ptr<Scene> SceneManager::GetUIScene()
{
    return s_Instance->InternalGetUIScene();
}
void SceneManager::Update(float deltaTime)
{
    s_Instance->InternalUpdate(deltaTime);
}
void SceneManager::Render()
{
    s_Instance->InternalRender();
}
void SceneManager::SetOnSceneLoaded(std::function<void(std::shared_ptr<Scene>)> callback)
{
    s_Instance->InternalSetOnSceneLoaded(callback);
}
void SceneManager::SetOnSceneUnloaded(std::function<void(std::shared_ptr<Scene>)> callback)
{
    s_Instance->InternalSetOnSceneUnloaded(callback);
}

SceneManager::SceneManager() : m_ActiveScene(nullptr), m_UIScene(nullptr)
{
    CD_CORE_INFO("[SceneManager] Initialized");
}

void SceneManager::InternalLoadScene(std::shared_ptr<Scene> scene)
{
    if (!scene)
    {
        CD_CORE_ERROR("[SceneManager] Attempted to load null scene");
        return;
    }

    if (m_ActiveScene)
    {
        InternalUnloadCurrentScene();
    }

    m_ActiveScene = scene;
    CD_CORE_INFO("[SceneManager] Loaded scene: %s", scene->GetName().c_str());

    if (m_OnSceneLoaded)
        m_OnSceneLoaded(m_ActiveScene);
}

void SceneManager::InternalLoadUIScene(std::shared_ptr<Scene> scene)
{
    if (!scene)
    {
        CD_CORE_ERROR("[SceneManager] Attempted to load null UI scene");
        return;
    }

    m_UIScene = scene;
    CD_CORE_INFO("[SceneManager] Loaded UI overlay scene: %s", scene->GetName().c_str());
}

void SceneManager::InternalUnloadUIScene()
{
    if (!m_UIScene)
        return;

    CD_CORE_INFO("[SceneManager] Unloading UI scene: %s", m_UIScene->GetName().c_str());
    m_UIScene.reset();
}

void SceneManager::InternalUnloadCurrentScene()
{
    if (!m_ActiveScene)
        return;

    CD_CORE_INFO("[SceneManager] Unloading scene: %s", m_ActiveScene->GetName().c_str());

    if (m_OnSceneUnloaded)
        m_OnSceneUnloaded(m_ActiveScene);

    m_ActiveScene.reset();
}

void SceneManager::InternalUpdate(float deltaTime)
{
    if (m_ActiveScene)
        m_ActiveScene->OnUpdateRuntime(deltaTime);

    if (m_UIScene)
        m_UIScene->OnUpdateRuntime(deltaTime);
}

void SceneManager::InternalRender()
{
    if (m_ActiveScene)
        m_ActiveScene->OnRenderRuntime();

    if (m_UIScene)
        m_UIScene->OnRenderRuntime();
}
} // namespace CHEngine
