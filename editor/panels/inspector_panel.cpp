#include "inspector_panel.h"
#include "imgui.h"
#include "property_editor.h"
#include "engine/scene/scene_events.h"


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
        ImGui::PushID(this);

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
        ImGui::PopID();
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
        ImGui::PushID((uint32_t)entity);

        if (entity.HasComponent<IDComponent>())
        {
            uint64_t uuid = (uint64_t)entity.GetComponent<IDComponent>().ID;
            ImGui::Text("UUID: %llu", uuid);
        }

        PropertyEditor::DrawTag(entity);

        ImGui::SameLine();
        ImGui::PushItemWidth(-1);
        if (ImGui::Button("Add Component"))
            ImGui::OpenPopup("AddComponent");
        PropertyEditor::DrawAddComponentPopup(entity);
        ImGui::PopItemWidth();

        // Use our new dynamic registry to draw all other components
        PropertyEditor::DrawEntityProperties(entity);

        // Some special cases that might not be in the registry or need extra data
        PropertyEditor::DrawMaterial(entity, m_SelectedMeshIndex);

        ImGui::PopID();
    }
} // namespace CHEngine
