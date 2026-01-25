#ifndef CH_SPAWNZONE_H
#define CH_SPAWNZONE_H

#include "engine/core/events.h"
#include "engine/core/input.h"
#include "engine/scene/components.h"
#include "engine/scene/scriptable_entity.h"
#include <raymath.h>

namespace CHEngine
{
CH_SCRIPT(SpawnZoneRespawn){CH_START(){CH_CORE_INFO("Spawn Zone (Teleporter) Initialized!");
}

CH_UPDATE(dt)
{
    // Auto-respawn if falling too deep
    if (Translation().y < -100.0f)
        Respawn();
}

void Respawn()
{
    auto scene = GetEntity().GetScene();
    if (!scene)
        return;

    auto &registry = scene->GetRegistry();

    // Find the active spawn zone entity
    auto spawnZoneView = registry.view<SpawnComponent, TransformComponent>();

    for (auto spawnEntity : spawnZoneView)
    {
        auto &spawnComp = spawnZoneView.get<SpawnComponent>(spawnEntity);
        if (spawnComp.IsActive)
        {
            auto &spawnTransform = spawnZoneView.get<TransformComponent>(spawnEntity);

            // Teleport to the entity's position + the local offset
            Translation() = Vector3Add(spawnTransform.Translation, spawnComp.SpawnPoint);

            // Reset velocity if we have a RigidBody
            if (HasComponent<RigidBodyComponent>())
                RigidBody().Velocity = {0, 0, 0};

            CH_CORE_INFO("SpawnZone: Player teleported to entity world position.");
            return;
        }
    }
}

CH_EVENT(e)
{
    EventDispatcher dispatcher(e);
    dispatcher.Dispatch<KeyPressedEvent>(
        [this](KeyPressedEvent &ev)
        {
            // Respawn using the 'F' key
            if (ev.GetKeyCode() == KEY_F)
            {
                Respawn();
                return true;
            }
            return false;
        });
}
}
;
} // namespace CHEngine

#endif
