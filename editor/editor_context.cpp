#include "editor_context.h"

namespace CHEngine
{
EditorState EditorContext::s_State;
SceneState EditorContext::s_SceneState = SceneState::Edit;

void EditorContext::Init()
{
    s_State.DebugRenderFlags.DrawColliders = true;
    s_State.DebugRenderFlags.DrawLights = true;
    s_State.DebugRenderFlags.DrawSpawnZones = true;
}

void EditorContext::Shutdown()
{
    s_State.SelectedEntity = {};
}
} // namespace CHEngine
