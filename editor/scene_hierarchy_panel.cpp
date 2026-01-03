#include "scene_hierarchy_panel.h"
#include "engine/components.h"
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
        for (auto entityID : registry.storage<entt::entity>())
        {
            Entity entity{entityID, m_Context.get()};
            if (DrawEntityNode(entity))
                entityToDelete = entityID;
        }

        if (entityToDelete != entt::null)
        {
            m_Context->DestroyEntity(Entity{entityToDelete, m_Context.get()});
            if (m_SelectionContext == Entity{entityToDelete, m_Context.get()})
                m_SelectionContext = {};
        }

        if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered())
            m_SelectionContext = {};

        // Right-click on blank space
        if (ImGui::BeginPopupContextWindow(0, 1))
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
                {
                    auto entity = m_Context->CreateEntity("Cube");
                    entity.AddComponent<ModelComponent>(":cube:");
                }
                if (ImGui::MenuItem("Sphere"))
                {
                    auto entity = m_Context->CreateEntity("Sphere");
                    entity.AddComponent<ModelComponent>(":sphere:");
                }
                if (ImGui::MenuItem("Cylinder"))
                {
                    auto entity = m_Context->CreateEntity("Cylinder");
                    entity.AddComponent<ModelComponent>(":cylinder:");
                }
                if (ImGui::MenuItem("Cone"))
                {
                    auto entity = m_Context->CreateEntity("Cone");
                    entity.AddComponent<ModelComponent>(":cone:");
                }
                if (ImGui::MenuItem("Torus"))
                {
                    auto entity = m_Context->CreateEntity("Torus");
                    entity.AddComponent<ModelComponent>(":torus:");
                }
                if (ImGui::MenuItem("Knot"))
                {
                    auto entity = m_Context->CreateEntity("Knot");
                    entity.AddComponent<ModelComponent>(":knot:");
                }
                if (ImGui::MenuItem("Plane"))
                {
                    auto entity = m_Context->CreateEntity("Plane");
                    entity.AddComponent<ModelComponent>(":plane:");
                }
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
        ImGui::TreePop();
    }

    return entityDeleted;
}
} // namespace CH
