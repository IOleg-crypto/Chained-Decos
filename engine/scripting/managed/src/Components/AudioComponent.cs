using System;

namespace Chained
{
    /// <summary>Audio wrapper.</summary>
    [NativeProperty("Volume", "float", null, "AudioComponent_SetVolume")]
    [NativeProperty("Loop", "bool", null, "AudioComponent_SetLoop")]
    [NativeProperty("IsPlaying", "bool", "AudioComponent_IsPlaying")]
    [NativeProperty("SoundPath", "string", "AudioComponent_GetSoundPath")]
    [NativeCall("Chained.AudioComponent", "AudioComponent_Play", "void", "ulong")]
    [NativeCall("Chained.AudioComponent", "AudioComponent_Stop", "void", "ulong")]
    public partial class AudioComponent : Component
    {
        public void Play()
        {
            unsafe { if (AudioComponent_Play_Ptr != null) AudioComponent_Play_Ptr(Entity.ID); }
        }

        public void Stop()
        {
            unsafe { if (AudioComponent_Stop_Ptr != null) AudioComponent_Stop_Ptr(Entity.ID); }
        }
    }
}
