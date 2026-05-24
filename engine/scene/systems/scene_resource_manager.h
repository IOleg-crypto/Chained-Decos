#ifndef CH_SCENE_RESOURCE_MANAGER_H
#define CH_SCENE_RESOURCE_MANAGER_H

#include "engine/scene/scene_system.h"
#include <entt/entt.hpp>
#include "engine/scene/components/animation_component.h"
#include "engine/scene/components/audio_component.h"
#include "engine/scene/components/sprite_component.h"
#include "engine/scene/components/shader_component.h"
#include "engine/scene/components/mesh_component.h"
#include <entt/entt.hpp>

namespace CHEngine
{
class Scene;

class SceneResourceManager : public ISceneSystem
{
public:
    void RegisterObservers(entt::registry& reg) override;

    void OnUpdate(Scene* scene, Timestep ts) override;
    void OnRuntimeStart(Scene* scene) override;
    void OnRuntimeStop(Scene* scene) override;
    void OnUpdateEditor(Scene* scene, Timestep ts) override;

private:
    // Asset resolution callbacks
    void OnSpriteChanged(entt::registry& reg, entt::entity e);
    void OnShaderChanged(entt::registry& reg, entt::entity e);
    void OnModelChanged(entt::registry& reg, entt::entity e);
};

} // namespace CHEngine

#endif // CH_SCENE_RESOURCE_MANAGER_H
