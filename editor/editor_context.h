#ifndef CH_EDITOR_CONTEXT_H
#define CH_EDITOR_CONTEXT_H

#include "engine/scene/entity.h"
#include "engine/graphics/render.h"

namespace CHEngine
{
    enum class SceneState : uint8_t
    {
        Edit = 0,
        Play = 1
    };

    struct EditorState
    {
        Entity SelectedEntity;
        bool FullscreenGame = false;
        bool StandaloneActive = false;
        bool NeedsLayoutReset = false;
        int LastHitMeshIndex = -1;
        DebugRenderFlags DebugRenderFlags;
    };

    // EditorContext stores the global state of the editor, 
    // such as the selected entity, scene state, and debug flags.
    // This decouples the state from the EditorLayer.
    class EditorContext
    {
    public:
        static void Init();
        static void Shutdown();

        static Entity GetSelectedEntity() { return s_State.SelectedEntity; }
        static void SetSelectedEntity(Entity entity) { s_State.SelectedEntity = entity; }

        static SceneState GetSceneState() { return s_SceneState; }
        static void SetSceneState(SceneState state) { s_SceneState = state; }

        static DebugRenderFlags& GetDebugRenderFlags() { return s_State.DebugRenderFlags; }
        static EditorState& GetState() { return s_State; }

    private:
        static EditorState s_State;
        static SceneState s_SceneState;
    };
}

#endif // CH_EDITOR_CONTEXT_H
