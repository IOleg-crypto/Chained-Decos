using System;
using System.Runtime.InteropServices;

namespace Chained
{
    /// <summary>Animation component wrapper.</summary>
    public class AnimationComponent : Component
    {
#pragma warning disable 0649
        internal static unsafe delegate* unmanaged<ulong, int> AnimationComponent_GetCurrentAnimationIndex_Ptr;
        internal static unsafe delegate* unmanaged<ulong, int, void> AnimationComponent_SetCurrentAnimationIndex_Ptr;
        internal static unsafe delegate* unmanaged<ulong, uint> AnimationComponent_GetIsPlaying_Ptr;
        internal static unsafe delegate* unmanaged<ulong, uint, void> AnimationComponent_SetIsPlaying_Ptr;
        internal static unsafe delegate* unmanaged<ulong, byte> AnimationComponent_GetIsLooping_Ptr;
        internal static unsafe delegate* unmanaged<ulong, bool, void> AnimationComponent_SetIsLooping_Ptr;
        internal static unsafe delegate* unmanaged<ulong, byte> AnimationComponent_GetIsFinished_Ptr;
        internal static unsafe delegate* unmanaged<ulong, float> AnimationComponent_GetDuration_Ptr;
        internal static unsafe delegate* unmanaged<ulong, float> AnimationComponent_GetNormalizedTime_Ptr;
        internal static unsafe delegate* unmanaged<ulong, float> AnimationComponent_GetBlendDuration_Ptr;
        internal static unsafe delegate* unmanaged<ulong, float, void> AnimationComponent_SetBlendDuration_Ptr;
        internal static unsafe delegate* unmanaged<ulong, int, float, void> AnimationComponent_CrossFade_Ptr;
        internal static unsafe delegate* unmanaged<ulong, char*, float, void> AnimationComponent_SetFloat_Ptr;
        internal static unsafe delegate* unmanaged<ulong, char*, bool, void> AnimationComponent_SetBool_Ptr;
        internal static unsafe delegate* unmanaged<ulong, char*, float> AnimationComponent_GetFloat_Ptr;
#pragma warning restore 0649

        public int CurrentAnimationIndex
        {
            get { unsafe { return AnimationComponent_GetCurrentAnimationIndex_Ptr != null ? AnimationComponent_GetCurrentAnimationIndex_Ptr(Entity.ID) : 0; } }
            set { unsafe { if (AnimationComponent_SetCurrentAnimationIndex_Ptr != null) AnimationComponent_SetCurrentAnimationIndex_Ptr(Entity.ID, value); } }
        }

        public bool IsPlaying
        {
            get { unsafe { return AnimationComponent_GetIsPlaying_Ptr != null && AnimationComponent_GetIsPlaying_Ptr(Entity.ID) != 0; } }
            set { unsafe { if (AnimationComponent_SetIsPlaying_Ptr != null) AnimationComponent_SetIsPlaying_Ptr(Entity.ID, value ? 1u : 0u); } }
        }

        public bool IsLooping
        {
            get { unsafe { return AnimationComponent_GetIsLooping_Ptr != null && AnimationComponent_GetIsLooping_Ptr(Entity.ID) != 0; } }
            set { unsafe { if (AnimationComponent_SetIsLooping_Ptr != null) AnimationComponent_SetIsLooping_Ptr(Entity.ID, value); } }
        }

        public bool IsFinished
        {
            get { unsafe { return AnimationComponent_GetIsFinished_Ptr != null && AnimationComponent_GetIsFinished_Ptr(Entity.ID) != 0; } }
        }

        public float Duration
        {
            get { unsafe { return AnimationComponent_GetDuration_Ptr != null ? AnimationComponent_GetDuration_Ptr(Entity.ID) : 0.0f; } }
        }

        public float NormalizedTime
        {
            get { unsafe { return AnimationComponent_GetNormalizedTime_Ptr != null ? AnimationComponent_GetNormalizedTime_Ptr(Entity.ID) : 0.0f; } }
        }

        public float BlendDuration
        {
            get { unsafe { return AnimationComponent_GetBlendDuration_Ptr != null ? AnimationComponent_GetBlendDuration_Ptr(Entity.ID) : 0.25f; } }
            set { unsafe { if (AnimationComponent_SetBlendDuration_Ptr != null) AnimationComponent_SetBlendDuration_Ptr(Entity.ID, value); } }
        }

        public void Play() => IsPlaying = true;
        public void Pause() => IsPlaying = false;

        public void CrossFade(int targetIndex, float duration = 0.25f)
        {
            unsafe
            {
                if (AnimationComponent_CrossFade_Ptr != null)
                    AnimationComponent_CrossFade_Ptr(Entity.ID, targetIndex, duration);
            }
        }

        public unsafe void SetFloat(string name, float value)
        {
            if (AnimationComponent_SetFloat_Ptr != null)
                fixed (char* ptr = name)
                    AnimationComponent_SetFloat_Ptr(Entity.ID, ptr, value);
        }

        public unsafe void SetBool(string name, bool value)
        {
            if (AnimationComponent_SetBool_Ptr != null)
                fixed (char* ptr = name)
                    AnimationComponent_SetBool_Ptr(Entity.ID, ptr, value);
        }

        public unsafe float GetFloat(string name)
        {
            if (AnimationComponent_GetFloat_Ptr != null)
                fixed (char* ptr = name)
                    return AnimationComponent_GetFloat_Ptr(Entity.ID, ptr);
            return 0.0f;
        }
    }

    /// <summary>Deprecated: use AnimationComponent instead. Kept for backward compatibility.</summary>
    public class AnimationGraphComponent : AnimationComponent
    {
    }
}
