#include "scene_hierarchy_panel.h"
#include "editor_layer.h"
#include "engine/core/application.h"
#include "engine/core/application.h"
#include "engine/scene/scene_events.h"
#include "engine/scene/components.h"
#include "extras/IconsFontAwesome6.h"
#include "imgui.h"
#include "undo/entity_commands.h"

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

    void SceneHierarchyPanel::SetContext(const std::shared_ptr<Scene> &context)
    {
        Panel::SetContext(context);
        m_SelectionContext = {};
    }

    void SceneHierarchyPanel::OnImGuiRender(bool readOnly)
    {
        ImGui::Begin("Scene Hierarchy");
        ImGui::PushID(this);

        ImGui::BeginDisabled(readOnly);
        if (m_Context)
        {

            m_DrawnEntities.clear();
            auto &registry = m_Context->GetRegistry();

            std::vector<entt::entity> entitiesToDelete;
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

                    if (entity.HasComponent<ControlComponent>())
                    {
                        if (entity.GetComponent<ControlComponent>().HiddenInHierarchy)
                            return;
                    }

                    // Recursive search for entities to delete
                    auto collectDeletions = [&](Entity e, auto &self) -> void
                    {
                        if (DrawEntityNodeRecursive(e) != entt::null)
                            entitiesToDelete.push_back((entt::entity)e);
                        else if (m_Context->GetRegistry().valid(e) && e.HasComponent<HierarchyComponent>())
                        {
                            auto children = e.GetComponent<HierarchyComponent>().Children;
                            // Note: we don't recurse here because DrawEntityNode already recurse!
                            // We only need to check if DrawEntityNode(root) or its children inside
                            // DrawEntityNode returned true.
                        }
                    };

                    if (!isChild)
                    {
                        // I'll simplify: DrawEntityNode will return entity to delete
                        entt::entity toDel = DrawEntityNodeRecursive(entity);
                        if (toDel != entt::null)
                            entitiesToDelete.push_back(toDel);
                    }
                });

            for (auto ent : entitiesToDelete)
            {
                Entity entity{ent, m_Context.get()};
                EditorLayer::GetCommandHistory().PushCommand(std::make_unique<DestroyEntityCommand>(entity));

                if (m_SelectionContext == entity)
                    m_SelectionContext = {};
            }

            if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered())
                m_SelectionContext = {};

            // Right-click on blank space
            if (ImGui::BeginPopupContextWindow(0, ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
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
                        collider.AutoCalculate = false; // Important: Manual size
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
                            std::make_unique<CreateEntityCommand>(m_Context.get(), "Sphere", ":sphere:"));

                    if (ImGui::MenuItem("Cylinder"))
                        EditorLayer::GetCommandHistory().PushCommand(
                            std::make_unique<CreateEntityCommand>(m_Context.get(), "Cylinder", ":cylinder:"));

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

                if (ImGui::BeginMenu("Control"))
                {
                    if (ImGui::MenuItem("Panel"))
                        m_Context->CreateUIEntity("Panel");
                    if (ImGui::MenuItem("Button"))
                        m_Context->CreateUIEntity("Button");
                    if (ImGui::MenuItem("Label"))
                        m_Context->CreateUIEntity("Label");
                    if (ImGui::MenuItem("Slider"))
                        m_Context->CreateUIEntity("Slider");
                    if (ImGui::MenuItem("Checkbox"))
                        m_Context->CreateUIEntity("CheckBox");

                    ImGui::EndMenu();
                }
                ImGui::EndPopup();
            }
        }

        ImGui::EndDisabled();
        ImGui::PopID();
        ImGui::End();
    }

    entt::entity SceneHierarchyPanel::DrawEntityNodeRecursive(Entity entity)
    {
        if (!entity || !entity.IsValid())
            return entt::null;

        if (m_DrawnEntities.find(entity) != m_DrawnEntities.end())
            return entt::null;

        m_DrawnEntities.insert(entity);

        auto &tag = entity.GetComponent<TagComponent>().Tag;

        const char *icon = ICON_FA_CUBE;
        if (entity.HasComponent<ControlComponent>())
        {
            if (entity.HasComponent<ButtonControl>())
                icon = ICON_FA_ARROW_POINTER;
            else if (entity.HasComponent<LabelControl>())
                icon = ICON_FA_FONT;
            else if (entity.HasComponent<SliderControl>())
                icon = ICON_FA_SLIDERS;
            else if (entity.HasComponent<CheckboxControl>())
                icon = ICON_FA_SQUARE_CHECK;
            else
                icon = ICON_FA_SHAPES;
        }
        else if (entity.HasComponent<PointLightComponent>())
        {
            icon = ICON_FA_LIGHTBULB;
        }
        else if (entity.HasComponent<CameraComponent>())
        {
            icon = ICON_FA_VIDEO;
        }
        else if (entity.HasComponent<AudioComponent>())
        {
            icon = ICON_FA_VOLUME_HIGH;
        }

        std::string label = std::string(icon) + "  " + tag;

        ImGuiTreeNodeFlags flags =
            ((m_SelectionContext == entity) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;
        flags |= ImGuiTreeNodeFlags_SpanAvailWidth;

        ImGui::PushID((int)(uint32_t)entity);
        bool opened = ImGui::TreeNodeEx(label.c_str(), flags);

        if (ImGui::IsItemClicked())
        {
            m_SelectionContext = entity;
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
                auto &hc = entity.GetComponent<HierarchyComponent>();
                auto childrenCopy = hc.Children; // Copy to avoid iteration issues if destroyed
                for (auto childID : childrenCopy)
                {
                    Entity child{childID, m_Context.get()};
                    entt::entity childDel = DrawEntityNodeRecursive(child);
                    if (childDel != entt::null)
                        signaledForDelete = childDel;
                }
            }
            ImGui::TreePop();
        }
        ImGui::PopID();

        return signaledForDelete;
    }
} // namespace CHEngine
