#ifndef PROJECT_SCENE_MANAGER_H
#define PROJECT_SCENE_MANAGER_H

#include "editor/core/EditorContext.h"
#include "events/Event.h"
#include <functional>
#include <string>
#include <vector>

// ProjectSceneManager - Handles scene loading, saving, and recent files at project level
class ProjectSceneManager
{
public:
    explicit ProjectSceneManager(EditorContext &context);
    ~ProjectSceneManager();

    // Set callback for event propagation
    void SetEventCallback(std::function<void(CHEngine::Event &)> callback);

    // Scene operations
    void LoadScene(const std::string &path);
    void SaveScene(const std::string &path = "");
    void NewScene();

    // Recent scenes
    const std::vector<std::string> &GetRecentScenes() const;

    // State queries
    bool HasUnsavedChanges() const;

private:
    EditorContext &m_context;
    std::vector<std::string> m_recentScenes;
    std::function<void(CHEngine::Event &)> m_eventCallback;

    void AddToRecentScenes(const std::string &path);
    void LoadRecentScenes();
    void SaveRecentScenes();
};

#endif // PROJECT_SCENE_MANAGER_H
