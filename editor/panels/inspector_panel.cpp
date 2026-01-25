#include "inspector_panel.h"
#include "component_ui.h"
#include <imgui.h>

namespace CHEngine
{
InspectorPanel::InspectorPanel()
{
    m_Name = "Inspector";
}

void InspectorPanel::OnImGuiRender(bool readOnly)
{
    if (!m_IsOpen)
        return;

    ImGui::Begin(m_Name.c_str(), &m_IsOpen);

    if (m_SelectedEntity && m_SelectedEntity.GetScene() != m_Context.get())
        m_SelectedEntity = {};

    if (m_SelectedEntity && m_SelectedEntity.IsValid())
    {
        ImGui::BeginDisabled(readOnly);
        DrawComponents(m_SelectedEntity);
        ImGui::EndDisabled();
    }
    else
    {
        ImGui::Text("Selection: None");
        ImGui::TextDisabled("Select an entity in the Hierarchy to view its components.");
    }
    ImGui::End();
}

void InspectorPanel::OnEvent(Event &e)
{
    EventDispatcher dispatcher(e);
    dispatcher.Dispatch<EntitySelectedEvent>(
        [this](EntitySelectedEvent &ev)
        {
            m_SelectedEntity = Entity{ev.GetEntity(), ev.GetScene()};
            m_SelectedMeshIndex = ev.GetMeshIndex();
            return false;
        });
}

void InspectorPanel::DrawComponents(Entity entity)
{
    if (entity.HasComponent<IDComponent>())
    {
        uint64_t uuid = (uint64_t)entity.GetComponent<IDComponent>().ID;
        ImGui::Text("UUID: %llu", uuid);
    }

    ComponentUI::DrawTag(entity);

    ImGui::SameLine();
    ImGui::PushItemWidth(-1);
    if (ImGui::Button("Add Component"))
        ImGui::OpenPopup("AddComponent");
    ComponentUI::DrawAddComponentPopup(entity);
    ImGui::PopItemWidth();

    // Use our new dynamic registry to draw all other components
    ComponentUI::DrawEntityComponents(entity);

    // Some special cases that might not be in the registry or need extra data
    ComponentUI::DrawMaterial(entity, m_SelectedMeshIndex);
}
} // namespace CHEngine
