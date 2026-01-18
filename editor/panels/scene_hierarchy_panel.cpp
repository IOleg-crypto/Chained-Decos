#include "scene_hierarchy_panel.h"
#include "editor_layer.h"
#include "engine/scene/components.h"
#include "undo/entity_commands.h"
#include <imgui.h>

namespace CHEngine
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

void SceneHierarchyPanel::OnImGuiRender(bool readOnly)
{
    ImGui::Begin("Scene Hierarchy");

    ImGui::BeginDisabled(readOnly);
    if (m_Context)
    {
        m_DrawnEntities.clear();
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

            if (ImGui::BeginMenu("Create"))
            {
                if (ImGui::MenuItem("Static Box Collider"))
                {
                    auto entity = m_Context->CreateEntity("Static Collider");
                    auto &collider = entity.AddComponent<ColliderComponent>();
                    collider.Type = ColliderType::Box;
                    collider.bAutoCalculate = false; // Important: Manual size
                    collider.Size = {1.0f, 1.0f, 1.0f};
                    collider.Offset = {0.0f, 0.0f, 0.0f};
                }
                ImGui::EndMenu();
            }

            if (ImGui::MenuItem("Spawn Zone"))
            {
                auto entity = m_Context->CreateEntity("Spawn Zone");
                entity.AddComponent<SpawnComponent>();
            }

            if (ImGui::BeginMenu("3D Object"))
            {
                if (ImGui::MenuItem("Cube"))
                    EditorLayer::GetCommandHistory().PushCommand(
                        std::make_unique<CreateEntityCommand>(m_Context.get(), "Cube", ":cube:"));

                if (ImGui::MenuItem("Sphere"))
                    EditorLayer::GetCommandHistory().PushCommand(
                        std::make_unique<CreateEntityCommand>(m_Context.get(), "Sphere",
                                                              ":sphere:"));

                if (ImGui::MenuItem("Cylinder"))
                    EditorLayer::GetCommandHistory().PushCommand(
                        std::make_unique<CreateEntityCommand>(m_Context.get(), "Cylinder",
                                                              ":cylinder:"));

                if (ImGui::MenuItem("Cone"))
                    EditorLayer::GetCommandHistory().PushCommand(
                        std::make_unique<CreateEntityCommand>(m_Context.get(), "Cone", ":cone:"));

                if (ImGui::MenuItem("Torus"))
                    EditorLayer::GetCommandHistory().PushCommand(
                        std::make_unique<CreateEntityCommand>(m_Context.get(), "Torus", ":torus:"));

                if (ImGui::MenuItem("Knot"))
                    EditorLayer::GetCommandHistory().PushCommand(
                        std::make_unique<CreateEntityCommand>(m_Context.get(), "Knot", ":knot:"));

                if (ImGui::MenuItem("Plane"))
                    EditorLayer::GetCommandHistory().PushCommand(
                        std::make_unique<CreateEntityCommand>(m_Context.get(), "Plane", ":plane:"));

                ImGui::EndMenu();
            }

            ImGui::EndPopup();
        }
    }

    ImGui::EndDisabled();
    ImGui::End();
}

bool SceneHierarchyPanel::DrawEntityNode(Entity entity)
{
    if (!entity || !entity.IsValid())
        return false;

    if (m_DrawnEntities.find(entity) != m_DrawnEntities.end())
        return false;

    m_DrawnEntities.insert(entity);

    auto &tag = entity.GetComponent<TagComponent>().Tag;

    ImGuiTreeNodeFlags flags = ((m_SelectionContext == entity) ? ImGuiTreeNodeFlags_Selected : 0) |
                               ImGuiTreeNodeFlags_OpenOnArrow;
    flags |= ImGuiTreeNodeFlags_SpanAvailWidth;

    ImGui::PushID((int)(uint32_t)entity);
    bool opened = ImGui::TreeNodeEx(tag.c_str(), flags);

    if (ImGui::IsItemClicked())
    {
        m_SelectionContext = entity;
        if (m_EventCallback)
        {
            EntitySelectedEvent e(entity, m_Context.get());
            m_EventCallback(e);
        }
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
    ImGui::PopID();

    return entityDeleted;
}
} // namespace CHEngine
