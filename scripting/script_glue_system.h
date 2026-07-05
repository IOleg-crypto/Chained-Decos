#ifndef SCRIPT_GLUE_SYSTEM_H
#define SCRIPT_GLUE_SYSTEM_H
#include "script_glue_internal.h"
#include "script_internal_call_registry.h"
#include "engine/app/application.h"
#include "engine/scene/scene.h"
#include "scripting/scriptengine.h"

namespace Chained
{

void RegisterGlueSystem();

// ── Logging ──────────────────────────────────────────────────────────
static std::string ch_log_u16(const char16_t* ptr)
{
    if (!ptr) return {};
    std::u16string u16(ptr);
    std::string r; r.reserve(u16.size());
    for (char16_t c : u16) 
    {
        r += (c < 0x80) ? (char)c : '?';
    }
    return r;
}

CH_SCRIPT_FUNC void Log_Info(const char16_t* message);

CH_SCRIPT_FUNC void Log_Warn(const char16_t* message);

CH_SCRIPT_FUNC void Log_Error(const char16_t* message);

// ── Application / Window ─────────────────────────────────────────────
CH_SCRIPT_FUNC void Application_Close();

CH_SCRIPT_FUNC int Application_GetFPS();

CH_SCRIPT_FUNC float Application_GetFrameTime();

CH_SCRIPT_FUNC void Window_SetSize(int w, int h);

CH_SCRIPT_FUNC void Window_SetFullscreen(bool enabled);

CH_SCRIPT_FUNC void Window_SetVSync(bool enabled);

CH_SCRIPT_FUNC void Window_SetAntialiasing(bool enabled);

} // namespace Chained
#endif

