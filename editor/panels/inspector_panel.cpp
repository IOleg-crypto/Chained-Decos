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

        if (m_SelectedEntity && m_SelectedEntity.GetScene() != m_Context.get())
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
                m_SelectedEntity = Entity{ev.GetEntity(), ev.GetScene()};
                m_SelectedMeshIndex = ev.GetMeshIndex();
                return false;
            });
    }

    // ============================================================================
    // HAZEL-STYLE TEMPLATE COMPONENT DRAWER
    // ============================================================================
    
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

    // ============================================================================
    // DRAW ALL COMPONENTS (Hazel-style with lambdas)
    // ============================================================================
    
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
            EditorGUI::Property("Shader Path", component.ShaderPath);
            
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
              .Bool("Is Kinematic", component.IsKinematic)
              .Bool("Character Controller", component.IsCharacterController);
            
            if (component.IsCharacterController)
                ImGui::TextDisabled("Using Sphere-Mesh integration for smooth movement.");
            
            // if (EditorGUI::ActionButton(ICON_FA_ROTATE_LEFT, "Reset Velocity"))
            // {
            //     // Velocity reset logic would go here
            // }
        });

        // ========================================================================
        // COLLIDER COMPONENT
        // ========================================================================
        DrawComponent<ColliderComponent>("Collider", entity, [](auto& component)
        {
            const char* types[] = {"Box", "Mesh (BVH)"};
            int type = (int)component.Type;
            if (EditorGUI::Property("Type", type, types, 2))
                component.Type = (ColliderType)type;
            
            EditorGUI::Property("Enabled", component.Enabled);
            EditorGUI::DrawVec3("Offset", component.Offset);
            
            if (component.Type == ColliderType::Box)
            {
                EditorGUI::DrawVec3("Size", component.Size, 1.0f);
            }
            else if (component.Type == ColliderType::Mesh)
            {
                EditorGUI::Property("Model Path", component.ModelPath);
                
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
            pb.String("Sound Path", component.SoundPath)
              .Bool("Loop", component.Loop)
              .Bool("Play On Start", component.PlayOnStart)
              .Float("Volume", component.Volume, 0.05f, 0.0f, 2.0f)
              .Float("Pitch", component.Pitch, 0.05f, 0.1f, 5.0f);
        });

        // ========================================================================
        // NATIVE SCRIPT COMPONENT
        // ========================================================================
        DrawComponent<NativeScriptComponent>("Native Script", entity, [](auto& component)
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
                for (const auto& [name, funcs] : ScriptRegistry::GetScripts())
                {
                    if (ImGui::MenuItem(name.c_str()))
                        ScriptRegistry::AddScript(name, component);
                }
                ImGui::EndPopup();
            }
        });

        // ========================================================================
        // MODEL COMPONENT
        // ========================================================================
        DrawComponent<ModelComponent>("Model", entity, [](auto& component)
        {
            EditorGUI::Property("Model Path", component.ModelPath);

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
              .String("Spawn Texture", component.TexturePath)
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
        // BILLBOARD COMPONENT
        // ========================================================================
        DrawComponent<BillboardComponent>("Billboard", entity, [](auto& component)
        {
            auto pb = EditorGUI::Begin();
            pb.String("Texture", component.TexturePath)
              .Float("Size", component.Size);
        });

        // ========================================================================
        // SCENE TRANSITION COMPONENT
        // ========================================================================
        DrawComponent<SceneTransitionComponent>("Scene Transition", entity, [](auto& component)
        {
            EditorGUI::Property("Target Scene", component.TargetScenePath);
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
              .Float("Padding", style.Padding);
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
            auto pb = EditorGUI::Begin();
            pb.Vec2("Anchor Min", component.Transform.AnchorMin)
              .Vec2("Anchor Max", component.Transform.AnchorMax)
              .Vec2("Offset Min", component.Transform.OffsetMin)
              .Vec2("Offset Max", component.Transform.OffsetMax)
              .Vec2("Pivot", component.Transform.Pivot)
              .Float("Rotation", component.Transform.Rotation)
              .Vec2("Scale", component.Transform.Scale);
        });

        // BUTTON WIDGET
        DrawComponent<ButtonControl>("Button Widget", entity, [&DrawUIStyle, &DrawTextStyle](auto& component)
        {
            EditorGUI::Property("Label", component.Label);
            
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
            pb.String("Texture", component.TexturePath)
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

        // ========================================================================
        // FALLBACK: Use PropertyEditor for components not yet migrated
        // ========================================================================
        PropertyEditor::DrawEntityProperties(entity);

        // Some special cases that might not be in the registry or need extra data
        PropertyEditor::DrawMaterial(entity, m_SelectedMeshIndex);

        ImGui::PopID();
    }
} // namespace CHEngine
