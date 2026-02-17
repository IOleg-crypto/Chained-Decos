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
#include "engine/physics/bvh/bvh.h"
#include <yaml-cpp/yaml.h>

namespace CHEngine
{

    std::unordered_map<entt::id_type, PropertyEditor::ComponentMetadata> PropertyEditor::s_ComponentRegistry;

    void PropertyEditor::RegisterComponent(entt::id_type typeId, const ComponentMetadata &metadata)
    {
        s_ComponentRegistry[typeId] = metadata;
    }

    bool PropertyEditor::DrawTextStyle(TextStyle &style)
    {
        bool changed = false;
        auto pb = EditorGUI::Begin();
        if (pb.Float("Font Size", style.FontSize, 1).Color("Text Color", style.TextColor).Changed)
        {
            if (style.FontSize < 0.0f) style.FontSize = 0.0f;
            changed = true;
        }

        const char *alignments[] = {"Left", "Center", "Right"};
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

        pb.Float("Letter Spacing", style.LetterSpacing).Float("Line Height", style.LineHeight);
        if (pb.Bool("Shadow", style.Shadow) && style.Shadow)
        {
            pb.Float("Shadow Offset", style.ShadowOffset).Color("Shadow Color", style.ShadowColor);
        }
        return changed || pb.Changed;
    }

