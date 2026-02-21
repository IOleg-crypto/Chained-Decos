#include "editor_panels.h"
#include "panels/console_panel.h"
#include "panels/content_browser_panel.h"
#include "panels/environment_panel.h"
#include "panels/inspector_panel.h"
#include "panels/panel.h"
#include "panels/profiler_panel.h"
#include "panels/project_browser_panel.h"
#include "panels/project_settings_panel.h"
#include "panels/scene_hierarchy_panel.h"
#include "panels/viewport_panel.h"

namespace CHEngine
{

void EditorPanels::Init()
{
    Register<ViewportPanel>();
    Register<SceneHierarchyPanel>();
    Register<InspectorPanel>();
    Register<ContentBrowserPanel>();
    Register<ConsolePanel>();
    Register<EnvironmentPanel>();
    Register<ProfilerPanel>();
    Register<ProjectBrowserPanel>();
    Register<ProjectSettingsPanel>();
}

void EditorPanels::OnUpdate(Timestep ts)
{
    for (auto& panel : m_Panels)
    {
        panel->OnUpdate(ts);
    }
}

void EditorPanels::OnImGuiRender(bool readOnly)
{
    for (auto& panel : m_Panels)
    {
        if (panel->GetName() == "Project Browser")
        {
            continue;
        }

        panel->OnImGuiRender(readOnly);
    }
}

void EditorPanels::OnEvent(Event& e)
{
    for (auto& panel : m_Panels)
    {
        panel->OnEvent(e);
    }
}

void EditorPanels::SetContext(const std::shared_ptr<Scene>& context)
{
    for (auto& panel : m_Panels)
    {
        panel->SetContext(context);
    }
}

} // namespace CHEngine
