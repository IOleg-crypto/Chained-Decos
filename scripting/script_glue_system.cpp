#include "script_glue_internal.h"

namespace CHEngine {

    // ── Logging ──────────────────────────────────────────────────────────
    CH_SCRIPT_FUNC void Log_Info(Coral::String message) { CH_CORE_INFO("[C#] {}", (std::string)message); }
    CH_SCRIPT_FUNC void Log_Warn(Coral::String message) { CH_CORE_WARN("[C#] {}", (std::string)message); }
    CH_SCRIPT_FUNC void Log_Error(Coral::String message) { CH_CORE_ERROR("[C#] {}", (std::string)message); }

    // ── Application / Window ─────────────────────────────────────────────
    CH_SCRIPT_FUNC void Application_Close() { Application::Get().Close(); }
    CH_SCRIPT_FUNC int Application_GetFPS() { return (int)(1.0f / Application::Get().GetFrameTime()); }
    CH_SCRIPT_FUNC float Application_GetFrameTime() { return Application::Get().GetFrameTime(); }
    CH_SCRIPT_FUNC void Window_SetSize(int w, int h) { Application::Get().GetWindow().SetSize(w, h); }
    CH_SCRIPT_FUNC void Window_SetFullscreen(bool enabled) { Application::Get().GetWindow().SetFullscreen(enabled); }
    CH_SCRIPT_FUNC void Window_SetVSync(bool enabled) { Application::Get().GetWindow().SetVSync(enabled); }
    CH_SCRIPT_FUNC void Window_SetAntialiasing(bool enabled) { Application::Get().GetWindow().SetAntialiasing(enabled); }

    ManagedScriptInstance* s_PendingScriptInstance = nullptr;

    void ScriptGlue::SetPendingScriptInstance(ManagedScriptInstance* instance) {
        s_PendingScriptInstance = instance;
    }

    CH_SCRIPT_FUNC void RegisterLifecyclePointers(uint64_t entityID, void* onCreate, void* onStart, void* onUpdate, void* onDestroy, void* onGUI, void* onCollisionEnter) {
        if (s_PendingScriptInstance) {
            s_PendingScriptInstance->OnCreate = (void(*)())onCreate;
            s_PendingScriptInstance->OnStart = (void(*)())onStart;
            s_PendingScriptInstance->OnUpdate = (void(*)(float))onUpdate;
            s_PendingScriptInstance->OnDestroy = (void(*)())onDestroy;
            s_PendingScriptInstance->OnGUI = (void(*)())onGUI;
            s_PendingScriptInstance->OnCollisionEnter = (void(*)(uint64_t))onCollisionEnter;
        }
    }

    void RegisterSystemInternalCalls(Coral::ManagedAssembly& assembly) {
        #define CH_ADD_INTERNAL_CALL(className, fieldName, funcPtr) assembly.AddInternalCall("CHEngine." #className, #fieldName, (void*)funcPtr)
        
        CH_ADD_INTERNAL_CALL(Script, RegisterLifecyclePointers, RegisterLifecyclePointers);
        CH_ADD_INTERNAL_CALL(Log, Log_Info_Ptr, Log_Info);
        CH_ADD_INTERNAL_CALL(Log, Log_Warn_Ptr, Log_Warn);
        CH_ADD_INTERNAL_CALL(Log, Log_Error_Ptr, Log_Error);
        CH_ADD_INTERNAL_CALL(Application, Application_Close_Ptr, Application_Close);
        CH_ADD_INTERNAL_CALL(Application, Application_GetFPS_Ptr, Application_GetFPS);
        CH_ADD_INTERNAL_CALL(Application, Application_GetFrameTime_Ptr, Application_GetFrameTime);
        CH_ADD_INTERNAL_CALL(AppWindow, Window_SetSize_Ptr, Window_SetSize);
        CH_ADD_INTERNAL_CALL(AppWindow, Window_SetFullscreen_Ptr, Window_SetFullscreen);
        CH_ADD_INTERNAL_CALL(AppWindow, Window_SetVSync_Ptr, Window_SetVSync);
        CH_ADD_INTERNAL_CALL(AppWindow, Window_SetAntialiasing_Ptr, Window_SetAntialiasing);

        #undef CH_ADD_INTERNAL_CALL
    }

} // namespace CHEngine
