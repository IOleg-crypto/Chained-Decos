#ifndef CD_EDITOR_LOGIC_SCENE_SIMULATION_MANAGER_H
#define CD_EDITOR_LOGIC_SCENE_SIMULATION_MANAGER_H

#include "../editor_types.h"
#include <memory>
#include <string>

#include "core/application/application.h"
#include "runtime/runtime_layer.h"
#include "scene/core/scene.h"
#include "scene/core/scene_manager.h"

namespace CHEngine
{

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
    void OnScenePlay(Scene *newScene, CHD::RuntimeLayer **runtimeLayer, Application *app);
    void OnSceneStop(Scene *newScene, CHD::RuntimeLayer **runtimeLayer, Application *app);

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

#endif // CD_EDITOR_LOGIC_SCENE_SIMULATION_MANAGER_H
