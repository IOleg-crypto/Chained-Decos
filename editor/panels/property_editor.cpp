#include "property_editor.h"
#include "editor/ui/editor_gui.h"
#include "engine/scene/components.h"
#include "engine/scene/script_registry.h"
#include "extras/IconsFontAwesome6.h"
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

    static bool DrawTextStyle(TextStyle &style)
    {
        PropertyEditor::MetaBuilder mb(style);
        if (ImGui::TreeNodeEx("Text Style", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen))
        {
            mb.Prop("Font Size", style.FontSize).Prop("Text Color", style.TextColor);

            const char *alignments[] = {"Left", "Center", "Right"};
            int hAlign = (int)style.HorizontalAlignment;
            if (mb.Prop("H Align", hAlign, alignments, 3)) style.HorizontalAlignment = (TextAlignment)hAlign;

            int vAlign = (int)style.VerticalAlignment;
            if (mb.Prop("V Align", vAlign, alignments, 3)) style.VerticalAlignment = (TextAlignment)vAlign;

            mb.Prop("Letter Spacing", style.LetterSpacing).Prop("Line Height", style.LineHeight);
            if (mb.Prop("Shadow", style.Shadow) && style.Shadow)
            {
                mb.Prop("Shadow Offset", style.ShadowOffset).Prop("Shadow Color", style.ShadowColor);
            }
            ImGui::TreePop();
        }
        return (bool)mb;
    }

    static bool DrawUIStyle(UIStyle &style)
    {
        PropertyEditor::MetaBuilder mb(style);
        if (ImGui::TreeNodeEx("Visual Style", ImGuiTreeNodeFlags_Framed))
        {
            mb.Prop("Background", style.BackgroundColor)
              .Prop("Hover", style.HoverColor)
              .Prop("Pressed", style.PressedColor)
              .Prop("Rounding", style.Rounding)
              .Prop("Border", style.BorderSize)
              .Prop("Border Color", style.BorderColor)
              .Prop("Padding", style.Padding);
            ImGui::TreePop();
        }
        return (bool)mb;
    }

    static bool DrawControlComponentUI(ControlComponent &comp)
    {
        bool changed = false;
        auto &rt = comp.Transform;

        // --- Anchor Presets ---
        ImGui::Text("Presets:"); ImGui::SameLine();
        if (ImGui::Button("Center")) {
            rt.AnchorMin = {0.5f, 0.5f}; rt.AnchorMax = {0.5f, 0.5f};
            rt.OffsetMin = {-50, -50}; rt.OffsetMax = {50, 50};
            changed = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Stretch")) {
            rt.AnchorMin = {0.0f, 0.0f}; rt.AnchorMax = {1.0f, 1.0f};
            rt.OffsetMin = {0, 0}; rt.OffsetMax = {0, 0};
            changed = true;
        }

        GUI::BeginProperties();
        // Use compact Property instead of Vec2Control to reduce X/Y label clutter
        if (GUI::Property("Pivot", rt.Pivot)) changed = true;
        if (GUI::Property("Anchor Min", rt.AnchorMin, 0.01f, 0.0f, 1.0f)) changed = true;
        if (GUI::Property("Anchor Max", rt.AnchorMax, 0.01f, 0.0f, 1.0f)) changed = true;

        bool isPoint = (rt.AnchorMin.x == rt.AnchorMax.x && rt.AnchorMin.y == rt.AnchorMax.y);
        if (isPoint)
        {
            float width = rt.OffsetMax.x - rt.OffsetMin.x;
            float height = rt.OffsetMax.y - rt.OffsetMin.y;
            float posX = rt.OffsetMin.x + width * rt.Pivot.x;
            float posY = rt.OffsetMin.y + height * rt.Pivot.y;
            
            glm::vec2 pos = {posX, posY};
            glm::vec2 size = {width, height};

            if (GUI::Property("Pos", pos)) {
                rt.OffsetMin.x = pos.x - size.x * rt.Pivot.x;
                rt.OffsetMin.y = pos.y - size.y * rt.Pivot.y;
                rt.OffsetMax.x = pos.x + size.x * (1.0f - rt.Pivot.x);
                rt.OffsetMax.y = pos.y + size.y * (1.0f - rt.Pivot.y);
                changed = true;
            }
            if (GUI::Property("Size", size)) {
                rt.OffsetMin.x = pos.x - size.x * rt.Pivot.x;
                rt.OffsetMin.y = pos.y - size.y * rt.Pivot.y;
                rt.OffsetMax.x = pos.x + size.x * (1.0f - rt.Pivot.x);
                rt.OffsetMax.y = pos.y + size.y * (1.0f - rt.Pivot.y);
                changed = true;
            }
        }
        else
        {
            // For stretch, show padding (intuitive positive values)
            float rightPadding = -rt.OffsetMax.x;
            float bottomPadding = -rt.OffsetMax.y;

            if (GUI::Property("Left", rt.OffsetMin.x)) changed = true;
            if (GUI::Property("Top", rt.OffsetMin.y)) changed = true;
            if (GUI::Property("Right", rightPadding)) { rt.OffsetMax.x = -rightPadding; changed = true; }
            if (GUI::Property("Bottom", bottomPadding)) { rt.OffsetMax.y = -bottomPadding; changed = true; }
        }
        GUI::EndProperties();

        if (ImGui::TreeNodeEx("Extra Layout Settings", ImGuiTreeNodeFlags_SpanAvailWidth))
        {
            GUI::BeginProperties();
            if (GUI::Property("Rotation", rt.Rotation)) changed = true;
            if (GUI::Property("Scale", rt.Scale)) changed = true;
            if (GUI::Property("Z Order", comp.ZOrder)) changed = true;
            if (GUI::Property("Visible", comp.IsActive)) changed = true;
            GUI::EndProperties();
            ImGui::TreePop();
        }
        return changed;
    }

    void PropertyEditor::Init()
    {
        // --- Core & Rendering ---
        Register<TransformComponent>("Transform", [](auto &comp) {
            MetaBuilder mb(comp);
            mb.Vec3("Translation", comp.Translation);
            Vector3 rot = Vector3Scale(comp.Rotation, RAD2DEG);
            if (mb.Vec3("Rotation", rot)) comp.Rotation = Vector3Scale(rot, DEG2RAD);
            mb.Vec3("Scale", comp.Scale, 1.0f);
            return (bool)mb;
        });

        Register<ModelComponent>("Model", [](auto &comp) {
            return (bool)MetaBuilder(comp).Prop("Path", comp.ModelPath);
        });

        Register<PointLightComponent>("Point Light", [](auto &comp) {
            MetaBuilder mb(comp);
            mb.Prop("Color", comp.LightColor).Prop("Intensity", comp.Intensity).Prop("Radius", comp.Radius);
            return (bool)mb;
        });

        Register<SpotLightComponent>("Spot Light", [](auto &comp) {
            MetaBuilder mb(comp);
            mb.Prop("Color", comp.LightColor).Prop("Intensity", comp.Intensity).Prop("Range", comp.Range);
            mb.Prop("Inner Cutoff", comp.InnerCutoff).Prop("Outer Cutoff", comp.OuterCutoff);
            return (bool)mb;
        });

        Register<AnimationComponent>("Animation", [](auto &comp) {
            return (bool)MetaBuilder(comp).Prop("Playing", comp.IsPlaying);
        });

        // --- Physics ---
        Register<ColliderComponent>("Collider", [](auto &comp) {
            MetaBuilder mb(comp);
            mb.Prop("Enabled", comp.Enabled);
            const char* types[] = { "Box", "Mesh (BVH)" };
            int currentType = (int)comp.Type;
            if (mb.Prop("Type", currentType, types, 2)) comp.Type = (ColliderType)currentType;

            if (comp.Type == ColliderType::Box) {
                mb.Vec3("Offset", comp.Offset);
                mb.Vec3("Size", comp.Size, 1.0f);
                mb.Prop("Auto Calculate", comp.AutoCalculate);
            } else {
                mb.Prop("Model Path", comp.ModelPath);
                ImGui::Text("BVH Status"); ImGui::NextColumn();
                ImGui::TextDisabled(comp.BVHRoot ? "Loaded" : "Not Built"); ImGui::NextColumn();
                if (ImGui::Button("Build/Rebuild BVH")) mb.Changed = true;
            }
            return (bool)mb;
        });

        Register<BillboardComponent>("Billboard", [](auto &comp) {
            MetaBuilder mb(comp);
            mb.Prop("Icon Texture", comp.TexturePath).Prop("Tint", comp.Tint).Prop("Size", comp.Size);
            return (bool)mb;
        });


        Register<RigidBodyComponent>("RigidBody", [](auto &comp) {
            MetaBuilder mb(comp);
            mb.Prop("Mass", comp.Mass).Prop("Gravity", comp.UseGravity).Prop("Kinematic", comp.IsKinematic);
            return (bool)mb;
        });

        Register<SceneTransitionComponent>("Scene Transition", [](auto &comp) {
            MetaBuilder mb(comp);
            mb.Prop("Target Scene", comp.TargetScenePath).Prop("Triggered", comp.Triggered);
            return (bool)mb;
        });

        Register<AudioComponent>("Audio", [](auto &comp) {
            MetaBuilder mb(comp);
            mb.Prop("Path", comp.SoundPath).Prop("Volume", comp.Volume);
            return (bool)mb;
        });

        Register<NativeScriptComponent>("Native Script", [](auto &comp) {
            bool changed = false;
            for (size_t i = 0; i < comp.Scripts.size(); ++i) {
                auto& script = comp.Scripts[i];
                ImGui::TextDisabled("Script: %s", script.ScriptName.c_str());
                ImGui::SameLine();
                ImGui::PushID((int)i);
                if (ImGui::Button(ICON_FA_TRASH)) {
                    comp.Scripts.erase(comp.Scripts.begin() + i);
                    changed = true;
                    ImGui::PopID(); break; 
                }
                ImGui::PopID();
            }
            if (ImGui::Button("Add Script")) ImGui::OpenPopup("AddScriptPopup");
            if (ImGui::BeginPopup("AddScriptPopup")) {
                for (const auto& [name, funcs] : ScriptRegistry::GetScripts()) {
                    if (ImGui::MenuItem(name.c_str())) {
                        ScriptRegistry::AddScript(name, comp); changed = true;
                    }
                }
                ImGui::EndPopup();
            }
            return changed;
        });

        // --- UI Widgets (Classic Look) ---
        Register<ControlComponent>("Rect Transform",
                                   [](auto &comp, Entity e)
                                   {
                                       return DrawControlComponentUI(comp);
                                   });

        Register<ButtonControl>("Button Widget",
                                 [](auto &comp, Entity e)
                                 {
                                     bool changed = false;
                                     GUI::BeginProperties();
                                     if (GUI::Property("Label", comp.Label)) changed = true;
                                     if (GUI::Property("Interactable", comp.IsInteractable)) changed = true;
                                     GUI::EndProperties();

                                     if (DrawUIStyle(comp.Style)) changed = true;
                                     if (DrawTextStyle(comp.Text)) changed = true;
                                     
                                     if (e.HasComponent<ControlComponent>())
                                     {
                                         if (ImGui::TreeNodeEx("Layout", ImGuiTreeNodeFlags_Framed))
                                         {
                                             if (DrawControlComponentUI(e.GetComponent<ControlComponent>())) changed = true;
                                             ImGui::TreePop();
                                         }
                                     }
                                     return changed;
                                 });

        Register<PanelControl>("Panel Widget",
                                [](auto &comp, Entity e)
                                {
                                    bool changed = false;
                                    GUI::BeginProperties();
                                    if (GUI::Property("Fullscreen", comp.FullScreen)) changed = true;
                                    GUI::EndProperties();

                                    if (DrawUIStyle(comp.Style)) changed = true;

                                    if (e.HasComponent<ControlComponent>())
                                    {
                                        if (ImGui::TreeNodeEx("Layout", ImGuiTreeNodeFlags_Framed))
                                        {
                                            if (DrawControlComponentUI(e.GetComponent<ControlComponent>())) changed = true;
                                            ImGui::TreePop();
                                        }
                                    }
                                    return changed;
                                });

        Register<LabelControl>("Label Widget",
                                [](auto &comp, Entity e)
                                {
                                    bool changed = false;
                                    GUI::BeginProperties();
                                    if (GUI::Property("Text", comp.Text)) changed = true;
                                    GUI::EndProperties();

                                    if (DrawTextStyle(comp.Style)) changed = true;

                                    if (e.HasComponent<ControlComponent>())
                                    {
                                        if (ImGui::TreeNodeEx("Layout", ImGuiTreeNodeFlags_Framed))
                                        {
                                            if (DrawControlComponentUI(e.GetComponent<ControlComponent>())) changed = true;
                                            ImGui::TreePop();
                                        }
                                    }
                                    return changed;
                                });

        Register<SliderControl>("Slider Widget",
                                 [](auto &comp, Entity e)
                                 {
                                     bool changed = false;
                                     GUI::BeginProperties();
                                     if (GUI::Property("Label", comp.Label)) changed = true;
                                     if (GUI::Property("Min", comp.Min)) changed = true;
                                     if (GUI::Property("Max", comp.Max)) changed = true;
                                     if (GUI::Property("Value", comp.Value)) changed = true;
                                     GUI::EndProperties();
                                     
                                     if (DrawUIStyle(comp.Style)) changed = true;
                                     if (DrawTextStyle(comp.Text)) changed = true;

                                     if (e.HasComponent<ControlComponent>())
                                     {
                                         if (ImGui::TreeNodeEx("Layout", ImGuiTreeNodeFlags_Framed))
                                         {
                                             if (DrawControlComponentUI(e.GetComponent<ControlComponent>())) changed = true;
                                             ImGui::TreePop();
                                         }
                                     }
                                     return changed;
                                 });

        Register<CheckboxControl>("Checkbox Widget",
                                  [](auto &comp, Entity e)
                                  {
                                      bool changed = false;
                                      GUI::BeginProperties();
                                      if (GUI::Property("Label", comp.Label)) changed = true;
                                      if (GUI::Property("Value", comp.Checked)) changed = true;
                                      GUI::EndProperties();

                                      if (DrawUIStyle(comp.Style)) changed = true;
                                      if (DrawTextStyle(comp.Text)) changed = true;

                                      if (e.HasComponent<ControlComponent>())
                                      {
                                          if (ImGui::TreeNodeEx("Layout", ImGuiTreeNodeFlags_Framed))
                                          {
                                              if (DrawControlComponentUI(e.GetComponent<ControlComponent>())) changed = true;
                                              ImGui::TreePop();
                                          }
                                      }
                                      return changed;
                                  });

        // Hide UI internal components from generic "Add Component" menu
        s_ComponentRegistry[entt::type_hash<ControlComponent>::value()].AllowAdd = false;
        
        // Mark Widgets
        s_ComponentRegistry[entt::type_hash<PanelControl>::value()].IsWidget = true;
        s_ComponentRegistry[entt::type_hash<LabelControl>::value()].IsWidget = true;
        s_ComponentRegistry[entt::type_hash<ButtonControl>::value()].IsWidget = true;
        s_ComponentRegistry[entt::type_hash<SliderControl>::value()].IsWidget = true;
        s_ComponentRegistry[entt::type_hash<CheckboxControl>::value()].IsWidget = true;

        s_ComponentRegistry[entt::type_hash<PanelControl>::value()].AllowAdd = false;
        s_ComponentRegistry[entt::type_hash<LabelControl>::value()].AllowAdd = false;
        s_ComponentRegistry[entt::type_hash<ButtonControl>::value()].AllowAdd = false;
        s_ComponentRegistry[entt::type_hash<SliderControl>::value()].AllowAdd = false;
        s_ComponentRegistry[entt::type_hash<CheckboxControl>::value()].AllowAdd = false;
    }

    void PropertyEditor::DrawEntityProperties(CHEngine::Entity entity)
    {
        auto &registry = entity.GetScene()->GetRegistry();
        bool isUI = entity.HasComponent<ControlComponent>();
        
        bool hasWidget = false;
        for (auto [id, storage] : registry.storage())
        {
            if (storage.contains(entity) && s_ComponentRegistry.contains(id))
            {
                if (s_ComponentRegistry[id].IsWidget)
                {
                    hasWidget = true;
                    break;
                }
            }
        }

        for (auto [id, storage] : registry.storage())
        {
            if (storage.contains(entity))
            {
                if (s_ComponentRegistry.find(id) != s_ComponentRegistry.end())
                {
                    auto &metadata = s_ComponentRegistry[id];
                    if (!metadata.Visible)
                        continue;

                    // Logic to reduce clutter
                    if (isUI && id == entt::type_hash<TransformComponent>::value())
                        continue;
                    
                    if (hasWidget && id == entt::type_hash<ControlComponent>::value())
                        continue;

                    ImGui::PushID((int)id);
                    metadata.Draw(entity);
                    ImGui::PopID();
                }
            }
        }
    }

    void PropertyEditor::DrawTag(CHEngine::Entity entity)
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

    void PropertyEditor::DrawMaterial(CHEngine::Entity entity, int hitMeshIndex)
    {
    }

    void PropertyEditor::DrawAddComponentPopup(CHEngine::Entity entity)
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
