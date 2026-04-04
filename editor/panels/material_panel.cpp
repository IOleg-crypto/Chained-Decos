#include "material_panel.h"
#include "engine/scene/components/mesh_component.h"
#include "engine/scene/scene_events.h"
#include "imgui.h"
#include "property_editor.h"
#include "ui_properties.h"

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

        if (m_SelectedEntity.HasComponent<MaterialComponent>())
        {
            auto& component = m_SelectedEntity.GetComponent<MaterialComponent>();
            UIProperties ui;
            Properties props(ui);
            component.Reflect(props);
        }
        else
        {
            ImGui::TextColored({0.8f, 0.8f, 0.2f, 1.0f}, ICON_FA_CIRCLE_INFO " No Materials Component");
            if (ImGui::Button(ICON_FA_PLUS " Add Materials Component"))
            {
                m_SelectedEntity.AddComponent<MaterialComponent>();
            }
        }

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
