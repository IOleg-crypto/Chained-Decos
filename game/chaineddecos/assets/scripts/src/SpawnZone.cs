using System;
using Chained;

namespace ChainedDecos.Scripts
{
    public class SpawnZone : Script
    {
        public override void OnUpdate(float deltaTime)
        {
            ulong[] players = Entity.FindAllWithComponent<PlayerComponent>();
            if (players.Length == 0)
                return;

            Entity player = new Entity(players[0]);
            TransformComponent? playerTransform = player.GetComponent<TransformComponent>();
            if (playerTransform == null)
                return;

            if (playerTransform.Translation.Y < -50.0f)
            {
                Respawn(player);
            }

            if(Input.IsKeyPressed(Key.F))
            {
                Respawn(player);
            }
        }

        private void Respawn(Entity player)
        {
            ulong[] spawnEntities = Entity.FindAllWithComponent<SpawnComponent>();
            if (spawnEntities.Length == 0)
            {
                Log.Info("spawnzone: No spawn points found in the scene.");
                return;
            }

            foreach (ulong id in spawnEntities)
            {
                Entity spawner = new Entity(id);
                SpawnComponent? spawnComp = spawner.GetComponent<SpawnComponent>();
                if (spawnComp != null && spawnComp.IsActive)
                {
                    TeleportTo(player, spawner, spawnComp);
                    return;
                }
            }

            Entity fallback = new Entity(spawnEntities[0]);
            SpawnComponent? fallbackComp = fallback.GetComponent<SpawnComponent>();
            if (fallbackComp != null)
                TeleportTo(player, fallback, fallbackComp);
        }

        private void TeleportTo(Entity player, Entity spawnEntity, SpawnComponent spawnComp)
        {
            TransformComponent? spawnTransform = spawnEntity.GetComponent<TransformComponent>();
            TransformComponent? playerTransform = player.GetComponent<TransformComponent>();
            RigidBodyComponent? playerRb = player.GetComponent<RigidBodyComponent>();

            if (spawnTransform == null || playerTransform == null)
                return;

            float halfX = spawnComp.ZoneSize.X * 0.5f;
            float halfY = spawnComp.ZoneSize.Y * 0.5f;
            float halfZ = spawnComp.ZoneSize.Z * 0.5f;

            Random rng = new Random();
            float offsetX = (float)(rng.NextDouble() * 2.0 - 1.0) * halfX;
            float offsetY = (float)(rng.NextDouble() * 2.0 - 1.0) * halfY;
            float offsetZ = (float)(rng.NextDouble() * 2.0 - 1.0) * halfZ;

            playerTransform.Translation = new Vector3(
                spawnTransform.Translation.X + offsetX,
                spawnTransform.Translation.Y + offsetY,
                spawnTransform.Translation.Z + offsetZ
            );

            if (playerRb != null)
            {
                playerRb.ForceSetVelocity(Vector3.Zero);
            }

            Log.Info("spawnzone: Player respawned at random position within zone.");
        }
    }
}
