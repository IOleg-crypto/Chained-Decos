#ifndef CH_PROJECT_BROWSER_PANEL_H
#define CH_PROJECT_BROWSER_PANEL_H

#include <functional>
#include "panel.h"
#include <string>
#include <vector>

// Helper: Draw premium action card
enum class CardIconType
{
    NewProject,
    OpenProject
};

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

    bool m_ShowCreateDialog = false;
    bool m_OpenCreatePopupRequest = false;
    char m_ProjectNameBuffer[256] = "MyProject";
    char m_ProjectLocationBuffer[512] = "";

    void *m_NewProjectIcon =
        nullptr; // Using void* or Texture2D depends on headers, but we have raylib.h in cpp
    void *m_OpenProjectIcon = nullptr;
    bool m_IconsLoaded = false;
};
} // namespace CHEngine

#endif // CH_PROJECT_BROWSER_PANEL_H
