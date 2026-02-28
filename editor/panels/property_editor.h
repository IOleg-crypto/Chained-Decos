#ifndef CH_PROPERTY_EDITOR_H
#define CH_PROPERTY_EDITOR_H

#include "editor_gui.h"
#include "engine/scene/scene.h"
#include "extras/IconsFontAwesome6.h"
#include <functional>
#include <string>
#include <unordered_map>

namespace CHEngine
{
class PropertyEditor
{
public:
    struct ComponentMetadata
    {
        std::string Name;
        std::function<void(Entity)> Draw;
        std::function<bool(Entity)> Add;
        bool Visible = true;
        bool AllowAdd = true;
        bool IsWidget = false;
    };

    static void Init();

    // Registry API
    static void RegisterComponent(entt::id_type typeId, const ComponentMetadata& metadata);
    static void DrawEntityProperties(Entity entity);
    static void DrawAddComponentPopup(Entity entity);

    // Template helper for easy registration
    template <typename T> static void Register(const std::string& name, std::function<bool(T&, Entity)> drawer)
    {
        ComponentMetadata metadata;
        metadata.Name = name;
        metadata.Draw = [name, drawer](Entity e) { DrawComponentContainer<T>(name, e, drawer); };
        metadata.Add = [](Entity e) {
            if (!e.HasComponent<T>())
            {
                e.AddComponent<T>();
                return true;
            }
            return false;
        };
        RegisterComponent(entt::type_hash<T>::value(), metadata);
    }

    // Backward compatibility for simple drawers
    template <typename T> static void Register(const std::string& name, std::function<bool(T&)> drawer)
    {
        Register<T>(name, [drawer](T& comp, Entity e) { return drawer(comp); });
    }

    // Shared style drawers (used by inspector_panel and property_editor)
    static bool DrawTextStyle(TextStyle& style);
    static bool DrawUIStyle(UIStyle& style);

    // High-level UI Primitives (Special cases or reused structures)
    static void DrawTag(Entity entity);
    static void DrawMaterial(Entity entity, int hitMeshIndex = -1);

private:
    // Shared Container for all components (Handles headers, removal button, etc)
    template <typename T>
    static void DrawComponentContainer(const std::string& name, Entity entity, std::function<bool(T&, Entity)> drawer)
    {
        const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed |
                                                 ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowOverlap |
                                                 ImGuiTreeNodeFlags_FramePadding;

        if (entity.HasComponent<T>())
        {
            auto& component = entity.GetComponent<T>();
            ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();

            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{4, 4});
            float lineHeight = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0f;
            ImGui::Separator();
            bool open = ImGui::TreeNodeEx((void*)entt::type_hash<T>::value(), treeNodeFlags, name.c_str());
            ImGui::PopStyleVar();

            ImGui::SameLine(contentRegionAvailable.x - lineHeight * 0.5f);
            if (ImGui::Button(ICON_FA_GEAR, ImVec2{lineHeight, lineHeight}))
            {
                ImGui::OpenPopup("ComponentSettings");
            }

            bool removeComponent = false;
            if (ImGui::BeginPopup("ComponentSettings"))
            {
                if (ImGui::MenuItem("Remove Component"))
                    removeComponent = true;

                ImGui::EndPopup();
            }

            if (open)
            {
                T componentCopy = component;
                if (drawer(componentCopy, entity))
                {
                    entity.GetRegistry().template patch<T>(entity, [&componentCopy](T& comp) { comp = componentCopy; });
                }
                ImGui::TreePop();
            }

            if (removeComponent)
                entity.RemoveComponent<T>();
        }
    }

private:
    static std::unordered_map<entt::id_type, ComponentMetadata> s_ComponentRegistry;
};

} // namespace CHEngine

#endif // CH_PROPERTY_EDITOR_H
