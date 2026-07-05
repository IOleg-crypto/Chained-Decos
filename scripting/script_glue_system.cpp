#include "script_glue_system.h" 

namespace Chained {
void Log_Info(const char16_t* message)
{
    CH_CORE_INFO("[C#] {}", ch_log_u16(message));
}
void Log_Warn(const char16_t* message)
{
    CH_CORE_WARN("[C#] {}", ch_log_u16(message));
}
void Log_Error(const char16_t* message)
{
    CH_CORE_ERROR("[C#] {}", ch_log_u16(message));
}
void Application_Close()
{
    Application::Get().Close();
}
int Application_GetFPS()
{
    return (int)(1.0f / Application::Get().GetFrameTime());
}
float Application_GetFrameTime()
{
    return Application::Get().GetFrameTime();
}
void Window_SetSize(int w, int h)
{
    Application::Get().GetWindow().SetSize(w, h);
}
void Window_SetFullscreen(bool enabled)
{
    Application::Get().GetWindow().SetFullscreen(enabled);
}
void Window_SetVSync(bool enabled)
{
    Application::Get().GetWindow().SetVSync(enabled);
}
void Window_SetAntialiasing(bool enabled)
{
    Application::Get().GetWindow().SetAntialiasing(enabled);
}

void RegisterGlueSystem()
{
    CH_ADD_INTERNAL_CALL("System", Log_Info, Log_Info);
    CH_ADD_INTERNAL_CALL("System", Log_Warn, Log_Warn);
    CH_ADD_INTERNAL_CALL("System", Log_Error, Log_Error);
    CH_ADD_INTERNAL_CALL("System", Application_Close, Application_Close);
    CH_ADD_INTERNAL_CALL("System", Application_GetFPS, Application_GetFPS);
    CH_ADD_INTERNAL_CALL("System", Application_GetFrameTime, Application_GetFrameTime);
    CH_ADD_INTERNAL_CALL("System", Window_SetSize, Window_SetSize);
    CH_ADD_INTERNAL_CALL("System", Window_SetFullscreen, Window_SetFullscreen);
    CH_ADD_INTERNAL_CALL("System", Window_SetVSync, Window_SetVSync);
    CH_ADD_INTERNAL_CALL("System", Window_SetAntialiasing, Window_SetAntialiasing);
}
}
