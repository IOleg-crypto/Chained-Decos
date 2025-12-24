#pragma once

#include "editor/EditorTypes.h"

namespace CHEngine
{
class EditorLayer;

class ToolbarPanel
{
public:
    ToolbarPanel() = default;

    void OnImGuiRender(EditorLayer *layer);

private:
    void RenderToolButton(const char *label, Tool tool, const char *tooltip);
};
} // namespace CHEngine
