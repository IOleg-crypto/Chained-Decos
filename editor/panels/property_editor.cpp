#include "property_editor.h"
#include "editor/ui/editor_gui.h"
#include "engine/scene/components.h"
#include "imgui.h"
#include "raymath.h"

namespace CHEngine
{
    using GUI = EditorUI::GUI;

    std::unordered_map<entt::id_type, PropertyEditor::ComponentMetadata> PropertyEditor::s_ComponentRegistry;

    void PropertyEditor::RegisterComponent(entt::id_type typeId, const ComponentMetadata &metadata)
    {
        s_ComponentRegistry[typeId] = metadata;
    }

    // --- Declarative UI Property Helpers ---
    static bool DrawRectTransform(RectTransform &transform)
    {
        bool changed = false;

        // Anchors
        ImGui::Text("Anchors");
        ImGui::SameLine(100);
        ImGui::PushItemWidth(50);
        if (ImGui::DragFloat("##AnchorMinX", &transform.AnchorMin.x, 0.01f)) changed = true; ImGui::SameLine();
        if (ImGui::DragFloat("##AnchorMinY", &transform.AnchorMin.y, 0.01f)) changed = true;
        ImGui::SameLine(); ImGui::Text("Min"); ImGui::SameLine();
        if (ImGui::DragFloat("##AnchorMaxX", &transform.AnchorMax.x, 0.01f)) changed = true; ImGui::SameLine();
        if (ImGui::DragFloat("##AnchorMaxY", &transform.AnchorMax.y, 0.01f)) changed = true;
        ImGui::PopItemWidth();

        // Position (Shift)
        ImGui::Text("Position (Shift)");
        ImGui::SameLine(100);
        ImGui::PushItemWidth(60);
        if (ImGui::DragFloat("##RectX", &transform.RectCoordinates.x, 0.1f)) changed = true; ImGui::SameLine();
        if (ImGui::DragFloat("##RectY", &transform.RectCoordinates.y, 0.1f)) changed = true;
        ImGui::PopItemWidth();

        // Offsets
        ImGui::Text("Offsets");
        ImGui::SameLine(100);
        ImGui::PushItemWidth(50);
        if (ImGui::DragFloat("##OffsetMinX", &transform.OffsetMin.x, 1.0f)) changed = true; ImGui::SameLine();
        if (ImGui::DragFloat("##OffsetMinY", &transform.OffsetMin.y, 1.0f)) changed = true;
        ImGui::SameLine(); ImGui::Text("Min"); ImGui::SameLine();
        if (ImGui::DragFloat("##OffsetMaxX", &transform.OffsetMax.x, 1.0f)) changed = true; ImGui::SameLine();
        if (ImGui::DragFloat("##OffsetMaxY", &transform.OffsetMax.y, 1.0f)) changed = true;
        ImGui::PopItemWidth();

        // Pivot
        ImGui::Text("Pivot");
        ImGui::SameLine(100);
        ImGui::PushItemWidth(60);
        if (ImGui::DragFloat("##PivotX", &transform.Pivot.x, 0.01f)) changed = true; ImGui::SameLine();
        if (ImGui::DragFloat("##PivotY", &transform.Pivot.y, 0.01f)) changed = true;
        ImGui::PopItemWidth();

        return changed;
    }

    static bool DrawUIStyle(UIStyle &style)
    {
        bool changed = false;
        if (GUI::Property("BackgroundColor", style.BackgroundColor)) changed = true;
        if (GUI::Property("Rounding", style.Rounding)) changed = true;
        return changed;
    }

