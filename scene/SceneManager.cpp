#include "SceneManager.h"
#include "core/Log.h"
#include <raylib.h>

namespace CHEngine
{

void SceneManager::LoadScene(const std::string &scenePath)
{
    CD_CORE_INFO("[SceneManager] Loading scene: %s", scenePath.c_str());

    BeginTransition();
    LoadSceneInternal(scenePath);
}

void SceneManager::LoadSceneAsync(const std::string &scenePath)
{
    CD_CORE_INFO("[SceneManager] Async loading scene: %s", scenePath.c_str());

    m_isLoadingAsync = true;
    m_nextScenePath = scenePath;
    BeginTransition();
}

void SceneManager::PushScene(const std::string &scenePath)
{
    CD_CORE_INFO("[SceneManager] Pushing scene: %s", scenePath.c_str());

    // Save current scene to stack
    m_sceneStack.push_back({std::move(m_currentScene), m_currentScenePath});

    // Load new scene
    LoadSceneInternal(scenePath);
}

void SceneManager::PopScene()
{
    if (m_sceneStack.empty())
    {
        CD_CORE_WARN("[SceneManager] Cannot pop scene: stack is empty");
        return;
    }

    CD_CORE_INFO("[SceneManager] Popping scene");

    // Restore previous scene from stack
    auto [scene, path] = std::move(m_sceneStack.back());
    m_sceneStack.pop_back();

    m_currentScene = std::move(scene);
    m_currentScenePath = path;

    BeginTransition();
}

void SceneManager::ClearSceneStack()
{
    m_sceneStack.clear();
    CD_CORE_INFO("[SceneManager] Scene stack cleared");
}

GameScene *SceneManager::GetCurrentScene()
{
    return &m_currentScene;
}

const std::string &SceneManager::GetCurrentScenePath() const
{
    return m_currentScenePath;
}

void SceneManager::SetTransitionDuration(float seconds)
{
    m_transitionDuration = seconds;
}

bool SceneManager::IsTransitioning() const
{
    return m_isTransitioning;
}

float SceneManager::GetTransitionProgress() const
{
    return m_transitionProgress;
}

void SceneManager::Update(float deltaTime)
{
    if (!m_isTransitioning)
        return;

    // Update transition progress
    m_transitionProgress += deltaTime / m_transitionDuration;

    if (m_transitionProgress >= 1.0f)
    {
        // Transition complete
        if (m_isLoadingAsync && m_transitionProgress >= 0.5f)
        {
            // Load scene at midpoint of transition
            LoadSceneInternal(m_nextScenePath);
            m_isLoadingAsync = false;
        }

        EndTransition();
    }
}

void SceneManager::BeginTransition()
{
    m_isTransitioning = true;
    m_transitionProgress = 0.0f;
}

void SceneManager::EndTransition()
{
    m_isTransitioning = false;
    m_transitionProgress = 0.0f;
}

void SceneManager::LoadSceneInternal(const std::string &scenePath)
{
    SceneLoader loader;
    m_currentScene = loader.LoadScene(scenePath);
    m_currentScenePath = scenePath;

    CD_CORE_INFO("[SceneManager] Scene loaded: %s", scenePath.c_str());
}

} // namespace CHEngine
