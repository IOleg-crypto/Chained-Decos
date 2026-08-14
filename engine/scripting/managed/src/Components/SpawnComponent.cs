using System;
using Coral.Managed.Interop;

namespace Chained
{
    /// <summary>Spawn point component.</summary>
    [NativeProperty("IsActive", "bool", "SpawnComponent_GetIsActive", "SpawnComponent_SetIsActive")]
    [NativeProperty("IsCheckpoint", "bool", "SpawnComponent_IsCheckpoint", "SpawnComponent_SetIsCheckpoint")]
    [NativeProperty("SpawnPoint", "Vector3", "SpawnComponent_GetSpawnPoint", "SpawnComponent_SetSpawnPoint")]
    [NativeProperty("RenderSpawnZoneInScene", "bool", "SpawnComponent_GetRenderSpawnZoneInScene")]
    [NativeProperty("ZoneSize", "Vector3", "SpawnComponent_GetZoneSize")]
    public partial class SpawnComponent : Component
    {
    }
}
