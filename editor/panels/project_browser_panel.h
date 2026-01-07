#ifndef CH_PROJECT_BROWSER_PANEL_H
#define CH_PROJECT_BROWSER_PANEL_H

#include <functional>
#include <string>
#include <vector>


namespace CH
{
class ProjectBrowserPanel
{
public:
    ProjectBrowserPanel();
    ~ProjectBrowserPanel() = default;

    void OnImGuiRender();

    void SetOnProjectOpen(std::function<void(const std::string &)> callback)
    {
        m_OnProjectOpen = callback;
    }
    void SetOnProjectCreate(std::function<void(const std::string &, const std::string &)> callback)
    {
        m_OnProjectCreate = callback;
    }

private:
    void DrawWelcomeScreen();
    void DrawCreateProjectDialog();

    std::function<void(const std::string &)> m_OnProjectOpen;
    std::function<void(const std::string &, const std::string &)> m_OnProjectCreate;

    bool m_ShowCreateDialog = false;
    char m_ProjectNameBuffer[256] = "MyProject";
    char m_ProjectLocationBuffer[512] = "";
};
} // namespace CH

#endif // CH_PROJECT_BROWSER_PANEL_H
