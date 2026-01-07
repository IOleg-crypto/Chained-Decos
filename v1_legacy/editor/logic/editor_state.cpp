#include "editor_state.h"

namespace CHEngine
{
EditorState *EditorState::s_Instance = nullptr;

EditorState &EditorState::Get()
{
    return *s_Instance;
}

void EditorState::Init()
{
    if (!s_Instance)
        s_Instance = new EditorState();
}

void EditorState::Shutdown()
{
    delete s_Instance;
    s_Instance = nullptr;
}
} // namespace CHEngine
