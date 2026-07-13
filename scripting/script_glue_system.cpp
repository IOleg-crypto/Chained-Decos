#include "script_glue_system.h"
#include "engine/project/project.h"

namespace Chained {
void Log_Info(const Coral::UCChar* message)
{
    CH_CORE_INFO("[C#] {}", ch_u16_to_string(message));
}
void Log_Warn(const Coral::UCChar* message)
{
    CH_CORE_WARN("[C#] {}", ch_u16_to_string(message));
}
void Log_Error(const Coral::UCChar* message)
{
    CH_CORE_ERROR("[C#] {}", ch_u16_to_string(message));
}
void Application_Close()
{
    Application::Get().Close();
}
int Application_GetFPS()
{
    float dt = Application::Get().GetFrameTime();
    return dt > 0.0f ? (int)(1.0f / dt) : 0;
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

float Physics_GetGravity()
{
    if (auto project = Project::GetActive())
        return project->GetConfig().Physics.Gravity;
    return 20.0f;
}

} // namespace Chained
