#ifndef CD_EDITOR_LOGIC_EDITOR_SCENE_MANAGER_H
#define CD_EDITOR_LOGIC_EDITOR_SCENE_MANAGER_H

#include "scene/core/scene.h"
#include <memory>
#include <string>

namespace CHEngine
{
class SelectionManager;

/**
 * @brief Manages the active scenes in the editor (World and UI) using Pure ECS.
 */
class EditorSceneManager
{
public:
    EditorSceneManager(SelectionManager *selectionManager);
    ~EditorSceneManager() = default;

    // --- Scene Lifecycle ---
    void ClearScene();
    void SaveScene(const std::string &path = "");
    void LoadScene(const std::string &path);

    // --- Scene Access ---
    CHEngine::Scene *GetActiveScene()
    {
        return m_activeScene.get();
    }
    CHEngine::Scene *GetUIScene()
    {
        return m_uiScene.get();
    }

    enum class SceneContext
    {
        World,
        UI
    };
    void SetContext(SceneContext context)
    {
        m_currentContext = context;
    }
    SceneContext GetContext() const
    {
        return m_currentContext;
    }

    CHEngine::Scene *GetCurrentEditingScene()
    {
        return m_currentContext == SceneContext::World ? m_activeScene.get() : m_uiScene.get();
    }

    // --- Scene State ---
    bool IsSceneModified() const
    {
        return m_modified;
    }
    void SetSceneModified(bool modified)
    {
        m_modified = modified;
    }
    const std::string &GetCurrentMapPath() const
    {
        return m_currentMapPath;
    }

private:
    std::shared_ptr<CHEngine::Scene> m_activeScene;
    std::shared_ptr<CHEngine::Scene> m_uiScene;
    SceneContext m_currentContext = SceneContext::World;

    std::string m_currentMapPath;
    bool m_modified = false;

    SelectionManager *m_SelectionManager = nullptr;
};
} // namespace CHEngine

#endif // CD_EDITOR_LOGIC_EDITOR_SCENE_MANAGER_H
