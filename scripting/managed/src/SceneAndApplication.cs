using Coral.Managed.Interop;

namespace CHEngine
{

// ─────────────────────────────────────────────────────────────────────────────
//  Scene — static API for scene / entity queries
// ─────────────────────────────────────────────────────────────────────────────
/// <summary>Scene helpers.</summary>
public static class Scene
{
#pragma warning disable 0649
    internal static unsafe delegate*<NativeString, ulong> Scene_FindEntityByTag_Ptr;
    internal static unsafe delegate*<NativeString, void> Scene_LoadScene_Ptr;
    internal static unsafe delegate*<ulong> Scene_GetPrimaryCameraEntity_Ptr;
#pragma warning restore 0649

    private static unsafe ulong FindEntityByTag_Native(string tag) => Scene_FindEntityByTag_Ptr(tag);
    /// <summary>Loads a scene.</summary>
    public static unsafe void LoadScene(string path) => Scene_LoadScene_Ptr(path);
    private static unsafe ulong GetPrimaryCameraEntity_Native() => Scene_GetPrimaryCameraEntity_Ptr();

    /// <summary>Finds an entity by tag.</summary>
    public static Entity? FindEntityByTag(string tag)
    {
        ulong entityId = FindEntityByTag_Native(tag);
        return entityId != 0 ? new Entity(entityId) : null;
    }

    /// <summary>Returns the main camera entity.</summary>
    public static Entity? GetMainCamera()
    {
        ulong entityId = GetPrimaryCameraEntity_Native();
        if (entityId != 0) return new Entity(entityId);

        // Fallback: first camera in the scene.
        var entityIds = Entity.FindAllWithComponent<CameraComponent>();
        if (entityIds.Length > 0) return new Entity(entityIds[0]);

        return null;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Audio — static API for audio playback
// ─────────────────────────────────────────────────────────────────────────────
/// <summary>Audio helpers.</summary>
public static class Audio
{
#pragma warning disable 0649
    internal static unsafe delegate*<NativeString, float, float, bool, void> Audio_Play_Ptr;
    internal static unsafe delegate*<NativeString, void> Audio_Stop_Ptr;
    internal static unsafe delegate*<void> Audio_StopAll_Ptr;
#pragma warning restore 0649

    /// <summary>Plays an audio clip.</summary>
    public static unsafe void Play(string path, float volume = 1.0f, float pitch = 1.0f, bool loop = false) => Audio_Play_Ptr(path, volume, pitch, loop);
    /// <summary>Stops playback for one clip.</summary>
    public static unsafe void Stop(string path) => Audio_Stop_Ptr(path);
    /// <summary>Stops all audio.</summary>
    public static unsafe void StopAll() => Audio_StopAll_Ptr();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Application — static API for application control
// ─────────────────────────────────────────────────────────────────────────────
/// <summary>Application helpers.</summary>
public static class Application
{
#pragma warning disable 0649
    internal static unsafe delegate*<void> Application_Close_Ptr;
    internal static unsafe delegate*<int> Application_GetFPS_Ptr;
    internal static unsafe delegate*<float> Application_GetFrameTime_Ptr;
#pragma warning restore 0649

    /// <summary>Requests shutdown.</summary>
    public static unsafe void Close() => Application_Close_Ptr();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Time — static API for timing information
// ─────────────────────────────────────────────────────────────────────────────
/// <summary>Frame timing.</summary>
public static class Time
{
    /// <summary>Current frame rate.</summary>
    public static unsafe int FPS => Application.Application_GetFPS_Ptr();
    /// <summary>Current frame time.</summary>
    public static unsafe float DeltaTime => Application.Application_GetFrameTime_Ptr();
}

// ─────────────────────────────────────────────────────────────────────────────
//  AppWindow — static API for window control
// ─────────────────────────────────────────────────────────────────────────────
/// <summary>Window helpers.</summary>
public static class AppWindow
{
#pragma warning disable 0649
    internal static unsafe delegate*<int, int, void> Window_SetSize_Ptr;
    internal static unsafe delegate*<bool, void> Window_SetFullscreen_Ptr;
    internal static unsafe delegate*<bool, void> Window_SetVSync_Ptr;
    internal static unsafe delegate*<bool, void> Window_SetAntialiasing_Ptr;
#pragma warning restore 0649

    /// <summary>Sets the window size.</summary>
    public static unsafe void SetSize(int width, int height) => Window_SetSize_Ptr(width, height);
    /// <summary>Toggles fullscreen mode.</summary>
    public static unsafe void SetFullscreen(bool enabled) => Window_SetFullscreen_Ptr(enabled);
    /// <summary>Toggles vertical sync.</summary>
    public static unsafe void SetVSync(bool enabled) => Window_SetVSync_Ptr(enabled);
    /// <summary>Toggles multisampling.</summary>
    public static unsafe void SetAntialiasing(bool enabled) => Window_SetAntialiasing_Ptr(enabled);
}

}
