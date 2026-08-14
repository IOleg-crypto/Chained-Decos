using System;
using System.Runtime.InteropServices;

namespace Chained
{
    /// <summary>Audio wrapper.</summary>
    public class AudioComponent : Component
    {
#pragma warning disable 0649
        internal static unsafe delegate* unmanaged<ulong, float, void> AudioComponent_SetVolume_Ptr;
        internal static unsafe delegate* unmanaged<ulong, byte, void> AudioComponent_SetLoop_Ptr;
        internal static unsafe delegate* unmanaged<ulong, byte> AudioComponent_IsPlaying_Ptr;
        internal static unsafe delegate* unmanaged<ulong, char*> AudioComponent_GetSoundPath_Ptr;
        internal static unsafe delegate* unmanaged<ulong, void> AudioComponent_Play_Ptr;
        internal static unsafe delegate* unmanaged<ulong, void> AudioComponent_Stop_Ptr;
#pragma warning restore 0649

        private static unsafe void SetVolume(ulong entityID, float volume) => AudioComponent_SetVolume_Ptr(entityID, volume);
        private static unsafe void SetLoop(ulong entityID, bool loop) => AudioComponent_SetLoop_Ptr(entityID, (byte)(loop ? 1 : 0));
        private static unsafe bool IsPlaying_Native(ulong entityID) => AudioComponent_IsPlaying_Ptr(entityID) != 0;
        private static unsafe string GetSoundPath(ulong entityID) => Marshal.PtrToStringUni(new IntPtr(AudioComponent_GetSoundPath_Ptr(entityID))) ?? string.Empty;

        public float Volume { set => SetVolume(Entity.ID, value); }
        public bool  Loop   { set => SetLoop(Entity.ID, value); }
        public bool  IsPlaying  => IsPlaying_Native(Entity.ID);
        public string SoundPath => GetSoundPath(Entity.ID) ?? string.Empty;

        public void Play()
        {
            unsafe { AudioComponent_Play_Ptr(Entity.ID); }
        }

        public void Stop()
        {
            unsafe { AudioComponent_Stop_Ptr(Entity.ID); }
        }
    }
}
