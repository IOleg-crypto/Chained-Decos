#include "inspector_panel.h"
#include "editor_gui.h"
#include "engine/core/assets/asset_manager.h"
#include "engine/graphics/assets/model_asset.h"
#include "engine/scene/components.h"
#include "engine/scene/project.h"
#include "engine/scene/scene_events.h"
#include "engine/scene/scene_events.h"
#include "imgui/IconsFontAwesome6.h"

#include "imgui.h"
#include "property_editor.h"


namespace CHEngine
{
InspectorPanel::InspectorPanel()
{
    m_Name = "Inspector";
}

void InspectorPanel::OnImGuiRender(bool readOnly)
{
    if (!m_IsOpen)
    {
        return;
    }

    ImGui::Begin(m_Name.c_str(), &m_IsOpen);

    if (m_SelectedEntity && (!m_SelectedEntity.IsValid() || m_SelectedEntity.GetRegistry().ctx().get<Scene*>() != m_Context.get()))
    {
        m_SelectedEntity = {};
    }

    if (m_SelectedEntity && m_SelectedEntity.IsValid())
    {
        ImGui::BeginDisabled(readOnly);
        DrawComponents(m_SelectedEntity, readOnly);
        ImGui::EndDisabled();
    }
    else
    {
        ImGui::Text("Selection: None");
        ImGui::TextDisabled("Select an entity in the Hierarchy to view its components.");
    }
    ImGui::End();
}

void InspectorPanel::OnEvent(Event& e)
{
    EventDispatcher dispatcher(e);
    dispatcher.Dispatch<EntitySelectedEvent>([this](EntitySelectedEvent& ev) {
        m_SelectedEntity = Entity(ev.GetEntity(), &ev.GetScene()->GetRegistry());
        m_SelectedMeshIndex = ev.GetMeshIndex();
        return false;
    });
}

void InspectorPanel::SetContext(const std::shared_ptr<Scene>& context)
{
    Panel::SetContext(context);
    m_SelectedEntity = {};
}

void InspectorPanel::DrawComponents(Entity entity, bool readOnly)
{
    ImGui::PushID((uint32_t)entity);

    if (entity.HasComponent<IDComponent>())
    {
        uint64_t uuid = (uint64_t)entity.GetComponent<IDComponent>().ID;
        ImGui::TextDisabled("UUID: %llu", uuid);
    }

    PropertyEditor::DrawEntityHeader(entity);

    // Delegate all component drawing logic to PropertyEditor registry
    PropertyEditor::DrawEntityProperties(entity);

    ImGui::PopID();
}
} // namespace CHEngine
