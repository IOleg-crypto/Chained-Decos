#include "hierarchy_panel.h"
#include "editor/fa5_compat.h"
#include "scene/core/scene.h"
#include "scene/ecs/components/core/tag_component.h"
#include <imgui.h>

#include "editor/logic/editor_entity_factory.h"
#include "editor/logic/selection_manager.h"
#include "editor/logic/undo/command_history.h"

namespace CHEngine
{

HierarchyPanel::HierarchyPanel(const std::shared_ptr<Scene> &scene, SelectionManager *selection,
                               EditorEntityFactory *factory, CommandHistory *history)
    : m_Context(scene), m_SelectionManager(selection), m_EntityFactory(factory),
      m_CommandHistory(history)
{
}

void HierarchyPanel::SetContext(const std::shared_ptr<Scene> &scene)
{
    m_Context = scene;
}

void HierarchyPanel::OnImGuiRender()
{
    if (!m_IsVisible)
        return;

    ImGui::Begin("Scene Hierarchy");

    if (m_Context)
    {
        auto &registry = m_Context->GetRegistry();
        auto view = registry.view<TagComponent>();

        for (auto entity : view)
        {
            auto &tag = view.get<TagComponent>(entity);

            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow |
                                       ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_Leaf;

            if (m_SelectionManager->GetSelectedEntity() == entity)
            {
                flags |= ImGuiTreeNodeFlags_Selected;
            }

            ImGui::PushID(static_cast<int>(entity));
            bool opened = ImGui::TreeNodeEx("##Entity", flags, "%s", tag.Tag.c_str());

            if (ImGui::IsItemClicked())
            {
                m_SelectionManager->SetSelection(entity);
            }

            // Right-click on entity
            if (ImGui::BeginPopupContextItem())
            {
                if (ImGui::MenuItem("Delete Entity"))
                {
                    m_EntityFactory->DeleteEntity(entity);
                    if (m_SelectionManager->GetSelectedEntity() == entity)
                        m_SelectionManager->ClearSelection();
                }
                ImGui::EndPopup();
            }

            if (opened)
                ImGui::TreePop();

            ImGui::PopID();
        }
    }

    // Right-click on empty space
    if (ImGui::BeginPopupContextWindow("HierarchyEmptySpace", ImGuiPopupFlags_MouseButtonRight))
    {
        if (ImGui::MenuItem("Create Empty Entity"))
        {
            m_EntityFactory->CreateEntity("Empty Entity");
        }
        ImGui::EndPopup();
    }

    ImGui::End();
}

} // namespace CHEngine
