//
// InspectorPanel.h - Object properties inspector panel
//

#ifndef INSPECTORPANEL_H
#define INSPECTORPANEL_H

#include "IEditorPanel.h"
#include "scene/resources/map/core/MapData.h"

class IEditor;

// Displays and edits properties of the selected object
class InspectorPanel : public IEditorPanel
{
public:
    explicit InspectorPanel(IEditor *editor);
    ~InspectorPanel() override = default;

    // IEditorPanel interface
    void Render() override;
    const char *GetName() const override
    {
        return "Inspector";
    }
    const char *GetDisplayName() const override
    {
        return "Inspector";
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
    void RenderTransform(MapObjectData *obj);
    void RenderObjectProperties(MapObjectData *obj);

    IEditor *m_editor;
    bool m_visible = true;
};

#endif // INSPECTORPANEL_H
