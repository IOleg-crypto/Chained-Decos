#include "HierarchyPanel.h"
#include "scene/core/Scene.h"
#include "scene/ecs/components/core/TagComponent.h"
#include "scene/resources/map/GameScene.h"
#include "scene/resources/map/MapData.h"
#include <imgui.h>
#include <raylib.h>

namespace CHEngine
{

// =========================================================================
// Configuration & Context
// =========================================================================

HierarchyPanel::HierarchyPanel(const std::shared_ptr<GameScene> &scene) : m_Context(scene)
{
}

void HierarchyPanel::SetContext(const std::shared_ptr<GameScene> &scene)
{
    m_Context = scene;
}

void HierarchyPanel::SetSceneContext(const std::shared_ptr<Scene> &scene)
{
    m_SceneContext = scene;
}

bool HierarchyPanel::IsVisible() const
{
    return m_isVisible;
}

void HierarchyPanel::SetVisible(bool visible)
{
    m_isVisible = visible;
}

// =========================================================================
// Panel Lifecycle
// =========================================================================

void HierarchyPanel::OnImGuiRender(SelectionType selectionType, int selectedIndex,
                                   const std::function<void(SelectionType, int)> &onSelect,
                                   const std::function<void()> &onAddModel,
                                   const std::function<void(const std::string &)> &onAddUI,
                                   const std::function<void(int)> &onDelete,
                                   entt::entity selectedEntity,
                                   const std::function<void(entt::entity)> &onSelectEntity,
                                   const std::function<void()> &onCreateEntity,
                                   const std::function<void(entt::entity)> &onDeleteEntity)
{
    ImGui::Begin("Scene hierarchy");

    // New Scene System - Entities
    if (m_SceneContext)
    {
        if (ImGui::CollapsingHeader("Entities (New)", ImGuiTreeNodeFlags_DefaultOpen))
        {
            auto &registry = m_SceneContext->GetRegistry();
            auto view = registry.view<TagComponent>();

            int entityIndex = 0;
            for (auto entity : view)
            {
                auto &tag = view.get<TagComponent>(entity);

                ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow |
                                           ImGuiTreeNodeFlags_SpanAvailWidth |
                                           ImGuiTreeNodeFlags_Leaf;

                // Highlight if selected
                if (selectionType == SelectionType::ENTITY && selectedEntity == entity)
                {
                    flags |= ImGuiTreeNodeFlags_Selected;
                }

                ImGui::PushID(static_cast<int>(entity));
                bool opened = ImGui::TreeNodeEx("##Entity", flags, "%s", tag.Tag.c_str());
                if (ImGui::IsItemClicked() && onSelectEntity)
                {
                    onSelectEntity(entity);
                }
                if (opened)
                    ImGui::TreePop();
                ImGui::PopID();
                entityIndex++;
            }
        }

        ImGui::Spacing();
    }

    // Legacy GameScene - World Objects
    if (m_Context)
    {
        // 1. World Section (Legacy)
        if (ImGui::CollapsingHeader("World", ImGuiTreeNodeFlags_DefaultOpen))
        {
            auto &objects = m_Context->GetMapObjectsMutable();
            for (int i = 0; i < (int)objects.size(); i++)
            {
                auto &obj = objects[i];
                ImGuiTreeNodeFlags flags =
                    ((selectionType == SelectionType::WORLD_OBJECT && selectedIndex == i)
                         ? ImGuiTreeNodeFlags_Selected
                         : 0) |
                    ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth |
                    ImGuiTreeNodeFlags_Leaf;

                ImGui::PushID(i);
                bool opened = ImGui::TreeNodeEx("##Object", flags, "%s", obj.name.c_str());
                if (ImGui::IsItemClicked())
                    onSelect(SelectionType::WORLD_OBJECT, i);
                if (opened)
                    ImGui::TreePop();
                ImGui::PopID();
            }
        }

        ImGui::Spacing();

        // 2. UI Section
        if (ImGui::CollapsingHeader("User Interface", ImGuiTreeNodeFlags_DefaultOpen))
        {
            auto &elements = m_Context->GetUIElementsMutable();
            for (int i = 0; i < (int)elements.size(); i++)
            {
                auto &el = elements[i];
                ImGuiTreeNodeFlags flags =
                    ((selectionType == SelectionType::UI_ELEMENT && selectedIndex == i)
                         ? ImGuiTreeNodeFlags_Selected
                         : 0) |
                    ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth |
                    ImGuiTreeNodeFlags_Leaf;

                ImGui::PushID(i + 10000);
                bool opened = ImGui::TreeNodeEx("##UIEntry", flags, "[%s] %s", el.type.c_str(),
                                                el.name.c_str());
                if (ImGui::IsItemClicked())
                    onSelect(SelectionType::UI_ELEMENT, i);
                if (opened)
                    ImGui::TreePop();
                ImGui::PopID();
            }
        }
    }

    if (ImGui::BeginPopupContextWindow("HierarchyContext", ImGuiPopupFlags_MouseButtonRight))
    {
        if (ImGui::BeginMenu("Create..."))
        {
            if (ImGui::MenuItem("Entity (New)"))
            {
                if (onCreateEntity)
                    onCreateEntity();
            }
            ImGui::Separator();
            if (ImGui::BeginMenu("3D Object"))
            {
                auto addObject = [&](const std::string &name, MapObjectType type, Color color)
                {
                    if (m_Context)
                    {
                        MapObjectData obj;
                        obj.name = name;
                        obj.type = type;
                        obj.color = color;
                        if (type == MapObjectType::SPAWN_ZONE)
                            obj.scale = {2, 2, 2};
                        auto &objects = m_Context->GetMapObjectsMutable();
                        objects.push_back(obj);
                        onSelect(SelectionType::WORLD_OBJECT, (int)objects.size() - 1);
                    }
                };

                if (ImGui::MenuItem("Cube"))
                    addObject("Cube", MapObjectType::CUBE, {0, 121, 241, 255});
                if (ImGui::MenuItem("Sphere"))
                    addObject("Sphere", MapObjectType::SPHERE, {253, 249, 0, 255});
                if (ImGui::MenuItem("Plane"))
                    addObject("Plane", MapObjectType::PLANE, {160, 160, 160, 255});
                if (ImGui::MenuItem("Cylinder"))
                    addObject("Cylinder", MapObjectType::CYLINDER, {255, 161, 0, 255});
                if (ImGui::MenuItem("Spawn Zone"))
                    addObject("Spawn Zone", MapObjectType::SPAWN_ZONE, {0, 255, 255, 255});
                ImGui::EndMenu();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Model..."))
                onAddModel();
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("UI Element"))
        {
            if (ImGui::MenuItem("Button"))
                onAddUI("button");
            if (ImGui::MenuItem("Text"))
                onAddUI("text");
            if (ImGui::MenuItem("Image"))
                onAddUI("image");
            ImGui::EndMenu();
        }

        ImGui::Separator();
        if (ImGui::MenuItem("Delete Object", "Del"))
        {
            if (selectedIndex >= 0 && m_Context)
            {
                if (selectionType == SelectionType::WORLD_OBJECT)
                {
                    if (onDelete)
                        onDelete(selectedIndex);
                }
                else if (selectionType == SelectionType::ENTITY)
                {
                    if (onDeleteEntity)
                        onDeleteEntity(selectedEntity);
                }
                else if (selectionType == SelectionType::UI_ELEMENT)
                {
                    // Handle UI deletion separately or extend onDelete
                    auto &elements = m_Context->GetUIElementsMutable();
                    if (selectedIndex < (int)elements.size())
                    {
                        elements.erase(elements.begin() + selectedIndex);
                        onSelect(SelectionType::NONE, -1);
                    }
                }
            }
        }
        ImGui::EndPopup();
    }

    ImGui::End();
}
} // namespace CHEngine
