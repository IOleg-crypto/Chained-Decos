#include "MapManager.h"
#include "core/Log.h"
#include "resources/map/SceneLoader.h"
#include <raylib.h>

namespace CHEngine
{
static std::unique_ptr<MapManager> s_Instance = nullptr;

void MapManager::Init()
{
    s_Instance = std::unique_ptr<MapManager>(new MapManager());
}

bool MapManager::IsInitialized()
{
    return s_Instance != nullptr;
}

void MapManager::Shutdown()
{
    if (s_Instance)
    {
        s_Instance->InternalClearSceneStack();
        s_Instance.reset();
    }
}

void MapManager::LoadScene(const std::string &scenePath)
{
    s_Instance->InternalLoadScene(scenePath);
}
void MapManager::LoadSceneAsync(const std::string &scenePath)
{
    s_Instance->InternalLoadSceneAsync(scenePath);
}
void MapManager::PushScene(const std::string &scenePath)
{
    s_Instance->InternalPushScene(scenePath);
}
void MapManager::PopScene()
{
    s_Instance->InternalPopScene();
}
void MapManager::ClearSceneStack()
{
    s_Instance->InternalClearSceneStack();
}
std::shared_ptr<CHEngine::GameScene> MapManager::GetCurrentScene()
{
    return s_Instance->InternalGetCurrentScene();
}
void MapManager::SetCurrentScene(std::shared_ptr<CHEngine::GameScene> scene)
{
    s_Instance->InternalSetCurrentScene(scene);
}
const std::string &MapManager::GetCurrentScenePath()
{
    return s_Instance->InternalGetCurrentScenePath();
}
void MapManager::SetTransitionDuration(float seconds)
{
    s_Instance->InternalSetTransitionDuration(seconds);
}
bool MapManager::IsTransitioning()
{
    return s_Instance->InternalIsTransitioning();
}
float MapManager::GetTransitionProgress()
{
    return s_Instance->InternalGetTransitionProgress();
}
void MapManager::Update(float deltaTime)
{
    s_Instance->InternalUpdate(deltaTime);
}

MapManager::MapManager()
    : m_currentScene(nullptr), m_isTransitioning(false), m_transitionProgress(0.0f),
      m_isLoadingAsync(false)
{
}

void MapManager::InternalLoadScene(const std::string &scenePath)
{
    CD_CORE_INFO("[MapManager] Loading scene: %s", scenePath.c_str());
    BeginTransition();
    LoadSceneInternal(scenePath);
}

void MapManager::InternalLoadSceneAsync(const std::string &scenePath)
{
    CD_CORE_INFO("[MapManager] Async loading scene: %s", scenePath.c_str());
    m_nextScenePath = scenePath;
    m_isLoadingAsync = true;
    BeginTransition();
}

void MapManager::InternalPushScene(const std::string &scenePath)
{
    CD_CORE_INFO("[MapManager] Pushing scene: %s", scenePath.c_str());
    m_sceneStack.push_back({m_currentScene, m_currentScenePath});
    LoadSceneInternal(scenePath);
}

void MapManager::InternalPopScene()
{
    if (m_sceneStack.empty())
    {
        CD_CORE_WARN("[MapManager] Cannot pop scene: stack is empty");
        return;
    }

    CD_CORE_INFO("[MapManager] Popping scene");
    auto [scene, path] = std::move(m_sceneStack.back());
    m_sceneStack.pop_back();

    m_currentScene = std::move(scene);
    m_currentScenePath = path;
    BeginTransition();
}

void MapManager::InternalClearSceneStack()
{
    m_sceneStack.clear();
    CD_CORE_INFO("[MapManager] Scene stack cleared");
}

void MapManager::InternalUpdate(float deltaTime)
{
    if (!m_isTransitioning)
        return;

    m_transitionProgress += deltaTime / m_transitionDuration;

    if (m_transitionProgress >= 1.0f)
    {
        if (m_isLoadingAsync && m_transitionProgress >= 0.5f)
        {
            LoadSceneInternal(m_nextScenePath);
            m_isLoadingAsync = false;
        }
        EndTransition();
    }
}

std::shared_ptr<CHEngine::GameScene> MapManager::InternalGetCurrentScene()
{
    return m_currentScene;
}

void MapManager::BeginTransition()
{
    m_isTransitioning = true;
    m_transitionProgress = 0.0f;
}

void MapManager::EndTransition()
{
    m_isTransitioning = false;
    m_transitionProgress = 1.0f;
}

void MapManager::LoadSceneInternal(const std::string &scenePath)
{
    m_currentScenePath = scenePath;
    SceneLoader loader;
    auto newScene = std::make_shared<GameScene>();
    *newScene = loader.LoadScene(scenePath);
    m_currentScene = newScene;

    CD_CORE_INFO("[MapManager] Scene loaded: %s", scenePath.c_str());
}
} // namespace CHEngine
