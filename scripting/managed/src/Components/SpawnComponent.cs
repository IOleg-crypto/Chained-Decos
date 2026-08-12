using System;
using System.Runtime.InteropServices;

namespace Chained
{
    /// <summary>Spawn point component.</summary>
    public class SpawnComponent : Component
    {
#pragma warning disable 0649
        internal static unsafe delegate* unmanaged<ulong, byte> SpawnComponent_GetIsActive_Ptr;
        internal static unsafe delegate* unmanaged<ulong, bool, void> SpawnComponent_SetIsActive_Ptr;
        internal static unsafe delegate* unmanaged<ulong, byte> SpawnComponent_IsCheckpoint_Ptr;
        internal static unsafe delegate* unmanaged<ulong, bool, void> SpawnComponent_SetIsCheckpoint_Ptr;
        internal static unsafe delegate* unmanaged<ulong, Vector3*, void> SpawnComponent_GetSpawnPoint_Ptr;
        internal static unsafe delegate* unmanaged<ulong, Vector3*, void> SpawnComponent_SetSpawnPoint_Ptr;
        internal static unsafe delegate* unmanaged<ulong, byte> SpawnComponent_GetRenderSpawnZoneInScene_Ptr;
        internal static unsafe delegate* unmanaged<ulong, Vector3*, void> SpawnComponent_GetZoneSize_Ptr;
#pragma warning restore 0649

        public bool IsActive
        {
            get { unsafe { return SpawnComponent_GetIsActive_Ptr != null && SpawnComponent_GetIsActive_Ptr(Entity.ID) != 0; } }
            set { unsafe { if (SpawnComponent_SetIsActive_Ptr != null) SpawnComponent_SetIsActive_Ptr(Entity.ID, value); } }
        }

        public bool IsCheckpoint
        {
            get { unsafe { return SpawnComponent_IsCheckpoint_Ptr != null && SpawnComponent_IsCheckpoint_Ptr(Entity.ID) != 0; } }
            set { unsafe { if (SpawnComponent_SetIsCheckpoint_Ptr != null) SpawnComponent_SetIsCheckpoint_Ptr(Entity.ID, value); } }
        }

        public Vector3 SpawnPoint
        {
            get
            {
                unsafe
                {
                    if (SpawnComponent_GetSpawnPoint_Ptr == null) return Vector3.Zero;
                    Vector3 point;
                    SpawnComponent_GetSpawnPoint_Ptr(Entity.ID, &point);
                    return point;
                }
            }
            set
            {
                unsafe
                {
                    if (SpawnComponent_SetSpawnPoint_Ptr != null)
                        SpawnComponent_SetSpawnPoint_Ptr(Entity.ID, &value);
                }
            }
        }

        public bool RenderSpawnZoneInScene
        {
            get { unsafe { return SpawnComponent_GetRenderSpawnZoneInScene_Ptr != null && SpawnComponent_GetRenderSpawnZoneInScene_Ptr(Entity.ID) != 0; } }
        }

        public Vector3 ZoneSize
        {
            get
            {
                unsafe
                {
                    if (SpawnComponent_GetZoneSize_Ptr == null) return Vector3.One;
                    Vector3 size;
                    SpawnComponent_GetZoneSize_Ptr(Entity.ID, &size);
                    return size;
                }
            }
        }
    }
}
