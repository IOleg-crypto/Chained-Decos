#ifndef CH_PROJECT_BROWSER_PANEL_H
#define CH_PROJECT_BROWSER_PANEL_H

#include <functional>
#include "panel.h"
#include <string>
#include <vector>

namespace CHEngine
{
class ProjectBrowserPanel : public Panel
{
public:
    ProjectBrowserPanel();
    ~ProjectBrowserPanel();

    virtual void OnImGuiRender(bool readOnly = false) override;

    using EventCallbackFn = std::function<void(Event &)>;
    void SetEventCallback(const EventCallbackFn &callback)
    {
        m_EventCallback = callback;
    }

    // Testing accessors
    bool IsCreateDialogVisible() const { return m_ShowCreateDialog; }
    void SetCreateDialogVisible(bool visible) { m_ShowCreateDialog = visible; }
    bool HasPendingCreatePopupRequest() const { return m_OpenCreatePopupRequest; }

private:
    void DrawWelcomeScreen();
    void DrawCreateProjectDialog();

    EventCallbackFn m_EventCallback;

    // Simplified state
    bool m_ShowCreateDialog = false;
    bool m_OpenCreatePopupRequest = false;
    char m_ProjectNameBuffer[256] = "MyProject";
    char m_ProjectLocationBuffer[512] = "";

    // Icons
    Texture2D m_NewProjectIcon;
    Texture2D m_OpenProjectIcon;
};
} // namespace CHEngine

#endif // CH_PROJECT_BROWSER_PANEL_H
