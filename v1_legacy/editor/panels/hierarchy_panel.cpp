#include "hierarchy_panel.h"
#include "editor/fa5_compat.h"
#include "engine/scene/core/scene.h"
#include "engine/scene/ecs/components/core/tag_component.h"
#include <imgui.h>

#include "editor/logic/editor_entity_factory.h"
#include "editor/logic/selection_manager.h"
#include "editor/logic/undo/command_history.h"

namespace CHEngine
{

HierarchyPanel::HierarchyPanel() : EditorPanel("Scene Hierarchy")
{
}

void HierarchyPanel::OnImGuiRender()
{
    if (!m_IsVisible)
        return;

    ImGui::Begin("Scene Hierarchy");

    auto context = SceneManager::Get().GetActiveScene();
    if (context)
    {
        auto &registry = context->GetRegistry();
        auto view = registry.view<TagComponent>();

        for (auto entity : view)
        {
            auto &tag = view.get<TagComponent>(entity);
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow |
                                       ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_Leaf;

            if (SelectionManager::Get().GetSelectedEntity() == entity)
                flags |= ImGuiTreeNodeFlags_Selected;

            ImGui::PushID(static_cast<int>(entity));
            bool opened = ImGui::TreeNodeEx("##Entity", flags, "%s", tag.Tag.c_str());

            if (ImGui::IsItemClicked())
                SelectionManager::Get().SetSelection(entity);

            if (ImGui::BeginPopupContextItem())
            {
                if (ImGui::MenuItem("Delete Entity"))
                {
                    EditorEntityFactory factory;
                    factory.DeleteEntity(entity);
                }
                ImGui::EndPopup();
            }

            if (opened)
                ImGui::TreePop();
            ImGui::PopID();
        }
    }

    if (ImGui::BeginPopupContextWindow("HierarchyEmptySpace", ImGuiPopupFlags_MouseButtonRight))
    {
        if (ImGui::MenuItem("Create Empty Entity"))
        {
            EditorEntityFactory factory;
            factory.CreateEntity("Empty Entity");
        }
        ImGui::EndPopup();
    }
    ImGui::End();
}

} // namespace CHEngine
