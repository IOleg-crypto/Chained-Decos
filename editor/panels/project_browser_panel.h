#ifndef CH_PROJECT_BROWSER_PANEL_H
#define CH_PROJECT_BROWSER_PANEL_H

#include "engine/core/events.h"
#include <functional>
#include <string>
#include <vector>

namespace CHEngine
{
class ProjectBrowserPanel
{
public:
    ProjectBrowserPanel();
    ~ProjectBrowserPanel() = default;

    void OnImGuiRender();

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
