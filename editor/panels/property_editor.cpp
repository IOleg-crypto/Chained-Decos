#include "property_editor.h"
#include "editor_gui.h"
#include "engine/scene/components.h"
#include "engine/scene/script_registry.h"
#include "extras/IconsFontAwesome6.h"
#include "imgui.h"
#include "nfd.h"
#include "raymath.h"
#include "engine/graphics/asset_manager.h"
#include "engine/graphics/model_asset.h"
#include "engine/scene/project.h"

namespace CHEngine
{

    std::unordered_map<entt::id_type, PropertyEditor::ComponentMetadata> PropertyEditor::s_ComponentRegistry;

    void PropertyEditor::RegisterComponent(entt::id_type typeId, const ComponentMetadata &metadata)
    {
        s_ComponentRegistry[typeId] = metadata;
    }

    static bool DrawTextStyle(TextStyle &style)
    {
        auto pb = EditorGUI::Begin();
        pb.Float("Font Size", style.FontSize).Color("Text Color", style.TextColor);

        const char *alignments[] = {"Left", "Center", "Right"};
        int hAlign = (int)style.HorizontalAlignment;
        if (EditorGUI::Property("H Align", hAlign, alignments, 3)) style.HorizontalAlignment = (TextAlignment)hAlign;

        int vAlign = (int)style.VerticalAlignment;
        if (EditorGUI::Property("V Align", vAlign, alignments, 3)) style.VerticalAlignment = (TextAlignment)vAlign;

        pb.Float("Letter Spacing", style.LetterSpacing).Float("Line Height", style.LineHeight);
        if (pb.Bool("Shadow", style.Shadow) && style.Shadow)
        {
            pb.Float("Shadow Offset", style.ShadowOffset).Color("Shadow Color", style.ShadowColor);
        }
        return pb.Changed;
    }

    static bool DrawUIStyle(UIStyle &style)
    {
        auto pb = EditorGUI::Begin();
        pb.Color("Background", style.BackgroundColor)
          .Color("Hover", style.HoverColor)
          .Color("Pressed", style.PressedColor)
          .Float("Rounding", style.Rounding)
          .Float("Border", style.BorderSize)
          .Color("Border Color", style.BorderColor)
          .Float("Padding", style.Padding);
        return pb.Changed;
    }

    static bool DrawTransform(TransformComponent& comp)
    {
        bool changed = false;
        changed |= EditorGUI::DrawVec3("Position", comp.Translation);
        changed |= EditorGUI::DrawVec3("Rotation", comp.Rotation);
        changed |= EditorGUI::DrawVec3("Scale", comp.Scale, 1.0f);
        return changed;
    }

    static bool DrawAudio(AudioComponent& comp)
    {
        auto pb = EditorGUI::Begin();
        pb.String("Sound Path", comp.SoundPath)
          .Bool("Loop", comp.Loop)
          .Bool("Play On Start", comp.PlayOnStart)
          .Float("Volume", comp.Volume, 0.05f, 0.0f, 2.0f)
          .Float("Pitch", comp.Pitch, 0.05f, 0.1f, 5.0f);
        return pb.Changed;
    }

    static bool DrawPointLight(PointLightComponent& comp)
    {
        auto pb = EditorGUI::Begin();
        pb.Color("Color", comp.LightColor)
          .Float("Intensity", comp.Intensity, 0.1f, 0.0f, 100.0f)
          .Float("Radius", comp.Radius, 0.1f, 0.0f, 1000.0f);
        return pb.Changed;
    }

    static bool DrawCamera(CameraComponent& comp)
    {
        auto pb = EditorGUI::Begin();
        pb.Float("FOV", comp.Fov, 1.0f, 1.0f, 120.0f)
          .Float("Near", comp.NearPlane, 0.1f)
          .Float("Far", comp.FarPlane, 1.0f);
        return pb.Changed;
    }

    static bool DrawRigidBody(RigidBodyComponent& comp)
    {
        auto pb = EditorGUI::Begin();
        pb.Float("Mass", comp.Mass, 0.1f, 0.0f)
          .Bool("Use Gravity", comp.UseGravity);
        return pb.Changed;
    }

    static bool DrawSpawn(SpawnComponent& comp)
    {
        auto pb = EditorGUI::Begin();
        pb.Bool("Active", comp.IsActive)
          .Vec3("Zone Size", comp.ZoneSize)
          .Vec3("Spawn Point", comp.SpawnPoint);
        return pb.Changed;
    }

    static bool DrawPlayer(PlayerComponent& comp)
    {
        auto pb = EditorGUI::Begin();
        pb.Float("Speed", comp.MovementSpeed)
          .Float("Sensitivity", comp.LookSensitivity)
          .Float("Jump Force", comp.JumpForce);
        return pb.Changed;
    }

