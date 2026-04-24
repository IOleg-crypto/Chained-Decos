#ifndef CH_SCENE_SCRIPTING_MANAGER_H
#define CH_SCENE_SCRIPTING_MANAGER_H

#include "engine/core/events.h"
#include "engine/core/timestep.h"
#include "engine/scene/scene.h"

namespace CHEngine
{

class SceneScriptingManager
{
public:
    SceneScriptingManager(Scene* scene);
    ~SceneScriptingManager();

    void OnRuntimeStart();
    void OnRuntimeStop();
    void OnUpdate(Timestep deltaTime);
    void OnEvent(Event& e);
    void OnRenderUI();

    bool IsReloadInProgress() const { return m_ReloadInProgress; }
    void SetReloadInProgress(bool inProgress) { m_ReloadInProgress = inProgress; }

    static void Register(SceneScriptingManager* manager);
    static void Unregister(SceneScriptingManager* manager);
    static void ResetAll();

private:
    Scene* m_Scene = nullptr;
    bool m_ReloadInProgress = false;
};

} // namespace CHEngine

#endif // CH_SCENE_SCRIPTING_MANAGER_H
