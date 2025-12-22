#ifndef SCENE_MANAGER_H
#define SCENE_MANAGER_H

#include "core/events/Event.h"
#include "editor/core/EditorContext.h"
#include <functional>
#include <string>
#include <vector>


// SceneManager - Handles scene loading, saving, and recent files
class SceneManager
{
public:
    explicit SceneManager(EditorContext &context);
    ~SceneManager();

    // Set callback for event propagation
    void SetEventCallback(std::function<void(ChainedDecos::Event &)> callback);

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
    std::function<void(ChainedDecos::Event &)> m_eventCallback;

    void AddToRecentScenes(const std::string &path);
    void LoadRecentScenes();
    void SaveRecentScenes();
};

#endif // SCENE_MANAGER_H
