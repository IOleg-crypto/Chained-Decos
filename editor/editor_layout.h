#ifndef CH_EDITOR_LAYOUT_H
#define CH_EDITOR_LAYOUT_H

#include "editor/editor_panels.h"
#include <string>

namespace CHEngine
{

class EditorLayout
{
public:
    EditorLayout(EditorPanels& panels);

    void ResetLayout();

    void LoadPreset(const std::string& filepath);
    void SaveCurrent(const std::string& filepath);
    void SaveDefaultLayout();

    void OnImGuiRender();

private:
    EditorPanels& m_Panels;
    uint32_t m_DockSpaceID = 0;
};

} // namespace CHEngine

#endif // CH_EDITOR_LAYOUT_H

