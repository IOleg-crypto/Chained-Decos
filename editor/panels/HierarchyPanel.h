//
// HierarchyPanel.h - Scene hierarchy tree panel
//

#ifndef HIERARCHYPANEL_H
#define HIERARCHYPANEL_H

#include "IEditorPanel.h"

class IEditor;

// Displays a tree view of all scene objects
class HierarchyPanel : public IEditorPanel
{
public:
    explicit HierarchyPanel(IEditor *editor);
    ~HierarchyPanel() override = default;

    // IEditorPanel interface
    void Render() override;
    const char *GetName() const override
    {
        return "Hierarchy";
    }
    const char *GetDisplayName() const override
    {
        return "Hierarchy";
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
    IEditor *m_editor;
    bool m_visible = true;
};

#endif // HIERARCHYPANEL_H
