#ifndef CH_PROPERTY_EDITOR_H
#define CH_PROPERTY_EDITOR_H

#include "editor/ui/editor_gui.h"
#include "engine/scene/entity.h"
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
        };

        static void Init();

        // Registry API
        static void RegisterComponent(entt::id_type typeId, const ComponentMetadata &metadata);
        static void DrawEntityProperties(Entity entity);
        static void DrawAddComponentPopup(Entity entity);

        // Template helper for easy registration
        template <typename T> static void Register(const std::string &name, std::function<bool(T &)> drawer)
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

        // High-level UI Primitives (Special cases or reused structures)
        static void DrawTag(Entity entity);
        static void DrawMaterial(Entity entity, int hitMeshIndex = -1);

    private:
        // Shared Container for all components (Handles headers, removal button, etc)
        template <typename T>
        static void DrawComponentContainer(const std::string &name, Entity entity, std::function<bool(T &)> drawer)
        {
            const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed |
                                                     ImGuiTreeNodeFlags_SpanAvailWidth |
                                                     ImGuiTreeNodeFlags_AllowOverlap | ImGuiTreeNodeFlags_FramePadding;

            if (entity.HasComponent<T>())
            {
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{4, 4});
                ImGui::Separator();

                // Unique ID for the header
                ImGui::PushID(name.c_str());
                bool open = ImGui::TreeNodeEx((void *)entt::type_hash<T>::value(), treeNodeFlags, name.c_str());

                // Management button (X)
                ImGui::SameLine(ImGui::GetWindowWidth() - 35);
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.7f, 0.1f, 0.1f, 0.6f});
                if (ImGui::Button("X", ImVec2{20, 20}))
                {
                    ImGui::OpenPopup("ComponentSettings");
                }
                ImGui::PopStyleColor();

                bool removeComponent = false;
                if (ImGui::BeginPopup("ComponentSettings"))
                {
                    if (ImGui::MenuItem("Remove Component"))
                        removeComponent = true;
                    ImGui::EndPopup();
                }
                ImGui::PopID();
                ImGui::PopStyleVar();

                if (open)
                {
                    T &component = entity.GetComponent<T>();
                    T componentCopy = component;
                    if (drawer(componentCopy))
                    {
                        entity.GetScene()->GetRegistry().patch<T>(
                            entity, [&componentCopy](T &comp) { comp = componentCopy; });
                    }
                    ImGui::TreePop();
                }

                if (removeComponent)
                {
                    entity.GetScene()->GetRegistry().remove<T>(entity);
                }
            }
        }

    private:
        static std::unordered_map<entt::id_type, ComponentMetadata> s_ComponentRegistry;
    };

} // namespace CHEngine

#endif // CH_PROPERTY_EDITOR_H
