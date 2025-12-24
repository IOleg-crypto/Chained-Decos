#pragma once

#include "editor/EditorTypes.h"
#include <functional>

namespace CHEngine
{
class ToolbarPanel
{
public:
    ToolbarPanel() = default;

    void OnImGuiRender(SceneState sceneState, Tool activeTool, std::function<void()> onPlay,
                       std::function<void()> onStop, std::function<void()> onNew,
                       std::function<void()> onSave, std::function<void(Tool)> onToolChange);

private:
    void RenderToolButton(const char *label, Tool tool, const char *tooltip);
};
} // namespace CHEngine
