//
// EditorPanelManager.h - Manages all editor panels
//

#ifndef EDITORPANELMANAGER_H
#define EDITORPANELMANAGER_H

#include "IEditorPanel.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class IEditor;

// Manages registration, rendering, and lifecycle of all editor panels
class EditorPanelManager
{
public:
    explicit EditorPanelManager(IEditor *editor);
    ~EditorPanelManager() = default;

    // Panel registration
    template <typename T, typename... Args> T *AddPanel(Args &&...args)
    {
        auto panel = std::make_unique<T>(std::forward<Args>(args)...);
        T *ptr = panel.get();
        m_panels[panel->GetName()] = std::move(panel);
        m_panelOrder.push_back(ptr->GetName());
        return ptr;
    }

    // Access panels
    IEditorPanel *GetPanel(const std::string &name);

    template <typename T> T *GetPanel(const std::string &name)
    {
        return dynamic_cast<T *>(GetPanel(name));
    }

    // Lifecycle
    void Update(float deltaTime);
    void Render();

    // Visibility helpers
    void SetPanelVisible(const std::string &name, bool visible);
    bool IsPanelVisible(const std::string &name) const;
    void TogglePanelVisibility(const std::string &name);
    void SetAllPanelsVisible(bool visible);
    bool IsAnyPanelVisible() const;

    // Layout
    void ResetLayout()
    {
        m_needsLayoutReset = true;
    }
    void SetupDefaultLayout(unsigned int dockspaceId);

    // Menu rendering helper (for View menu)
    void RenderViewMenu();

private:
    IEditor *m_editor;
    std::unordered_map<std::string, std::unique_ptr<IEditorPanel>> m_panels;
    std::vector<std::string> m_panelOrder; // Maintains insertion order
    bool m_needsLayoutReset = true;        // Setup layout on first run
};

#endif // EDITORPANELMANAGER_H