    bool PropertyEditor::DrawUIStyle(UIStyle &style)
    {
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
    }
    void PropertyEditor::Init()
    {
#define REG_HIDDEN(T, name) \
        Register<T>(name, [](auto&, auto) { return false; }); \
        s_ComponentRegistry[entt::type_hash<T>::value()].Visible = false;

        // --- Core & Rendering ---
        Register<TransformComponent>("Transform", [](auto& component, auto entity) {
            bool changed = false;
            if (EditorGUI::DrawVec3("Position", component.Translation)) changed = true;
            if (EditorGUI::DrawVec3("Rotation", component.Rotation))
            {
                component.RotationQuat = QuaternionFromEuler(
                    component.Rotation.x * DEG2RAD, 
                    component.Rotation.y * DEG2RAD, 
                    component.Rotation.z * DEG2RAD
                );
                changed = true;
            }
            if (EditorGUI::DrawVec3("Scale", component.Scale, 1.0f)) changed = true;
            return changed;
        });
        s_ComponentRegistry[entt::type_hash<TransformComponent>::value()].AllowAdd = false;

        Register<CameraComponent>("Camera", [](auto& component, auto entity) {
            bool changed = false;
            auto& camera = component.Camera;

            const char* projectionTypeStrings[] = { "Perspective", "Orthographic" };
            int projectionType = (int)camera.GetProjectionType();
            if (EditorGUI::Property("Projection", projectionType, projectionTypeStrings, 2))
            {
                camera.SetProjectionType((CHEngine::ProjectionType)projectionType);
                changed = true;
            }

            if (camera.GetProjectionType() == CHEngine::ProjectionType::Perspective)
            {
                float verticalFov = camera.GetPerspectiveVerticalFOV() * RAD2DEG;
                if (EditorGUI::Property("Vertical FOV", verticalFov, 1.0f, 1.0f, 180.0f))
                {
                    camera.SetPerspectiveVerticalFOV(verticalFov * DEG2RAD);
                    changed = true;
                }

                float nearClip = camera.GetPerspectiveNearClip();
                if (EditorGUI::Property("Near", nearClip, 0.01f))
                {
                    camera.SetPerspectiveNearClip(nearClip);
                    changed = true;
                }

                float farClip = camera.GetPerspectiveFarClip();
                if (EditorGUI::Property("Far", farClip, 1.0f))
                {
                    camera.SetPerspectiveFarClip(farClip);
                    changed = true;
                }
            }

            if (camera.GetProjectionType() == CHEngine::ProjectionType::Orthographic)
            {
                float orthoSize = camera.GetOrthographicSize();
                if (EditorGUI::Property("Size", orthoSize, 0.1f))
                {
                    camera.SetOrthographicSize(orthoSize);
                    changed = true;
                }

                float nearClip = camera.GetOrthographicNearClip();
                if (EditorGUI::Property("Near", nearClip, 0.01f))
                {
                    camera.SetOrthographicNearClip(nearClip);
                    changed = true;
                }

                float farClip = camera.GetOrthographicFarClip();
                if (EditorGUI::Property("Far", farClip, 0.1f))
                {
                    camera.SetOrthographicFarClip(farClip);
                    changed = true;
                }

                if (EditorGUI::Property("Fixed Aspect Ratio", component.FixedAspectRatio)) changed = true;
            }

            if (EditorGUI::Property("Primary", component.Primary)) changed = true;
            
            ImGui::Separator();
            if (EditorGUI::Property("Orbit Camera Setup", component.IsOrbitCamera)) changed = true;
            
            if (component.IsOrbitCamera)
            {
                if (EditorGUI::Property("Target Tag", component.TargetEntityTag)) changed = true;
                if (EditorGUI::Property("Distance", component.OrbitDistance, 0.1f, 0.0f, 100.0f)) changed = true;
                if (EditorGUI::Property("Yaw", component.OrbitYaw, 0.5f)) changed = true;
                if (EditorGUI::Property("Pitch", component.OrbitPitch, 0.5f, -89.0f, 89.0f)) changed = true;
                if (EditorGUI::Property("Sensitivity", component.LookSensitivity, 0.1f, 0.1f, 5.0f)) changed = true;
            }
            return changed;
        });

        Register<LightComponent>("Light", [](auto& component, auto entity) {
            auto pb = EditorGUI::Begin();
            
            const char* lightTypeStrings[] = { "Point", "Spot" };
            int lightType = (int)component.Type;
            if (EditorGUI::Property("Type", lightType, lightTypeStrings, 2))
            {
                component.Type = (LightType)lightType;
                pb.Changed = true;
            }

            pb.Color("Color", component.LightColor)
              .Float("Intensity", component.Intensity, 0.1f, 0.0f, 100.0f)
              .Float("Radius", component.Radius, 0.1f, 0.0f, 1000.0f);
            
            if (component.Type == LightType::Spot)
            {
                pb.Float("Inner Cutoff", component.InnerCutoff, 0.1f, 0.0f, 90.0f)
                  .Float("Outer Cutoff", component.OuterCutoff, 0.1f, 0.0f, 90.0f);
            }

            if (component.Radius <= 0.01f)
                ImGui::TextColored({ 1, 1, 0, 1 }, ICON_FA_CIRCLE_EXCLAMATION " Radius is 0 (Invisible!)");

            return pb.Changed;
        });

        Register<RigidBodyComponent>("RigidBody", [](auto& component, auto entity) {
            auto pb = EditorGUI::Begin();
            pb.Float("Mass", component.Mass, 0.1f, 0.0f, 1000.0f)
              .Bool("Use Gravity", component.UseGravity)
              .Bool("Is Kinematic", component.IsKinematic);
            return pb.Changed;
        });

        Register<ColliderComponent>("Collider", [](auto& component, auto entity) {
            bool changed = false;
            const char* types[] = {"Box", "Mesh (BVH)", "Capsule"};
            int type = (int)component.Type;
            if (EditorGUI::Property("Type", type, types, (int)std::size(types)))
            {
                component.Type = (ColliderType)type;
                changed = true;
            }
            
            if (EditorGUI::Property("Enabled", component.Enabled)) changed = true;
            if (EditorGUI::DrawVec3("Offset", component.Offset)) changed = true;
            
            if (component.Type == ColliderType::Box)
            {
                if (EditorGUI::DrawVec3("Size", component.Size, 1.0f)) changed = true;
            }
            else if (component.Type == ColliderType::Capsule)
            {
                if (EditorGUI::Property("Radius", component.Radius, 0.05f)) changed = true;
                if (EditorGUI::Property("Height", component.Height, 0.05f)) changed = true;
            }
            else if (component.Type == ColliderType::Mesh)
            {
                if (EditorGUI::Begin().File("Model Path", component.ModelPath, "obj,gltf,glb")) changed = true;
                
                ImGui::Text("BVH Status: %s", component.BVHRoot ? "Built" : "Missing");
                if (ImGui::Button(ICON_FA_HAMMER " Rebuild BVH"))
                {
                    if (auto project = Project::GetActive())
                    {
                        auto asset = project->GetAssetManager()->Get<ModelAsset>(component.ModelPath);
                        if (asset)
                        {
                            component.BVHRoot = BVH::Build(asset);
                            if (component.AutoCalculate)
                            {
                                BoundingBox box = asset->GetBoundingBox();
                                component.Offset = box.min;
                                component.Size = Vector3Subtract(box.max, box.min);
                            }
                            changed = true;
                        }
                    }
                }
            }
            
            if (EditorGUI::Property("Auto Calculate", component.AutoCalculate)) changed = true;
            return changed;
        });

        Register<ShaderComponent>("Shader", [](auto& component, auto entity) {
            auto pb = EditorGUI::Begin();
            
            // Hazel-style Shader Selection
            if (Renderer::IsInitialized())
            {
                auto& lib = Renderer::Get().GetShaderLibrary();
                std::vector<std::string> names = lib.GetNames();
                std::sort(names.begin(), names.end());
                
                // Find current name by path
                std::string currentName = "Custom";
                for (const auto& name : names)
                {
                    if (lib.Get(name)->GetPath() == component.ShaderPath)
                    {
                        currentName = name;
                        break;
                    }
                }

                if (ImGui::BeginCombo("Shader", currentName.c_str()))
                {
                    if (ImGui::Selectable("Custom", currentName == "Custom"))
                    {
                        // Keep current path
                    }

                    for (const auto& name : names)
                    {
                        if (ImGui::Selectable(name.c_str(), currentName == name))
                        {
                            component.ShaderPath = lib.Get(name)->GetPath();
                            pb.Changed = true;
                            
                            // Auto-sync uniforms on change
                            // (We'll trigger the sync button logic below)
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }
            }

            if (EditorGUI::Begin().File("Shader Path", component.ShaderPath, "chshader")) pb.Changed = true;
            
            pb.Bool("Enabled", component.Enabled);
            
            // Automatic Pruning of engine-managed uniforms
            if (!component.Uniforms.empty())
            {
                auto it = std::remove_if(component.Uniforms.begin(), component.Uniforms.end(), [](const auto& u) {
                    std::string name = u.Name;
                    return name == "mvp" || name == "matModel" || name == "matNormal" || 
                           name == "viewPos" || name == "lightDir" || name == "lightColor" || 
                           name == "ambient" || name == "uTime" || name == "useTexture" ||
                           name == "colDiffuse" || name == "texture0" || name == "shininess" ||
                           name == "fogEnabled" || name == "fogColor" || name == "fogDensity" ||
                           name == "fogStart" || name == "fogEnd" || name == "uMode" ||
                           name.find("lights[") != std::string::npos ||
                           name.find("pointLights") != std::string::npos ||
                           name.find("spotLights") != std::string::npos ||
                           name == "boneMatrices";
                });
                if (it != component.Uniforms.end()) {
                    component.Uniforms.erase(it, component.Uniforms.end());
                    pb.Changed = true;
                }
            }

            if (!component.Uniforms.empty() && ImGui::TreeNodeEx("Uniforms", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen))
            {
                for (auto& u : component.Uniforms)
                {
                    ImGui::PushID(u.Name.c_str());
                    if (u.Type == 0) { if (ImGui::DragFloat(u.Name.c_str(), &u.Value[0], 0.05f)) pb.Changed = true; }
                    else if (u.Type == 1) { if (ImGui::DragFloat2(u.Name.c_str(), u.Value, 0.05f)) pb.Changed = true; }
                    else if (u.Type == 2) { if (ImGui::DragFloat3(u.Name.c_str(), u.Value, 0.05f)) pb.Changed = true; }
                    else if (u.Type == 3) { if (ImGui::DragFloat4(u.Name.c_str(), u.Value, 0.05f)) pb.Changed = true; }
                    else if (u.Type == 4) { if (ImGui::ColorEdit4(u.Name.c_str(), u.Value)) pb.Changed = true; }
                    ImGui::PopID();
                }
                ImGui::TreePop();
            }

            ImGui::Separator();
            if (ImGui::Button(ICON_FA_ARROWS_ROTATE " Sync Uniforms"))
            {
                if (auto project = Project::GetActive())
                {
                    std::string fullPath = project->GetAssetManager()->ResolvePath(component.ShaderPath);
                    if (std::filesystem::exists(fullPath))
                    {
                        try {
                            YAML::Node config = YAML::LoadFile(fullPath);
                            if (config["Uniforms"])
                            {
                                // We don't want to lose values if possible, 
                                // but we want to match the metadata list.
                                std::vector<ShaderUniform> newUniforms;
                                for (auto uNode : config["Uniforms"])
                                {
                                    std::string name = uNode.as<std::string>();
                                    // Skip standard uniforms that are managed by engine
                                    bool isEngineManaged = 
                                        name == "mvp" || name == "matModel" || name == "matNormal" || 
                                        name == "viewPos" || name == "lightDir" || name == "lightColor" || 
                                        name == "ambient" || name == "uTime" || name == "useTexture" ||
                                        name == "colDiffuse" || name == "texture0" || name == "shininess" ||
                                        name == "fogEnabled" || name == "fogColor" || name == "fogDensity" ||
                                        name == "fogStart" || name == "fogEnd" ||
                                        name.find("lights[") != std::string::npos ||
                                        name.find("pointLights") != std::string::npos || // Legacy
                                        name.find("spotLights") != std::string::npos ||  // Legacy
                                        name == "boneMatrices";

                                    if (isEngineManaged)
                                        continue;

                                    // Find existing to preserve value
                                    auto it = std::find_if(component.Uniforms.begin(), component.Uniforms.end(), [&](const auto& existing) { return existing.Name == name; });
                                    if (it != component.Uniforms.end())
                                    {
                                        newUniforms.push_back(*it);
                                    }
                                    else
                                    {
                                        // Default to float for now, user can change if we add type detection
                                        // or if we add type to .chshader
                                        ShaderUniform u;
                                        u.Name = name;
                                        u.Type = name.find("Color") != std::string::npos ? 4 : 0; // Guess type
                                        newUniforms.push_back(u);
                                    }
                                }
                                component.Uniforms = newUniforms;
                                pb.Changed = true;
                            }
                        } catch (...) {
                            CH_CORE_ERROR("PropertyEditor: Failed to sync uniforms from {}", component.ShaderPath);
                        }
                    }
                }
            }
            ImGui::SameLine();
            if (ImGui::Button(ICON_FA_TRASH " Clear"))
            {
                component.Uniforms.clear();
                pb.Changed = true;
            }

            return pb.Changed;
        });

        Register<AudioComponent>("Audio", [](auto& component, auto entity) {
            auto pb = EditorGUI::Begin();
            pb.File("Sound Path", component.SoundPath, "wav,ogg,mp3")
              .Bool("Loop", component.Loop)
              .Bool("Play On Start", component.PlayOnStart)
              .Float("Volume", component.Volume, 0.05f, 0.0f, 2.0f)
              .Float("Pitch", component.Pitch, 0.05f, 0.1f, 5.0f);
            return pb.Changed;
        });

        Register<SpawnComponent>("Spawn Zone", [](auto& component, auto entity) {
            auto pb = EditorGUI::Begin();
            pb.Vec3("Zone Size", component.ZoneSize)
              .File("Spawn Texture", component.TexturePath, "png,jpg,tga")
              .Bool("Render Zone", component.RenderSpawnZoneInScene);
            return pb.Changed;
        });

        Register<PlayerComponent>("Player", [](auto& component, auto entity) {
            auto pb = EditorGUI::Begin();
            pb.Float("Speed", component.MovementSpeed)
              .Float("Sensitivity", component.LookSensitivity)
              .Float("Jump Force", component.JumpForce);
            return pb.Changed;
        });

        Register<SceneTransitionComponent>("Scene Transition", [](auto& component, auto entity) {
            return EditorGUI::Begin().File("Target Scene", component.TargetScenePath, "chscene");
        });

        Register<NativeScriptComponent>("Native Script", [](auto& component, Entity entity) {
            bool changed = false;
            for (size_t i = 0; i < component.Scripts.size(); ++i)
            {
                ImGui::TextDisabled(ICON_FA_CODE " %s", component.Scripts[i].ScriptName.c_str());
                ImGui::SameLine();
                ImGui::PushID((int)i);
                if (ImGui::Button(ICON_FA_TRASH))
                {
                    component.Scripts.erase(component.Scripts.begin() + i);
                    ImGui::PopID();
                    changed = true;
                    // Potential crash if we continue loop after erase
                    break;
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
                        {
                            scriptRegistry.AddScript(name, component);
                            changed = true;
                        }
                    }
                }
                ImGui::EndPopup();
            }
            return changed;
        });

        Register<AnimationComponent>("Animation", [](auto& component, auto entity) {
            auto pb = EditorGUI::Begin();
            bool changed = false;

            if (pb.File("Animation Source", component.AnimationPath, "glb,gltf,iqm,m3d").Changed) changed = true;

            int animCount = 0;
            if (entity.template HasComponent<ModelComponent>())
            {
                auto& mc = entity.template GetComponent<ModelComponent>();
                if (mc.Asset) animCount = mc.Asset->GetAnimationCount();
            }

            if (animCount > 0)
            {
                std::shared_ptr<ModelAsset> asset = entity.template GetComponent<ModelComponent>().Asset;
                std::string currentAnimName = asset->GetAnimationName(component.CurrentAnimationIndex);

                if (ImGui::BeginCombo("Current Animation", currentAnimName.c_str()))
                {
                    for (int i = 0; i < animCount; i++)
                    {
                        bool isSelected = (component.CurrentAnimationIndex == i);
                        if (ImGui::Selectable(asset->GetAnimationName(i).c_str(), isSelected))
                        {
                            component.CurrentAnimationIndex = i;
                            changed = true;
                        }
                        if (isSelected) ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
            }
            else
            {
                ImGui::TextDisabled(ICON_FA_CIRCLE_EXCLAMATION " No animations found in ModelAsset.");
            }

            if (pb.Bool("Loop", component.IsLooping).Changed) changed = true;
            if (pb.Bool("Playing", component.IsPlaying).Changed) changed = true;

            ImGui::Text("Current Frame: %d", component.CurrentFrame);

            return changed || pb.Changed;
        });

        Register<ModelComponent>("Model", [](auto& component, auto entity) {
            bool changed = false;
            if (EditorGUI::Begin().File("Model Path", component.ModelPath, "obj,gltf,glb")) changed = true;



            if (EditorGUI::ActionButton(ICON_FA_DOWNLOAD, "Reload Model"))
            {
                component.MaterialsInitialized = false;
                changed = true;
            }
            return changed;
        });

        Register<SpriteComponent>("Sprite", [](auto& component, auto entity) {
            auto pb = EditorGUI::Begin();
            pb.File("Texture", component.TexturePath, "png,jpg,tga")
              .Color("Tint", component.Tint)
              .Bool("Flip X", component.FlipX)
              .Bool("Flip Y", component.FlipY)
              .Int("Z Order", component.ZOrder);
            return pb.Changed;
        });


        // --- UI Widgets ---
        Register<ControlComponent>("Rect Transform", [](auto& component, auto entity) {
            auto& rectTransform = component.Transform;
            bool changed = false;
            
            // --- Anchor Presets ---
            ImGui::Text("Presets:"); ImGui::SameLine();
            if (ImGui::Button("Center")) {
                rectTransform.AnchorMin = {0.5f, 0.5f}; rectTransform.AnchorMax = {0.5f, 0.5f};
                rectTransform.OffsetMin = {-50, -50}; rectTransform.OffsetMax = {50, 50};
                changed = true;
            }
            ImGui::SameLine();
            if (ImGui::Button("Stretch")) {
                rectTransform.AnchorMin = {0.0f, 0.0f}; rectTransform.AnchorMax = {1.0f, 1.0f};
                rectTransform.OffsetMin = {0, 0}; rectTransform.OffsetMax = {0, 0};
                changed = true;
            }

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
                    changed = true;
                }
                if (EditorGUI::Property("Size", size)) {
                    rectTransform.OffsetMin.x = pos.x - size.x * rectTransform.Pivot.x;
                    rectTransform.OffsetMin.y = pos.y - size.y * rectTransform.Pivot.y;
                    rectTransform.OffsetMax.x = pos.x + size.x * (1.0f - rectTransform.Pivot.x);
                    rectTransform.OffsetMax.y = pos.y + size.y * (1.0f - rectTransform.Pivot.y);
                    changed = true;
                }
            }
            else
            {
                float rightPadding = -rectTransform.OffsetMax.x;
                float bottomPadding = -rectTransform.OffsetMax.y;

                if (EditorGUI::Property("Left", rectTransform.OffsetMin.x)) changed = true;
                if (EditorGUI::Property("Top", rectTransform.OffsetMin.y)) changed = true;
                if (EditorGUI::Property("Right", rightPadding)) { rectTransform.OffsetMax.x = -rightPadding; changed = true; }
                if (EditorGUI::Property("Bottom", bottomPadding)) { rectTransform.OffsetMax.y = -bottomPadding; changed = true; }
            }

            if (ImGui::TreeNodeEx("Advanced Layout Settings", ImGuiTreeNodeFlags_SpanAvailWidth))
            {
                if (EditorGUI::Property("Pivot", rectTransform.Pivot)) changed = true;
                if (EditorGUI::Property("Anchor Min", rectTransform.AnchorMin)) changed = true;
                if (EditorGUI::Property("Anchor Max", rectTransform.AnchorMax)) changed = true;
                if (EditorGUI::Property("Rotation", rectTransform.Rotation)) changed = true;
                if (EditorGUI::Property("Scale", rectTransform.Scale)) changed = true;
                if (EditorGUI::Property("Z Order", component.ZOrder)) changed = true;
                if (EditorGUI::Property("Visible", component.IsActive)) changed = true;
                ImGui::TreePop();
            }
            return changed;
        });

        Register<ButtonControl>("Button Widget", [](auto& component, auto entity) {
            bool changed = false;
            if (EditorGUI::Property("Label", component.Label)) changed = true;
            if (EditorGUI::Property("Interactable", component.IsInteractable)) changed = true;
            if (EditorGUI::Property("Auto Size", component.AutoSize)) changed = true;
         
            
            if (ImGui::TreeNodeEx("Style", ImGuiTreeNodeFlags_Framed))
            {
                if (PropertyEditor::DrawUIStyle(component.Style)) changed = true;
                ImGui::TreePop();
            }
            if (ImGui::TreeNodeEx("Text Style", ImGuiTreeNodeFlags_Framed))
            {
                if (PropertyEditor::DrawTextStyle(component.Text)) changed = true;
                ImGui::TreePop();
            }
            return changed;
        });

        Register<PanelControl>("Panel Widget", [](auto& component, auto entity) {
            bool changed = false;
            auto pb = EditorGUI::Begin();
            pb.File("Texture", component.TexturePath, "png,jpg,tga")
              .Bool("Full Screen", component.FullScreen);
            if (pb.Changed) changed = true;
            
            if (ImGui::TreeNodeEx("Style", ImGuiTreeNodeFlags_Framed))
            {
                if (PropertyEditor::DrawUIStyle(component.Style)) changed = true;
                ImGui::TreePop();
            }
            return changed;
        });

        Register<LabelControl>("Label Widget", [](auto& component, auto entity) {
            bool changed = false;
            if (EditorGUI::Property("Text", component.Text)) changed = true;
            if (EditorGUI::Property("Auto Size", component.AutoSize)) changed = true;
            
            if (ImGui::TreeNodeEx("Style", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (PropertyEditor::DrawTextStyle(component.Style)) changed = true;
                ImGui::TreePop();
            }
            return changed;
        });

        Register<SliderControl>("Slider Widget", [](auto& component, auto entity) {
            bool changed = false;
            auto pb = EditorGUI::Begin();
            pb.String("Label", component.Label)
              .Float("Value", component.Value, 0.01f, component.Min, component.Max);
            if (pb.Changed) changed = true;
            
            if (ImGui::TreeNodeEx("Style", ImGuiTreeNodeFlags_Framed))
            {
                if (PropertyEditor::DrawUIStyle(component.Style)) changed = true;
                ImGui::TreePop();
            }
            return changed;
        });

        Register<CheckboxControl>("Checkbox Widget", [](auto& component, auto entity) {
            bool changed = false;
            auto pb = EditorGUI::Begin();
            pb.String("Label", component.Label)
              .Bool("Checked", component.Checked);
            if (pb.Changed) changed = true;
            
            if (ImGui::TreeNodeEx("Style", ImGuiTreeNodeFlags_Framed))
            {
                if (PropertyEditor::DrawUIStyle(component.Style)) changed = true;
                ImGui::TreePop();
            }
            return changed;
        });

        Register<InputTextControl>("Input Text Widget", [](auto& component, auto entity) {
            bool changed = false;
            auto pb = EditorGUI::Begin();
            pb.String("Label", component.Label)
              .String("Text", component.Text)
              .String("Placeholder", component.Placeholder)
              .Int("Max Length", component.MaxLength)
              .Bool("Multiline", component.Multiline)
              .Bool("Read Only", component.ReadOnly)
              .Bool("Password", component.Password);
            if (pb.Changed) changed = true;

            if (ImGui::TreeNodeEx("Text Style", ImGuiTreeNodeFlags_Framed)) { if (PropertyEditor::DrawTextStyle(component.Style)) changed = true; ImGui::TreePop(); }
            if (ImGui::TreeNodeEx("Box Style", ImGuiTreeNodeFlags_Framed)) { if (PropertyEditor::DrawUIStyle(component.BoxStyle)) changed = true; ImGui::TreePop(); }
            return changed;
        });

        Register<ComboBoxControl>("ComboBox Widget", [](auto& component, auto entity) {
            bool changed = false;
            auto pb = EditorGUI::Begin();
            pb.String("Label", component.Label)
              .Int("Selected Index", component.SelectedIndex);
            if (pb.Changed) changed = true;

            if (ImGui::TreeNodeEx("Items", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen))
            {
                for (int i = 0; i < (int)component.Items.size(); i++)
                {
                    ImGui::PushID(i);
                    char buf[256];
                    strncpy(buf, component.Items[i].c_str(), sizeof(buf) - 1);
                    if (ImGui::InputText("##item", buf, sizeof(buf))) { component.Items[i] = buf; changed = true; }
                    ImGui::SameLine();
                    if (ImGui::Button("X")) { component.Items.erase(component.Items.begin() + i); changed = true; ImGui::PopID(); break; }
                    ImGui::PopID();
                }
                if (ImGui::Button("Add Item")) { component.Items.push_back("New Option"); changed = true; }
                ImGui::TreePop();
            }

            if (ImGui::TreeNodeEx("Text Style", ImGuiTreeNodeFlags_Framed)) { if (PropertyEditor::DrawTextStyle(component.Style)) changed = true; ImGui::TreePop(); }
            if (ImGui::TreeNodeEx("Box Style", ImGuiTreeNodeFlags_Framed)) { if (PropertyEditor::DrawUIStyle(component.BoxStyle)) changed = true; ImGui::TreePop(); }
            return changed;
        });

        Register<ProgressBarControl>("ProgressBar Widget", [](auto& component, auto entity) {
            bool changed = false;
            auto pb = EditorGUI::Begin();
            pb.Float("Progress", component.Progress, 0.0f, 1.0f)
              .String("Overlay Text", component.OverlayText)
              .Bool("Show Percentage", component.ShowPercentage);
            if (pb.Changed) changed = true;

            if (ImGui::TreeNodeEx("Text Style", ImGuiTreeNodeFlags_Framed)) { if (PropertyEditor::DrawTextStyle(component.Style)) changed = true; ImGui::TreePop(); }
            if (ImGui::TreeNodeEx("Bar Style", ImGuiTreeNodeFlags_Framed)) { if (PropertyEditor::DrawUIStyle(component.BarStyle)) changed = true; ImGui::TreePop(); }
            return changed;
        });

        Register<ImageControl>("Image Widget", [](auto& component, auto entity) {
            bool changed = false;
            auto pb = EditorGUI::Begin();
            pb.File("Texture Path", component.TexturePath, "png,jpg,tga")
              .Color("Tint Color", component.TintColor)
              .Color("Border Color", component.BorderColor);
            if (pb.Changed) changed = true;

            if (ImGui::TreeNodeEx("Style", ImGuiTreeNodeFlags_Framed)) { if (PropertyEditor::DrawUIStyle(component.Style)) changed = true; ImGui::TreePop(); }
            return changed;
        });

        Register<ImageButtonControl>("Image Button Widget", [](auto& component, auto entity) {
            bool changed = false;
            auto pb = EditorGUI::Begin();
            pb.String("Label", component.Label)
              .File("Texture Path", component.TexturePath, "png,jpg,tga")
              .Color("Tint Color", component.TintColor)
              .Color("Background Color", component.BackgroundColor)
              .Int("Frame Padding", component.FramePadding);
            if (pb.Changed) changed = true;

            if (ImGui::TreeNodeEx("Style", ImGuiTreeNodeFlags_Framed)) { if (PropertyEditor::DrawUIStyle(component.Style)) changed = true; ImGui::TreePop(); }
            return changed;
        });

        Register<SeparatorControl>("Separator Widget", [](auto& component, auto entity) {
            auto pb = EditorGUI::Begin();
            pb.Float("Thickness", component.Thickness)
              .Color("Color", component.LineColor);
            return pb.Changed;
        });

        Register<RadioButtonControl>("RadioButton Widget", [](auto& component, auto entity) {
            bool changed = false;
            auto pb = EditorGUI::Begin();
            pb.String("Label", component.Label)
              .Int("Selected Index", component.SelectedIndex)
              .Bool("Horizontal", component.Horizontal);
            if (pb.Changed) changed = true;

            if (ImGui::TreeNodeEx("Options", ImGuiTreeNodeFlags_Framed))
            {
                for (int i = 0; i < (int)component.Options.size(); i++)
                {
                    ImGui::PushID(i);
                    char buf[256];
                    strncpy(buf, component.Options[i].c_str(), 255);
                    if (ImGui::InputText("##opt", buf, 255)) { component.Options[i] = buf; changed = true; }
                    ImGui::SameLine();
                    if (ImGui::Button("X")) { component.Options.erase(component.Options.begin() + i); changed = true; ImGui::PopID(); break; }
                    ImGui::PopID();
                }
                if (ImGui::Button("Add Option")) { component.Options.push_back("New Option"); changed = true; }
                ImGui::TreePop();
            }
            if (ImGui::TreeNodeEx("Text Style", ImGuiTreeNodeFlags_Framed)) { if (PropertyEditor::DrawTextStyle(component.Style)) changed = true; ImGui::TreePop(); }
            return changed;
        });

        Register<ColorPickerControl>("ColorPicker Widget", [](auto& component, auto entity) {
            bool changed = false;
            auto pb = EditorGUI::Begin();
            pb.String("Label", component.Label)
              .Color("Color", component.SelectedColor)
              .Bool("Show Alpha", component.ShowAlpha)
              .Bool("Show Picker", component.ShowPicker);
            if (pb.Changed) changed = true;

            if (ImGui::TreeNodeEx("Style", ImGuiTreeNodeFlags_Framed)) { if (PropertyEditor::DrawUIStyle(component.Style)) changed = true; ImGui::TreePop(); }
            return changed;
        });

        Register<DragFloatControl>("DragFloat Widget", [](auto& component, auto entity) {
            bool changed = false;
            auto pb = EditorGUI::Begin();
            pb.String("Label", component.Label)
              .Float("Value", component.Value)
              .Float("Speed", component.Speed)
              .Float("Min", component.Min)
              .Float("Max", component.Max)
              .String("Format", component.Format);
            if (pb.Changed) changed = true;

            if (ImGui::TreeNodeEx("Text Style", ImGuiTreeNodeFlags_Framed)) { if (PropertyEditor::DrawTextStyle(component.Style)) changed = true; ImGui::TreePop(); }
            if (ImGui::TreeNodeEx("Box Style", ImGuiTreeNodeFlags_Framed)) { if (PropertyEditor::DrawUIStyle(component.BoxStyle)) changed = true; ImGui::TreePop(); }
            return changed;
        });

        Register<DragIntControl>("DragInt Widget", [](auto& component, auto entity) {
            bool changed = false;
            auto pb = EditorGUI::Begin();
            pb.String("Label", component.Label)
              .Int("Value", component.Value)
              .Float("Speed", component.Speed)
              .Int("Min", component.Min)
              .Int("Max", component.Max)
              .String("Format", component.Format);
            if (pb.Changed) changed = true;

            if (ImGui::TreeNodeEx("Text Style", ImGuiTreeNodeFlags_Framed)) { if (PropertyEditor::DrawTextStyle(component.Style)) changed = true; ImGui::TreePop(); }
            if (ImGui::TreeNodeEx("Box Style", ImGuiTreeNodeFlags_Framed)) { if (PropertyEditor::DrawUIStyle(component.BoxStyle)) changed = true; ImGui::TreePop(); }
            return changed;
        });

        Register<TabBarControl>("TabBar Widget", [](auto& component, auto entity) {
            bool changed = false;
            auto pb = EditorGUI::Begin();
            pb.String("Label", component.Label)
              .Bool("Reorderable", component.Reorderable)
              .Bool("Auto Select New Tabs", component.AutoSelectNewTabs);
            if (pb.Changed) changed = true;

            if (ImGui::TreeNodeEx("Style", ImGuiTreeNodeFlags_Framed)) { if (PropertyEditor::DrawUIStyle(component.Style)) changed = true; ImGui::TreePop(); }
            return changed;
        });

        Register<TabItemControl>("Tab Item Widget", [](auto& component, auto entity) {
            bool changed = false;
            auto pb = EditorGUI::Begin();
            pb.String("Label", component.Label)
              .Bool("Is Open", component.IsOpen);
            if (pb.Changed) changed = true;

            if (ImGui::TreeNodeEx("Text Style", ImGuiTreeNodeFlags_Framed)) { if (PropertyEditor::DrawTextStyle(component.Style)) changed = true; ImGui::TreePop(); }
            return changed;
        });

        Register<CollapsingHeaderControl>("CollapsingHeader Widget", [](auto& component, auto entity) {
            bool changed = false;
            auto pb = EditorGUI::Begin();
            pb.String("Label", component.Label)
              .Bool("Default Open", component.DefaultOpen);
            if (pb.Changed) changed = true;

            if (ImGui::TreeNodeEx("Text Style", ImGuiTreeNodeFlags_Framed)) { if (PropertyEditor::DrawTextStyle(component.Style)) changed = true; ImGui::TreePop(); }
            return changed;
        });

        Register<VerticalLayoutGroup>("Vertical Layout Group", [](auto& component, auto entity) {
            auto pb = EditorGUI::Begin();
            pb.Float("Spacing", component.Spacing)
              .Vec2("Padding", component.Padding);
            return pb.Changed;
        });

        // Helper to setup widgets
        auto setupWidget = [](entt::id_type id) {
            auto& metadata = s_ComponentRegistry[id];
            metadata.IsWidget = true;
            metadata.AllowAdd = true;
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
        setupWidget(entt::type_hash<TabBarControl>::value());
        setupWidget(entt::type_hash<TabItemControl>::value());
        setupWidget(entt::type_hash<CollapsingHeaderControl>::value());

        // Allow adding Rect Transform directly too
        s_ComponentRegistry[entt::type_hash<ControlComponent>::value()].AllowAdd = true;
    }

    void PropertyEditor::DrawEntityProperties(CHEngine::Entity entity)
    {
        auto &registry = entity.GetRegistry();
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
        if (!entity.HasComponent<ModelComponent>())
            return;

        auto &mc = entity.GetComponent<ModelComponent>();
        if (!mc.Asset)
            return;

        const Model &model = mc.Asset->GetModel();
        
        // Helper to draw a single material instance
        auto DrawMaterialInstance = [](MaterialInstance &mat, int index) {
            std::string header = "Material " + std::to_string(index);
            if (ImGui::CollapsingHeader(header.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::PushID(index);
                
                // Albedo
                ImGui::Text("Albedo");
                ImGui::PushID("Albedo");
                EditorGUI::Property("Color", mat.AlbedoColor);
                EditorGUI::Property("Texture", mat.AlbedoPath, "png,jpg,tga,bmp");
                EditorGUI::Property("Use Texture", mat.OverrideAlbedo);
                ImGui::PopID();
                
                // PBR Maps
                ImGui::Text("PBR Maps");
                ImGui::PushID("PBRMaps");
                EditorGUI::Property("Normal Map", mat.NormalMapPath, "png,jpg,tga,bmp");
                EditorGUI::Property("Metallic/Roughness", mat.MetallicRoughnessPath, "png,jpg,tga,bmp");
                EditorGUI::Property("Occlusion", mat.OcclusionMapPath, "png,jpg,tga,bmp");
                ImGui::PopID();
                
                ImGui::Separator();
                
                // Parameters
                ImGui::Text("Parameters");
                ImGui::PushID("Parameters");
                EditorGUI::Property("Metalness", mat.Metalness, 0.01f, 0.0f, 1.0f);
                EditorGUI::Property("Roughness", mat.Roughness, 0.01f, 0.0f, 1.0f);
                ImGui::PopID();

                ImGui::Separator();

                // Emissive
                ImGui::Text("Emissive Bloom");
                ImGui::PushID("Emissive");
                if (EditorGUI::Property("Emissive Color", mat.EmissiveColor))
                {
                    mat.OverrideEmissive = true;
                }
                EditorGUI::Property("Intensity", mat.EmissiveIntensity, 0.1f, 0.0f, 100.0f);
                EditorGUI::Property("Texture", mat.EmissivePath, "png,jpg,tga,bmp");
                ImGui::PopID();
                
                // Rendering
                ImGui::Separator();
                ImGui::Text("Rendering");
                ImGui::PushID("Rendering");
                EditorGUI::Property("Double Sided", mat.DoubleSided);
                EditorGUI::Property("Transparent", mat.Transparent);
                if (mat.Transparent)
                {
                    EditorGUI::Property("Alpha", mat.Alpha, 0.01f, 0.0f, 1.0f);
                }
                ImGui::PopID();
                ImGui::PopID();
            }
        };

        if (hitMeshIndex >= 0 && hitMeshIndex < model.meshCount)
        {
            // Find specific material for this mesh
            // 1. Check for Mesh Index override
            int slotIndex = -1;
            for (int i = 0; i < mc.Materials.size(); i++)
            {
                if (mc.Materials[i].Target == MaterialSlotTarget::MeshIndex && mc.Materials[i].Index == hitMeshIndex)
                {
                    slotIndex = i;
                    break;
                }
            }
            
            // 2. Check for Material Index override
            if (slotIndex == -1 && model.meshMaterial)
            {
                int matIndex = model.meshMaterial[hitMeshIndex];
                for (int i = 0; i < mc.Materials.size(); i++)
                {
                    if (mc.Materials[i].Target == MaterialSlotTarget::MaterialIndex && mc.Materials[i].Index == matIndex)
                    {
                        slotIndex = i;
                        break;
                    }
                }
            }

            if (slotIndex != -1)
            {
                DrawMaterialInstance(mc.Materials[slotIndex].Material, slotIndex);
            }
            else
            {
                ImGui::Text("No Material Slot assigned to this mesh.");
                if (ImGui::Button("Create Override"))
                {
                    // Create new slot for this mesh
                    MaterialSlot newSlot;
                    newSlot.Name = "Mesh Override " + std::to_string(hitMeshIndex);
                    newSlot.Target = MaterialSlotTarget::MeshIndex;
                    newSlot.Index = hitMeshIndex;
                    mc.Materials.push_back(newSlot);
                }
            }
        }
        else
        {
            // Show all materials
            for (int i = 0; i < mc.Materials.size(); i++)
            {
                DrawMaterialInstance(mc.Materials[i].Material, i);
            }
        }
    }

    void PropertyEditor::DrawAddComponentPopup(CHEngine::Entity entity)
    {
        if (ImGui::BeginPopup("AddComponent"))
        {
            bool isUIEntity = entity.HasComponent<ControlComponent>();

            for (auto &[id, metadata] : s_ComponentRegistry)
            {
                if (!metadata.AllowAdd)
                    continue;

                // Filtering: Only show widgets if the entity is a UI entity
                // (or if it's the ControlComponent itself which can be added to any transform)
                if (metadata.IsWidget && !isUIEntity)
                    continue;

                auto &registry = entity.GetRegistry();
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
