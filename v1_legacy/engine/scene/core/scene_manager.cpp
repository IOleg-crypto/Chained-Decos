#include "scene_manager.h"
#include "core/log.h"
#include "scene_serializer.h"
#include <algorithm>
#include <memory>

namespace CHEngine
{
SceneManager *SceneManager::s_Instance = nullptr;

void SceneManager::Init()
{
    if (!s_Instance)
        s_Instance = new SceneManager();
}

void SceneManager::Shutdown()
{
    if (s_Instance)
    {
        s_Instance->InternalUnloadCurrentScene();
        s_Instance->InternalUnloadUIScene();
        s_Instance->ClearSceneStack();
        delete s_Instance;
        s_Instance = nullptr;
    }
}

SceneManager &SceneManager::Get()
{
    return *s_Instance;
}

std::shared_ptr<Scene> SceneManager::GetActiveScene()
{
    return s_Instance ? s_Instance->InternalGetActiveScene() : nullptr;
}

std::shared_ptr<Scene> SceneManager::GetUIScene()
{
    return s_Instance ? s_Instance->InternalGetUIScene() : nullptr;
}

bool SceneManager::IsInitialized()
{
    return s_Instance != nullptr;
}

void SceneManager::LoadScene(std::shared_ptr<Scene> scene)
{
    if (s_Instance)
        s_Instance->InternalLoadScene(scene);
}

void SceneManager::UnloadCurrentScene()
{
    if (s_Instance)
        s_Instance->InternalUnloadCurrentScene();
}

void SceneManager::LoadUIScene(std::shared_ptr<Scene> scene)
{
    if (s_Instance)
        s_Instance->InternalLoadUIScene(scene);
}

void SceneManager::UnloadUIScene()
{
    if (s_Instance)
        s_Instance->InternalUnloadUIScene();
}

void SceneManager::NewScene()
{
    InternalUnloadCurrentScene();
    m_ActiveScene = std::make_shared<Scene>("Untitled");
    m_ActiveScenePath = "";
    CD_CORE_INFO("[SceneManager] Created new empty scene.");
}

bool SceneManager::OpenScene(const std::string &path)
{
    InternalUnloadCurrentScene();

    auto newScene = std::make_shared<Scene>("NewScene");
    ECSSceneSerializer serializer(newScene);
    if (serializer.Deserialize(path))
    {
        m_ActiveScene = newScene;
        m_ActiveScenePath = path;
        CD_CORE_INFO("[SceneManager] Opened scene: %s", path.c_str());
        return true;
    }

    CD_CORE_ERROR("[SceneManager] Failed to open scene: %s", path.c_str());
    return false;
}

void SceneManager::SaveScene()
{
    if (m_ActiveScenePath.empty())
    {
        CD_CORE_WARN("[SceneManager] Cannot save unnamed scene. Use SaveSceneAs.");
        return;
    }

    ECSSceneSerializer serializer(m_ActiveScene);
    serializer.Serialize(m_ActiveScenePath);
    CD_CORE_INFO("[SceneManager] Saved scene to: %s", m_ActiveScenePath.c_str());
}

void SceneManager::SaveSceneAs(const std::string &path)
{
    m_ActiveScenePath = path;
    SaveScene();
}

void SceneManager::InternalUnloadUIScene()

    void SceneManager::Update(float deltaTime)
{
    if (s_Instance)
        s_Instance->InternalUpdate(deltaTime);
}

void SceneManager::Render()
{
    if (s_Instance)
        s_Instance->InternalRender();
}

void SceneManager::InternalUpdate(float deltaTime)
{
    if (m_ActiveScene)
        m_ActiveScene->OnUpdateRuntime(deltaTime);

    if (m_UIScene)
        m_UIScene->OnUpdateRuntime(deltaTime);

    for (auto &scene : m_SceneStack)
        scene->OnUpdateRuntime(deltaTime);
}

void SceneManager::InternalRender()
{
    if (m_ActiveScene)
        m_ActiveScene->OnRenderRuntime();

    for (auto &scene : m_SceneStack)
        scene->OnRenderRuntime();

    if (m_UIScene)
        m_UIScene->OnRenderRuntime();
}

SceneManager::SceneManager() : m_ActiveScene(nullptr), m_UIScene(nullptr)
{
    CD_CORE_INFO("[SceneManager] Instance created");
}

void SceneManager::InternalUnloadUIScene()
{
    if (!m_UIScene)
        return;
    m_UIScene.reset();
}

void SceneManager::InternalUnloadCurrentScene()
{
    if (!m_ActiveScene)
        return;
    if (m_OnSceneUnloaded)
        m_OnSceneUnloaded(m_ActiveScene);
    m_ActiveScene.reset();
    m_ActiveScenePath = "";
}

void SceneManager::PushScene(std::shared_ptr<Scene> scene)
{
    m_SceneStack.push_back(scene);
}

void SceneManager::PopScene()
{
    if (!m_SceneStack.empty())
        m_SceneStack.pop_back();
}

void SceneManager::ClearSceneStack()
{
    m_SceneStack.clear();
}
} // namespace CHEngine
