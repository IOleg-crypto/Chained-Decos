#ifndef PROJECT_MANAGER_H
#define PROJECT_MANAGER_H

#include "scene/resources/map/GameScene.h"
#include <functional>
#include <memory>
#include <string>


namespace CHEngine
{
class ProjectManager
{
public:
    ProjectManager();
    ~ProjectManager() = default;

    void NewScene();
    bool OpenScene(); // Returns true if successful
    bool OpenScene(const std::string &path);
    void SaveScene();
    void SaveSceneAs();

    std::shared_ptr<GameScene> GetActiveScene() const
    {
        return m_ActiveScene;
    }
    void SetActiveScene(std::shared_ptr<GameScene> scene)
    {
        m_ActiveScene = scene;
    }

    const std::string &GetScenePath() const
    {
        return m_ScenePath;
    }

    // Callbacks for when scene changes (e.g. to update panels)
    void SetSceneChangedCallback(std::function<void(std::shared_ptr<GameScene>)> callback)
    {
        m_SceneChangedCallback = callback;
    }

private:
    std::shared_ptr<GameScene> m_ActiveScene;
    std::string m_ScenePath;

    std::function<void(std::shared_ptr<GameScene>)> m_SceneChangedCallback;
};
} // namespace CHEngine

#endif // PROJECT_MANAGER_H
