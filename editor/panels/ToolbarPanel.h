#pragma once

#include "editor/EditorTypes.h"
#include <functional>

namespace CHEngine
{
class ToolbarPanel
{
public:
    ToolbarPanel() = default;

    void OnImGuiRender(SceneState sceneState, RuntimeMode runtimeMode, Tool activeTool,
                       const std::function<void()> &onPlay, const std::function<void()> &onStop,
                       const std::function<void()> &onNew, const std::function<void()> &onSave,
                       const std::function<void(Tool)> &onToolChange,
                       const std::function<void(RuntimeMode)> &onRuntimeModeChange);

private:
    void RenderToolButton(const char *label, Tool tool, const char *tooltip);
};
} // namespace CHEngine
