#ifndef CH_PROPERTY_EDITOR_H
#define CH_PROPERTY_EDITOR_H

#include "editor_gui.h"
#include "engine/scene/scene.h"
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
        static void RegisterComponent(entt::id_type typeId, const ComponentMetadata &metadata);
        static void DrawEntityProperties(Entity entity);
        static void DrawAddComponentPopup(Entity entity);



        // Template helper for easy registration
        template <typename T> static void Register(const std::string &name, std::function<bool(T &, Entity)> drawer)
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
        template <typename T> static void Register(const std::string &name, std::function<bool(T &)> drawer)
        {
            Register<T>(name, [drawer](T &comp, Entity e) { return drawer(comp); });
        }

        // Shared style drawers (used by inspector_panel and property_editor)
        static bool DrawTextStyle(TextStyle &style);
        static bool DrawUIStyle(UIStyle &style);

        // High-level UI Primitives (Special cases or reused structures)
        static void DrawTag(Entity entity);
        static void DrawMaterial(Entity entity, int hitMeshIndex = -1);

    private:
        // Shared Container for all components (Handles headers, removal button, etc)
        template <typename T>
        static void DrawComponentContainer(const std::string &name, Entity entity, std::function<bool(T &, Entity)> drawer)
        {
            const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed |
                                                     ImGuiTreeNodeFlags_SpanAvailWidth |
                                                     ImGuiTreeNodeFlags_AllowOverlap | ImGuiTreeNodeFlags_FramePadding;

            if (entity.HasComponent<T>())
            {
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{4, 4});
                
                // Unique ID for the header
                ImGui::PushID(name.c_str());
                
                float contentWidth = ImGui::GetContentRegionAvail().x;
                
                // Draw the header
                bool open = ImGui::TreeNodeEx((void *)entt::type_hash<T>::value(), treeNodeFlags, name.c_str());

                // Management button (Gear Icon)
                ImGui::SameLine(contentWidth - 22.0f);
                
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0, 0, 0, 0});
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{1, 1, 1, 0.1f});
                if (ImGui::Button(ICON_FA_GEAR, ImVec2{22, 22}))
                {
                    ImGui::OpenPopup("ComponentSettings");
                }
                ImGui::PopStyleColor(2);

                bool removeComponent = false;
                if (ImGui::BeginPopup("ComponentSettings"))
                {
                    if (ImGui::MenuItem(ICON_FA_TRASH " Remove Component"))
                        removeComponent = true;
                    ImGui::EndPopup();
                }
                
                ImGui::PopID();
                ImGui::PopStyleVar();

                if (open)
                {
                    ImGui::Spacing();
                    T &component = entity.GetComponent<T>();
                    T componentCopy = component;
                    if (drawer(componentCopy, entity))
                    {
                        entity.GetRegistry().template patch<T>(
                            entity, [&componentCopy](T &comp) { comp = componentCopy; });
                    }
                    ImGui::Spacing();
                    ImGui::TreePop();
                }

                if (removeComponent)
                {
                    entity.GetRegistry().template remove<T>(entity);
                }
            }
        }

    private:
        static std::unordered_map<entt::id_type, ComponentMetadata> s_ComponentRegistry;
    };

} // namespace CHEngine

#endif // CH_PROPERTY_EDITOR_H
