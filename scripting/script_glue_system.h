#ifndef SCRIPT_GLUE_SYSTEM_H
#define SCRIPT_GLUE_SYSTEM_H
#include "script_glue_internal.h"
#include "engine/app/application.h"
#include "engine/scene/scene.h"
#include "scripting/scriptengine.h"

namespace Chained
{

// ── Logging ──────────────────────────────────────────────────────────
CH_SCRIPT_FUNC void Log_Info(const Coral::UCChar* message);

CH_SCRIPT_FUNC void Log_Warn(const Coral::UCChar* message);

CH_SCRIPT_FUNC void Log_Error(const Coral::UCChar* message);

// ── Application / Window ─────────────────────────────────────────────
CH_SCRIPT_FUNC void Application_Close();

CH_SCRIPT_FUNC int Application_GetFPS();

CH_SCRIPT_FUNC float Application_GetFrameTime();

CH_SCRIPT_FUNC void Window_SetSize(int w, int h);

CH_SCRIPT_FUNC void Window_SetFullscreen(bool enabled);

CH_SCRIPT_FUNC void Window_SetVSync(bool enabled);

CH_SCRIPT_FUNC void Window_SetAntialiasing(bool enabled);

CH_SCRIPT_FUNC void Window_SetAntiAliasingSamples(int samples);

CH_SCRIPT_FUNC const Coral::UCChar* Window_GetSupportedResolution();

// ── Physics ────────────────────────────────────────────────────────
CH_SCRIPT_FUNC float Physics_GetGravity();

} // namespace Chained
#endif
