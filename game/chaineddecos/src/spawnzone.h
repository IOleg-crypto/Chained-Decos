#ifndef CH_SPAWNZONE_H
#define CH_SPAWNZONE_H

#include "engine/core/events.h"
#include "engine/core/input.h"
#include "engine/scene/components.h"
#include "engine/scene/scriptable_entity.h"
#include "raymath.h"

namespace CHEngine
{
CH_SCRIPT(SpawnZoneRespawn){public : CH_START(){CH_CORE_INFO("Spawn Zone (Teleporter) Initialized!");
}

CH_UPDATE(dt)
{
    // Auto-respawn if falling too deep
    if (GetComponent<TransformComponent>().Translation.y < -100.0f)
    {
        Respawn();
    }
}

void Respawn()
{
    auto scene = GetScene();
    if (!scene)
    {
        return;
    }

    auto& registry = scene->GetRegistry();

    // Find the active spawn zone entity
    auto spawnZoneView = registry.view<SpawnComponent, TransformComponent>();

    for (auto spawnEntity : spawnZoneView)
    {
        auto& spawnComp = spawnZoneView.get<SpawnComponent>(spawnEntity);
        if (spawnComp.IsActive)
        {
            auto& spawnTransform = spawnZoneView.get<TransformComponent>(spawnEntity);

            // Teleporter logic using Vector3
            Vector3 worldPos = spawnTransform.Translation;
            Vector3 localOffset = spawnComp.SpawnPoint;
            Vector3 result = Vector3Add(worldPos, localOffset);

            GetComponent<TransformComponent>().Translation = result;

            // Reset velocity if we have a RigidBody
            if (HasComponent<RigidBodyComponent>())
            {
                GetComponent<RigidBodyComponent>().Velocity = {0, 0, 0};
            }

            CH_CORE_INFO("SpawnZone: Player teleported to entity world position.");
            return;
        }
    }
}

CH_EVENT(e)
{
    EventDispatcher dispatcher(e);
    dispatcher.Dispatch<KeyPressedEvent>([this](KeyPressedEvent& ev) {
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

#endif // CH_SPAWNZONE_H
