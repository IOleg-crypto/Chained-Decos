#ifndef CH_EDITOR_CONTEXT_H
#define CH_EDITOR_CONTEXT_H

#include "engine/graphics/pipeline/renderer.h"
#include "engine/scene/scene.h"

namespace CHEngine
{
enum class SceneState : uint8_t
{
    Edit = 0, // Editor-only update mode.
    Play = 1  // Runtime simulation mode.
};

// Mutable editor session state shared across panels and the editor layer.
struct EditorState
{
    Entity SelectedEntity;
    bool FullscreenGame = false;
    bool StandaloneActive = false;
    bool NeedsLayoutReset = false;
    int LastHitMeshIndex = -1;
    DebugRenderFlags DebugRenderFlags;
};

// EditorContext stores global editor state such as the selected entity,
// scene mode, and debug flags so panels do not need direct EditorLayer access.
class EditorContext
{
public:
    static void Init();
    static void Shutdown();

    static Entity GetSelectedEntity()
    {
        return s_State.SelectedEntity;
    }
    static void SetSelectedEntity(Entity entity)
    {
        s_State.SelectedEntity = entity;
    }

    static SceneState GetSceneState()
    {
        return s_SceneState;
    }
    static void SetSceneState(SceneState state)
    {
        s_SceneState = state;
    }

    static DebugRenderFlags& GetDebugRenderFlags()
    {
        return s_State.DebugRenderFlags;
    }
    static EditorState& GetState()
    {
        return s_State;
    }

private:
    static EditorState s_State;
    static SceneState s_SceneState;
};
} // namespace CHEngine

#endif // CH_EDITOR_CONTEXT_H
