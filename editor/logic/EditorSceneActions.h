#ifndef EDITOR_SCENE_ACTIONS_H
#define EDITOR_SCENE_ACTIONS_H

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

    void NewScene();
    void OpenScene();
    void SaveScene();
    void SaveSceneAs();

private:
    EditorSceneManager *m_SceneManager;
    SceneSimulationManager *m_SimulationManager;
    std::shared_ptr<CHEngine::Scene> m_Scene;
    CHD::RuntimeLayer **m_RuntimeLayer;
};
} // namespace CHEngine

#endif // EDITOR_SCENE_ACTIONS_H
