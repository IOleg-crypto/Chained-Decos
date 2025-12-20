//
// ToolbarPanel.h - Main editor toolbar
//

#ifndef TOOLBARPANEL_H
#define TOOLBARPANEL_H

#include "IEditorPanel.h"
#include "editor/EditorTypes.h"

class IEditor;

// Main editor toolbar with tool selection and common actions
class ToolbarPanel : public IEditorPanel
{
public:
    explicit ToolbarPanel(IEditor *editor);
    ~ToolbarPanel() override = default;

    // IEditorPanel interface
    void Render() override;
    const char *GetName() const override
    {
        return "Toolbar";
    }
    const char *GetDisplayName() const override
    {
        return "Toolbar";
    }
    bool IsVisible() const override
    {
        return m_visible;
    }
    void SetVisible(bool visible) override
    {
        m_visible = visible;
    }

private:
    void RenderToolButton(const char *label, Tool tool, const char *tooltip);
    void RenderSeparator();

    IEditor *m_editor;
    bool m_visible = true;
};

#endif // TOOLBARPANEL_H
