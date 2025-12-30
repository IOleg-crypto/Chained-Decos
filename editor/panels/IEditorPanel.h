//
// IEditorPanel.h - Base interface for all editor panels
//

#ifndef IEDITORPANEL_H
#define IEDITORPANEL_H

#include <string>

// Forward declarations
class IEditor;

// Base interface for all editor panels (Hierarchy, Inspector, Viewport, etc.)
class IEditorPanel
{
public:
    virtual ~IEditorPanel() = default;

    // Core panel lifecycle
    virtual void Render() = 0;
    virtual void Update(float deltaTime)
    {
    }

    // Panel metadata
    virtual const char *GetName() const = 0;
    virtual const char *GetDisplayName() const
    {
        return GetName();
    }

    // Visibility control
    virtual bool IsVisible() const = 0;
    virtual void SetVisible(bool visible) = 0;

    // Optional: Panel-specific shortcuts or focus handling
    virtual void OnFocus()
    {
    }
    virtual void OnLostFocus()
    {
    }
};

#endif // IEDITORPANEL_H
