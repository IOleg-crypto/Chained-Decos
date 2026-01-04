#include "scene_hierarchy_panel.h"
#include "editor_layer.h"
#include "engine/scene/components.h"
#include "logic/undo/entity_commands.h"
#include <imgui.h>

namespace CH
{
SceneHierarchyPanel::SceneHierarchyPanel(const Ref<Scene> &context)
{
    SetContext(context);
}

void SceneHierarchyPanel::SetContext(const Ref<Scene> &context)
{
    m_Context = context;
    m_SelectionContext = {};
}

void SceneHierarchyPanel::OnImGuiRender()
{
    ImGui::Begin("Scene Hierarchy");

    if (m_Context)
    {
        auto &registry = m_Context->GetRegistry();

        entt::entity entityToDelete = entt::null;
        registry.view<TagComponent>().each(
            [&](auto entityID, auto &tag)
            {
                Entity entity{entityID, m_Context.get()};
                bool isChild = false;
                if (entity.HasComponent<HierarchyComponent>())
                {
                    if (entity.GetComponent<HierarchyComponent>().Parent != entt::null)
                        isChild = true;
                }

                if (!isChild)
                {
                    if (DrawEntityNode(entity))
                        entityToDelete = entityID;
                }
            });

        if (entityToDelete != entt::null)
        {
            Entity entity{entityToDelete, m_Context.get()};
            EditorLayer::GetCommandHistory().PushCommand(
                std::make_unique<DestroyEntityCommand>(entity));

            if (m_SelectionContext == entity)
                m_SelectionContext = {};
        }

        if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered())
            m_SelectionContext = {};

        // Right-click on blank space
        if (ImGui::BeginPopupContextWindow(0, ImGuiPopupFlags_MouseButtonRight |
                                                  ImGuiPopupFlags_NoOpenOverItems))
        {
            if (ImGui::MenuItem("Create Empty Entity"))
                m_Context->CreateEntity("Empty Entity");

            if (ImGui::MenuItem("Spawn Zone"))
            {
                auto entity = m_Context->CreateEntity("Spawn Zone");
                entity.AddComponent<SpawnComponent>();
            }

            if (ImGui::BeginMenu("3D Object"))
            {
                if (ImGui::MenuItem("Cube"))
                    EditorLayer::GetCommandHistory().PushCommand(
                        std::make_unique<CreateEntityCommand>(m_Context.get(), "Cube"));

                if (ImGui::MenuItem("Sphere"))
                    EditorLayer::GetCommandHistory().PushCommand(
                        std::make_unique<CreateEntityCommand>(m_Context.get(), "Sphere"));

                if (ImGui::MenuItem("Cylinder"))
                    EditorLayer::GetCommandHistory().PushCommand(
                        std::make_unique<CreateEntityCommand>(m_Context.get(), "Cylinder"));

                if (ImGui::MenuItem("Cone"))
                    EditorLayer::GetCommandHistory().PushCommand(
                        std::make_unique<CreateEntityCommand>(m_Context.get(), "Cone"));

                if (ImGui::MenuItem("Torus"))
                    EditorLayer::GetCommandHistory().PushCommand(
                        std::make_unique<CreateEntityCommand>(m_Context.get(), "Torus"));

                if (ImGui::MenuItem("Knot"))
                    EditorLayer::GetCommandHistory().PushCommand(
                        std::make_unique<CreateEntityCommand>(m_Context.get(), "Knot"));

                if (ImGui::MenuItem("Plane"))
                    EditorLayer::GetCommandHistory().PushCommand(
                        std::make_unique<CreateEntityCommand>(m_Context.get(), "Plane"));

                ImGui::EndMenu();
            }

            ImGui::EndPopup();
        }
    }

    ImGui::End();
}

bool SceneHierarchyPanel::DrawEntityNode(Entity entity)
{
    auto &tag = entity.GetComponent<TagComponent>().Tag;

    ImGuiTreeNodeFlags flags = ((m_SelectionContext == entity) ? ImGuiTreeNodeFlags_Selected : 0) |
                               ImGuiTreeNodeFlags_OpenOnArrow;
    flags |= ImGuiTreeNodeFlags_SpanAvailWidth;
    bool opened = ImGui::TreeNodeEx((void *)(uint64_t)(uint32_t)entity, flags, tag.c_str());

    if (ImGui::IsItemClicked())
    {
        m_SelectionContext = entity;
    }

    bool entityDeleted = false;
    if (ImGui::BeginPopupContextItem())
    {
        if (ImGui::MenuItem("Delete Entity"))
            entityDeleted = true;

        ImGui::EndPopup();
    }

    if (opened)
    {
        if (entity.HasComponent<HierarchyComponent>())
        {
            auto &hc = entity.GetComponent<HierarchyComponent>();
            for (auto childID : hc.Children)
            {
                Entity child{childID, m_Context.get()};
                DrawEntityNode(child);
            }
        }
        ImGui::TreePop();
    }

    return entityDeleted;
}
} // namespace CH
