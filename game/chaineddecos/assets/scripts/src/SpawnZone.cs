using System;
using Chained;

namespace ChainedDecos.Scripts
{
    public class SpawnZone : Script
    {
        public override void OnUpdate(float deltaTime)
        {
            TransformComponent? transform = Entity.GetComponent<TransformComponent>();
            if (transform != null && transform.Translation.Y < -100.0f)
            {
                Respawn();
            }

            if (Input.IsKeyPressed(Key.F)) // KEY_F
            {
                Respawn();
            }
        }

        private void Respawn()
        {
            // Find all entities with SpawnComponent instead of relying solely on a generic tag
            ulong[] spawnEntities = Entity.FindAllWithComponent<SpawnComponent>();
            if (spawnEntities.Length == 0)
            {
                // Fallback to Tag if no Component is found (for backwards compatibility)
                Entity? fallback = Scene.FindEntityByTag("SpawnPoint");
                if (fallback != null)
                {
                    TeleportTo(fallback);
                }
                else
                {
                    Log.Info("spawnzone: No spawn points found in the scene.");
                }
                return;
            }

            // Iterate and pick the active one (or just use the first match)
            foreach (ulong id in spawnEntities)
            {
                Entity spawner = new Entity(id);
                SpawnComponent? spawnComp = spawner.GetComponent<SpawnComponent>();
                if (spawnComp != null && spawnComp.IsActive)
                {
                    TeleportTo(spawner);
                    return;
                }
            }

            // If none are specifically "active", just use the very first one
            TeleportTo(new Entity(spawnEntities[0]));
        }

        private void TeleportTo(Entity spawnEntity)
        {
            SpawnComponent? spawnComp = spawnEntity.GetComponent<SpawnComponent>();
            TransformComponent? spawnTransform = spawnEntity.GetComponent<TransformComponent>();
            TransformComponent? myTransform = Entity.GetComponent<TransformComponent>();
            RigidBodyComponent? myRb = Entity.GetComponent<RigidBodyComponent>();

            if (spawnTransform != null && myTransform != null)
            {
                // If it has a spawn component, add its offset; otherwise just use its transform translation
                Vector3 offset = spawnComp != null ? spawnComp.SpawnPoint : Vector3.Zero;
                myTransform.Translation = spawnTransform.Translation + offset;
                
                if (myRb != null)
                {
                    myRb.Velocity = Vector3.Zero;
                }
                
                Log.Info("spawnzone: Player respawned successfully.");
            }
        }
    }
}

