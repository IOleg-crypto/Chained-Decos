#pragma once

#include "scene/resources/map/SceneLoader.h"
#include <memory>
#include <string>
#include <vector>

namespace CHEngine
{

/**
 * SceneManager - Singleton for managing scene loading and transitions
 * Handles scene switching, scene stack for overlays/menus, and transitions
 */
class SceneManager
{
public:
    SceneManager() = default;
    ~SceneManager() = default;

    // Scene loading
    void LoadScene(const std::string &scenePath);
    void LoadSceneAsync(const std::string &scenePath);

    // Scene stack (for menus/overlays that can be pushed/popped)
    void PushScene(const std::string &scenePath);
    void PopScene();
    void ClearSceneStack();

    // Current scene access
    GameScene *GetCurrentScene();
    const std::string &GetCurrentScenePath() const;

    // Scene transitions
    void SetTransitionDuration(float seconds);
    bool IsTransitioning() const;
    float GetTransitionProgress() const;

    // Update (call each frame to handle async loading and transitions)
    void Update(float deltaTime);

private:
    // Non-copyable
    SceneManager(const SceneManager &) = delete;
    SceneManager &operator=(const SceneManager &) = delete;

    // Scene data
    GameScene m_currentScene;
    std::string m_currentScenePath;
    std::vector<std::pair<GameScene, std::string>> m_sceneStack;

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
