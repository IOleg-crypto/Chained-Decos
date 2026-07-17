#ifndef CH_SCENE_CONTEXT_H
#define CH_SCENE_CONTEXT_H

namespace Chained
{
class Physics;
class ScriptEngine;
class WidgetRenderer;

/// @brief Explicit set of engine service pointers a Scene needs for its lifecycle
/// (state transitions, runtime start/stop, per-frame update).
///
/// Resolved ONCE by the owning layer (RuntimeLayer / EditorLayer) from ServiceLocator
/// — typically in its constructor, since ServiceLocator::Lock() has already run by the
/// time any layer is constructed (see Application::Application). Scene's public API
/// takes this by const-reference instead of reaching into ServiceLocator itself, so:
///   - the dependency is visible in the function signature,
///   - no per-call ServiceLocator mutex lock on the hot per-frame update path,
///   - Scene can be unit-tested with fake/mock services.
///
/// Scene::OnRuntimeStart caches a copy of this into the registry's entt::ctx() (as
/// SceneContext) for code that only has access to an entt::registry& and can't take
/// an extra parameter — e.g. SceneResources::OnRigidBodyConstruct, which is invoked
/// with a fixed (registry&, entity) signature. Scene::OnRuntimeStop erases it again.
///
/// UI may be null in headless mode (WidgetRenderer is not created when
/// ApplicationSpecification::Headless is true) — always null-check before use.
struct SceneContext
{
    Physics* PhysicsSystem = nullptr;
    ScriptEngine* Scripting = nullptr;
    WidgetRenderer* UI = nullptr;
};

} // namespace Chained

#endif // CH_SCENE_CONTEXT_H
