#ifndef CH_COMPONENT_UI_H
#define CH_COMPONENT_UI_H

#include "editor/ui/editor_gui.h"
#include "engine/scene/entity.h"

#include "unordered_map"

namespace CHEngine
{
class ComponentUI
{
public:
    struct ComponentMetadata
    {
        std::string Name;
        std::function<void(Entity)> Draw;
        std::function<void(Entity)> Add;
    };

    static void Init();

    // Registry-based API
    static void RegisterComponent(entt::id_type typeId, const ComponentMetadata &metadata);
    static void DrawEntityComponents(Entity entity);
    static void DrawAddComponentPopup(Entity entity);

    template <typename T>
    static void Register(const std::string &name, std::function<void(Entity)> drawer)
    {
        ComponentMetadata metadata;
        metadata.Name = name;
        metadata.Draw = drawer;
        metadata.Add = [](Entity e) { e.AddComponent<T>(); };
        RegisterComponent(entt::type_hash<T>::value(), metadata);
    }

    // Shared Template for Drawers
    template <typename T, typename UIFunction>
    static void DrawComponent(const std::string &name, Entity entity, UIFunction uiFunction);

    // Shared UI Primitives
    // (Deprecated: Use EditorUI::GUI::Property for Color)

    static void DrawTag(Entity entity);
    static void DrawTransform(Entity entity);
    static void DrawModel(Entity entity);
    static void DrawMaterial(Entity entity, int hitMeshIndex = -1);
    static void DrawCollider(Entity entity);
    static void DrawRigidBody(Entity entity);
    static void DrawSpawn(Entity entity);
    static void DrawPlayer(Entity entity);
    static void DrawPointLight(Entity entity);
    static void DrawAudio(Entity entity);
    static void DrawHierarchy(Entity entity);
    static void DrawNativeScript(Entity entity);
    static void DrawAnimation(Entity entity);

    // Helpers (Internal Use)
    static void DrawTextStyle(struct TextStyle &style);
    static void DrawUIStyle(struct UIStyle &style);

    // UI Controls
    static void DrawControl(Entity entity);
    static void DrawPanelControl(Entity entity);
    static void DrawLabelControl(Entity entity);
    static void DrawButtonControl(Entity entity);
    static void DrawSliderControl(Entity entity);
    static void DrawCheckboxControl(Entity entity);

private:
    static std::unordered_map<entt::id_type, ComponentMetadata> s_ComponentRegistry;
};

// --- Inline Implementations ---

template <typename T, typename UIFunction>
inline void ComponentUI::DrawComponent(const std::string &name, Entity entity,
                                       UIFunction uiFunction)
{
    const ImGuiTreeNodeFlags treeNodeFlags =
        ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed |
        ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowOverlap |
        ImGuiTreeNodeFlags_FramePadding;

    if (entity.HasComponent<T>())
    {
        auto &component = entity.GetComponent<T>();
        float lineHeight = 24.0f; // Simplified for now

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{4, 4});
        ImGui::Separator();

        // --- Header with Removal Button ---
        ImGui::PushID(name.c_str());
        bool open = ImGui::TreeNodeEx((void *)typeid(T).hash_code(), treeNodeFlags, name.c_str());

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
            uiFunction(component);
            ImGui::TreePop();
        }

        if (removeComponent)
        {
            auto &registry = entity.GetScene()->GetRegistry();
            registry.template remove<T>(entity);
        }
    }
}

} // namespace CHEngine

#endif // CH_COMPONENT_UI_H
