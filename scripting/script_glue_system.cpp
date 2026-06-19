#include "script_glue_internal.h"
#include "script_internal_call_registry.h"
#include "engine/app/application.h"
#include "engine/scene/scene.h"
#include "scripting/scriptengine.h"

namespace Chained
{



// ── Logging ──────────────────────────────────────────────────────────
CH_SCRIPT_FUNC void Log_Info(Coral::String message)
{
    CH_CORE_INFO("[C#] {}", (std::string)message);
}


CH_SCRIPT_FUNC void Log_Warn(Coral::String message)
{
    CH_CORE_WARN("[C#] {}", (std::string)message);
}


CH_SCRIPT_FUNC void Log_Error(Coral::String message)
{
    CH_CORE_ERROR("[C#] {}", (std::string)message);
}


// ── Application / Window ─────────────────────────────────────────────
CH_SCRIPT_FUNC void Application_Close()
{
    Application::Get().Close();
}


CH_SCRIPT_FUNC int Application_GetFPS()
{
    return (int)(1.0f / Application::Get().GetFrameTime());
}


CH_SCRIPT_FUNC float Application_GetFrameTime()
{
    return Application::Get().GetFrameTime();
}


CH_SCRIPT_FUNC void Window_SetSize(int w, int h)
{
    Application::Get().GetWindow().SetSize(w, h);
}


CH_SCRIPT_FUNC void Window_SetFullscreen(bool enabled)
{
    Application::Get().GetWindow().SetFullscreen(enabled);
}


CH_SCRIPT_FUNC void Window_SetVSync(bool enabled)
{
    Application::Get().GetWindow().SetVSync(enabled);
}


CH_SCRIPT_FUNC void Window_SetAntialiasing(bool enabled)
{
    Application::Get().GetWindow().SetAntialiasing(enabled);
}



    void RegisterGlueSystem(Coral::ManagedAssembly& assembly) {
            assembly.AddInternalCall("Chained.Log", "Log_Info_Ptr", (void*)Log_Info);
            assembly.AddInternalCall("Chained.Log", "Log_Warn_Ptr", (void*)Log_Warn);
            assembly.AddInternalCall("Chained.Log", "Log_Error_Ptr", (void*)Log_Error);
            assembly.AddInternalCall("Chained.Application", "Application_Close_Ptr", (void*)Application_Close);
            assembly.AddInternalCall("Chained.Application", "Application_GetFPS_Ptr", (void*)Application_GetFPS);
            assembly.AddInternalCall("Chained.Application", "Application_GetFrameTime_Ptr", (void*)Application_GetFrameTime);
            assembly.AddInternalCall("Chained.AppWindow", "Window_SetSize_Ptr", (void*)Window_SetSize);
            assembly.AddInternalCall("Chained.AppWindow", "Window_SetFullscreen_Ptr", (void*)Window_SetFullscreen);
            assembly.AddInternalCall("Chained.AppWindow", "Window_SetVSync_Ptr", (void*)Window_SetVSync);
            assembly.AddInternalCall("Chained.AppWindow", "Window_SetAntialiasing_Ptr", (void*)Window_SetAntialiasing);
        }
} // namespace Chained

