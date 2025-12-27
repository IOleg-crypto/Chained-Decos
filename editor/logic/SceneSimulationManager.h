#ifndef SCENE_SIMULATION_MANAGER_H
#define SCENE_SIMULATION_MANAGER_H

#include "editor/EditorTypes.h"
#include "scene/resources/map/GameScene.h"
#include <memory>
#include <string>

#include "core/application/EngineApplication.h"
#include "project/Runtime/RuntimeLayer.h"
#include "scene/core/Scene.h"

namespace CHEngine
{

class SceneSimulationManager
{
public:
    SceneSimulationManager();
    ~SceneSimulationManager() = default;

    void OnScenePlay(std::shared_ptr<GameScene> &activeScene,
                     std::shared_ptr<GameScene> &editorScene, std::shared_ptr<Scene> &newScene,
                     RuntimeMode runtimeMode, CHD::RuntimeLayer **runtimeLayer,
                     EngineApplication *app);
    void OnSceneStop(std::shared_ptr<GameScene> &activeScene,
                     std::shared_ptr<GameScene> editorScene, std::shared_ptr<Scene> &newScene,
                     CHD::RuntimeLayer **runtimeLayer, EngineApplication *app);

    SceneState GetSceneState() const
    {
        return m_SceneState;
    }
    void SetSceneState(SceneState state)
    {
        m_SceneState = state;
    }

    RuntimeMode GetRuntimeMode() const
    {
        return m_RuntimeMode;
    }
    void SetRuntimeMode(RuntimeMode mode)
    {
        m_RuntimeMode = mode;
    }

private:
    SceneState m_SceneState = SceneState::Edit;
    RuntimeMode m_RuntimeMode = RuntimeMode::Standalone;
};
} // namespace CHEngine

#endif // SCENE_SIMULATION_MANAGER_H
