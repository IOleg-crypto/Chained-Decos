#ifndef TOOLBARPANEL_H
#define TOOLBARPANEL_H

#include "editor/EditorTypes.h"
#include <functional>

namespace CHEngine
{
/**
 * @brief Top toolbar for quick scene actions and tool selection
 */
class ToolbarPanel
{
public:
    ToolbarPanel() = default;
    ~ToolbarPanel() = default;

    // --- Panel Lifecycle ---
public:
    void OnImGuiRender(SceneState sceneState, RuntimeMode runtimeMode, Tool activeTool,
                       const std::function<void()> &onPlay, const std::function<void()> &onStop,
                       const std::function<void()> &onNew, const std::function<void()> &onSave,
                       const std::function<void(Tool)> &onToolChange,
                       const std::function<void(RuntimeMode)> &onRuntimeModeChange);
};
} // namespace CHEngine
#endif
