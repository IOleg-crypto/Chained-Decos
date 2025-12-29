#include "project/Project.h"
#include "scene/resources/map/GameScene.h"
#include <functional>
#include <memory>
#include <string>

namespace CHEngine
{
/**
 * @brief Manager for project-level operations in the editor.
 *
 * Coordinates between the Project system and the Editor's active scene.
 */
class ProjectManager
{
public:
    ProjectManager();
    ~ProjectManager() = default;

    // --- Project Operations ---
public:
    std::shared_ptr<Project> NewProject(const std::string &name, const std::string &location);
    std::shared_ptr<Project> OpenProject(const std::string &path);
    void SaveProject();
    void CloseProject();

    std::shared_ptr<Project> GetActiveProject() const
    {
        return m_ActiveProject;
    }

    // --- Scene Operations ---
public:
    void NewScene();
    bool OpenScene(); // UI dialog
    bool OpenScene(const std::string &path);
    void SaveScene();
    void SaveSceneAs();

    // --- Getters & Setters ---
public:
    std::shared_ptr<GameScene> GetActiveScene() const;
    void SetActiveScene(std::shared_ptr<GameScene> scene);

    const std::string &GetScenePath() const;

    // --- Callbacks ---
public:
    void SetSceneChangedCallback(std::function<void(std::shared_ptr<GameScene>)> callback);

    // --- Member Variables ---
private:
    std::shared_ptr<Project> m_ActiveProject;
    std::shared_ptr<GameScene> m_ActiveScene;
    std::string m_ScenePath;

    std::function<void(std::shared_ptr<GameScene>)> m_SceneChangedCallback;
};
} // namespace CHEngine
