#ifndef CH_EDITOR_TYPES_H
#define CH_EDITOR_TYPES_H

#include "engine/scene/scene.h"
#include "engine/graphics/pipeline/renderer.h"
#include <cstdint>
#include <string>

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
    bool IsLoading = false;
    std::string LoadingStatus = "";
};

} // namespace CHEngine

#endif // CH_EDITOR_TYPES_H
