#ifndef CH_SCENE_SCRIPTING_MANAGER_H
#define CH_SCENE_SCRIPTING_MANAGER_H

#include "engine/core/events/events.h"
#include "engine/common/timestep.h"
#include <Coral/Type.hpp>

namespace Chained
{
class Scene;
class ScriptEngine;

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
    struct ScriptEngineContext
    {
        ScriptEngine* engine = nullptr;
        Coral::Type scriptEngineType = {};
    };
    ScriptEngineContext AcquireScriptEngine();

private:
    Scene* m_Scene = nullptr;
    bool m_ReloadInProgress = false;
};

} // namespace Chained

#endif // CH_SCENE_SCRIPTING_MANAGER_H
