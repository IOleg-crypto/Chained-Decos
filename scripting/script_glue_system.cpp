#include "script_glue_internal.h"
#include "script_internal_call_registry.h"

namespace CHEngine
{

// ── Logging ──────────────────────────────────────────────────────────
CH_SCRIPT_FUNC void Log_Info(Coral::String message)
{
    CH_CORE_INFO("[C#] {}", (std::string)message);
}
CH_ADD_INTERNAL_CALL(Log, Log_Info_Ptr, Log_Info);

CH_SCRIPT_FUNC void Log_Warn(Coral::String message)
{
    CH_CORE_WARN("[C#] {}", (std::string)message);
}
CH_ADD_INTERNAL_CALL(Log, Log_Warn_Ptr, Log_Warn);

CH_SCRIPT_FUNC void Log_Error(Coral::String message)
{
    CH_CORE_ERROR("[C#] {}", (std::string)message);
}
CH_ADD_INTERNAL_CALL(Log, Log_Error_Ptr, Log_Error);

// ── Application / Window ─────────────────────────────────────────────
CH_SCRIPT_FUNC void Application_Close()
{
    Application::Get().Close();
}
CH_ADD_INTERNAL_CALL(Application, Application_Close_Ptr, Application_Close);

CH_SCRIPT_FUNC int Application_GetFPS()
{
    return (int)(1.0f / Application::Get().GetFrameTime());
}
CH_ADD_INTERNAL_CALL(Application, Application_GetFPS_Ptr, Application_GetFPS);

CH_SCRIPT_FUNC float Application_GetFrameTime()
{
    return Application::Get().GetFrameTime();
}
CH_ADD_INTERNAL_CALL(Application, Application_GetFrameTime_Ptr, Application_GetFrameTime);

CH_SCRIPT_FUNC void Window_SetSize(int w, int h)
{
    Application::Get().GetWindow().SetSize(w, h);
}
CH_ADD_INTERNAL_CALL(AppWindow, Window_SetSize_Ptr, Window_SetSize);

CH_SCRIPT_FUNC void Window_SetFullscreen(bool enabled)
{
    Application::Get().GetWindow().SetFullscreen(enabled);
}
CH_ADD_INTERNAL_CALL(AppWindow, Window_SetFullscreen_Ptr, Window_SetFullscreen);

CH_SCRIPT_FUNC void Window_SetVSync(bool enabled)
{
    Application::Get().GetWindow().SetVSync(enabled);
}
CH_ADD_INTERNAL_CALL(AppWindow, Window_SetVSync_Ptr, Window_SetVSync);

CH_SCRIPT_FUNC void Window_SetAntialiasing(bool enabled)
{
    Application::Get().GetWindow().SetAntialiasing(enabled);
}
CH_ADD_INTERNAL_CALL(AppWindow, Window_SetAntialiasing_Ptr, Window_SetAntialiasing);

ManagedScriptInstance* s_PendingScriptInstance = nullptr;

void ScriptGlue::SetPendingScriptInstance(ManagedScriptInstance* instance)
{
    s_PendingScriptInstance = instance;
}

CH_SCRIPT_FUNC void RegisterLifecyclePointers(uint64_t entityID, void* onCreate, void* onStart, void* onUpdate,
                                              void* onDestroy, void* onGUI, void* onCollisionEnter, void* onEvent)
{
    if (s_PendingScriptInstance)
    {
        s_PendingScriptInstance->OnCreate = (void (*)())onCreate;
        s_PendingScriptInstance->OnStart = (void (*)())onStart;
        s_PendingScriptInstance->OnUpdate = (void (*)(float))onUpdate;
        s_PendingScriptInstance->OnDestroy = (void (*)())onDestroy;
        s_PendingScriptInstance->OnGUI = (void (*)())onGUI;
        s_PendingScriptInstance->OnCollisionEnter = (void (*)(uint64_t))onCollisionEnter;
        s_PendingScriptInstance->OnEvent = (void (*)(int))onEvent;
    }
}
CH_ADD_INTERNAL_CALL(Script, RegisterLifecyclePointers, RegisterLifecyclePointers);

} // namespace CHEngine