    void PropertyEditor::Init()
    {
        // --- Core & Rendering ---
        Register<TransformComponent>("Transform",
                                     [](auto &comp)
                                     {
                                         bool changed = false;
                                         GUI::BeginProperties();
                                         if (GUI::DrawVec3Control("Translation", comp.Translation)) changed = true;
                                         Vector3 rot = Vector3Scale(comp.Rotation, RAD2DEG);
                                         if (GUI::DrawVec3Control("Rotation", rot))
                                         {
                                             comp.Rotation = Vector3Scale(rot, DEG2RAD);
                                             changed = true;
                                         }
                                         if (GUI::DrawVec3Control("Scale", comp.Scale, 1.0f)) changed = true;
                                         GUI::EndProperties();
                                         return changed;
                                     });

        Register<ModelComponent>("Model",
                                 [](auto &comp)
                                 {
                                     bool changed = false;
                                     GUI::BeginProperties();
                                     if (GUI::Property("Path", comp.ModelPath)) changed = true;
                                     GUI::EndProperties();
                                     return changed;
                                 });

        Register<PointLightComponent>("Point Light",
                                      [](auto &comp)
                                      {
                                          bool changed = false;
                                          GUI::BeginProperties();
                                          if (GUI::Property("Radiance", comp.Radiance)) changed = true;
                                          if (GUI::Property("Radius", comp.Radius)) changed = true;
                                          if (GUI::Property("Falloff", comp.Falloff)) changed = true;
                                          if (GUI::Property("Color", comp.LightColor)) changed = true;
                                          GUI::EndProperties();
                                          return changed;
                                      });

        Register<AnimationComponent>("Animation",
                                     [](auto &comp)
                                     {
                                         bool changed = false;
                                         GUI::BeginProperties();
                                         if (GUI::Property("Playing", comp.IsPlaying)) changed = true;
                                         GUI::EndProperties();
                                         return changed;
                                     });

        // --- Physics ---
        Register<ColliderComponent>("Collider",
                                    [](auto &comp)
                                    {
                                        bool changed = false;
                                        GUI::BeginProperties();
                                        if (GUI::Property("Enabled", comp.Enabled)) changed = true;
                                        if (GUI::DrawVec3Control("Offset", comp.Offset)) changed = true;
                                        if (GUI::DrawVec3Control("Size", comp.Size, 1.0f)) changed = true;
                                        if (GUI::Property("Auto Calculate", comp.AutoCalculate)) changed = true;
                                        GUI::EndProperties();
                                        return changed;
                                    });

        Register<RigidBodyComponent>("RigidBody",
                                     [](auto &comp)
                                     {
                                         bool changed = false;
                                         GUI::BeginProperties();
                                         if (GUI::Property("Mass", comp.Mass)) changed = true;
                                         if (GUI::Property("Gravity", comp.UseGravity)) changed = true;
                                         if (GUI::Property("Kinematic", comp.IsKinematic)) changed = true;
                                         GUI::EndProperties();
                                         return changed;
                                     });

        // --- Scripts & Gameplay ---
        Register<PlayerComponent>("Player",
                                  [](auto &comp)
                                  {
                                      bool changed = false;
                                      GUI::BeginProperties();
                                      if (GUI::Property("Speed", comp.MovementSpeed)) changed = true;
                                      if (GUI::Property("Jump", comp.JumpForce)) changed = true;
                                      if (GUI::Property("Sensitivity", comp.LookSensitivity)) changed = true;
                                      GUI::EndProperties();
                                      return changed;
                                  });

        Register<AudioComponent>("Audio",
                                 [](auto &comp)
                                 {
                                     bool changed = false;
                                     GUI::BeginProperties();
                                     if (GUI::Property("Path", comp.SoundPath)) changed = true;
                                     if (GUI::Property("Volume", comp.Volume)) changed = true;
                                     GUI::EndProperties();
                                     return changed;
                                 });

        Register<NativeScriptComponent>("Native Script",
                                        [](auto &comp)
                                        {
                                            for (auto &script : comp.Scripts)
                                            {
                                                ImGui::TextDisabled("Script:");
                                                ImGui::SameLine();
                                                ImGui::Text("%s", script.ScriptName.c_str());
                                            }
                                            return false;
                                        });

        Register<SpawnComponent>("Spawn",
                                 [](auto &comp)
                                 {
                                     bool changed = false;
                                     GUI::BeginProperties();
                                     if (GUI::Property("Active", comp.IsActive)) changed = true;
                                     if (GUI::DrawVec3Control("Zone Size", comp.ZoneSize, 1.0f)) changed = true;
                                     if (GUI::DrawVec3Control("Spawn Point", comp.SpawnPoint)) changed = true;
                                     GUI::EndProperties();
                                     return changed;
                                 });

        // --- UI Widgets (Classic Look) ---
        Register<ControlComponent>("Widget Component",
                                   [](auto &comp)
                                   {
                                       bool changed = false;
                                       GUI::BeginProperties();
                                       if (GUI::Property("Z Order", comp.ZOrder)) changed = true;
                                       GUI::EndProperties();

                                       if (DrawRectTransform(comp.Transform)) changed = true;

                                       GUI::BeginProperties();
                                       if (GUI::Property("Is Active", comp.IsActive)) changed = true;
                                       if (GUI::Property("Hide in Hierarchy", comp.HiddenInHierarchy)) changed = true;
                                       GUI::EndProperties();
                                       return changed;
                                   });

        Register<ButtonControl>("Button Widget",
                                 [](auto &comp)
                                 {
                                     bool changed = false;
                                     GUI::BeginProperties();
                                     if (GUI::Property("Label", comp.Label)) changed = true;
                                     if (GUI::Property("Interactable", comp.IsInteractable)) changed = true;

                                     // Button Actions
                                     const char* actions[] = { "None", "Load Scene", "Quit" };
                                     int currentAction = (int)comp.Action;
                                     if (GUI::Property("On Click", currentAction, actions, 3))
                                     {
                                         comp.Action = (ButtonAction)currentAction;
                                         changed = true;
                                     }

                                     if (comp.Action == ButtonAction::LoadScene)
                                     {
                                         if (GUI::Property("Target Scene", comp.TargetScene)) changed = true;
                                     }
                                     
                                     ImGui::Text("Pressed State");
                                     ImGui::NextColumn();
                                     ImGui::TextDisabled(comp.IsDown ? "PRESSED" : "IDLE");
                                     ImGui::NextColumn();

                                     GUI::EndProperties();
                                     return changed;
                                 });

        Register<PanelControl>("Panel Widget",
                                [](auto &comp)
                                {
                                    bool changed = false;
                                    GUI::BeginProperties();
                                    if (DrawUIStyle(comp.Style)) changed = true;
                                    GUI::EndProperties();
                                    return changed;
                                });

        Register<LabelControl>("Label Widget",
                                [](auto &comp)
                                {
                                    bool changed = false;
                                    GUI::BeginProperties();
                                    if (GUI::Property("Text", comp.Text)) changed = true;
                                    GUI::EndProperties();
                                    return changed;
                                });

        Register<SliderControl>("Slider Widget",
                                 [](auto &comp)
                                 {
                                     bool changed = false;
                                     GUI::BeginProperties();
                                     if (GUI::Property("Min", comp.Min)) changed = true;
                                     if (GUI::Property("Max", comp.Max)) changed = true;
                                     if (GUI::Property("Value", comp.Value)) changed = true;
                                     GUI::EndProperties();
                                     return changed;
                                 });

        Register<CheckboxControl>("Checkbox Widget",
                                  [](auto &comp)
                                  {
                                      bool changed = false;
                                      GUI::BeginProperties();
                                      if (GUI::Property("Value", comp.Checked)) changed = true;
                                      GUI::EndProperties();
                                      return changed;
                                  });

        // Hide UI internal components from generic "Add Component" menu
        s_ComponentRegistry[entt::type_hash<ControlComponent>::value()].AllowAdd = false;
        s_ComponentRegistry[entt::type_hash<PanelControl>::value()].AllowAdd = false;
        s_ComponentRegistry[entt::type_hash<LabelControl>::value()].AllowAdd = false;
        s_ComponentRegistry[entt::type_hash<ButtonControl>::value()].AllowAdd = false;
        s_ComponentRegistry[entt::type_hash<SliderControl>::value()].AllowAdd = false;
        s_ComponentRegistry[entt::type_hash<CheckboxControl>::value()].AllowAdd = false;
    }

