#include "inspector_panel.h"
#include "imgui.h"
#include "property_editor.h"
#include "editor_gui.h"
#include "engine/scene/scene_events.h"
#include "engine/scene/components.h"
#include "engine/scene/script_registry.h"
#include "engine/scene/project.h"
#include "engine/graphics/asset_manager.h"
#include "engine/graphics/model_asset.h"
#include "engine/physics/bvh/bvh.h"
#include "extras/IconsFontAwesome6.h"
#include "raymath.h"

namespace CHEngine
{
    InspectorPanel::InspectorPanel()
    {
        m_Name = "Inspector";
    }

    void InspectorPanel::OnImGuiRender(bool readOnly)
    {
        if (!m_IsOpen)
            return;

        ImGui::Begin(m_Name.c_str(), &m_IsOpen);
        ImGui::PushID(this);

        if (m_SelectedEntity && m_SelectedEntity.GetRegistry().ctx().get<Scene*>() != m_Context.get())
            m_SelectedEntity = {};

        if (m_SelectedEntity && m_SelectedEntity.IsValid())
        {
            ImGui::BeginDisabled(readOnly);
            DrawComponents(m_SelectedEntity);
            ImGui::EndDisabled();
        }
        else
        {
            ImGui::Text("Selection: None");
            ImGui::TextDisabled("Select an entity in the Hierarchy to view its components.");
        }
        ImGui::PopID();
        ImGui::End();
    }

