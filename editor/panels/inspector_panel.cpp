#include "inspector_panel.h"
#include "component_ui.h"
#include <imgui.h>

namespace CHEngine
{
void InspectorPanel::OnImGuiRender(Scene *scene, Entity entity, bool readOnly)
{
    ImGui::Begin("Inspector");
    if (entity && entity.IsValid())
    {
        ImGui::BeginDisabled(readOnly);
        DrawComponents(entity);
        ImGui::EndDisabled();
    }
    else
    {
        ImGui::Text("Selection: None");
        ImGui::TextDisabled("Select an entity to view its components.");
    }
    ImGui::End();
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

    ComponentUI::DrawTransform(entity);
    ComponentUI::DrawModel(entity);
    ComponentUI::DrawMaterial(entity, m_SelectedMeshIndex);
    ComponentUI::DrawCollider(entity);
    ComponentUI::DrawRigidBody(entity);
    ComponentUI::DrawSpawn(entity);
    ComponentUI::DrawPlayer(entity);
    ComponentUI::DrawPointLight(entity);
    ComponentUI::DrawAudio(entity);
    ComponentUI::DrawHierarchy(entity);
    ComponentUI::DrawNativeScript(entity);
    ComponentUI::DrawAnimation(entity);
}
} // namespace CHEngine
