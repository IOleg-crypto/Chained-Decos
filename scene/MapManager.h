#pragma once

#include "resources/map/GameScene.h"
#include "resources/map/SceneLoader.h"
#include <memory>
#include <string>
#include <vector>

namespace CHEngine
{

/**
 * MapManager - Singleton for managing scene loading and transitions
 * Handles scene switching, scene stack for overlays/menus, and transitions
 */
class MapManager
{
public:
    static void Init();
    static void Shutdown();
    static bool IsInitialized();

    // Scene loading
    static void LoadScene(const std::string &scenePath);
    static void LoadSceneAsync(const std::string &scenePath);

    // Scene stack (for menus/overlays that can be pushed/popped)
    static void PushScene(const std::string &scenePath);
    static void PopScene();
    static void ClearSceneStack();

    // Current scene access
    static std::shared_ptr<CHEngine::GameScene> GetCurrentScene();
    static void SetCurrentScene(std::shared_ptr<CHEngine::GameScene> scene);
    static const std::string &GetCurrentScenePath();

    // Scene transitions
    static void SetTransitionDuration(float seconds);
    static bool IsTransitioning();
    static float GetTransitionProgress();

    // Update (call each frame to handle async loading and transitions)
    static void Update(float deltaTime);

public:
    MapManager();
    ~MapManager() = default;

    // Internal implementations
    void InternalLoadScene(const std::string &scenePath);
    void InternalLoadSceneAsync(const std::string &scenePath);
    void InternalPushScene(const std::string &scenePath);
    void InternalPopScene();
    void InternalClearSceneStack();
    std::shared_ptr<CHEngine::GameScene> InternalGetCurrentScene();
    void InternalSetCurrentScene(std::shared_ptr<CHEngine::GameScene> scene)
    {
        m_currentScene = scene;
    }
    const std::string &InternalGetCurrentScenePath() const
    {
        return m_currentScenePath;
    }
    void InternalSetTransitionDuration(float seconds)
    {
        m_transitionDuration = seconds;
    }
    bool InternalIsTransitioning() const
    {
        return m_isTransitioning;
    }
    float InternalGetTransitionProgress() const
    {
        return m_transitionProgress;
    }
    void InternalUpdate(float deltaTime);

    // Non-copyable
    MapManager(const MapManager &) = delete;
    MapManager &operator=(const MapManager &) = delete;

    // Scene data
    std::shared_ptr<CHEngine::GameScene> m_currentScene;
    std::string m_currentScenePath;
    std::vector<std::pair<std::shared_ptr<CHEngine::GameScene>, std::string>> m_sceneStack;

    // Transition state
    bool m_isTransitioning = false;
    float m_transitionProgress = 0.0f;
    float m_transitionDuration = 0.5f;

    // Async loading
    bool m_isLoadingAsync = false;
    std::string m_nextScenePath;

    // Helper methods
    void BeginTransition();
    void EndTransition();
    void LoadSceneInternal(const std::string &scenePath);
};

} // namespace CHEngine