    void InspectorPanel::OnEvent(Event &e)
    {
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<EntitySelectedEvent>(
            [this](EntitySelectedEvent &ev)
            {
                m_SelectedEntity = Entity(ev.GetEntity(), &ev.GetScene()->GetRegistry());
                m_SelectedMeshIndex = ev.GetMeshIndex();
                return false;
            });
    }

    
    template<typename T, typename UIFunction>
    static void DrawComponent(const std::string& name, Entity entity, UIFunction uiFunction)
    {
        const ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | 
                                         ImGuiTreeNodeFlags_Framed |
                                         ImGuiTreeNodeFlags_SpanAvailWidth | 
                                         ImGuiTreeNodeFlags_AllowOverlap | 
                                         ImGuiTreeNodeFlags_FramePadding;

        if (entity.HasComponent<T>())
        {
            auto& component = entity.GetComponent<T>();
            ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();

            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{4, 4});
            float lineHeight = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0f;
            ImGui::Separator();
            
            bool open = ImGui::TreeNodeEx((void*)typeid(T).hash_code(), flags, name.c_str());
            ImGui::PopStyleVar();
            
            // Settings button
            ImGui::SameLine(contentRegionAvailable.x - lineHeight * 0.5f);
            if (ImGui::Button((std::string(ICON_FA_GEAR "##") + name).c_str(), ImVec2{lineHeight, lineHeight}))
                ImGui::OpenPopup("ComponentSettings");

            bool removeComponent = false;
            if (ImGui::BeginPopup("ComponentSettings"))
            {
                if (ImGui::MenuItem(ICON_FA_TRASH " Remove component"))
                    removeComponent = true;
                ImGui::EndPopup();
            }

            if (open)
            {
                uiFunction(component);
                ImGui::TreePop();
            }

            if (removeComponent)
                entity.RemoveComponent<T>();
        }
    }
    
    void InspectorPanel::DrawComponents(Entity entity)
    {
        ImGui::PushID((uint32_t)entity);

        if (entity.HasComponent<IDComponent>())
        {
            uint64_t uuid = (uint64_t)entity.GetComponent<IDComponent>().ID;
            ImGui::Text("UUID: %llu", uuid);
        }

        PropertyEditor::DrawTag(entity);

        ImGui::SameLine();
        ImGui::PushItemWidth(-1);
        if (ImGui::Button("Add Component"))
            ImGui::OpenPopup("AddComponent");
        PropertyEditor::DrawAddComponentPopup(entity);
        ImGui::PopItemWidth();

        // ========================================================================
        // TRANSFORM COMPONENT
        // ========================================================================
        DrawComponent<TransformComponent>("Transform", entity, [](auto& component)
        {
            EditorGUI::DrawVec3("Position", component.Translation);
            
            if (EditorGUI::DrawVec3("Rotation", component.Rotation))
            {
                // Sync quaternion with Euler rotation
                component.RotationQuat = QuaternionFromEuler(
                    component.Rotation.x * DEG2RAD, 
                    component.Rotation.y * DEG2RAD, 
                    component.Rotation.z * DEG2RAD
                );
            }
            
            EditorGUI::DrawVec3("Scale", component.Scale, 1.0f);
        });

        // ========================================================================
        // CAMERA COMPONENT
        // ========================================================================
        DrawComponent<CameraComponent>("Camera", entity, [](auto& component)
        {
            auto& camera = component.Camera;

            const char* projectionTypeStrings[] = { "Perspective", "Orthographic" };
            int projectionType = (int)camera.GetProjectionType();
            if (EditorGUI::Property("Projection", projectionType, projectionTypeStrings, 2))
                camera.SetProjectionType((CHEngine::ProjectionType)projectionType);

            if (camera.GetProjectionType() == CHEngine::ProjectionType::Perspective)
            {
                float verticalFov = camera.GetPerspectiveVerticalFOV() * RAD2DEG;
                if (EditorGUI::Property("Vertical FOV", verticalFov, 1.0f, 1.0f, 180.0f))
                    camera.SetPerspectiveVerticalFOV(verticalFov * DEG2RAD);

                float nearClip = camera.GetPerspectiveNearClip();
                if (EditorGUI::Property("Near", nearClip, 0.01f))
                    camera.SetPerspectiveNearClip(nearClip);

                float farClip = camera.GetPerspectiveFarClip();
                if (EditorGUI::Property("Far", farClip, 1.0f))
                    camera.SetPerspectiveFarClip(farClip);
            }

            if (camera.GetProjectionType() == CHEngine::ProjectionType::Orthographic)
            {
                float orthoSize = camera.GetOrthographicSize();
                if (EditorGUI::Property("Size", orthoSize, 0.1f))
                    camera.SetOrthographicSize(orthoSize);

                float nearClip = camera.GetOrthographicNearClip();
                if (EditorGUI::Property("Near", nearClip, 0.01f))
                    camera.SetOrthographicNearClip(nearClip);

                float farClip = camera.GetOrthographicFarClip();
                if (EditorGUI::Property("Far", farClip, 0.1f))
                    camera.SetOrthographicFarClip(farClip);

                ImGui::Checkbox("Fixed Aspect Ratio", &component.FixedAspectRatio);
            }

            ImGui::Checkbox("Primary", &component.Primary);
            
            ImGui::Separator();
            ImGui::Checkbox("Orbit Camera Setup", &component.IsOrbitCamera);
            
            if (component.IsOrbitCamera)
            {
                EditorGUI::Property("Target Tag", component.TargetEntityTag);
                EditorGUI::Property("Distance", component.OrbitDistance, 0.1f, 0.0f, 100.0f);
                EditorGUI::Property("Yaw", component.OrbitYaw, 0.5f);
                EditorGUI::Property("Pitch", component.OrbitPitch, 0.5f, -89.0f, 89.0f);
                EditorGUI::Property("Sensitivity", component.LookSensitivity, 0.1f, 0.1f, 5.0f);
            }
        });

        // ========================================================================
        // POINT LIGHT COMPONENT
        // ========================================================================
        DrawComponent<PointLightComponent>("Point Light", entity, [](auto& component)
        {
            auto pb = EditorGUI::Begin();
            pb.Color("Color", component.LightColor)
              .Float("Intensity", component.Intensity, 0.1f, 0.0f, 100.0f)
              .Float("Radius", component.Radius, 0.1f, 0.0f, 1000.0f);
        });

        // ========================================================================
        // SPOT LIGHT COMPONENT
        // ========================================================================
        DrawComponent<SpotLightComponent>("Spot Light", entity, [](auto& component)
        {
            auto pb = EditorGUI::Begin();
            pb.Color("Color", component.LightColor)
              .Float("Intensity", component.Intensity, 0.1f, 0.0f, 100.0f)
              .Float("Range", component.Range, 0.1f, 0.0f, 1000.0f)
              .Float("Inner Cutoff", component.InnerCutoff, 0.1f, 0.0f, 90.0f)
              .Float("Outer Cutoff", component.OuterCutoff, 0.1f, 0.0f, 90.0f);
        });

        // ========================================================================
        // SHADER COMPONENT
        // ========================================================================
        DrawComponent<ShaderComponent>("Shader", entity, [](auto& component)
        {
            EditorGUI::Begin().File("Shader Path", component.ShaderPath, "glsl,vert,frag");
            
            if (ImGui::TreeNodeEx("Uniforms", ImGuiTreeNodeFlags_DefaultOpen))
            {
                for (auto& u : component.Uniforms)
                    ImGui::Text("%s", u.Name.c_str());
                ImGui::TreePop();
            }
        });

        // ========================================================================
        // RIGIDBODY COMPONENT
        // ========================================================================
        DrawComponent<RigidBodyComponent>("RigidBody", entity, [](auto& component)
        {
            auto pb = EditorGUI::Begin();
            pb.Float("Mass", component.Mass, 0.1f, 0.0f, 1000.0f)
              .Bool("Use Gravity", component.UseGravity)
              .Bool("Is Kinematic", component.IsKinematic);
        });

        // ========================================================================
        // COLLIDER COMPONENT
        // ========================================================================
        DrawComponent<ColliderComponent>("Collider", entity, [](auto& component)
        {
            const char* types[] = {"Box", "Mesh (BVH)", "Capsule"};
            int type = (int)component.Type;
            if (EditorGUI::Property("Type", type, types, 3))
                component.Type = (ColliderType)type;
            
            EditorGUI::Property("Enabled", component.Enabled);
            EditorGUI::DrawVec3("Offset", component.Offset);
            
            if (component.Type == ColliderType::Box)
            {
                EditorGUI::DrawVec3("Size", component.Size, 1.0f);
            }
            else if (component.Type == ColliderType::Capsule)
            {
                EditorGUI::Property("Radius", component.Radius, 0.05f);
                EditorGUI::Property("Height", component.Height, 0.05f);
            }
            else if (component.Type == ColliderType::Mesh)
            {
                EditorGUI::Begin().File("Model Path", component.ModelPath, "obj,gltf,glb");
                
                ImGui::Text("BVH Status: %s", component.BVHRoot ? "Built" : "Missing");
                if (ImGui::Button(ICON_FA_HAMMER " Rebuild BVH"))
                {
                    if (auto project = Project::GetActive())
                    {
                        auto asset = project->GetAssetManager()->Get<ModelAsset>(component.ModelPath);
                        if (asset)
                            component.BVHRoot = BVH::Build(asset->GetModel());
                    }
                }
            }
            
            EditorGUI::Property("Auto Calculate", component.AutoCalculate);
        });

        // ========================================================================
        // AUDIO COMPONENT
        // ========================================================================
        DrawComponent<AudioComponent>("Audio", entity, [](auto& component)
        {
            auto pb = EditorGUI::Begin();
            pb.File("Sound Path", component.SoundPath, "wav,ogg,mp3")
              .Bool("Loop", component.Loop)
              .Bool("Play On Start", component.PlayOnStart)
              .Float("Volume", component.Volume, 0.05f, 0.0f, 2.0f)
              .Float("Pitch", component.Pitch, 0.05f, 0.1f, 5.0f);
        });

        // ========================================================================
        // NATIVE SCRIPT COMPONENT
        // ========================================================================
        DrawComponent<NativeScriptComponent>("Native Script", entity, [entity](auto& component) mutable
        {
            for (size_t i = 0; i < component.Scripts.size(); ++i)
            {
                ImGui::TextDisabled(ICON_FA_CODE " %s", component.Scripts[i].ScriptName.c_str());
                ImGui::SameLine();
                ImGui::PushID((int)i);
                if (ImGui::Button(ICON_FA_TRASH))
                {
                    component.Scripts.erase(component.Scripts.begin() + i);
                    ImGui::PopID();
                    return;
                }
                ImGui::PopID();
            }
            
            if (EditorGUI::ActionButton(ICON_FA_PLUS, "Add Script"))
                ImGui::OpenPopup("AddScriptPopup");

            if (ImGui::BeginPopup("AddScriptPopup"))
            {
                auto* scene = entity.GetRegistry().ctx().get<Scene*>();
                if (scene)
                {
                    auto& scriptRegistry = scene->GetScriptRegistry();
                    for (const auto& [name, funcs] : scriptRegistry.GetScripts())
                    {
                        if (ImGui::MenuItem(name.c_str()))
                            scriptRegistry.AddScript(name, component);
                    }
                }
                ImGui::EndPopup();
            }
        });

        // ========================================================================
        // MODEL COMPONENT
        // ========================================================================
        DrawComponent<ModelComponent>("Model", entity, [](auto& component)
        {
            EditorGUI::Begin().File("Model Path", component.ModelPath, "obj,gltf,glb");

            if (ImGui::TreeNodeEx("Materials", ImGuiTreeNodeFlags_Framed))
            {
                for (auto& slot : component.Materials)
                {
                    ImGui::PushID(slot.Index);
                    if (ImGui::TreeNodeEx(slot.Name.c_str(), ImGuiTreeNodeFlags_Framed))
                    {
                        auto pb = EditorGUI::Begin();
                        pb.Color("Albedo", slot.Material.AlbedoColor)
                          .Float("Metalness", slot.Material.Metalness, 0.02f, 0.0f, 1.0f)
                          .Float("Roughness", slot.Material.Roughness, 0.02f, 0.0f, 1.0f)
                          .Bool("Double Sided", slot.Material.DoubleSided)
                          .Bool("Transparent", slot.Material.Transparent)
                          .Float("Alpha", slot.Material.Alpha, 0.01f, 0.0f, 1.0f);
                        ImGui::TreePop();
                    }
                    ImGui::PopID();
                }
                ImGui::TreePop();
            }

            if (EditorGUI::ActionButton(ICON_FA_DOWNLOAD, "Reload Model"))
                component.MaterialsInitialized = false;
        });

        // ========================================================================
        // SPAWN COMPONENT
        // ========================================================================
        DrawComponent<SpawnComponent>("Spawn", entity, [](auto& component)
        {
            auto pb = EditorGUI::Begin();
            pb.Vec3("Zone Size", component.ZoneSize)
              .File("Spawn Texture", component.TexturePath, "png,jpg,tga")
              .Bool("Render Zone", component.RenderSpawnZoneInScene);
        });

        // ========================================================================
        // PLAYER COMPONENT
        // ========================================================================
        DrawComponent<PlayerComponent>("Player", entity, [](auto& component)
        {
            auto pb = EditorGUI::Begin();
            pb.Float("Speed", component.MovementSpeed)
              .Float("Sensitivity", component.LookSensitivity)
              .Float("Jump Force", component.JumpForce);
        });

        // ========================================================================
        // SCENE TRANSITION COMPONENT
        // ========================================================================
        DrawComponent<SceneTransitionComponent>("Scene Transition", entity, [](auto& component)
        {
            EditorGUI::Begin().File("Target Scene", component.TargetScenePath, "chscene");
        });

        // ========================================================================
        // SPRITE COMPONENT
        // ========================================================================
        DrawComponent<SpriteComponent>("Sprite", entity, [](auto& component)
        {
            auto pb = EditorGUI::Begin();
            pb.File("Texture", component.TexturePath, "png,jpg,tga")
              .Color("Tint", component.Tint)
              .Bool("Flip X", component.FlipX)
              .Bool("Flip Y", component.FlipY)
              .Int("Z Order", component.ZOrder);
        });

        // ========================================================================
        // UI WIDGET COMPONENTS
        // ========================================================================
        
        // Helper lambdas for UI widgets
        auto DrawUIStyle = [](UIStyle& style) {
            auto pb = EditorGUI::Begin();
            pb.Color("Background", style.BackgroundColor)
              .Color("Hover", style.HoverColor)
              .Color("Pressed", style.PressedColor)
              .Float("Rounding", style.Rounding)
              .Float("Border", style.BorderSize)
              .Color("Border Color", style.BorderColor)
              .Float("Padding", style.Padding)
              .Bool("Use Gradient", style.UseGradient)
              .Color("Gradient Color", style.GradientColor)
              .Float("Hover Scale", style.HoverScale, 0.01f, 0.5f, 2.0f)
              .Float("Pressed Scale", style.PressedScale, 0.01f, 0.5f, 2.0f)
              .Float("Transition Speed", style.TransitionSpeed, 0.01f, 0.0f, 1.0f);
            return pb.Changed;
        };

        auto DrawTextStyle = [](TextStyle& style) {
            bool changed = false;
            auto pb = EditorGUI::Begin();
            pb.Float("Font Size", style.FontSize)
              .Color("Text Color", style.TextColor);

            const char* alignments[] = {"Left", "Center", "Right"};
            int hAlign = (int)style.HorizontalAlignment;
            if (EditorGUI::Property("H Align", hAlign, alignments, 3))
            {
                style.HorizontalAlignment = (TextAlignment)hAlign;
                changed = true;
            }

            int vAlign = (int)style.VerticalAlignment;
            if (EditorGUI::Property("V Align", vAlign, alignments, 3))
            {
                style.VerticalAlignment = (TextAlignment)vAlign;
                changed = true;
            }

            pb.Float("Letter Spacing", style.LetterSpacing)
              .Float("Line Height", style.LineHeight);
            
            if (pb.Bool("Shadow", style.Shadow) && style.Shadow)
            {
                pb.Float("Shadow Offset", style.ShadowOffset)
                  .Color("Shadow Color", style.ShadowColor);
            }
            
            return changed || pb.Changed;
        };

        // CONTROL COMPONENT (Rect Transform)
        DrawComponent<ControlComponent>("Rect Transform", entity, [](auto& component)
        {
            auto& rectTransform = component.Transform;
            
            // --- Anchor Presets ---
            ImGui::Text("Presets:"); ImGui::SameLine();
            if (ImGui::Button("Center")) {
                rectTransform.AnchorMin = {0.5f, 0.5f}; rectTransform.AnchorMax = {0.5f, 0.5f};
                rectTransform.OffsetMin = {-50, -50}; rectTransform.OffsetMax = {50, 50};
            }
            ImGui::SameLine();
            if (ImGui::Button("Stretch")) {
                rectTransform.AnchorMin = {0.0f, 0.0f}; rectTransform.AnchorMax = {1.0f, 1.0f};
                rectTransform.OffsetMin = {0, 0}; rectTransform.OffsetMax = {0, 0};
            }

            // KISS: Basic Pos/Size first
            bool isPoint = (rectTransform.AnchorMin.x == rectTransform.AnchorMax.x && rectTransform.AnchorMin.y == rectTransform.AnchorMax.y);
            if (isPoint)
            {
                float width = rectTransform.OffsetMax.x - rectTransform.OffsetMin.x;
                float height = rectTransform.OffsetMax.y - rectTransform.OffsetMin.y;
                float posX = rectTransform.OffsetMin.x + width * rectTransform.Pivot.x;
                float posY = rectTransform.OffsetMin.y + height * rectTransform.Pivot.y;
                
                Vector2 pos = {posX, posY};
                Vector2 size = {width, height};

                if (EditorGUI::Property("Pos", pos)) {
                    rectTransform.OffsetMin.x = pos.x - size.x * rectTransform.Pivot.x;
                    rectTransform.OffsetMin.y = pos.y - size.y * rectTransform.Pivot.y;
                    rectTransform.OffsetMax.x = pos.x + size.x * (1.0f - rectTransform.Pivot.x);
                    rectTransform.OffsetMax.y = pos.y + size.y * (1.0f - rectTransform.Pivot.y);
                }
                if (EditorGUI::Property("Size", size)) {
                    rectTransform.OffsetMin.x = pos.x - size.x * rectTransform.Pivot.x;
                    rectTransform.OffsetMin.y = pos.y - size.y * rectTransform.Pivot.y;
                    rectTransform.OffsetMax.x = pos.x + size.x * (1.0f - rectTransform.Pivot.x);
                    rectTransform.OffsetMax.y = pos.y + size.y * (1.0f - rectTransform.Pivot.y);
                }
            }
            else
            {
                float rightPadding = -rectTransform.OffsetMax.x;
                float bottomPadding = -rectTransform.OffsetMax.y;

                if (EditorGUI::Property("Left", rectTransform.OffsetMin.x)) {}
                if (EditorGUI::Property("Top", rectTransform.OffsetMin.y)) {}
                if (EditorGUI::Property("Right", rightPadding)) { rectTransform.OffsetMax.x = -rightPadding; }
                if (EditorGUI::Property("Bottom", bottomPadding)) { rectTransform.OffsetMax.y = -bottomPadding; }
            }

            // Advanced settings hidden by default
            if (ImGui::TreeNodeEx("Advanced Layout Settings", ImGuiTreeNodeFlags_SpanAvailWidth))
            {
                EditorGUI::Property("Pivot", rectTransform.Pivot);
                EditorGUI::Property("Anchor Min", rectTransform.AnchorMin);
                EditorGUI::Property("Anchor Max", rectTransform.AnchorMax);
                EditorGUI::Property("Rotation", rectTransform.Rotation);
                EditorGUI::Property("Scale", rectTransform.Scale);
                EditorGUI::Property("Z Order", component.ZOrder);
                EditorGUI::Property("Visible", component.IsActive);
                ImGui::TreePop();
            }
        });

        // BUTTON WIDGET
        DrawComponent<ButtonControl>("Button Widget", entity, [&DrawUIStyle, &DrawTextStyle](auto& component)
        {
            EditorGUI::Property("Label", component.Label);
            ImGui::Checkbox("Interactable", &component.IsInteractable);
            ImGui::Checkbox("Auto Size", &component.AutoSize);
            
            if (ImGui::TreeNodeEx("Style", ImGuiTreeNodeFlags_Framed))
            {
                DrawUIStyle(component.Style);
                ImGui::TreePop();
            }
            
            if (ImGui::TreeNodeEx("Text Style", ImGuiTreeNodeFlags_Framed))
            {
                DrawTextStyle(component.Text);
                ImGui::TreePop();
            }
        });

        // PANEL WIDGET
        DrawComponent<PanelControl>("Panel Widget", entity, [&DrawUIStyle](auto& component)
        {
            auto pb = EditorGUI::Begin();
            pb.File("Texture", component.TexturePath, "png,jpg,tga")
              .Bool("Full Screen", component.FullScreen);
            
            if (ImGui::TreeNodeEx("Style", ImGuiTreeNodeFlags_Framed))
            {
                DrawUIStyle(component.Style);
                ImGui::TreePop();
            }
        });

        // LABEL WIDGET
        DrawComponent<LabelControl>("Label Widget", entity, [&DrawTextStyle](auto& component)
        {
            EditorGUI::Property("Text", component.Text);
            ImGui::Checkbox("Auto Size", &component.AutoSize);
            
            if (ImGui::TreeNodeEx("Style", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen))
            {
                DrawTextStyle(component.Style);
                ImGui::TreePop();
            }
        });

        // SLIDER WIDGET
        DrawComponent<SliderControl>("Slider Widget", entity, [&DrawUIStyle](auto& component)
        {
            auto pb = EditorGUI::Begin();
            pb.String("Label", component.Label)
              .Float("Value", component.Value, 0.01f, component.Min, component.Max);
            
            if (ImGui::TreeNodeEx("Style", ImGuiTreeNodeFlags_Framed))
            {
                DrawUIStyle(component.Style);
                ImGui::TreePop();
            }
        });

        // CHECKBOX WIDGET
        DrawComponent<CheckboxControl>("Checkbox Widget", entity, [&DrawUIStyle](auto& component)
        {
            auto pb = EditorGUI::Begin();
            pb.String("Label", component.Label)
              .Bool("Checked", component.Checked);
            
            if (ImGui::TreeNodeEx("Style", ImGuiTreeNodeFlags_Framed))
            {
                DrawUIStyle(component.Style);
                ImGui::TreePop();
            }
        });

        // INPUT TEXT WIDGET
        DrawComponent<InputTextControl>("Input Text Widget", entity, [&DrawUIStyle, &DrawTextStyle](auto& component)
        {
            auto pb = EditorGUI::Begin();
            pb.String("Label", component.Label)
              .String("Text", component.Text)
              .String("Placeholder", component.Placeholder)
              .Int("Max Length", component.MaxLength)
              .Bool("Multiline", component.Multiline)
              .Bool("Read Only", component.ReadOnly)
              .Bool("Password", component.Password);

            if (ImGui::TreeNodeEx("Text Style", ImGuiTreeNodeFlags_Framed)) { DrawTextStyle(component.Style); ImGui::TreePop(); }
            if (ImGui::TreeNodeEx("Box Style", ImGuiTreeNodeFlags_Framed)) { DrawUIStyle(component.BoxStyle); ImGui::TreePop(); }
        });

        // COMBO BOX WIDGET
        DrawComponent<ComboBoxControl>("ComboBox Widget", entity, [&DrawUIStyle, &DrawTextStyle](auto& component)
        {
            auto pb = EditorGUI::Begin();
            pb.String("Label", component.Label)
              .Int("Selected Index", component.SelectedIndex);

            if (ImGui::TreeNodeEx("Items", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen))
            {
                for (int i = 0; i < (int)component.Items.size(); i++)
                {
                    ImGui::PushID(i);
                    char buf[256];
                    strncpy(buf, component.Items[i].c_str(), sizeof(buf) - 1);
                    if (ImGui::InputText("##item", buf, sizeof(buf))) component.Items[i] = buf;
                    ImGui::SameLine();
                    if (ImGui::Button("X")) { component.Items.erase(component.Items.begin() + i); ImGui::PopID(); break; }
                    ImGui::PopID();
                }
                if (ImGui::Button("Add Item")) component.Items.push_back("New Option");
                ImGui::TreePop();
            }

            if (ImGui::TreeNodeEx("Text Style", ImGuiTreeNodeFlags_Framed)) { DrawTextStyle(component.Style); ImGui::TreePop(); }
            if (ImGui::TreeNodeEx("Box Style", ImGuiTreeNodeFlags_Framed)) { DrawUIStyle(component.BoxStyle); ImGui::TreePop(); }
        });

        // PROGRESS BAR WIDGET
        DrawComponent<ProgressBarControl>("ProgressBar Widget", entity, [&DrawUIStyle, &DrawTextStyle](auto& component)
        {
            auto pb = EditorGUI::Begin();
            pb.Float("Progress", component.Progress, 0.0f, 1.0f)
              .String("Overlay Text", component.OverlayText)
              .Bool("Show Percentage", component.ShowPercentage);

            if (ImGui::TreeNodeEx("Text Style", ImGuiTreeNodeFlags_Framed)) { DrawTextStyle(component.Style); ImGui::TreePop(); }
            if (ImGui::TreeNodeEx("Bar Style", ImGuiTreeNodeFlags_Framed)) { DrawUIStyle(component.BarStyle); ImGui::TreePop(); }
        });

        // IMAGE WIDGET
        DrawComponent<ImageControl>("Image Widget", entity, [&DrawUIStyle](auto& component)
        {
            EditorGUI::Begin().File("Texture Path", component.TexturePath, "png,jpg,tga")
                               .Color("Tint Color", component.TintColor)
                               .Color("Border Color", component.BorderColor);

            if (ImGui::TreeNodeEx("Style", ImGuiTreeNodeFlags_Framed)) { DrawUIStyle(component.Style); ImGui::TreePop(); }
        });

        // IMAGE BUTTON WIDGET
        DrawComponent<ImageButtonControl>("Image Button Widget", entity, [&DrawUIStyle](auto& component)
        {
            EditorGUI::Begin().String("Label", component.Label)
                               .File("Texture Path", component.TexturePath, "png,jpg,tga")
                               .Color("Tint Color", component.TintColor)
                               .Color("Background Color", component.BackgroundColor)
                               .Int("Frame Padding", component.FramePadding);

            if (ImGui::TreeNodeEx("Style", ImGuiTreeNodeFlags_Framed)) { DrawUIStyle(component.Style); ImGui::TreePop(); }
        });

        // SEPARATOR WIDGET
        DrawComponent<SeparatorControl>("Separator Widget", entity, [](auto& component)
        {
            auto pb = EditorGUI::Begin();
            pb.Float("Thickness", component.Thickness)
              .Color("Color", component.LineColor);
        });

        // RADIO BUTTON WIDGET
        DrawComponent<RadioButtonControl>("RadioButton Widget", entity, [&DrawTextStyle](auto& component)
        {
            auto pb = EditorGUI::Begin();
            pb.String("Label", component.Label)
              .Int("Selected Index", component.SelectedIndex)
              .Bool("Horizontal", component.Horizontal);

            if (ImGui::TreeNodeEx("Options", ImGuiTreeNodeFlags_Framed))
            {
                for (int i = 0; i < (int)component.Options.size(); i++)
                {
                    ImGui::PushID(i);
                    char buf[256];
                    strncpy(buf, component.Options[i].c_str(), 255);
                    if (ImGui::InputText("##opt", buf, 255)) component.Options[i] = buf;
                    ImGui::SameLine();
                    if (ImGui::Button("X")) { component.Options.erase(component.Options.begin() + i); ImGui::PopID(); break; }
                    ImGui::PopID();
                }
                if (ImGui::Button("Add Option")) component.Options.push_back("New Option");
                ImGui::TreePop();
            }
            if (ImGui::TreeNodeEx("Text Style", ImGuiTreeNodeFlags_Framed)) { DrawTextStyle(component.Style); ImGui::TreePop(); }
        });

        // COLOR PICKER WIDGET
        DrawComponent<ColorPickerControl>("ColorPicker Widget", entity, [&DrawUIStyle](auto& component)
        {
            EditorGUI::Begin().String("Label", component.Label)
                               .Color("Color", component.SelectedColor)
                               .Bool("Show Alpha", component.ShowAlpha)
                               .Bool("Show Picker", component.ShowPicker);

            if (ImGui::TreeNodeEx("Style", ImGuiTreeNodeFlags_Framed)) { DrawUIStyle(component.Style); ImGui::TreePop(); }
        });

        // DRAG FLOAT WIDGET
        DrawComponent<DragFloatControl>("DragFloat Widget", entity, [&DrawUIStyle, &DrawTextStyle](auto& component)
        {
            EditorGUI::Begin().String("Label", component.Label)
                               .Float("Value", component.Value)
                               .Float("Speed", component.Speed)
                               .Float("Min", component.Min)
                               .Float("Max", component.Max)
                               .String("Format", component.Format);

            if (ImGui::TreeNodeEx("Text Style", ImGuiTreeNodeFlags_Framed)) { DrawTextStyle(component.Style); ImGui::TreePop(); }
            if (ImGui::TreeNodeEx("Box Style", ImGuiTreeNodeFlags_Framed)) { DrawUIStyle(component.BoxStyle); ImGui::TreePop(); }
        });

        // DRAG INT WIDGET
        DrawComponent<DragIntControl>("DragInt Widget", entity, [&DrawUIStyle, &DrawTextStyle](auto& component)
        {
            EditorGUI::Begin().String("Label", component.Label)
                               .Int("Value", component.Value)
                               .Float("Speed", component.Speed)
                               .Int("Min", component.Min)
                               .Int("Max", component.Max)
                               .String("Format", component.Format);

            if (ImGui::TreeNodeEx("Text Style", ImGuiTreeNodeFlags_Framed)) { DrawTextStyle(component.Style); ImGui::TreePop(); }
            if (ImGui::TreeNodeEx("Box Style", ImGuiTreeNodeFlags_Framed)) { DrawUIStyle(component.BoxStyle); ImGui::TreePop(); }
        });

        // TAB BAR WIDGET
        DrawComponent<TabBarControl>("TabBar Widget", entity, [&DrawUIStyle](auto& component)
        {
            EditorGUI::Begin().String("Label", component.Label)
                               .Bool("Reorderable", component.Reorderable)
                               .Bool("Auto Select New Tabs", component.AutoSelectNewTabs);

            if (ImGui::TreeNodeEx("Style", ImGuiTreeNodeFlags_Framed)) { DrawUIStyle(component.Style); ImGui::TreePop(); }
        });

        // TAB ITEM WIDGET
        DrawComponent<TabItemControl>("Tab Item Widget", entity, [&DrawTextStyle](auto& component)
        {
            EditorGUI::Begin().String("Label", component.Label)
                               .Bool("Is Open", component.IsOpen);

            if (ImGui::TreeNodeEx("Text Style", ImGuiTreeNodeFlags_Framed)) { DrawTextStyle(component.Style); ImGui::TreePop(); }
        });

        // COLLAPSING HEADER WIDGET
        DrawComponent<CollapsingHeaderControl>("CollapsingHeader Widget", entity, [&DrawTextStyle](auto& component)
        {
            EditorGUI::Begin().String("Label", component.Label)
                               .Bool("Default Open", component.DefaultOpen);

            if (ImGui::TreeNodeEx("Text Style", ImGuiTreeNodeFlags_Framed)) { DrawTextStyle(component.Style); ImGui::TreePop(); }
        });

        DrawComponent<VerticalLayoutGroup>("Vertical Layout Group", entity, [](auto& component)
        {
            auto pb = EditorGUI::Begin();
            pb.Float("Spacing", component.Spacing)
              .Vec2("Padding", component.Padding);
        });

        // ========================================================================
        // FALLBACK: Use PropertyEditor for components not yet migrated
        // ========================================================================
        PropertyEditor::DrawEntityProperties(entity);

        // Some special cases that might not be in the registry or need extra data
        PropertyEditor::DrawMaterial(entity, m_SelectedMeshIndex);

        ImGui::PopID();
    }
} // namespace CHEngine
