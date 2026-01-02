#ifndef CD_EDITOR_LOGIC_EDITOR_SCENE_ACTIONS_H
#define CD_EDITOR_LOGIC_EDITOR_SCENE_ACTIONS_H

#include <memory>
#include <string>

namespace CHD
{
class RuntimeLayer;
}

namespace CHEngine
{
class Scene;
class EditorSceneManager;
class SceneSimulationManager;

class EditorSceneActions
{
public:
    EditorSceneActions(EditorSceneManager *sceneManager, SceneSimulationManager *simulationManager,
                       std::shared_ptr<CHEngine::Scene> scene, CHD::RuntimeLayer **runtimeLayer);

    void OnScenePlay();
    void OnSceneStop();

    EditorSceneManager *GetSceneManager()
    {
        return m_SceneManager;
    }
    std::shared_ptr<CHEngine::Scene> GetActiveScene()
    {
        return m_Scene;
    }

    void NewScene();
    void OpenScene();
    void OpenScene(const std::string &path);
    void SaveScene();
    void SaveSceneAs();

private:
    EditorSceneManager *m_SceneManager;
    SceneSimulationManager *m_SimulationManager;
    std::shared_ptr<CHEngine::Scene> m_Scene;
    CHD::RuntimeLayer **m_RuntimeLayer;
};
} // namespace CHEngine

#endif // CD_EDITOR_LOGIC_EDITOR_SCENE_ACTIONS_H
