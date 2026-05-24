#include "editor_panels.h"
#include "panels/console_panel.h"
#include "panels/content_browser_panel.h"
#include "panels/world_panel.h"
#include "panels/effects_panel.h"
#include "panels/material_panel.h"
#include "panels/inspector_panel.h"
#include "panels/panel.h"
#include "panels/profiler_panel.h"
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
    Register<WorldPanel>();
    Register<EffectsPanel>();
    Register<MaterialPanel>();
    Register<ProfilerPanel>();
    Register<ProjectSettingsPanel>();
    
}

void EditorPanels::OnUpdate(Timestep ts)
{
    // Update all visible panels
    for (auto& panel : m_RootPanels)
    {
        if (panel->IsVisible() && !panel->IsPendingKill())
        {
            panel->OnUpdate(ts);
        }
    }

    // Cleanup pass for pending kills
    m_RootPanels.erase(std::remove_if(m_RootPanels.begin(), m_RootPanels.end(),
        [this](const std::shared_ptr<Panel>& panel) {
            if (panel->IsPendingKill())
            {
                // Remove from registries
                m_PanelNameRegistry.erase(panel->GetName());
                // Note: type_index registry removing is more complex since we'd have to map back type_index from instance
                // For deferred removal, usually removing from roots is enough, but properly we would erase from m_PanelRegistry as well.
                // In this refactoring, assuming we just hide or remove standard panels.
                return true;
            }
            return false;
        }),
        m_RootPanels.end());
}

void EditorPanels::OnImGuiRender(bool readOnly)
{
    for (auto& panel : m_RootPanels)
    {
        if (!panel->IsVisible() || panel->IsPendingKill())
        {
            continue;
        }

        panel->OnImGuiRender(readOnly);
    }
}

void EditorPanels::OnEvent(Event& e)
{
    for (auto& panel : m_RootPanels)
    {
        if (panel->IsVisible() && !panel->IsPendingKill())
        {
            panel->OnEvent(e);
        }
    }
}

void EditorPanels::SetContext(const std::shared_ptr<Scene>& context)
{
    for (auto& panel : m_RootPanels)
    {
        panel->SetContext(context);
    }
}

} // namespace CHEngine
