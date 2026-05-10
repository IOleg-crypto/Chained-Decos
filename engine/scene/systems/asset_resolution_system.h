#ifndef CH_ASSET_RESOLUTION_SYSTEM_H
#define CH_ASSET_RESOLUTION_SYSTEM_H

#include "engine/scene/scene_system.h"
#include <entt/entt.hpp>

namespace CHEngine
{

class Scene;

class AssetResolutionSystem : public ISceneSystem
{
public:
    void RegisterObservers(entt::registry& reg) override;
    
    void OnUpdate(Scene* scene, Timestep ts) override;

private:
    void OnSpriteChanged(entt::registry& reg, entt::entity e);
    void OnShaderChanged(entt::registry& reg, entt::entity e);
    void OnModelChanged(entt::registry& reg, entt::entity e);
};

} // namespace CHEngine

#endif // CH_ASSET_RESOLUTION_SYSTEM_H
