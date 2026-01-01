#ifndef SCENE_SIMULATION_MANAGER_H
#define SCENE_SIMULATION_MANAGER_H

#include "EditorTypes.h"
#include "scene/resources/map/GameScene.h"
#include <memory>
#include <string>

#include "core/application/Application.h"
#include "core/scripting/ScriptManager.h"
#include "runtime/RuntimeLayer.h"
#include "scene/core/Scene.h"
#include "scene/core/SceneManager.h"

namespace CHEngine
{
class GameScene;

/**
 * @brief Manager for toggling between Edit and Play modes
 */
class SceneSimulationManager
{
public:
    SceneSimulationManager();
    ~SceneSimulationManager() = default;

    // --- Simulation Lifecycle ---
public:
    void OnScenePlay(std::shared_ptr<GameScene> &activeScene,
                     std::shared_ptr<GameScene> &editorScene, std::shared_ptr<Scene> &newScene,
                     CHD::RuntimeLayer **runtimeLayer, Application *app);
    void OnSceneStop(std::shared_ptr<GameScene> &activeScene,
                     std::shared_ptr<GameScene> editorScene, std::shared_ptr<Scene> &newScene,
                     CHD::RuntimeLayer **runtimeLayer, Application *app);

    // --- Getters & Setters ---
public:
    SceneState GetSceneState() const;
    void SetSceneState(SceneState state);

    RuntimeMode GetRuntimeMode() const;
    void SetRuntimeMode(RuntimeMode mode);

    // --- Member Variables ---
private:
    SceneState m_SceneState = SceneState::Edit;
    RuntimeMode m_RuntimeMode = RuntimeMode::Standalone;
};
} // namespace CHEngine

#endif // SCENE_SIMULATION_MANAGER_H
