#ifndef CH_PROJECT_BROWSER_PANEL_H
#define CH_PROJECT_BROWSER_PANEL_H

#include "panel.h"
#include "engine/graphics/importers/texture_importer.h"
#include <filesystem>
#include <functional>
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

    using EventCallbackFn = std::function<void(Event&)>;
    void SetEventCallback(const EventCallbackFn& callback)
    {
        m_EventCallback = callback;
    }

    // Testing accessors
    bool IsCreateDialogVisible() const
    {
        return m_ShowCreateDialog;
    }
    void SetCreateDialogVisible(bool visible)
    {
        m_ShowCreateDialog = visible;
    }
    bool HasPendingCreatePopupRequest() const
    {
        return m_OpenCreatePopupRequest;
    }

private:
    void DrawWelcomeScreen();
    void DrawCreateProjectDialog();

    EventCallbackFn m_EventCallback;

    // Simplified state
    bool m_ShowCreateDialog = false;
    bool m_OpenCreatePopupRequest = false;
    char m_ProjectNameBuffer[256] = "MyProject";
    char m_ProjectLocationBuffer[512] = "";

    Texture m_NewProjectIcon;
    Texture m_OpenProjectIcon;

    std::shared_ptr<TextureAsset> m_NewProjectIconAsset;
    std::shared_ptr<TextureAsset> m_OpenProjectIconAsset;
};
} // namespace CHEngine

#endif // CH_PROJECT_BROWSER_PANEL_H
