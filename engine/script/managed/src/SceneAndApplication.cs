using Coral.Managed.Interop;

namespace CHEngine
{

// ─────────────────────────────────────────────────────────────────────────────
//  Scene — static API for scene / entity queries
// ─────────────────────────────────────────────────────────────────────────────
public static class Scene
{
#pragma warning disable 0649
    internal static unsafe delegate*<NativeString, ulong> Scene_FindEntityByTag_Ptr;
    internal static unsafe delegate*<NativeString, void> Scene_LoadScene_Ptr;
    internal static unsafe delegate*<ulong> Scene_GetPrimaryCameraEntity_Ptr;
#pragma warning restore 0649

    private static unsafe ulong FindEntityByTag_Native(string tag) => Scene_FindEntityByTag_Ptr(tag);
    public static unsafe void LoadScene(string path) => Scene_LoadScene_Ptr(path);
    private static unsafe ulong GetPrimaryCameraEntity_Native() => Scene_GetPrimaryCameraEntity_Ptr();

    /// <summary>Find an entity by its Tag component. Returns null if not found.</summary>
    public static Entity? FindEntityByTag(string tag)
    {
        ulong id = FindEntityByTag_Native(tag);
        return id != 0 ? new Entity(id) : null;
    }

    /// <summary>Returns the primary camera entity in the scene.</summary>
    public static Entity? GetMainCamera()
    {
        ulong id = GetPrimaryCameraEntity_Native();
        if (id != 0) return new Entity(id);

        // Fallback: Find first entity with CameraComponent
        var ids = Entity.FindAllWithComponent<CameraComponent>();
        if (ids.Length > 0) return new Entity(ids[0]);

        return null;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Audio — static API for audio playback
// ─────────────────────────────────────────────────────────────────────────────
public static class Audio
{
#pragma warning disable 0649
    internal static unsafe delegate*<NativeString, float, float, bool, void> Audio_Play_Ptr;
    internal static unsafe delegate*<NativeString, void> Audio_Stop_Ptr;
#pragma warning restore 0649

    public static unsafe void Play(string path, float volume = 1.0f, float pitch = 1.0f, bool loop = false) => Audio_Play_Ptr(path, volume, pitch, loop);
    public static unsafe void Stop(string path) => Audio_Stop_Ptr(path);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Application — static API for application control
// ─────────────────────────────────────────────────────────────────────────────
public static class Application
{
#pragma warning disable 0649
    internal static unsafe delegate*<void> Application_Close_Ptr;
#pragma warning restore 0649

    public static unsafe void Close() => Application_Close_Ptr();
}

// ─────────────────────────────────────────────────────────────────────────────
//  AppWindow — static API for window control
// ─────────────────────────────────────────────────────────────────────────────
public static class AppWindow
{
#pragma warning disable 0649
    internal static unsafe delegate*<int, int, void> Window_SetSize_Ptr;
    internal static unsafe delegate*<bool, void> Window_SetFullscreen_Ptr;
    internal static unsafe delegate*<bool, void> Window_SetVSync_Ptr;
    internal static unsafe delegate*<bool, void> Window_SetAntialiasing_Ptr;
#pragma warning restore 0649

    public static unsafe void SetSize(int width, int height) => Window_SetSize_Ptr(width, height);
    public static unsafe void SetFullscreen(bool enabled) => Window_SetFullscreen_Ptr(enabled);
    public static unsafe void SetVSync(bool enabled) => Window_SetVSync_Ptr(enabled);
    public static unsafe void SetAntialiasing(bool enabled) => Window_SetAntialiasing_Ptr(enabled);
}

}
