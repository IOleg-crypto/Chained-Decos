#include "scene_manager.h"
#include "core/log.h"
#include <algorithm>

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
        s_Instance->ClearSceneStack();
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

    if (m_ActiveScene && m_OnSceneUnloaded)
    {
        m_OnSceneUnloaded(m_ActiveScene);
    }

    m_ActiveScene = scene;
    CD_CORE_INFO("[SceneManager] Loaded scene: %s", scene->GetName().c_str());

    if (m_OnSceneLoaded)
        m_OnSceneLoaded(m_ActiveScene);

    // Start transition
    m_IsTransitioning = true;
    m_TransitionProgress = 0.0f;
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
    // Handle transitions
    if (m_IsTransitioning)
    {
        m_TransitionProgress += deltaTime / m_TransitionDuration;
        if (m_TransitionProgress >= 1.0f)
        {
            m_TransitionProgress = 1.0f;
            m_IsTransitioning = false;
        }
    }

    if (m_ActiveScene)
        m_ActiveScene->OnUpdateRuntime(deltaTime);

    if (m_UIScene)
        m_UIScene->OnUpdateRuntime(deltaTime);

    // Update scene stack (overlays)
    for (auto &scene : m_SceneStack)
    {
        scene->OnUpdateRuntime(deltaTime);
    }
}

void SceneManager::InternalRender()
{
    if (m_ActiveScene)
        m_ActiveScene->OnRenderRuntime();

    // Render scene stack (overlays)
    for (auto &scene : m_SceneStack)
    {
        scene->OnRenderRuntime();
    }

    if (m_UIScene)
        m_UIScene->OnRenderRuntime();
}

void SceneManager::PushScene(std::shared_ptr<Scene> scene)
{
    s_Instance->m_SceneStack.push_back(scene);
}

void SceneManager::PopScene()
{
    if (!s_Instance->m_SceneStack.empty())
        s_Instance->m_SceneStack.pop_back();
}

void SceneManager::ClearSceneStack()
{
    s_Instance->m_SceneStack.clear();
}
} // namespace CHEngine
