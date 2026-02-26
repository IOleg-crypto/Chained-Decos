using System;
using CHEngine;

namespace ChainedDecos.Scripts
{
public class SpawnZone : Script
{
    public override void OnUpdate(float deltaTime)
    {
        TransformComponent transform = Entity.GetComponent<TransformComponent>();
        if (transform != null && transform.Translation.Y < -100.0f)
        {
            Respawn();
        }

        if (Input.IsKeyPressed((Key)70)) // KEY_F
        {
            Respawn();
        }
    }

    private void Respawn()
    {
        // Note: In C# we don't have direct registry access, so we use Scene.FindEntityByTag
        // to find a spawn point if we can't iterate components yet.
        // Optimization: If the engine has a dedicated SpawnSystem, we could call an internal method.
        // For now, let's try to find an entity named "SpawnPoint" or with a tag.

        Entity spawnPoint = Scene.FindEntityByTag("SpawnPoint");
        if (spawnPoint != null)
        {
            SpawnComponent spawnComp = spawnPoint.GetComponent<SpawnComponent>();
            TransformComponent spawnTransform = spawnPoint.GetComponent<TransformComponent>();
            TransformComponent myTransform = Entity.GetComponent<TransformComponent>();
            RigidBodyComponent myRb = Entity.GetComponent<RigidBodyComponent>();

            if (spawnComp != null && spawnTransform != null && myTransform != null)
            {
                Vector3 pos = spawnComp.SpawnPoint;
                myTransform.Translation = spawnTransform.Translation + pos;
                if (myRb != null)
                {
                    myRb.Velocity = Vector3.Zero;
                }
                Log.Info("spawnzone: Player respawned");
            }
        }
    }
}
}
