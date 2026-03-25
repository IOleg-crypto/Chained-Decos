#include "material_panel.h"
#include "property_editor.h"
#include "imgui.h"
#include "engine/scene/scene_events.h"

namespace CHEngine
{

MaterialPanel::MaterialPanel()
{
    m_Name = "Material Editor";
}

void MaterialPanel::OnImGuiRender(bool readOnly)
{
    if (!m_IsOpen)
    {
        return;
    }

    ImGui::Begin(m_Name.c_str(), &m_IsOpen);

    if (m_SelectedEntity && m_SelectedEntity.GetRegistry().ctx().get<Scene*>() != m_Context.get())
    {
        m_SelectedEntity = {};
    }

    if (m_SelectedEntity && m_SelectedEntity.IsValid())
    {
        ImGui::BeginDisabled(readOnly);
        PropertyEditor::DrawMaterial(m_SelectedEntity, m_SelectedMeshIndex);
        ImGui::EndDisabled();
    }
    else
    {
        ImGui::Text("No entity selected.");
        ImGui::TextDisabled("Select an entity in the Hierarchy to edit its materials.");
    }

    ImGui::End();
}

void MaterialPanel::OnEvent(Event& e)
{
    EventDispatcher dispatcher(e);
    dispatcher.Dispatch<EntitySelectedEvent>([this](EntitySelectedEvent& ev) {
        m_SelectedEntity = Entity(ev.GetEntity(), &ev.GetScene()->GetRegistry());
        m_SelectedMeshIndex = ev.GetMeshIndex();
        return false;
    });
}

} // namespace CHEngine
