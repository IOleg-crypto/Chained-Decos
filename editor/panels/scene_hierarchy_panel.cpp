#include "scene_hierarchy_panel.h"
#include "editor_layer.h"
#include "engine/core/application.h"
#include "engine/core/application.h"
#include "engine/scene/scene_events.h"
#include "engine/scene/components.h"
#include "extras/IconsFontAwesome6.h"
#include "imgui.h"
#include "undo/entity_commands.h"
#include <functional>
#include <vector>

namespace CHEngine
{
    SceneHierarchyPanel::SceneHierarchyPanel()
    {
        m_Name = "Scene Hierarchy";
    }

    SceneHierarchyPanel::SceneHierarchyPanel(const std::shared_ptr<Scene> &context)
    {
        m_Name = "Scene Hierarchy";
        SetContext(context);
    }



    void SceneHierarchyPanel::OnImGuiRender(bool readOnly)
    {
        ImGui::Begin("Scene Hierarchy");
        ImGui::PushID(this);

        if (m_Context)
        {
            m_DrawnEntities.clear();
            std::vector<entt::entity> entitiesToDelete;

            ImGui::BeginDisabled(readOnly);
            
            // Draw root entities (those without parents)
            auto view = m_Context->GetRegistry().view<IDComponent>();
            for (auto entityID : view)
            {
                Entity entity{entityID, m_Context.get()};
                
                // Skip child entities, they will be drawn recursively
                if (entity.HasComponent<HierarchyComponent>() && entity.GetComponent<HierarchyComponent>().Parent != entt::null)
                    continue;

                // Skip hidden UI components
                if (entity.HasComponent<ControlComponent>() && entity.GetComponent<ControlComponent>().HiddenInHierarchy)
                    continue;

                entt::entity toDelete = DrawEntityNodeRecursive(entity);
                if (toDelete != entt::null) entitiesToDelete.push_back(toDelete);
            }

            // Execute deletions via commands
            for (auto ent : entitiesToDelete)
            {
                Entity entity{ent, m_Context.get()};
                EditorLayer::GetCommandHistory().PushCommand(std::make_unique<DestroyEntityCommand>(entity));
            }

            if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered())
            {
                 EntitySelectedEvent e(entt::null, m_Context.get());
                 Application::Get().OnEvent(e);
            }

            // Blank space context menu
            if (ImGui::BeginPopupContextWindow(0, ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
            {
                DrawContextMenu();
                ImGui::EndPopup();
            }
            
            ImGui::EndDisabled();
        }

        ImGui::PopID();
        ImGui::End();
    }

    const char* SceneHierarchyPanel::GetEntityIcon(Entity entity)
    {
        if (entity.HasComponent<ButtonControl>())   return ICON_FA_ARROW_POINTER;
        if (entity.HasComponent<LabelControl>())    return ICON_FA_FONT;
        if (entity.HasComponent<SliderControl>())   return ICON_FA_SLIDERS;
        if (entity.HasComponent<CheckboxControl>()) return ICON_FA_SQUARE_CHECK;
        if (entity.HasComponent<ControlComponent>())return ICON_FA_SHAPES;
        if (entity.HasComponent<PointLightComponent>()) return ICON_FA_LIGHTBULB;
        if (entity.HasComponent<CameraComponent>()) return ICON_FA_VIDEO;
        if (entity.HasComponent<AudioComponent>())  return ICON_FA_VOLUME_HIGH;
        
        return ICON_FA_CUBE;
    }

    entt::entity SceneHierarchyPanel::DrawEntityNodeRecursive(Entity entity)
    {
        if (!entity || !entity.IsValid() || m_DrawnEntities.contains(entity))
            return entt::null;

        m_DrawnEntities.insert(entity);

        auto &tag = entity.GetComponent<TagComponent>().Tag;
        std::string label = std::string(GetEntityIcon(entity)) + "  " + tag;

        auto selectedEntity = EditorLayer::Get().GetSelectedEntity();
        ImGuiTreeNodeFlags flags = ((selectedEntity == entity) ? ImGuiTreeNodeFlags_Selected : 0);
        flags |= ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

        ImGui::PushID((int)(uint32_t)entity);
        bool opened = ImGui::TreeNodeEx(label.c_str(), flags);

        if (ImGui::IsItemClicked())
        {
            EntitySelectedEvent e(entity, m_Context.get());
            Application::Get().OnEvent(e);
        }

        entt::entity signaledForDelete = entt::null;
        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Delete Entity"))
                signaledForDelete = (entt::entity)entity;
            ImGui::EndPopup();
        }

        if (opened)
        {
            if (entity.HasComponent<HierarchyComponent>())
            {
                auto children = entity.GetComponent<HierarchyComponent>().Children; // Copy to avoid iteration issues
                for (auto childID : children)
                {
                    entt::entity childDel = DrawEntityNodeRecursive({childID, m_Context.get()});
                    if (childDel != entt::null) signaledForDelete = childDel;
                }
            }
            ImGui::TreePop();
        }
        ImGui::PopID();

        return signaledForDelete;
    }

    void SceneHierarchyPanel::DrawContextMenu()
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
                collider.AutoCalculate = false;
                collider.Size = {1.0f, 1.0f, 1.0f};
                collider.Offset = {0.0f, 0.0f, 0.0f};
            }
            ImGui::EndMenu();
        }

        if (ImGui::MenuItem("Spawn Zone"))
            m_Context->CreateEntity("Spawn Zone").AddComponent<SpawnComponent>();

        if (ImGui::BeginMenu("3D Object"))
        {
            auto create = [this](const char* name, const char* mesh) {
                EditorLayer::GetCommandHistory().PushCommand(std::make_unique<CreateEntityCommand>(m_Context.get(), name, mesh));
            };
            if (ImGui::MenuItem("Cube")) create("Cube", ":cube:");
            if (ImGui::MenuItem("Sphere")) create("Sphere", ":sphere:");
            if (ImGui::MenuItem("Cylinder")) create("Cylinder", ":cylinder:");
            if (ImGui::MenuItem("Cone")) create("Cone", ":cone:");
            if (ImGui::MenuItem("Torus")) create("Torus", ":torus:");
            if (ImGui::MenuItem("Knot")) create("Knot", ":knot:");
            if (ImGui::MenuItem("Plane")) create("Plane", ":plane:");
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Control"))
        {
            if (ImGui::MenuItem("Panel"))    m_Context->CreateUIEntity("Panel");
            if (ImGui::MenuItem("Button"))   m_Context->CreateUIEntity("Button");
            if (ImGui::MenuItem("Label"))    m_Context->CreateUIEntity("Label");
            if (ImGui::MenuItem("Slider"))   m_Context->CreateUIEntity("Slider");
            if (ImGui::MenuItem("Checkbox")) m_Context->CreateUIEntity("CheckBox");
            ImGui::EndMenu();
        }
    }
} // namespace CHEngine
