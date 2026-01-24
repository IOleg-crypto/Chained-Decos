#ifndef CH_PROJECT_BROWSER_PANEL_H
#define CH_PROJECT_BROWSER_PANEL_H

#include "panel.h"
#include <functional>
#include <string>
#include <vector>

namespace CHEngine
{
class ProjectBrowserPanel : public Panel
{
public:
    ProjectBrowserPanel();
    ~ProjectBrowserPanel() = default;

    virtual void OnImGuiRender(bool readOnly = false) override;

    using EventCallbackFn = std::function<void(Event &)>;
    void SetEventCallback(const EventCallbackFn &callback)
    {
        m_EventCallback = callback;
    }

private:
    void DrawWelcomeScreen();
    void DrawCreateProjectDialog();

    EventCallbackFn m_EventCallback;

    bool m_ShowCreateDialog = false;
    bool m_OpenCreatePopupRequest = false;
    char m_ProjectNameBuffer[256] = "MyProject";
    char m_ProjectLocationBuffer[512] = "";
};
} // namespace CHEngine

#endif // CH_PROJECT_BROWSER_PANEL_H
