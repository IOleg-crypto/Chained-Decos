using System;
using System.Runtime.InteropServices;

namespace Chained
{
    /// <summary>Camera wrapper.</summary>
    public class CameraComponent : Component
    {
#pragma warning disable 0649
        internal static unsafe delegate* unmanaged<ulong, Vector3*, void> Camera_GetForward_Ptr;
        internal static unsafe delegate* unmanaged<ulong, Vector3*, void> Camera_GetRight_Ptr;
        internal static unsafe delegate* unmanaged<ulong, float*, float*, float*, void> Camera_GetOrbit_Ptr;
        internal static unsafe delegate* unmanaged<ulong, float, float, float, void> Camera_SetOrbit_Ptr;
        internal static unsafe delegate* unmanaged<ulong, byte> Camera_GetPrimary_Ptr;
        internal static unsafe delegate* unmanaged<ulong, bool, void> Camera_SetPrimary_Ptr;
        internal static unsafe delegate* unmanaged<ulong, byte> Camera_GetIsOrbit_Ptr;
        internal static unsafe delegate* unmanaged<ulong, bool, void> Camera_SetIsOrbit_Ptr;
        internal static unsafe delegate* unmanaged<ulong, char*> Camera_GetTargetTag_Ptr;
        internal static unsafe delegate* unmanaged<ulong, char*, void> Camera_SetTargetTag_Ptr;
#pragma warning restore 0649

        private static void GetForward(ulong entityID, out Vector3 outForward)
        {
            unsafe { fixed (Vector3* p = &outForward) Camera_GetForward_Ptr(entityID, p); }
        }

        private static void GetRight(ulong entityID, out Vector3 outRight)
        {
            unsafe { fixed (Vector3* p = &outRight) Camera_GetRight_Ptr(entityID, p); }
        }

        private static void GetOrbit(ulong entityID, out float yaw, out float pitch, out float distance)
        {
            unsafe { fixed (float* yawPtr = &yaw, pitchPtr = &pitch, distancePtr = &distance) Camera_GetOrbit_Ptr(entityID, yawPtr, pitchPtr, distancePtr); }
        }

        private static void SetOrbit(ulong entityID, float yaw, float pitch, float distance)
        {
            unsafe { Camera_SetOrbit_Ptr(entityID, yaw, pitch, distance); }
        }

        public Vector3 Forward
        {
            get { GetForward(Entity.ID, out Vector3 forward); return forward; }
        }

        public Vector3 Right
        {
            get { GetRight(Entity.ID, out Vector3 right); return right; }
        }

        public void GetOrbit(out float yaw, out float pitch, out float distance)
            => GetOrbit(Entity.ID, out yaw, out pitch, out distance);

        public void SetOrbit(float yaw, float pitch, float distance)
            => SetOrbit(Entity.ID, yaw, pitch, distance);

        public bool Primary
        {
            get { unsafe { return Camera_GetPrimary_Ptr(Entity.ID) != 0; } }
            set { unsafe { Camera_SetPrimary_Ptr(Entity.ID, value); } }
        }

        public bool IsOrbitCamera
        {
            get { unsafe { return Camera_GetIsOrbit_Ptr(Entity.ID) != 0; } }
            set { unsafe { Camera_SetIsOrbit_Ptr(Entity.ID, value); } }
        }

        public string TargetEntityTag
        {
            get { unsafe { return Marshal.PtrToStringUni(new IntPtr(Camera_GetTargetTag_Ptr(Entity.ID))) ?? string.Empty; } }
            set { unsafe { if (Camera_SetTargetTag_Ptr != null) fixed (char* ptr = value) Camera_SetTargetTag_Ptr(Entity.ID, ptr); } }
        }
    }
}