    void PropertyEditor::DrawEntityProperties(Entity entity)
    {
        auto &registry = entity.GetScene()->GetRegistry();
        bool isUI = entity.HasComponent<ControlComponent>();

        for (auto [id, storage] : registry.storage())
        {
            if (storage.contains(entity))
            {
                if (s_ComponentRegistry.find(id) != s_ComponentRegistry.end())
                {
                    auto &metadata = s_ComponentRegistry[id];
                    if (!metadata.Visible)
                        continue;

                    // Special Rule: Hide Transform for UI elements if you want to declutter
                    if (isUI && id == entt::type_hash<TransformComponent>::value())
                        continue;

                    ImGui::PushID((int)id);
                    metadata.Draw(entity);
                    ImGui::PopID();
                }
            }
        }
    }

    void PropertyEditor::DrawTag(Entity entity)
    {
        if (entity.HasComponent<TagComponent>())
        {
            auto &tag = entity.GetComponent<TagComponent>().Tag;
            char buffer[256];
            memset(buffer, 0, sizeof(buffer));
            strncpy(buffer, tag.c_str(), sizeof(buffer) - 1);

            ImGui::Text("Tag");
            ImGui::SameLine();
            if (ImGui::InputText("##Tag", buffer, sizeof(buffer)))
                tag = std::string(buffer);
        }
    }

    void PropertyEditor::DrawMaterial(Entity entity, int hitMeshIndex)
    {
        // Reserved for future material property editing
    }

    void PropertyEditor::DrawAddComponentPopup(Entity entity)
    {
        if (ImGui::BeginPopup("AddComponent"))
        {
            for (auto &[id, metadata] : s_ComponentRegistry)
            {
                if (!metadata.AllowAdd)
                    continue;

                auto &registry = entity.GetScene()->GetRegistry();
                auto *storage = registry.storage(id);
                if (storage && storage->contains(entity))
                    continue;

                if (ImGui::MenuItem(metadata.Name.c_str()))
                {
                    metadata.Add(entity);
                    ImGui::CloseCurrentPopup();
                }
            }
            ImGui::EndPopup();
        }
    }

} // namespace CHEngine