    static bool DrawModel(ModelComponent& comp)
    {
        bool changed = false;
        if (EditorGUI::Property("Path", comp.ModelPath)) changed = true;
        
        if (ImGui::Button(ICON_FA_FOLDER_OPEN " Browse##Model"))
        {
            nfdchar_t *outPath = nullptr;
            nfdfilteritem_t filterItem[1] = {{"3D Models", "glb,gltf,obj,fbx"}};
            nfdresult_t result = NFD_OpenDialog(&outPath, filterItem, 1, nullptr);
            if (result == NFD_OKAY && outPath)
            {
                comp.ModelPath = outPath;
                comp.Asset = nullptr;
                NFD_FreePath(outPath);
                changed = true;
            }
        }
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_ROTATE " Reload##Model"))
        {
            if (auto project = Project::GetActive())
                project->GetAssetManager()->Remove<ModelAsset>(comp.ModelPath);
            comp.Asset = nullptr;
            changed = true;
        }

        if (!comp.Materials.empty())
        {
            if (ImGui::TreeNodeEx("Materials", ImGuiTreeNodeFlags_SpanAvailWidth))
            {
                for (size_t i = 0; i < comp.Materials.size(); i++)
                {
                    auto& slot = comp.Materials[i];
                    std::string label = slot.Name + "##" + std::to_string(i);
                    if (ImGui::TreeNodeEx(label.c_str()))
                    {
                        auto pb = EditorGUI::Begin();
                        pb.Color("Albedo Color", slot.Material.AlbedoColor)
                          .Float("Metalness", slot.Material.Metalness, 0.01f, 0.0f, 1.0f)
                          .Float("Roughness", slot.Material.Roughness, 0.01f, 0.0f, 1.0f)
                          .Bool("Double Sided", slot.Material.DoubleSided)
                          .Bool("Transparent", slot.Material.Transparent);
                        if (slot.Material.Transparent)
                            pb.Float("Alpha", slot.Material.Alpha, 0.01f, 0.0f, 1.0f);
                        
                        if (pb.Changed) changed = true;
                        ImGui::TreePop();
                    }
                }
                ImGui::TreePop();
            }
        }
        return changed;
    }

    static bool DrawLight(PointLightComponent& comp)
    {
        return DrawPointLight(comp);
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

        // Use compact Property instead of Vec2Control to reduce X/Y label clutter
        if (EditorGUI::Property("Pivot", rt.Pivot)) changed = true;
        if (EditorGUI::Property("Anchor Min", rt.AnchorMin, 0.01f, 0.0f, 1.0f)) changed = true;
        if (EditorGUI::Property("Anchor Max", rt.AnchorMax, 0.01f, 0.0f, 1.0f)) changed = true;

        bool isPoint = (rt.AnchorMin.x == rt.AnchorMax.x && rt.AnchorMin.y == rt.AnchorMax.y);
        if (isPoint)
        {
            float width = rt.OffsetMax.x - rt.OffsetMin.x;
            float height = rt.OffsetMax.y - rt.OffsetMin.y;
            float posX = rt.OffsetMin.x + width * rt.Pivot.x;
            float posY = rt.OffsetMin.y + height * rt.Pivot.y;
            
            glm::vec2 pos = {posX, posY};
            glm::vec2 size = {width, height};

            if (EditorGUI::Property("Pos", pos)) {
                rt.OffsetMin.x = pos.x - size.x * rt.Pivot.x;
                rt.OffsetMin.y = pos.y - size.y * rt.Pivot.y;
                rt.OffsetMax.x = pos.x + size.x * (1.0f - rt.Pivot.x);
                rt.OffsetMax.y = pos.y + size.y * (1.0f - rt.Pivot.y);
                changed = true;
            }
            if (EditorGUI::Property("Size", size)) {
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

            if (EditorGUI::Property("Left", rt.OffsetMin.x)) changed = true;
            if (EditorGUI::Property("Top", rt.OffsetMin.y)) changed = true;
            if (EditorGUI::Property("Right", rightPadding)) { rt.OffsetMax.x = -rightPadding; changed = true; }
            if (EditorGUI::Property("Bottom", bottomPadding)) { rt.OffsetMax.y = -bottomPadding; changed = true; }
        }

        if (ImGui::TreeNodeEx("Extra Layout Settings", ImGuiTreeNodeFlags_SpanAvailWidth))
        {
            if (EditorGUI::Property("Rotation", rt.Rotation)) changed = true;
            if (EditorGUI::Property("Scale", rt.Scale)) changed = true;
            if (EditorGUI::Property("Z Order", comp.ZOrder)) changed = true;
            if (EditorGUI::Property("Visible", comp.IsActive)) changed = true;
            ImGui::TreePop();
        }
        return changed;
    }

    static bool DrawCollider(ColliderComponent& comp)
    {
        auto pb = EditorGUI::Begin();
        pb.Bool("Enabled", comp.Enabled);
        const char* types[] = { "Box", "Mesh (BVH)" };
        int currentType = (int)comp.Type;
        if (EditorGUI::Property("Type", currentType, types, 2)) comp.Type = (ColliderType)currentType;

        if (comp.Type == ColliderType::Box) {
            pb.Vec3("Offset", comp.Offset);
            pb.Vec3("Size", comp.Size);
            pb.Bool("Auto Calculate", comp.AutoCalculate);
        } else {
            pb.String("Model Path", comp.ModelPath);
            ImGui::TextDisabled("BVH Status: %s", comp.BVHRoot ? "Loaded" : "Not Built");
            if (ImGui::Button("Build/Rebuild BVH")) pb.Changed = true;
        }
        return pb.Changed;
    }


    static bool DrawControlWidget(ControlComponent& comp)
    {
        return DrawControlComponentUI(comp);
    }

    static bool DrawButtonWidget(ButtonControl& comp, Entity e)
    {
        bool changed = EditorGUI::Begin()
            .String("Label", comp.Label)
            .Bool("Interactable", comp.IsInteractable);
        
        if (ImGui::TreeNodeEx("Visual Style", ImGuiTreeNodeFlags_Framed))
        {
            if (DrawUIStyle(comp.Style)) changed = true;
            ImGui::TreePop();
        }

        if (ImGui::TreeNodeEx("Text Style", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (DrawTextStyle(comp.Text)) changed = true;
            ImGui::TreePop();
        }

        if (e.HasComponent<ControlComponent>())
        {
            if (ImGui::TreeNodeEx("Layout", ImGuiTreeNodeFlags_Framed))
            {
                if (DrawControlComponentUI(e.GetComponent<ControlComponent>())) changed = true;
                ImGui::TreePop();
            }
        }
        return changed;
    }

    static bool DrawPanelWidget(PanelControl& comp, Entity e)
    {
        bool changed = false;
        auto pb = EditorGUI::Begin();
        pb.String("Texture Path", comp.TexturePath)
          .Bool("Full Screen", comp.FullScreen);
        if (pb.Changed) changed = true;

        if (ImGui::TreeNodeEx("Style", ImGuiTreeNodeFlags_Framed))
        {
            if (DrawUIStyle(comp.Style)) changed = true;
            ImGui::TreePop();
        }

        if (e.HasComponent<ControlComponent>())
        {
            if (ImGui::TreeNodeEx("Layout", ImGuiTreeNodeFlags_Framed))
            {
                if (DrawControlComponentUI(e.GetComponent<ControlComponent>())) changed = true;
                ImGui::TreePop();
            }
        }
        return changed;
    }

    static bool DrawLabelWidget(LabelControl& comp, Entity e)
    {
        bool changed = false;
        auto pb = EditorGUI::Begin();
        pb.String("Text", comp.Text);
        if (pb.Changed) changed = true;

        if (ImGui::TreeNodeEx("Style", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (DrawTextStyle(comp.Style)) changed = true;
            ImGui::TreePop();
        }

        if (e.HasComponent<ControlComponent>())
        {
            if (ImGui::TreeNodeEx("Layout", ImGuiTreeNodeFlags_Framed))
            {
                if (DrawControlComponentUI(e.GetComponent<ControlComponent>())) changed = true;
                ImGui::TreePop();
            }
        }
        return changed;
    }

    static bool DrawSliderWidget(SliderControl& comp, Entity e)
    {
        bool changed = false;
        auto pb = EditorGUI::Begin();
        pb.String("Label", comp.Label)
          .Float("Value", comp.Value)
          .Float("Min", comp.Min)
          .Float("Max", comp.Max);
        if (pb.Changed) changed = true;

        if (ImGui::TreeNodeEx("Style", ImGuiTreeNodeFlags_Framed))
        {
            if (DrawUIStyle(comp.Style)) changed = true;
            ImGui::TreePop();
        }

        if (e.HasComponent<ControlComponent>())
        {
            if (ImGui::TreeNodeEx("Layout", ImGuiTreeNodeFlags_Framed))
            {
                if (DrawControlComponentUI(e.GetComponent<ControlComponent>())) changed = true;
                ImGui::TreePop();
            }
        }
        return changed;
    }

    static bool DrawCheckboxWidget(CheckboxControl& comp, Entity e)
    {
        bool changed = false;
        auto pb = EditorGUI::Begin();
        pb.String("Label", comp.Label)
          .Bool("Checked", comp.Checked);
        if (pb.Changed) changed = true;

        if (ImGui::TreeNodeEx("Style", ImGuiTreeNodeFlags_Framed))
        {
            if (DrawUIStyle(comp.Style)) changed = true;
            ImGui::TreePop();
        }

        if (e.HasComponent<ControlComponent>())
        {
            if (ImGui::TreeNodeEx("Layout", ImGuiTreeNodeFlags_Framed))
            {
                if (DrawControlComponentUI(e.GetComponent<ControlComponent>())) changed = true;
                ImGui::TreePop();
            }
        }
        return changed;
    }

    static bool DrawInputTextWidget(InputTextControl& comp, Entity e)
    {
        bool changed = false;
        auto pb = EditorGUI::Begin();
        pb.String("Label", comp.Label)
          .String("Text", comp.Text)
          .String("Placeholder", comp.Placeholder)
          .Int("Max Length", comp.MaxLength)
          .Bool("Multiline", comp.Multiline)
          .Bool("Read Only", comp.ReadOnly)
          .Bool("Password", comp.Password);
        if (pb.Changed) changed = true;

        if (ImGui::TreeNodeEx("Text Style", ImGuiTreeNodeFlags_Framed))
        {
            if (DrawTextStyle(comp.Style)) changed = true;
            ImGui::TreePop();
        }

        if (ImGui::TreeNodeEx("Box Style", ImGuiTreeNodeFlags_Framed))
        {
            if (DrawUIStyle(comp.BoxStyle)) changed = true;
            ImGui::TreePop();
        }

        if (e.HasComponent<ControlComponent>())
        {
            if (ImGui::TreeNodeEx("Layout", ImGuiTreeNodeFlags_Framed))
            {
                if (DrawControlComponentUI(e.GetComponent<ControlComponent>())) changed = true;
                ImGui::TreePop();
            }
        }
        return changed;
    }

    static bool DrawComboBoxWidget(ComboBoxControl& comp, Entity e)
    {
        bool changed = false;
        auto pb = EditorGUI::Begin();
        pb.String("Label", comp.Label)
          .Int("Selected Index", comp.SelectedIndex);
        if (pb.Changed) changed = true;

        if (ImGui::TreeNodeEx("Items", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen))
        {
            for (int i = 0; i < comp.Items.size(); i++)
            {
                ImGui::PushID(i);
                char buf[256];
                strncpy(buf, comp.Items[i].c_str(), sizeof(buf) - 1);
                if (ImGui::InputText("##item", buf, sizeof(buf)))
                {
                    comp.Items[i] = buf;
                    changed = true;
                }
                ImGui::SameLine();
                if (ImGui::Button("X"))
                {
                    comp.Items.erase(comp.Items.begin() + i);
                    changed = true;
                    ImGui::PopID();
                    break;
                }
                ImGui::PopID();
            }
            if (ImGui::Button("Add Item"))
            {
                comp.Items.push_back("New Option");
                changed = true;
            }
            ImGui::TreePop();
        }

        if (ImGui::TreeNodeEx("Text Style", ImGuiTreeNodeFlags_Framed))
        {
            if (DrawTextStyle(comp.Style)) changed = true;
            ImGui::TreePop();
        }

        if (ImGui::TreeNodeEx("Box Style", ImGuiTreeNodeFlags_Framed))
        {
            if (DrawUIStyle(comp.BoxStyle)) changed = true;
            ImGui::TreePop();
        }

        if (e.HasComponent<ControlComponent>())
        {
            if (ImGui::TreeNodeEx("Layout", ImGuiTreeNodeFlags_Framed))
            {
                if (DrawControlComponentUI(e.GetComponent<ControlComponent>())) changed = true;
                ImGui::TreePop();
            }
        }
        return changed;
    }

    static bool DrawProgressBarWidget(ProgressBarControl& comp, Entity e)
    {
        bool changed = false;
        auto pb = EditorGUI::Begin();
        pb.Float("Progress", comp.Progress, 0.0f, 1.0f)
          .String("Overlay Text", comp.OverlayText)
          .Bool("Show Percentage", comp.ShowPercentage);
        if (pb.Changed) changed = true;

        if (ImGui::TreeNodeEx("Text Style", ImGuiTreeNodeFlags_Framed))
        {
            if (DrawTextStyle(comp.Style)) changed = true;
            ImGui::TreePop();
        }

        if (ImGui::TreeNodeEx("Bar Style", ImGuiTreeNodeFlags_Framed))
        {
            if (DrawUIStyle(comp.BarStyle)) changed = true;
            ImGui::TreePop();
        }

        if (e.HasComponent<ControlComponent>())
        {
            if (ImGui::TreeNodeEx("Layout", ImGuiTreeNodeFlags_Framed))
            {
                if (DrawControlComponentUI(e.GetComponent<ControlComponent>())) changed = true;
                ImGui::TreePop();
            }
        }
        return changed;
    }

    static bool DrawImageWidget(ImageControl& comp, Entity e)
    {
        bool changed = false;
        auto pb = EditorGUI::Begin();
        pb.String("Texture Path", comp.TexturePath)
          .Vec2("Size", comp.Size)
          .Color("Tint Color", comp.TintColor)
          .Color("Border Color", comp.BorderColor);
        if (pb.Changed) changed = true;

        if (ImGui::TreeNodeEx("Style", ImGuiTreeNodeFlags_Framed))
        {
            if (DrawUIStyle(comp.Style)) changed = true;
            ImGui::TreePop();
        }

        if (e.HasComponent<ControlComponent>())
        {
            if (ImGui::TreeNodeEx("Layout", ImGuiTreeNodeFlags_Framed))
            {
                if (DrawControlComponentUI(e.GetComponent<ControlComponent>())) changed = true;
                ImGui::TreePop();
            }
        }
        return changed;
    }

    static bool DrawImageButtonWidget(ImageButtonControl& comp, Entity e)
    {
        bool changed = false;
        auto pb = EditorGUI::Begin();
        pb.String("Label", comp.Label)
          .String("Texture Path", comp.TexturePath)
          .Vec2("Size", comp.Size)
          .Color("Tint Color", comp.TintColor)
          .Color("Background Color", comp.BackgroundColor)
          .Int("Frame Padding", comp.FramePadding);
        if (pb.Changed) changed = true;

        if (ImGui::TreeNodeEx("Style", ImGuiTreeNodeFlags_Framed))
        {
            if (DrawUIStyle(comp.Style)) changed = true;
            ImGui::TreePop();
        }

        if (e.HasComponent<ControlComponent>())
        {
            if (ImGui::TreeNodeEx("Layout", ImGuiTreeNodeFlags_Framed))
            {
                if (DrawControlComponentUI(e.GetComponent<ControlComponent>())) changed = true;
                ImGui::TreePop();
            }
        }
        return changed;
    }

    static bool DrawSeparatorWidget(SeparatorControl& comp, Entity e)
    {
        bool changed = false;
        auto pb = EditorGUI::Begin();
        pb.Float("Thickness", comp.Thickness)
          .Color("Color", comp.LineColor);
        if (pb.Changed) changed = true;

        if (e.HasComponent<ControlComponent>())
        {
            if (ImGui::TreeNodeEx("Layout", ImGuiTreeNodeFlags_Framed))
            {
                if (DrawControlComponentUI(e.GetComponent<ControlComponent>())) changed = true;
                ImGui::TreePop();
            }
        }
        return changed;
    }

    static bool DrawRadioButtonWidget(RadioButtonControl& comp, Entity e)
    {
        bool changed = false;
        auto pb = EditorGUI::Begin();
        pb.String("Label", comp.Label)
          .Int("Selected Index", comp.SelectedIndex)
          .Bool("Horizontal", comp.Horizontal);
        if (pb.Changed) changed = true;

        if (ImGui::TreeNodeEx("Options", ImGuiTreeNodeFlags_Framed))
        {
            for (int i = 0; i < comp.Options.size(); i++)
            {
                ImGui::PushID(i);
                char buf[256];
                strncpy(buf, comp.Options[i].c_str(), 255);
                if (ImGui::InputText("##opt", buf, 255)) {
                    comp.Options[i] = buf;
                    changed = true;
                }
                ImGui::SameLine();
                if (ImGui::Button("X")) {
                    comp.Options.erase(comp.Options.begin() + i);
                    changed = true;
                    ImGui::PopID();
                    break;
                }
                ImGui::PopID();
            }
            if (ImGui::Button("Add Option")) {
                comp.Options.push_back("New Option");
                changed = true;
            }
            ImGui::TreePop();
        }

        if (ImGui::TreeNodeEx("Text Style", ImGuiTreeNodeFlags_Framed))
        {
            if (DrawTextStyle(comp.Style)) changed = true;
            ImGui::TreePop();
        }

        if (e.HasComponent<ControlComponent>())
        {
            if (ImGui::TreeNodeEx("Layout", ImGuiTreeNodeFlags_Framed))
            {
                if (DrawControlComponentUI(e.GetComponent<ControlComponent>())) changed = true;
                ImGui::TreePop();
            }
        }
        return changed;
    }

    static bool DrawColorPickerWidget(ColorPickerControl& comp, Entity e)
    {
        bool changed = false;
        auto pb = EditorGUI::Begin();
        pb.String("Label", comp.Label)
          .Color("Color", comp.SelectedColor)
          .Bool("Show Alpha", comp.ShowAlpha)
          .Bool("Show Picker", comp.ShowPicker);
        if (pb.Changed) changed = true;

        if (ImGui::TreeNodeEx("Style", ImGuiTreeNodeFlags_Framed))
        {
            if (DrawUIStyle(comp.Style)) changed = true;
            ImGui::TreePop();
        }

        if (e.HasComponent<ControlComponent>())
        {
            if (ImGui::TreeNodeEx("Layout", ImGuiTreeNodeFlags_Framed))
            {
                if (DrawControlComponentUI(e.GetComponent<ControlComponent>())) changed = true;
                ImGui::TreePop();
            }
        }
        return changed;
    }

    static bool DrawDragFloatWidget(DragFloatControl& comp, Entity e)
    {
        bool changed = false;
        auto pb = EditorGUI::Begin();
        pb.String("Label", comp.Label)
          .Float("Value", comp.Value)
          .Float("Speed", comp.Speed)
          .Float("Min", comp.Min)
          .Float("Max", comp.Max)
          .String("Format", comp.Format);
        if (pb.Changed) changed = true;

        if (ImGui::TreeNodeEx("Text Style", ImGuiTreeNodeFlags_Framed))
        {
            if (DrawTextStyle(comp.Style)) changed = true;
            ImGui::TreePop();
        }

        if (ImGui::TreeNodeEx("Box Style", ImGuiTreeNodeFlags_Framed))
        {
            if (DrawUIStyle(comp.BoxStyle)) changed = true;
            ImGui::TreePop();
        }

        if (e.HasComponent<ControlComponent>())
        {
            if (ImGui::TreeNodeEx("Layout", ImGuiTreeNodeFlags_Framed))
            {
                if (DrawControlComponentUI(e.GetComponent<ControlComponent>())) changed = true;
                ImGui::TreePop();
            }
        }
        return changed;
    }

    static bool DrawDragIntWidget(DragIntControl& comp, Entity e)
    {
        bool changed = false;
        auto pb = EditorGUI::Begin();
        pb.String("Label", comp.Label)
          .Int("Value", comp.Value)
          .Float("Speed", comp.Speed)
          .Int("Min", comp.Min)
          .Int("Max", comp.Max)
          .String("Format", comp.Format);
        if (pb.Changed) changed = true;

        if (ImGui::TreeNodeEx("Text Style", ImGuiTreeNodeFlags_Framed))
        {
            if (DrawTextStyle(comp.Style)) changed = true;
            ImGui::TreePop();
        }

        if (ImGui::TreeNodeEx("Box Style", ImGuiTreeNodeFlags_Framed))
        {
            if (DrawUIStyle(comp.BoxStyle)) changed = true;
            ImGui::TreePop();
        }

        if (e.HasComponent<ControlComponent>())
        {
            if (ImGui::TreeNodeEx("Layout", ImGuiTreeNodeFlags_Framed))
            {
                if (DrawControlComponentUI(e.GetComponent<ControlComponent>())) changed = true;
                ImGui::TreePop();
            }
        }
        return changed;
    }

    static bool DrawTreeNodeWidget(TreeNodeControl& comp, Entity e)
    {
        bool changed = false;
        auto pb = EditorGUI::Begin();
        pb.String("Label", comp.Label)
          .Bool("Default Open", comp.DefaultOpen)
          .Bool("Is Leaf", comp.IsLeaf);
        if (pb.Changed) changed = true;

        if (ImGui::TreeNodeEx("Text Style", ImGuiTreeNodeFlags_Framed))
        {
            if (DrawTextStyle(comp.Style)) changed = true;
            ImGui::TreePop();
        }

        if (e.HasComponent<ControlComponent>())
        {
            if (ImGui::TreeNodeEx("Layout", ImGuiTreeNodeFlags_Framed))
            {
                if (DrawControlComponentUI(e.GetComponent<ControlComponent>())) changed = true;
                ImGui::TreePop();
            }
        }
        return changed;
    }

    static bool DrawTabBarWidget(TabBarControl& comp, Entity e)
    {
        bool changed = false;
        auto pb = EditorGUI::Begin();
        pb.String("Label", comp.Label)
          .Bool("Reorderable", comp.Reorderable)
          .Bool("Auto Select New Tabs", comp.AutoSelectNewTabs);
        if (pb.Changed) changed = true;

        if (ImGui::TreeNodeEx("Style", ImGuiTreeNodeFlags_Framed))
        {
            if (DrawUIStyle(comp.Style)) changed = true;
            ImGui::TreePop();
        }

        if (e.HasComponent<ControlComponent>())
        {
            if (ImGui::TreeNodeEx("Layout", ImGuiTreeNodeFlags_Framed))
            {
                if (DrawControlComponentUI(e.GetComponent<ControlComponent>())) changed = true;
                ImGui::TreePop();
            }
        }
        return changed;
    }

    static bool DrawTabItemWidget(TabItemControl& comp, Entity e)
    {
        bool changed = false;
        auto pb = EditorGUI::Begin();
        pb.String("Label", comp.Label)
          .Bool("Is Open", comp.IsOpen);
        if (pb.Changed) changed = true;

        if (ImGui::TreeNodeEx("Text Style", ImGuiTreeNodeFlags_Framed))
        {
            if (DrawTextStyle(comp.Style)) changed = true;
            ImGui::TreePop();
        }

        if (e.HasComponent<ControlComponent>())
        {
            if (ImGui::TreeNodeEx("Layout", ImGuiTreeNodeFlags_Framed))
            {
                if (DrawControlComponentUI(e.GetComponent<ControlComponent>())) changed = true;
                ImGui::TreePop();
            }
        }
        return changed;
    }

    static bool DrawCollapsingHeaderWidget(CollapsingHeaderControl& comp, Entity e)
    {
        bool changed = false;
        auto pb = EditorGUI::Begin();
        pb.String("Label", comp.Label)
          .Bool("Default Open", comp.DefaultOpen);
        if (pb.Changed) changed = true;

        if (ImGui::TreeNodeEx("Text Style", ImGuiTreeNodeFlags_Framed))
        {
            if (DrawTextStyle(comp.Style)) changed = true;
            ImGui::TreePop();
        }

        if (e.HasComponent<ControlComponent>())
        {
            if (ImGui::TreeNodeEx("Layout", ImGuiTreeNodeFlags_Framed))
            {
                if (DrawControlComponentUI(e.GetComponent<ControlComponent>())) changed = true;
                ImGui::TreePop();
            }
        }
        return changed;
    }

    static bool DrawPlotLinesWidget(PlotLinesControl& comp, Entity e)
    {
        bool changed = false;
        auto pb = EditorGUI::Begin();
        pb.String("Label", comp.Label)
          .String("Overlay Text", comp.OverlayText)
          .Float("Scale Min", comp.ScaleMin)
          .Float("ScaleMax", comp.ScaleMax)
          .Vec2("Graph Size", comp.GraphSize);
        if (pb.Changed) changed = true;

        if (ImGui::TreeNodeEx("Values", ImGuiTreeNodeFlags_Framed))
        {
            for (int i = 0; i < comp.Values.size(); i++)
            {
                ImGui::PushID(i);
                if (ImGui::DragFloat("##val", &comp.Values[i], 0.01f)) changed = true;
                ImGui::SameLine();
                if (ImGui::Button("X")) {
                    comp.Values.erase(comp.Values.begin() + i);
                    changed = true; ImGui::PopID(); break;
                }
                ImGui::PopID();
            }
            if (ImGui::Button("Add Value")) {
                comp.Values.push_back(0.0f);
                changed = true;
            }
            ImGui::TreePop();
        }

        if (ImGui::TreeNodeEx("Text Style", ImGuiTreeNodeFlags_Framed))
        {
            if (DrawTextStyle(comp.Style)) changed = true;
            ImGui::TreePop();
        }

        if (ImGui::TreeNodeEx("Box Style", ImGuiTreeNodeFlags_Framed))
        {
            if (DrawUIStyle(comp.BoxStyle)) changed = true;
            ImGui::TreePop();
        }

        if (e.HasComponent<ControlComponent>())
        {
            if (ImGui::TreeNodeEx("Layout", ImGuiTreeNodeFlags_Framed))
            {
                if (DrawControlComponentUI(e.GetComponent<ControlComponent>())) changed = true;
                ImGui::TreePop();
            }
        }
        return changed;
    }

    static bool DrawPlotHistogramWidget(PlotHistogramControl& comp, Entity e)
    {
        bool changed = false;
        auto pb = EditorGUI::Begin();
        pb.String("Label", comp.Label)
          .String("Overlay Text", comp.OverlayText)
          .Float("Scale Min", comp.ScaleMin)
          .Float("ScaleMax", comp.ScaleMax)
          .Vec2("Graph Size", comp.GraphSize);
        if (pb.Changed) changed = true;

        if (ImGui::TreeNodeEx("Values", ImGuiTreeNodeFlags_Framed))
        {
            for (int i = 0; i < comp.Values.size(); i++)
            {
                ImGui::PushID(i);
                if (ImGui::DragFloat("##val", &comp.Values[i], 0.01f)) changed = true;
                ImGui::SameLine();
                if (ImGui::Button("X")) {
                    comp.Values.erase(comp.Values.begin() + i);
                    changed = true; ImGui::PopID(); break;
                }
                ImGui::PopID();
            }
            if (ImGui::Button("Add Value")) {
                comp.Values.push_back(0.0f);
                changed = true;
            }
            ImGui::TreePop();
        }

        if (ImGui::TreeNodeEx("Text Style", ImGuiTreeNodeFlags_Framed))
        {
            if (DrawTextStyle(comp.Style)) changed = true;
            ImGui::TreePop();
        }

        if (ImGui::TreeNodeEx("Box Style", ImGuiTreeNodeFlags_Framed))
        {
            if (DrawUIStyle(comp.BoxStyle)) changed = true;
            ImGui::TreePop();
        }

        if (e.HasComponent<ControlComponent>())
        {
            if (ImGui::TreeNodeEx("Layout", ImGuiTreeNodeFlags_Framed))
            {
                if (DrawControlComponentUI(e.GetComponent<ControlComponent>())) changed = true;
                ImGui::TreePop();
            }
        }
        return changed;
    }


    void PropertyEditor::Init()
    {
        // --- Core & Rendering ---
        Register<TransformComponent>("Transform", DrawTransform);
        Register<ModelComponent>("Model", DrawModel);
        Register<PointLightComponent>("Point Light", DrawPointLight);
        Register<CameraComponent>("Camera", DrawCamera);
        Register<AnimationComponent>("Animation", [](auto& comp) { return false; }); 

        // --- Physics ---
        Register<ColliderComponent>("Collider", DrawCollider);
        Register<RigidBodyComponent>("RigidBody", DrawRigidBody);

        Register<AudioComponent>("Audio", DrawAudio);
        Register<SpawnComponent>("Spawn Zone", DrawSpawn);
        Register<PlayerComponent>("Player", DrawPlayer);
        Register<SceneTransitionComponent>("Scene Transition", [](SceneTransitionComponent& comp) { 
            return EditorGUI::Property("Target Scene", comp.TargetScenePath); 
        });
        Register<BillboardComponent>("Billboard", [](BillboardComponent& comp) { 
            return EditorGUI::Property("Texture", comp.TexturePath); 
        });

        Register<NativeScriptComponent>("Native Script", [](auto &comp) {
            bool changed = false;
            for (size_t i = 0; i < comp.Scripts.size(); ++i) {
                ImGui::TextDisabled("Script: %s", comp.Scripts[i].ScriptName.c_str());
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
        Register<ControlComponent>("Rect Transform", DrawControlWidget);
        Register<ButtonControl>("Button Widget", DrawButtonWidget);
        Register<PanelControl>("Panel Widget", DrawPanelWidget);
        Register<LabelControl>("Label Widget", DrawLabelWidget);
        Register<SliderControl>("Slider Widget", DrawSliderWidget);
        Register<CheckboxControl>("Checkbox Widget", DrawCheckboxWidget);
        Register<InputTextControl>("Input Text Widget", DrawInputTextWidget);
        Register<ComboBoxControl>("ComboBox Widget", DrawComboBoxWidget);
        Register<ProgressBarControl>("ProgressBar Widget", DrawProgressBarWidget);
        Register<ImageControl>("Image Widget", DrawImageWidget);
        Register<ImageButtonControl>("Image Button Widget", DrawImageButtonWidget);
        Register<SeparatorControl>("Separator Widget", DrawSeparatorWidget);
        Register<RadioButtonControl>("RadioButton Widget", DrawRadioButtonWidget);
        Register<ColorPickerControl>("ColorPicker Widget", DrawColorPickerWidget);
        Register<DragFloatControl>("DragFloat Widget", DrawDragFloatWidget);
        Register<DragIntControl>("DragInt Widget", DrawDragIntWidget);
        Register<TreeNodeControl>("TreeNode Widget", DrawTreeNodeWidget);
        Register<TabBarControl>("TabBar Widget", DrawTabBarWidget);
        Register<TabItemControl>("TabItem Widget", DrawTabItemWidget);
        Register<CollapsingHeaderControl>("CollapsingHeader Widget", DrawCollapsingHeaderWidget);
        Register<PlotLinesControl>("PlotLines Widget", DrawPlotLinesWidget);
        Register<PlotHistogramControl>("PlotHistogram Widget", DrawPlotHistogramWidget);

        // Helper to setup widgets
        auto setupWidget = [](entt::id_type id) {
            auto& metadata = s_ComponentRegistry[id];
            metadata.IsWidget = true;
            metadata.AllowAdd = true;
            auto originalAdd = metadata.Add;
            metadata.Add = [originalAdd](Entity e) {
                bool added = originalAdd(e);
                if (added && !e.HasComponent<ControlComponent>())
                    e.AddComponent<ControlComponent>();
                return added;
            };
        };

        setupWidget(entt::type_hash<ButtonControl>::value());
        setupWidget(entt::type_hash<PanelControl>::value());
        setupWidget(entt::type_hash<LabelControl>::value());
        setupWidget(entt::type_hash<SliderControl>::value());
        setupWidget(entt::type_hash<CheckboxControl>::value());
        setupWidget(entt::type_hash<InputTextControl>::value());
        setupWidget(entt::type_hash<ComboBoxControl>::value());
        setupWidget(entt::type_hash<ProgressBarControl>::value());
        setupWidget(entt::type_hash<ImageControl>::value());
        setupWidget(entt::type_hash<ImageButtonControl>::value());
        setupWidget(entt::type_hash<SeparatorControl>::value());
        setupWidget(entt::type_hash<RadioButtonControl>::value());
        setupWidget(entt::type_hash<ColorPickerControl>::value());
        setupWidget(entt::type_hash<DragFloatControl>::value());
        setupWidget(entt::type_hash<DragIntControl>::value());
        setupWidget(entt::type_hash<TreeNodeControl>::value());
        setupWidget(entt::type_hash<TabBarControl>::value());
        setupWidget(entt::type_hash<TabItemControl>::value());
        setupWidget(entt::type_hash<CollapsingHeaderControl>::value());
        setupWidget(entt::type_hash<PlotLinesControl>::value());
        setupWidget(entt::type_hash<PlotHistogramControl>::value());

        // Allow adding Rect Transform directly too
        s_ComponentRegistry[entt::type_hash<ControlComponent>::value()].AllowAdd = true;
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
