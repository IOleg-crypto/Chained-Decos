#include "HierarchyPanel.h"
#include <imgui.h>

namespace CHEngine
{
HierarchyPanel::HierarchyPanel(const std::shared_ptr<GameScene> &scene) : m_Context(scene)
{
}

void HierarchyPanel::SetContext(const std::shared_ptr<GameScene> &scene)
{
    m_Context = scene;
}

void HierarchyPanel::OnImGuiRender(SelectionType selectionType, int selectedIndex,
                                   std::function<void(SelectionType, int)> onSelect,
                                   std::function<void()> onAddModel,
                                   std::function<void(const std::string &)> onAddUI)
{
    ImGui::Begin("Hierarchy");

    if (m_Context)
    {
        // 1. World Section
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
                    ImGuiTreeNodeFlags_OpenOnArrow;
                flags |= ImGuiTreeNodeFlags_SpanAvailWidth;
                flags |= ImGuiTreeNodeFlags_Leaf;

                ImGui::PushID(i);
                bool opened = ImGui::TreeNodeEx("##Object", flags, "%s", obj.name.c_str());

                if (ImGui::IsItemClicked())
                {
                    onSelect(SelectionType::WORLD_OBJECT, i);
                }

                if (opened)
                {
                    ImGui::TreePop();
                }
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
                    ImGuiTreeNodeFlags_OpenOnArrow;
                flags |= ImGuiTreeNodeFlags_SpanAvailWidth;
                flags |= ImGuiTreeNodeFlags_Leaf;

                ImGui::PushID(i + 10000); // Offset for UI IDs
                bool opened = ImGui::TreeNodeEx("##UIEntry", flags, "[%s] %s", el.type.c_str(),
                                                el.name.c_str());

                if (ImGui::IsItemClicked())
                {
                    onSelect(SelectionType::UI_ELEMENT, i);
                }

                if (opened)
                {
                    ImGui::TreePop();
                }
                ImGui::PopID();
            }
        }
    }

    if (ImGui::BeginPopupContextWindow("HierarchyContext", ImGuiPopupFlags_MouseButtonRight))
    {
        if (ImGui::BeginMenu("Create..."))
        {
            if (ImGui::BeginMenu("3D Object"))
            {
                if (ImGui::MenuItem("Cube"))
                {
                    if (m_Context)
                    {
                        MapObjectData obj;
                        obj.name = "Cube";
                        obj.type = MapObjectType::CUBE;
                        obj.color = {0, 121, 241, 255};
                        auto &objects = m_Context->GetMapObjectsMutable();
                        objects.push_back(obj);
                        onSelect(SelectionType::WORLD_OBJECT, (int)objects.size() - 1);
                    }
                }
                if (ImGui::MenuItem("Sphere"))
                {
                    if (m_Context)
                    {
                        MapObjectData obj;
                        obj.name = "Sphere";
                        obj.type = MapObjectType::SPHERE;
                        obj.color = {253, 249, 0, 255};
                        auto &objects = m_Context->GetMapObjectsMutable();
                        objects.push_back(obj);
                        onSelect(SelectionType::WORLD_OBJECT, (int)objects.size() - 1);
                    }
                }
                if (ImGui::MenuItem("Plane"))
                {
                    if (m_Context)
                    {
                        MapObjectData obj;
                        obj.name = "Plane";
                        obj.type = MapObjectType::PLANE;
                        obj.size = {10, 10};
                        obj.color = {160, 160, 160, 255};
                        auto &objects = m_Context->GetMapObjectsMutable();
                        objects.push_back(obj);
                        onSelect(SelectionType::WORLD_OBJECT, (int)objects.size() - 1);
                    }
                }
                if (ImGui::MenuItem("Cylinder"))
                {
                    if (m_Context)
                    {
                        MapObjectData obj;
                        obj.name = "Cylinder";
                        obj.type = MapObjectType::CYLINDER;
                        obj.radius = 1.0f;
                        obj.height = 2.0f;
                        obj.color = {255, 161, 0, 255};
                        auto &objects = m_Context->GetMapObjectsMutable();
                        objects.push_back(obj);
                        onSelect(SelectionType::WORLD_OBJECT, (int)objects.size() - 1);
                    }
                }
                if (ImGui::MenuItem("Spawn Zone"))
                {
                    if (m_Context)
                    {
                        MapObjectData obj;
                        obj.name = "Spawn Zone";
                        obj.type = MapObjectType::SPAWN_ZONE;
                        obj.scale = {2, 2, 2};
                        obj.color = {0, 255, 255, 255};
                        auto &objects = m_Context->GetMapObjectsMutable();
                        objects.push_back(obj);
                        onSelect(SelectionType::WORLD_OBJECT, (int)objects.size() - 1);
                    }
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Model..."))
                {
                    onAddModel();
                }
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

            ImGui::EndMenu();
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Delete Object", "Del"))
        {
            if (selectedIndex >= 0 && m_Context)
            {
                if (selectionType == SelectionType::WORLD_OBJECT)
                {
                    auto &objects = m_Context->GetMapObjectsMutable();
                    if (selectedIndex < (int)objects.size())
                    {
                        objects.erase(objects.begin() + selectedIndex);
                        onSelect(SelectionType::NONE, -1);
                    }
                }
                else if (selectionType == SelectionType::UI_ELEMENT)
                {
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
