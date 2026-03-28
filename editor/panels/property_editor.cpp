#include "property_editor.h"
#include "editor/editor_layer.h"
#include "editor_gui.h"
#include "engine/core/assets/asset_manager.h"
#include "engine/graphics/assets/model_asset.h"
#include "engine/graphics/assets/texture_asset.h"
#include "engine/physics/physics.h"
#include "engine/scene/components.h"
#include "engine/scene/project.h"
#include "engine/scene/scene.h"
#include "engine/scene/scene_settings.h"
#include "imgui/IconsFontAwesome6.h"
#include "panel.h"
#include "imgui.h"
#include <memory>


#include "nfd.h"
#include "scripting/scriptengine.h"

#include <algorithm>
#include <iterator>
#include <yaml-cpp/yaml.h>

namespace CHEngine
{

std::unordered_map<entt::id_type, PropertyEditor::ComponentMetadata> PropertyEditor::s_ComponentRegistry;

void PropertyEditor::RegisterComponent(entt::id_type typeId, const ComponentMetadata& metadata)
{
    s_ComponentRegistry[typeId] = metadata;
}

bool PropertyEditor::DrawTextStyle(TextStyle& style)
{
    bool changed = false;
    auto pb = EditorGUI::Begin();
    if (pb.Float("Font Size", style.FontSize, 1).Color("Text Color", style.TextColor).Changed)
    {
        if (style.FontSize < 0.0f)
        {
            style.FontSize = 0.0f;
        }
        changed = true;
    }

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

    pb.Float("Letter Spacing", style.LetterSpacing).Float("Line Height", style.LineHeight);
    if (pb.Bool("Shadow", style.Shadow) && style.Shadow)
    {
        pb.Float("Shadow Offset", style.ShadowOffset).Color("Shadow Color", style.ShadowColor);
    }
    return changed || pb.Changed;
}

bool PropertyEditor::DrawUIStyle(UIStyle& style)
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
#define REG_HIDDEN(T, name)                                                                                            \
    Register<T>(name, [](auto&, auto) { return false; });                                                              \
    s_ComponentRegistry[entt::type_hash<T>::value()].Visible = false;

    Register<TransformComponent>("Transform", [](auto& component, auto entity) {
        bool changed = false;
        Vector3 translation = *reinterpret_cast<Vector3*>(&component.Translation);
        if (EditorGUI::DrawVec3("Position", translation))
        {
            component.Translation = *reinterpret_cast<glm::vec3*>(&translation);
            component.IsDirty = true;
            changed = true;
        }

        Vector3 rotation = *reinterpret_cast<Vector3*>(&component.Rotation);
        if (EditorGUI::DrawVec3("Rotation", rotation))
        {
            component.Rotation = *reinterpret_cast<glm::vec3*>(&rotation);
            component.SetRotation(component.Rotation * (glm::pi<float>() / 180.0f));
            component.Rotation = component.Rotation * (1.0f / (glm::pi<float>() / 180.0f)); // Keep in degrees for UI
            changed = true;
        }

        Vector3 scaleVec = *reinterpret_cast<Vector3*>(&component.Scale);
        if (EditorGUI::DrawVec3("Scale", scaleVec, 1.0f))
        {
            component.Scale = *reinterpret_cast<glm::vec3*>(&scaleVec);
            component.IsDirty = true;
            changed = true;
        }

        return changed;
    });
    s_ComponentRegistry[entt::type_hash<TransformComponent>::value()].AllowAdd = false;

    Register<CameraComponent>("Camera", [](auto& component, auto entity) {
        bool changed = false;
        auto& camera = component.Camera;

        const char* projectionTypeStrings[] = {"Perspective", "Orthographic"};
        int projectionType = (int)camera.GetProjectionType();
        if (EditorGUI::Property("Projection", projectionType, projectionTypeStrings, 2))
        {
            camera.SetProjectionType((CHEngine::ProjectionType)projectionType);
            changed = true;
        }

        if (camera.GetProjectionType() == CHEngine::ProjectionType::Perspective)
        {
            float verticalFov = camera.GetPerspectiveVerticalFOV() * 57.2957795f;
            if (EditorGUI::Property("Vertical FOV", verticalFov, 1.0f, 1.0f, 180.0f))
            {
                camera.SetPerspectiveVerticalFOV(verticalFov * 0.0174532925f);
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

            if (EditorGUI::Property("Fixed Aspect Ratio", component.FixedAspectRatio))
            {
                changed = true;
            }
        }

        if (EditorGUI::Property("Primary", component.Primary))
        {
            changed = true;
        }

        ImGui::Separator();
        if (EditorGUI::Property("Orbit Camera Setup", component.IsOrbitCamera))
        {
            changed = true;
        }

        if (component.IsOrbitCamera)
        {
            if (EditorGUI::Property("Target Tag", component.TargetEntityTag))
            {
                changed = true;
            }
            if (EditorGUI::Property("Distance", component.OrbitDistance, 0.1f, 0.0f, 100.0f))
            {
                changed = true;
            }
            if (EditorGUI::Property("Yaw", component.OrbitYaw, 0.5f))
            {
                changed = true;
            }
            if (EditorGUI::Property("Pitch", component.OrbitPitch, 0.5f, -89.0f, 89.0f))
            {
                changed = true;
            }
            if (EditorGUI::Property("Sensitivity", component.LookSensitivity, 0.1f, 0.1f, 5.0f))
            {
                changed = true;
            }
        }
        return changed;
    });

    Register<LightComponent>("Light", [](auto& component, auto entity) {
        bool changed = false;

        const char* lightTypeStrings[] = {"Point", "Spot", "Directional"};
        int lightType = (int)component.Type;
        if (EditorGUI::Property("Type", lightType, lightTypeStrings, 3))
        {
            component.Type = (LightType)lightType;
            changed = true;
        }

        if (EditorGUI::Property("Color", component.LightColor))
        {
            changed = true;
        }
        if (EditorGUI::Property("Intensity", component.Intensity, 0.1f, 0.0f, 100.0f))
        {
            changed = true;
        }
        if (EditorGUI::Property("Radius", component.Radius, 0.1f, 0.0f, 1000.0f))
        {
            changed = true;
        }

        if (component.Type == LightType::Spot)
        {
            if (EditorGUI::Property("Inner Cutoff", component.InnerCutoff, 0.1f, 0.0f, 90.0f))
            {
                changed = true;
            }
            if (EditorGUI::Property("Outer Cutoff", component.OuterCutoff, 0.1f, 0.0f, 90.0f))
            {
                changed = true;
            }
        }

        if (component.Radius <= 0.01f)
        {
            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, 100.0f);
            ImGui::NextColumn();
            ImGui::TextColored({1, 1, 0, 1}, ICON_FA_CIRCLE_EXCLAMATION " Radius is 0");
            ImGui::Columns(1);
        }

        return changed;
    });

    Register<RigidBodyComponent>("RigidBody", [](auto& component, auto entity) {
        bool changed = false;
        if (EditorGUI::Property("Mass", component.Mass, 0.1f, 0.0f, 1000.0f))
        {
            changed = true;
        }
        if (EditorGUI::Property("Use Gravity", component.UseGravity))
        {
            changed = true;
        }
        if (EditorGUI::Property("Is Kinematic", component.IsKinematic))
        {
            changed = true;
        }

        Vector3 velocity = *reinterpret_cast<Vector3*>(&component.Velocity);
        if (EditorGUI::DrawVec3("Velocity", velocity))
        {
            component.Velocity = *reinterpret_cast<glm::vec3*>(&velocity);
            // Syncing velocity is handled by SyncECSToJolt so we don't necessarily need RecreateBody here
        }

        if (changed)
        {
            // Physics::RecreateBody(entity); // JOLT REMOVED
        }

        return changed;
    });

    Register<ColliderComponent>("Collider", [](auto& component, auto entity) {
        bool changed = false;
        const char* types[] = {"Box", "Mesh", "Capsule"};
        int type = (int)component.Type;
        if (EditorGUI::Property("Type", type, types, (int)std::size(types)))
        {
            component.Type = (ColliderType)type;
            changed = true;
        }

        if (EditorGUI::Property("Enabled", component.Enabled))
        {
            changed = true;
        }

        ImGui::BeginDisabled(component.AutoCalculate);
        Vector3 offset = *reinterpret_cast<Vector3*>(&component.Offset);
        if (EditorGUI::DrawVec3("Offset", offset))
        {
            component.Offset = *reinterpret_cast<glm::vec3*>(&offset);
            changed = true;
        }
        ImGui::EndDisabled();

        if (component.Type == ColliderType::Box)
        {
            ImGui::BeginDisabled(component.AutoCalculate);
            Vector3 size = *reinterpret_cast<Vector3*>(&component.Size);
            if (EditorGUI::DrawVec3("Size", size, 1.0f))
            {
                component.Size = *reinterpret_cast<glm::vec3*>(&size);
                changed = true;
            }
            ImGui::EndDisabled();
        }
        else if (component.Type == ColliderType::Capsule)
        {
            ImGui::BeginDisabled(component.AutoCalculate);
            if (EditorGUI::Property("Radius", component.Radius, 0.05f))
            {
                changed = true;
            }
            if (EditorGUI::Property("Height", component.Height, 0.05f))
            {
                changed = true;
            }
            ImGui::EndDisabled();
        }
        else if (component.Type == ColliderType::Mesh)
        {
            if (EditorGUI::Property("Model Path", component.ModelPath, "obj,gltf,glb"))
            {
                changed = true;
            }

            ImGui::BeginDisabled(component.AutoCalculate);
            Vector3 size = *reinterpret_cast<Vector3*>(&component.Size);
            if (EditorGUI::DrawVec3("Size", size, 1.0f))
            {
                component.Size = *reinterpret_cast<glm::vec3*>(&size);
                changed = true;
            }
            ImGui::EndDisabled();

            // Action row
            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, 100.0f);
            ImGui::NextColumn();
            if (ImGui::Button(ICON_FA_HAMMER " Rebuild Body", {-1, 0}))
            {
                auto scene = entity.GetRegistry().ctx().template get<Scene*>();
                if (scene)
                {
                    // Physics::DestroyBody(entity); // JOLT REMOVED
                    
                    auto asset = AssetManager::Get().Get<ModelAsset>(component.ModelPath);
                    if (asset && asset->IsReady())
                    {
                        if (component.AutoCalculate)
                        {
                            BoundingBox box = asset->GetBoundingBox();
                            component.Offset = { box.Min.x, box.Min.y, box.Min.z }; 
                            glm::vec3 sz = box.Max - box.Min;
                            component.Size = { sz.x, sz.y, sz.z };
                            
                            CH_CORE_INFO("Rebuild Collider: '{}' -> Offset({:.2f}, {:.2f}, {:.2f}), Size({:.2f}, {:.2f}, {:.2f})", 
                                         component.ModelPath, component.Offset.x, component.Offset.y, component.Offset.z,
                                         component.Size.x, component.Size.y, component.Size.z);
                        }

                        PhysicsSystem::Get().InvalidateBVH(component.ModelPath);
                    }
                    // Physics::CreateBody(entity, asset); // JOLT REMOVED
                    changed = true;
                }
            }
            ImGui::Columns(1);
        }

        if (EditorGUI::Property("Auto Calculate", component.AutoCalculate))
        {
            changed = true;
        }

        return changed;
    });

    Register<ShaderComponent>("Shader", [](auto& component, auto entity) {
        bool changed = false;

        // Hazel-style Shader Selection
        if (Renderer::IsInitialized())
        {
            auto& lib = Renderer::Get().GetShaderLibrary();
            std::vector<std::string> names = lib.GetNames();
            std::sort(names.begin(), names.end());

            std::string currentName = "Custom";
            for (const auto& name : names)
            {
                if (lib.Get(name)->GetPath() == component.ShaderPath)
                {
                    currentName = name;
                    break;
                }
            }

            EditorGUI::BeginProperty("Shader");
            if (ImGui::BeginCombo("##ShaderCombo", currentName.c_str()))
            {
                if (ImGui::Selectable("Custom", currentName == "Custom"))
                {
                }
                for (const auto& name : names)
                {
                    if (ImGui::Selectable(name.c_str(), currentName == name))
                    {
                        component.ShaderPath = lib.Get(name)->GetPath();
                        changed = true;
                    }
                }
                ImGui::EndCombo();
            }
            EditorGUI::EndProperty();
        }

        if (EditorGUI::Property("Shader Path", component.ShaderPath, "chshader"))
        {
            changed = true;
        }
        if (EditorGUI::Property("Enabled", component.Enabled))
        {
            changed = true;
        }

        if (!component.Uniforms.empty() &&
            ImGui::TreeNodeEx("Uniforms", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen))
        {
            for (auto& u : component.Uniforms)
            {
                EditorGUI::BeginProperty(u.Name.c_str());
                if (u.Type == 0)
                {
                    if (ImGui::DragFloat("##U", &u.Value[0], 0.05f))
                    {
                        changed = true;
                    }
                }
                else if (u.Type == 1)
                {
                    if (ImGui::DragFloat2("##U", u.Value, 0.05f))
                    {
                        changed = true;
                    }
                }
                else if (u.Type == 2)
                {
                    if (ImGui::DragFloat3("##U", u.Value, 0.05f))
                    {
                        changed = true;
                    }
                }
                else if (u.Type == 4)
                {
                    if (ImGui::ColorEdit4("##U", u.Value))
                    {
                        changed = true;
                    }
                }
                EditorGUI::EndProperty();
            }
            ImGui::TreePop();
        }

        ImGui::Separator();
        ImGui::Columns(2);
        ImGui::SetColumnWidth(0, 100.0f);
        ImGui::NextColumn();
        if (ImGui::Button(ICON_FA_ARROWS_ROTATE " Sync Uniforms", {-1, 0}))
        {
            if (auto project = Project::GetActive())
            {
                std::string fullPath = AssetManager::Get().ResolvePath(component.ShaderPath);
                if (std::filesystem::exists(fullPath))
                {
                    try
                    {
                        YAML::Node config = YAML::LoadFile(fullPath);
                        if (config["Uniforms"])
                        {
                            std::vector<ShaderUniform> newUniforms;
                            for (auto uNode : config["Uniforms"])
                            {
                                std::string name = uNode.as<std::string>();
                                auto it = std::find_if(component.Uniforms.begin(), component.Uniforms.end(),
                                                       [&](const auto& e) { return e.Name == name; });
                                if (it != component.Uniforms.end())
                                {
                                    newUniforms.push_back(*it);
                                }
                                else
                                {
                                    ShaderUniform u;
                                    u.Name = name;
                                    u.Type = name.find("Color") != std::string::npos ? 4 : 0;
                                    newUniforms.push_back(u);
                                }
                            }
                            component.Uniforms = newUniforms;
                            changed = true;
                        }
                    } catch (...)
                    {
                    }
                }
            }
        }
        ImGui::Columns(1);
        return changed;
    });

    Register<AudioComponent>("Audio", [](auto& component, auto entity) {
        bool changed = false;
        if (EditorGUI::Property("Sound Path", component.SoundPath, "wav,ogg,mp3"))
        {
            changed = true;
        }
        if (EditorGUI::Property("Loop", component.Loop))
        {
            changed = true;
        }
        if (EditorGUI::Property("Play On Start", component.PlayOnStart))
        {
            changed = true;
        }
        if (EditorGUI::Property("Volume", component.Volume, 0.05f, 0.0f, 2.0f))
        {
            changed = true;
        }
        if (EditorGUI::Property("Pitch", component.Pitch, 0.05f, 0.1f, 5.0f))
        {
            changed = true;
        }
        return changed;
    });

    Register<SpawnComponent>("Spawn Zone", [](auto& component, auto entity) {
        bool changed = false;
        if (EditorGUI::DrawVec3("Zone Size", component.ZoneSize))
        {
            changed = true;
        }
        if (EditorGUI::Property("Spawn Texture", component.TexturePath, "png,jpg,tga"))
        {
            changed = true;
        }
        if (EditorGUI::Property("Render Zone", component.RenderSpawnZoneInScene))
        {
            changed = true;
        }
        return changed;
    });

    Register<PlayerComponent>("Player", [](auto& component, auto entity) {
        bool changed = false;
        if (EditorGUI::Property("Speed", component.MovementSpeed))
        {
            changed = true;
        }
        if (EditorGUI::Property("Sensitivity", component.LookSensitivity))
        {
            changed = true;
        }
        if (EditorGUI::Property("Jump Force", component.JumpForce))
        {
            changed = true;
        }
        return changed;
    });

    Register<SceneTransitionComponent>("Scene Transition", [](auto& component, auto entity) {
        return EditorGUI::Property("Target Scene", component.TargetScenePath, "chscene");
    });

    Register<AudioComponent>("Audio Source", [](auto& component, auto entity) {
        auto pb = EditorGUI::Begin();
        pb.File("Sound File", component.SoundPath, "wav,mp3,ogg")
            .Float("Volume", component.Volume, 0.05f, 0.0f, 10.0f)
            .Float("Pitch", component.Pitch, 0.05f, 0.1f, 5.0f)
            .Bool("Loop", component.Loop)
            .Bool("Play on Start", component.PlayOnStart);
        
        ImGui::Separator();
        if (ImGui::Button("Play")) { component.IsPlaying = true; }
        ImGui::SameLine();
        if (ImGui::Button("Stop")) { component.IsPlaying = false; }
        return pb.Changed;
    });

    Register<AnimationComponent>("Animation", [](auto& component, auto entity) {
        auto pb = EditorGUI::Begin();
        pb.File("Animation File", component.AnimationPath, "gltf,glb")
            .Bool("Is Looping", component.IsLooping)
            .Bool("Is Playing", component.IsPlaying)
            .Int("Current Index", component.CurrentAnimationIndex);
        return pb.Changed;
    });

    Register<NavigationComponent>("UI Navigation", [](auto& component, auto entity) {
        bool changed = false;
        auto pb = EditorGUI::Begin();
        pb.Bool("Default Focus", component.IsDefaultFocus);
        
        int up = (int)(uint32_t)component.Up;
        int down = (int)(uint32_t)component.Down;
        int left = (int)(uint32_t)component.Left;
        int right = (int)(uint32_t)component.Right;

        pb.Int("Up (ID)", up).Int("Down (ID)", down).Int("Left (ID)", left).Int("Right (ID)", right);
        if (pb.Changed)
        {
            component.Up = (entt::entity)up;
            component.Down = (entt::entity)down;
            component.Left = (entt::entity)left;
            component.Right = (entt::entity)right;
            changed = true;
        }

        return changed;
    });

    Register<ModelComponent>("Model", [](auto& component, auto entity) {
        bool changed = false;
        if (EditorGUI::Property("Model Path", component.ModelPath, "obj,gltf,glb,iqm,m3d"))
        {
            // Signal reload by resetting the handle; the asset system will re-import
            component.ModelHandle    = 0;
            component.MaterialsInitialized = false;
            changed = true;
        }
        return changed;
    });

    Register<SpriteComponent>("Sprite", [](auto& component, auto entity) {
        auto pb = EditorGUI::Begin();
        pb.File("Texture", component.TexturePath, "png,jpg,bmp,tga")
          .Color("Tint", component.Tint)
          .Bool("Flip X", component.FlipX)
          .Bool("Flip Y", component.FlipY)
          .Int("Z Order", component.ZOrder);
        return pb.Changed;
    });

    Register<PrimitiveComponent>("Primitive", [](auto& component, auto entity) {
        bool changed = false;
        const char* primitiveTypes[] = {"None", "Cube", "Sphere", "Plane", "Cylinder", "Cone", "Torus", "Knot", "Hemisphere"};
        int type = (int)component.Type;
        if (EditorGUI::Property("Type", type, primitiveTypes, 9))
        {
            component.Type = (PrimitiveType)type;
            component.Dirty = true;
            changed = true;
        }

        auto pb = EditorGUI::Begin();
        if (component.Type == PrimitiveType::Cube || component.Type == PrimitiveType::Plane)
        {
            if (EditorGUI::DrawVec3("Dimensions", component.Dimensions, 1.0f)) { component.Dirty = true; changed = true; }
        }
        else if (component.Type == PrimitiveType::Sphere || component.Type == PrimitiveType::Hemisphere)
        {
            pb.Float("Radius", component.Radius, 0.1f);
            pb.Int("Slices", component.Slices, 3, 64);
            pb.Int("Stacks", component.Stacks, 3, 64);
        }
        else if (component.Type == PrimitiveType::Cylinder || component.Type == PrimitiveType::Cone)
        {
            pb.Float("Radius", component.Radius, 0.1f);
            pb.Float("Height", component.Height, 0.1f);
            pb.Int("Slices", component.Slices, 3, 64);
        }
        else if (component.Type == PrimitiveType::Torus)
        {
            pb.Float("Radius", component.Radius, 0.1f);
            pb.Float("Inner Radius", component.InnerRadius, 0.1f);
            pb.Int("Slices", component.Slices, 3, 64);
            pb.Int("Stacks", component.Stacks, 3, 64);
        }
        else if (component.Type == PrimitiveType::Knot)
        {
            pb.Float("Radius", component.Radius, 0.1f);
            pb.Float("Inner Radius", component.InnerRadius, 0.1f);
            pb.Int("Slices", component.Slices, 3, 128);
            pb.Int("Stacks", component.Stacks, 3, 128);
        }

        if (pb.Changed)
        {
            component.Dirty = true;
            changed = true;
        }

        return changed;
    });

    Register<ManagedScriptComponent>("Scripts", [](auto& component, Entity entity) {
        bool changed = false;
        auto& scriptClasses = ScriptEngine::Get().GetScriptClasses();

        for (size_t i = 0; i < component.Scripts.size(); i++)
        {
            auto& script = component.Scripts[i];
            ImGui::PushID((int)i);

            EditorGUI::BeginProperty("Script Class");
            if (ImGui::Button(script.ClassName.empty() ? "None" : script.ClassName.c_str(), {-1, 0}))
            {
                ImGui::OpenPopup("SelectScript");
            }

            if (ImGui::BeginPopup("SelectScript"))
            {
                if (ImGui::Selectable("None", script.ClassName.empty()))
                {
                    script.ClassName = "";
                    changed = true;
                }
                for (const auto& [className, type] : scriptClasses)
                {
                    bool isSelected = (script.ClassName == className);
                    if (ImGui::Selectable(className.c_str(), isSelected))
                    {
                        script.ClassName = className;
                        changed = true;
                    }
                    if (isSelected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndPopup();
            }
            EditorGUI::EndProperty();

            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, 100.0f);
            ImGui::NextColumn();
            if (ImGui::Button(ICON_FA_TRASH " Remove", {-1, 0}))
            {
                component.Scripts.erase(component.Scripts.begin() + i);
                changed = true;
                ImGui::Columns(1);
                ImGui::PopID();
                break;
            }
            ImGui::Columns(1);

            // Draw Fields
            auto* scriptType = ScriptEngine::Get().GetScriptClass(script.ClassName);
            if (scriptType)
            {
                auto fields = scriptType->GetFields();
                for (auto& fieldInfo : fields)
                {
                    if (fieldInfo.GetAccessibility() != Coral::TypeAccessibility::Public)
                        continue;

                    std::string fieldName = (std::string)fieldInfo.GetName();
                    auto& fieldType = fieldInfo.GetType();

                    // Check if we already have a persistent value
                    if (script.Fields.find(fieldName) == script.Fields.end())
                    {
                        // Add to map with default if possible
                        ScriptField field;
                        field.Name = fieldName;
                        
                        auto mt = fieldType.GetManagedType();
                        if (mt == Coral::ManagedType::Float) { field.Type = ScriptFieldType::Float; field.Value = 0.0f; }
                        else if (mt == Coral::ManagedType::Int) { field.Type = ScriptFieldType::Int; field.Value = 0; }
                        else if (mt == Coral::ManagedType::Bool) { field.Type = ScriptFieldType::Bool; field.Value = false; }
                        else if (mt == Coral::ManagedType::String) { field.Type = ScriptFieldType::String; field.Value = std::string(""); }
                        
                        if (field.Type != ScriptFieldType::None)
                        {
                            // Try to get current value if instance exists
                            if (script.Instance)
                            {
                                auto* obj = static_cast<Coral::ManagedObject*>(script.Instance);
                                if (field.Type == ScriptFieldType::Float) field.Value = obj->GetFieldValue<float>(fieldName);
                                else if (field.Type == ScriptFieldType::Int) field.Value = obj->GetFieldValue<int>(fieldName);
                                else if (field.Type == ScriptFieldType::Bool) field.Value = obj->GetFieldValue<bool>(fieldName);
                                else if (field.Type == ScriptFieldType::String) field.Value = obj->GetFieldValue<std::string>(fieldName);
                            }
                            script.Fields[fieldName] = field;
                        }
                    }

                    if (script.Fields.count(fieldName))
                    {
                        auto& field = script.Fields[fieldName];
                        bool fieldChanged = false;
                        
                        EditorGUI::BeginProperty(fieldName.c_str());
                        if (field.Type == ScriptFieldType::Float)
                        {
                            float val = std::get<float>(field.Value);
                            if (ImGui::DragFloat("##F", &val, 0.1f)) { field.Value = val; fieldChanged = true; }
                        }
                        else if (field.Type == ScriptFieldType::Int)
                        {
                            int val = std::get<int>(field.Value);
                            if (ImGui::DragInt("##I", &val)) { field.Value = val; fieldChanged = true; }
                        }
                        else if (field.Type == ScriptFieldType::Bool)
                        {
                            bool val = std::get<bool>(field.Value);
                            if (ImGui::Checkbox("##B", &val)) { field.Value = val; fieldChanged = true; }
                        }
                        else if (field.Type == ScriptFieldType::String)
                        {
                            std::string val = std::get<std::string>(field.Value);
                            char buffer[256];
                            memset(buffer, 0, sizeof(buffer));
                            strncpy(buffer, val.c_str(), sizeof(buffer) - 1);
                            if (ImGui::InputText("##S", buffer, sizeof(buffer))) { field.Value = std::string(buffer); fieldChanged = true; }
                        }
                        EditorGUI::EndProperty();

                        if (fieldChanged)
                        {
                            changed = true;
                            if (script.Instance)
                            {
                                auto* obj = static_cast<Coral::ManagedObject*>(script.Instance);
                                std::visit([&](auto&& v) { obj->SetFieldValue(fieldName, v); }, field.Value);
                            }
                        }
                    }
                }
            }

            ImGui::PopID();
            ImGui::Separator();
        }

        ImGui::Columns(2);
        ImGui::SetColumnWidth(0, 100.0f);
        ImGui::NextColumn();
        if (ImGui::Button(ICON_FA_PLUS " Add Script", {-1, 0}))
        {
            component.Scripts.push_back({});
            changed = true;
        }
        ImGui::Columns(1);

        return changed;
    });

    Register<AnimationComponent>("Animations", [](auto& component, auto entity) {
        bool changed = false;
        if (EditorGUI::Property("Looping", component.IsLooping))
        {
            changed = true;
        }

        if (EditorGUI::Property("Playing", component.IsPlaying))
        {
            changed = true;
        }

        int animCount = 0;
        std::shared_ptr<ModelAsset> modelAsset;
        if (entity.template HasComponent<ModelComponent>())
        {
            auto& mc = entity.template GetComponent<ModelComponent>();
            modelAsset = AssetManager::Get().Get<ModelAsset>(mc.ModelPath);
            if (modelAsset)
            {
                animCount = modelAsset->GetAnimationCount();
            }
        }

        if (animCount > 0)
        {
            std::string currentAnimName = modelAsset->GetAnimationName(component.CurrentAnimationIndex);

            EditorGUI::BeginProperty("Current Animation");
            if (ImGui::BeginCombo("##AnimCombo", currentAnimName.c_str()))
            {
                for (int i = 0; i < animCount; i++)
                {
                    bool isSelected = (component.CurrentAnimationIndex == i);
                    if (ImGui::Selectable(modelAsset->GetAnimationName(i).c_str(), isSelected))
                    {
                        component.CurrentAnimationIndex = i;
                        changed = true;
                    }
                    if (isSelected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            EditorGUI::EndProperty();
        }
        else
        {
            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, 100.0f);
            ImGui::NextColumn();
            ImGui::TextDisabled(ICON_FA_CIRCLE_EXCLAMATION " No animations found");
            ImGui::Columns(1);
        }

        ImGui::Columns(2);
        ImGui::SetColumnWidth(0, 100.0f);
        ImGui::Text("Frame");
        ImGui::NextColumn();
        ImGui::Text("%d", component.CurrentFrame);
        ImGui::Columns(1);

        return changed;
    });

    Register<UIActionComponent>("UI Action", [](auto& component, auto entity) {
        auto pb = EditorGUI::Begin();

        // Target Entity Selection (Simplified for now - using UUID as string/hidden)
        // Ideally we'd have a picker
        std::string uuidStr = component.TargetEntityID.ToString();
        if (EditorGUI::Property("Target UUID", uuidStr))
        {
            component.TargetEntityID = UUID(uuidStr);
            pb.Changed = true;
        }

        pb.String("Parameter", component.ParameterName).Float("Value", component.Value);

        return pb.Changed;
    });

    Register<ModelComponent>("Model", [](auto& component, auto entity) {
        bool changed = false;
        if (EditorGUI::Begin().File("Model Path", component.ModelPath, "obj,gltf,glb"))
        {
            changed = true;
        }

        if (EditorGUI::ActionButton(ICON_FA_DOWNLOAD, "Reload Model"))
        {
            component.MaterialsInitialized = false;
            changed = true;
        }

        return changed;
    });

    Register<PrimitiveComponent>("Primitive", [](auto& component, auto entity) {
        bool changed = false;
        const char* primitiveTypes[] = {"None", "Cube",  "Sphere", "Plane",     "Cylinder",
                                        "Cone", "Torus", "Knot",   "Hemisphere"};
        int type = (int)component.Type;
        if (EditorGUI::Property("Shape", type, primitiveTypes, (int)std::size(primitiveTypes)))
        {
            component.Type = (PrimitiveType)type;
            component.Asset = nullptr; // Reset asset cache for type change
            component.Dirty = true;
            changed = true;
        }

        if (component.Type == PrimitiveType::None)
        {
            return changed;
        }

        ImGui::Separator();

        if (component.Type == PrimitiveType::Cube)
        {
            if (EditorGUI::DrawVec3("Dimensions", component.Dimensions, 1.0f))
            {
                changed = true;
            }
        }
        else if (component.Type == PrimitiveType::Sphere || component.Type == PrimitiveType::Hemisphere)
        {
            if (EditorGUI::Property("Radius", component.Radius, 0.05f))
            {
                changed = true;
            }
            if (EditorGUI::Property("Slices", component.Slices, 3, 128))
            {
                changed = true;
            }
            if (EditorGUI::Property("Stacks", component.Stacks, 3, 128))
            {
                changed = true;
            }
        }
        else if (component.Type == PrimitiveType::Plane)
        {
            Vector2 size = {component.Dimensions.x, component.Dimensions.z};
            if (EditorGUI::Property("Size", size))
            {
                component.Dimensions.x = size.x;
                component.Dimensions.z = size.y;
                changed = true;
            }
            if (EditorGUI::Property("Res X", component.Slices, 1, 128))
            {
                changed = true;
            }
            if (EditorGUI::Property("Res Z", component.Stacks, 1, 128))
            {
                changed = true;
            }
        }
        else if (component.Type == PrimitiveType::Cylinder || component.Type == PrimitiveType::Cone)
        {
            if (EditorGUI::Property("Radius", component.Radius, 0.05f))
            {
                changed = true;
            }
            if (EditorGUI::Property("Height", component.Height, 0.05f))
            {
                changed = true;
            }
            if (EditorGUI::Property("Slices", component.Slices, 3, 128))
            {
                changed = true;
            }
        }
        else if (component.Type == PrimitiveType::Torus || component.Type == PrimitiveType::Knot)
        {
            if (EditorGUI::Property("Radius", component.Radius, 0.05f))
            {
                changed = true;
            }
            if (EditorGUI::Property("Inner Radius", component.InnerRadius, 0.05f))
            {
                changed = true;
            }
            if (EditorGUI::Property("Slices", component.Slices, 3, 128))
            {
                changed = true;
            }
            if (EditorGUI::Property("Stacks", component.Stacks, 3, 128))
            {
                changed = true;
            }
        }

        if (changed)
        {
            component.Dirty = true;
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
        ImGui::Text("Presets:");
        ImGui::SameLine();
        if (ImGui::Button("Center"))
        {
            rectTransform.AnchorMin = {0.5f, 0.5f};
            rectTransform.AnchorMax = {0.5f, 0.5f};
            rectTransform.OffsetMin = {-50, -50};
            rectTransform.OffsetMax = {50, 50};
            changed = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Stretch"))
        {
            rectTransform.AnchorMin = {0.0f, 0.0f};
            rectTransform.AnchorMax = {1.0f, 1.0f};
            rectTransform.OffsetMin = {0, 0};
            rectTransform.OffsetMax = {0, 0};
            changed = true;
        }

        bool isPoint = (rectTransform.AnchorMin.x == rectTransform.AnchorMax.x &&
                        rectTransform.AnchorMin.y == rectTransform.AnchorMax.y);
        if (isPoint)
        {
            float width = rectTransform.OffsetMax.x - rectTransform.OffsetMin.x;
            float height = rectTransform.OffsetMax.y - rectTransform.OffsetMin.y;
            float posX = rectTransform.OffsetMin.x + width * rectTransform.Pivot.x;
            float posY = rectTransform.OffsetMin.y + height * rectTransform.Pivot.y;

            Vector2 pos = {posX, posY};
            Vector2 size = {width, height};

            if (EditorGUI::Property("Pos", pos))
            {
                rectTransform.OffsetMin.x = pos.x - size.x * rectTransform.Pivot.x;
                rectTransform.OffsetMin.y = pos.y - size.y * rectTransform.Pivot.y;
                rectTransform.OffsetMax.x = pos.x + size.x * (1.0f - rectTransform.Pivot.x);
                rectTransform.OffsetMax.y = pos.y + size.y * (1.0f - rectTransform.Pivot.y);
                changed = true;
            }
            if (EditorGUI::Property("Size", size))
            {
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

            if (EditorGUI::Property("Left", rectTransform.OffsetMin.x))
            {
                changed = true;
            }
            if (EditorGUI::Property("Top", rectTransform.OffsetMin.y))
            {
                changed = true;
            }
            if (EditorGUI::Property("Right", rightPadding))
            {
                rectTransform.OffsetMax.x = -rightPadding;
                changed = true;
            }
            if (EditorGUI::Property("Bottom", bottomPadding))
            {
                rectTransform.OffsetMax.y = -bottomPadding;
                changed = true;
            }
        }

        if (ImGui::TreeNodeEx("Advanced Layout Settings", ImGuiTreeNodeFlags_SpanAvailWidth))
        {
            if (EditorGUI::Property("Pivot", rectTransform.Pivot))
            {
                changed = true;
            }
            if (EditorGUI::Property("Anchor Min", rectTransform.AnchorMin))
            {
                changed = true;
            }
            if (EditorGUI::Property("Anchor Max", rectTransform.AnchorMax))
            {
                changed = true;
            }
            if (EditorGUI::Property("Rotation", rectTransform.Rotation))
            {
                changed = true;
            }
            if (EditorGUI::Property("Scale", rectTransform.Scale))
            {
                changed = true;
            }
            if (EditorGUI::Property("Z Order", component.ZOrder))
            {
                changed = true;
            }
            if (EditorGUI::Property("Visible", component.IsActive))
            {
                changed = true;
            }
            ImGui::TreePop();
        }
        return changed;
    });

    Register<ButtonControl>("Button Widget", [](auto& component, auto entity) {
        bool changed = false;
        if (EditorGUI::Property("Label", component.Label))
        {
            changed = true;
        }
        if (EditorGUI::Property("Interactable", component.IsInteractable))
        {
            changed = true;
        }
        if (EditorGUI::Property("Auto Size", component.AutoSize))
        {
            changed = true;
        }

        if (ImGui::TreeNodeEx("Style", ImGuiTreeNodeFlags_Framed))
        {
            if (PropertyEditor::DrawUIStyle(component.Style))
            {
                changed = true;
            }
            ImGui::TreePop();
        }
        if (ImGui::TreeNodeEx("Text Style", ImGuiTreeNodeFlags_Framed))
        {
            if (PropertyEditor::DrawTextStyle(component.Text))
            {
                changed = true;
            }
            ImGui::TreePop();
        }
        return changed;
    });

    Register<PanelControl>("Panel Widget", [](auto& component, auto entity) {
        bool changed = false;
        auto pb = EditorGUI::Begin();
        pb.File("Texture", component.TexturePath, "png,jpg,tga").Bool("Full Screen", component.FullScreen);
        if (pb.Changed)
        {
            changed = true;
        }

        if (ImGui::TreeNodeEx("Style", ImGuiTreeNodeFlags_Framed))
        {
            if (PropertyEditor::DrawUIStyle(component.Style))
            {
                changed = true;
            }
            ImGui::TreePop();
        }
        return changed;
    });

    Register<LabelControl>("Label Widget", [](auto& component, auto entity) {
        bool changed = false;
        if (EditorGUI::Property("Text", component.Text))
        {
            changed = true;
        }
        if (EditorGUI::Property("Auto Size", component.AutoSize))
        {
            changed = true;
        }

        if (ImGui::TreeNodeEx("Style", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (PropertyEditor::DrawTextStyle(component.Style))
            {
                changed = true;
            }
            ImGui::TreePop();
        }
        return changed;
    });

    Register<SliderControl>("Slider Widget", [](auto& component, auto entity) {
        bool changed = false;
        auto pb = EditorGUI::Begin();
        pb.String("Label", component.Label).Float("Value", component.Value, 0.01f, component.Min, component.Max);
        if (pb.Changed)
        {
            changed = true;
        }

        if (ImGui::TreeNodeEx("Style", ImGuiTreeNodeFlags_Framed))
        {
            if (PropertyEditor::DrawUIStyle(component.Style))
            {
                changed = true;
            }
            ImGui::TreePop();
        }
        return changed;
    });

    Register<CheckboxControl>("Checkbox Widget", [](auto& component, auto entity) {
        bool changed = false;
        auto pb = EditorGUI::Begin();
        pb.String("Label", component.Label).Bool("Checked", component.Checked);
        if (pb.Changed)
        {
            changed = true;
        }

        if (ImGui::TreeNodeEx("Style", ImGuiTreeNodeFlags_Framed))
        {
            if (PropertyEditor::DrawUIStyle(component.Style))
            {
                changed = true;
            }
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
        if (pb.Changed)
        {
            changed = true;
        }

        if (ImGui::TreeNodeEx("Text Style", ImGuiTreeNodeFlags_Framed))
        {
            if (PropertyEditor::DrawTextStyle(component.Style))
            {
                changed = true;
            }
            ImGui::TreePop();
        }
        if (ImGui::TreeNodeEx("Box Style", ImGuiTreeNodeFlags_Framed))
        {
            if (PropertyEditor::DrawUIStyle(component.BoxStyle))
            {
                changed = true;
            }
            ImGui::TreePop();
        }
        return changed;
    });

    Register<ComboBoxControl>("ComboBox Widget", [](auto& component, auto entity) {
        bool changed = false;
        auto pb = EditorGUI::Begin();
        pb.String("Label", component.Label).Int("Selected Index", component.SelectedIndex);
        if (pb.Changed)
        {
            changed = true;
        }

        if (ImGui::TreeNodeEx("Items", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen))
        {
            for (int i = 0; i < (int)component.Items.size(); i++)
            {
                ImGui::PushID(i);
                char buf[256];
                strncpy(buf, component.Items[i].c_str(), sizeof(buf) - 1);
                if (ImGui::InputText("##item", buf, sizeof(buf)))
                {
                    component.Items[i] = buf;
                    changed = true;
                }
                ImGui::SameLine();
                if (ImGui::Button("X"))
                {
                    component.Items.erase(component.Items.begin() + i);
                    changed = true;
                    ImGui::PopID();
                    break;
                }
                ImGui::PopID();
            }
            if (ImGui::Button("Add Item"))
            {
                component.Items.push_back("New Option");
                changed = true;
            }
            ImGui::TreePop();
        }

        if (ImGui::TreeNodeEx("Text Style", ImGuiTreeNodeFlags_Framed))
        {
            if (PropertyEditor::DrawTextStyle(component.Style))
            {
                changed = true;
            }
            ImGui::TreePop();
        }
        if (ImGui::TreeNodeEx("Box Style", ImGuiTreeNodeFlags_Framed))
        {
            if (PropertyEditor::DrawUIStyle(component.BoxStyle))
            {
                changed = true;
            }
            ImGui::TreePop();
        }
        return changed;
    });

    Register<ProgressBarControl>("ProgressBar Widget", [](auto& component, auto entity) {
        bool changed = false;
        auto pb = EditorGUI::Begin();
        pb.Float("Progress", component.Progress, 0.0f, 1.0f)
            .String("Overlay Text", component.OverlayText)
            .Bool("Show Percentage", component.ShowPercentage);
        if (pb.Changed)
        {
            changed = true;
        }

        if (ImGui::TreeNodeEx("Text Style", ImGuiTreeNodeFlags_Framed))
        {
            if (PropertyEditor::DrawTextStyle(component.Style))
            {
                changed = true;
            }
            ImGui::TreePop();
        }
        if (ImGui::TreeNodeEx("Bar Style", ImGuiTreeNodeFlags_Framed))
        {
            if (PropertyEditor::DrawUIStyle(component.BarStyle))
            {
                changed = true;
            }
            ImGui::TreePop();
        }
        return changed;
    });

    Register<ImageControl>("Image Widget", [](auto& component, auto entity) {
        bool changed = false;
        auto pb = EditorGUI::Begin();
        pb.File("Texture Path", component.TexturePath, "png,jpg,tga")
            .Color("Tint Color", component.TintColor)
            .Color("Border Color", component.BorderColor);
        if (pb.Changed)
        {
            changed = true;
        }

        if (ImGui::TreeNodeEx("Style", ImGuiTreeNodeFlags_Framed))
        {
            if (PropertyEditor::DrawUIStyle(component.Style))
            {
                changed = true;
            }
            ImGui::TreePop();
        }
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
        if (pb.Changed)
        {
            changed = true;
        }

        if (ImGui::TreeNodeEx("Style", ImGuiTreeNodeFlags_Framed))
        {
            if (PropertyEditor::DrawUIStyle(component.Style))
            {
                changed = true;
            }
            ImGui::TreePop();
        }
        return changed;
    });

    Register<SeparatorControl>("Separator Widget", [](auto& component, auto entity) {
        auto pb = EditorGUI::Begin();
        pb.Float("Thickness", component.Thickness).Color("Color", component.LineColor);
        return pb.Changed;
    });

    Register<RadioButtonControl>("RadioButton Widget", [](auto& component, auto entity) {
        bool changed = false;
        auto pb = EditorGUI::Begin();
        pb.String("Label", component.Label)
            .Int("Selected Index", component.SelectedIndex)
            .Bool("Horizontal", component.Horizontal);
        if (pb.Changed)
        {
            changed = true;
        }

        if (ImGui::TreeNodeEx("Options", ImGuiTreeNodeFlags_Framed))
        {
            for (int i = 0; i < (int)component.Options.size(); i++)
            {
                ImGui::PushID(i);
                char buf[256];
                strncpy(buf, component.Options[i].c_str(), 255);
                if (ImGui::InputText("##opt", buf, 255))
                {
                    component.Options[i] = buf;
                    changed = true;
                }
                ImGui::SameLine();
                if (ImGui::Button("X"))
                {
                    component.Options.erase(component.Options.begin() + i);
                    changed = true;
                    ImGui::PopID();
                    break;
                }
                ImGui::PopID();
            }
            if (ImGui::Button("Add Option"))
            {
                component.Options.push_back("New Option");
                changed = true;
            }
            ImGui::TreePop();
        }
        if (ImGui::TreeNodeEx("Text Style", ImGuiTreeNodeFlags_Framed))
        {
            if (PropertyEditor::DrawTextStyle(component.Style))
            {
                changed = true;
            }
            ImGui::TreePop();
        }
        return changed;
    });

    Register<ColorPickerControl>("ColorPicker Widget", [](auto& component, auto entity) {
        bool changed = false;
        auto pb = EditorGUI::Begin();
        pb.String("Label", component.Label)
            .Color("Color", component.SelectedColor)
            .Bool("Show Alpha", component.ShowAlpha)
            .Bool("Show Picker", component.ShowPicker);
        if (pb.Changed)
        {
            changed = true;
        }

        if (ImGui::TreeNodeEx("Style", ImGuiTreeNodeFlags_Framed))
        {
            if (PropertyEditor::DrawUIStyle(component.Style))
            {
                changed = true;
            }
            ImGui::TreePop();
        }
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
        if (pb.Changed)
        {
            changed = true;
        }

        if (ImGui::TreeNodeEx("Text Style", ImGuiTreeNodeFlags_Framed))
        {
            if (PropertyEditor::DrawTextStyle(component.Style))
            {
                changed = true;
            }
            ImGui::TreePop();
        }
        if (ImGui::TreeNodeEx("Box Style", ImGuiTreeNodeFlags_Framed))
        {
            if (PropertyEditor::DrawUIStyle(component.BoxStyle))
            {
                changed = true;
            }
            ImGui::TreePop();
        }
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
        if (pb.Changed)
        {
            changed = true;
        }

        if (ImGui::TreeNodeEx("Text Style", ImGuiTreeNodeFlags_Framed))
        {
            if (PropertyEditor::DrawTextStyle(component.Style))
            {
                changed = true;
            }
            ImGui::TreePop();
        }
        if (ImGui::TreeNodeEx("Box Style", ImGuiTreeNodeFlags_Framed))
        {
            if (PropertyEditor::DrawUIStyle(component.BoxStyle))
            {
                changed = true;
            }
            ImGui::TreePop();
        }
        return changed;
    });

    Register<TabBarControl>("TabBar Widget", [](auto& component, auto entity) {
        bool changed = false;
        auto pb = EditorGUI::Begin();
        pb.String("Label", component.Label)
            .Bool("Reorderable", component.Reorderable)
            .Bool("Auto Select New Tabs", component.AutoSelectNewTabs);
        if (pb.Changed)
        {
            changed = true;
        }

        if (ImGui::TreeNodeEx("Style", ImGuiTreeNodeFlags_Framed))
        {
            if (PropertyEditor::DrawUIStyle(component.Style))
            {
                changed = true;
            }
            ImGui::TreePop();
        }
        return changed;
    });

    Register<TabItemControl>("Tab Item Widget", [](auto& component, auto entity) {
        bool changed = false;
        auto pb = EditorGUI::Begin();
        pb.String("Label", component.Label).Bool("Is Open", component.IsOpen);
        if (pb.Changed)
        {
            changed = true;
        }

        if (ImGui::TreeNodeEx("Text Style", ImGuiTreeNodeFlags_Framed))
        {
            if (PropertyEditor::DrawTextStyle(component.Style))
            {
                changed = true;
            }
            ImGui::TreePop();
        }
        return changed;
    });

    Register<CollapsingHeaderControl>("CollapsingHeader Widget", [](auto& component, auto entity) {
        bool changed = false;
        auto pb = EditorGUI::Begin();
        pb.String("Label", component.Label).Bool("Default Open", component.DefaultOpen);
        if (pb.Changed)
        {
            changed = true;
        }

        if (ImGui::TreeNodeEx("Text Style", ImGuiTreeNodeFlags_Framed))
        {
            if (PropertyEditor::DrawTextStyle(component.Style))
            {
                changed = true;
            }
            ImGui::TreePop();
        }
        return changed;
    });

    Register<VerticalLayoutGroup>("Vertical Layout Group", [](auto& component, auto entity) {
        auto pb = EditorGUI::Begin();
        pb.Float("Spacing", component.Spacing).Vec2("Padding", component.Padding);
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
    auto& registry = entity.GetRegistry();
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
                auto& metadata = s_ComponentRegistry[id];
                if (!metadata.Visible)
                {
                    continue;
                }

                // Logic to reduce clutter
                if (isUI && id == entt::type_hash<TransformComponent>::value())
                {
                    continue;
                }

                if (hasWidget && id == entt::type_hash<ControlComponent>::value())
                {
                    continue;
                }

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
        auto& tag = entity.GetComponent<TagComponent>().Tag;
        char buffer[256];
        memset(buffer, 0, sizeof(buffer));
        strncpy(buffer, tag.c_str(), sizeof(buffer) - 1);

        ImGui::Text("Tag");
        ImGui::SameLine();
        if (ImGui::InputText("##Tag", buffer, sizeof(buffer)))
        {
            tag = std::string(buffer);
        }
    }
}

static bool DrawTextureProperty(const char* label, std::string& path)
{
    bool changed = EditorGUI::Property(label, path, "png,jpg,tga,bmp");

    if (!path.empty())
    {
        auto textureAsset = AssetManager::Get().Get<TextureAsset>(path);
        if (textureAsset && textureAsset->GetState() == AssetState::Ready)
        {
            ImGui::SameLine();
            ImTextureID id = (ImTextureID)(intptr_t)textureAsset->GetTexture().id;
            ImGui::Image(id, {20, 20}, {0, 1}, {1, 0});
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Image(id, {256, 256}, {0, 1}, {1, 0});
                ImGui::Text("%s", path.c_str());
                ImGui::EndTooltip();
            }
        }
    }
    return changed;
}

static std::string GetTexturePathFromID(uint32_t id, const std::vector<std::shared_ptr<CHEngine::TextureAsset>>& textures)
{
    if (id == 0)
        return "";

    for (const auto& tex : textures)
    {
        if (tex->GetTexture().id == id)
            return tex->GetPath();
    }
    return "";
}


void PropertyEditor::DrawMaterial(CHEngine::Entity entity, int hitMeshIndex)
{
    if (!entity.HasComponent<ModelComponent>())
        return;

    auto& mc = entity.GetComponent<ModelComponent>();
    auto mcAsset = AssetManager::Get().Get<ModelAsset>(mc.ModelPath);
    if (!mcAsset || mcAsset->GetState() != AssetState::Ready)
        return;

    const Model& model = mcAsset->GetModel();
    auto modelTextures = mcAsset->GetTextures();

    // Helper to draw a single material instance
    auto DrawMaterialInstance = [&](MaterialInstance& mat, int index, bool isOverride) {
        std::string header = "Material " + std::to_string(index) + (isOverride ? " (Override)" : " (Default)");
        if (ImGui::CollapsingHeader(header.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::PushID(index);
            ImGui::BeginDisabled(!isOverride);

            // Albedo
            ImGui::Text("Albedo");
            ImGui::PushID("Albedo");
            EditorGUI::Property("Color", mat.AlbedoColor);
            DrawTextureProperty("Texture", mat.AlbedoPath);
            EditorGUI::Property("Use Texture", mat.OverrideAlbedo);
            ImGui::PopID();

            // PBR Maps
            ImGui::Text("PBR Maps");
            ImGui::PushID("PBRMaps");
            DrawTextureProperty("Normal Map", mat.NormalMapPath);
            DrawTextureProperty("Metallic/Roughness", mat.MetallicRoughnessPath);
            DrawTextureProperty("Occlusion", mat.OcclusionMapPath);
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
                mat.OverrideEmissive = true;
            EditorGUI::Property("Intensity", mat.EmissiveIntensity, 0.1f, 0.0f, 100.0f);
            DrawTextureProperty("Texture", mat.EmissivePath);
            ImGui::PopID();

            // Rendering
            ImGui::Separator();
            ImGui::Text("Rendering");
            ImGui::PushID("Rendering");
            EditorGUI::Property("Double Sided", mat.DoubleSided);
            EditorGUI::Property("Transparent", mat.Transparent);
            if (mat.Transparent)
                EditorGUI::Property("Alpha", mat.Alpha, 0.01f, 0.0f, 1.0f);

            ImGui::PopID();
            ImGui::EndDisabled();

            if (!isOverride)
            {
                ImGui::PushStyleColor(ImGuiCol_Button, {0.2f, 0.4f, 0.2f, 1.0f});
                if (ImGui::Button("Create Override from Defaults"))
                {
                    MaterialSlot newSlot;
                    newSlot.Name = "Override " + std::to_string(index);
                    newSlot.Target = MaterialSlotTarget::MaterialIndex;
                    newSlot.Index = index;
                    newSlot.Material = mat; // mat is already filled from defaults here
                    mc.Materials.push_back(newSlot);
                }
                ImGui::PopStyleColor();
            }

            ImGui::PopID();
        }
    };

    if (hitMeshIndex >= 0 && hitMeshIndex < (int)model.Meshes.size())
    {
        int matIndex = model.Meshes[hitMeshIndex].MaterialIndex;
        int slotIndex = -1;

        // 1. Check for Mesh Index override
        for (int i = 0; i < (int)mc.Materials.size(); i++)
        {
            if (mc.Materials[i].Target == MaterialSlotTarget::MeshIndex && mc.Materials[i].Index == hitMeshIndex)
            {
                slotIndex = i;
                break;
            }
        }

        // 2. Check for Material Index override
        if (slotIndex == -1)
        {
            for (int i = 0; i < (int)mc.Materials.size(); i++)
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
            DrawMaterialInstance(mc.Materials[slotIndex].Material, slotIndex, true);
        }
        else
        {
            // Synthesis default material info for display
            MaterialInstance defaultMat;
            const Material& rMat = model.Materials[matIndex];
            
            defaultMat.AlbedoColor = { (unsigned char)(rMat.AlbedoColor.r * 255), (unsigned char)(rMat.AlbedoColor.g * 255), (unsigned char)(rMat.AlbedoColor.b * 255), (unsigned char)(rMat.AlbedoColor.a * 255) };
            defaultMat.AlbedoPath = GetTexturePathFromID(rMat.AlbedoMap, modelTextures);
            defaultMat.OverrideAlbedo = !defaultMat.AlbedoPath.empty();

            defaultMat.NormalMapPath = GetTexturePathFromID(rMat.NormalMap, modelTextures);
            defaultMat.MetallicRoughnessPath = GetTexturePathFromID(rMat.MetallicRoughnessMap, modelTextures);
            defaultMat.OcclusionMapPath = GetTexturePathFromID(rMat.OcclusionMap, modelTextures);
            defaultMat.EmissivePath = GetTexturePathFromID(rMat.EmissiveMap, modelTextures);
            defaultMat.EmissiveColor = { (unsigned char)(rMat.EmissiveColor.r * 255), (unsigned char)(rMat.EmissiveColor.g * 255), (unsigned char)(rMat.EmissiveColor.b * 255), (unsigned char)(rMat.EmissiveColor.a * 255) };
            defaultMat.EmissiveIntensity = rMat.EmissiveIntensity;
            defaultMat.Metalness = rMat.Metalness;
            defaultMat.Roughness = rMat.Roughness;

            DrawMaterialInstance(defaultMat, matIndex, false);
            
            ImGui::Separator();
            if (ImGui::Button("Create Mesh Specific Override"))
            {
                MaterialSlot newSlot;
                newSlot.Name = "Mesh Override " + std::to_string(hitMeshIndex);
                newSlot.Target = MaterialSlotTarget::MeshIndex;
                newSlot.Index = hitMeshIndex;
                newSlot.Material = defaultMat;
                mc.Materials.push_back(newSlot);
            }
        }
    }
    else
    {
        // Show all materials of the model first
        for (int m = 0; m < (int)model.Materials.size(); m++)
        {
            int slotIdx = -1;
            for (int i = 0; i < (int)mc.Materials.size(); i++)
            {
                if (mc.Materials[i].Target == MaterialSlotTarget::MaterialIndex && mc.Materials[i].Index == m)
                {
                    slotIdx = i;
                    break;
                }
            }

            if (slotIdx != -1)
            {
                DrawMaterialInstance(mc.Materials[slotIdx].Material, slotIdx, true);
            }
            else
            {
                MaterialInstance defaultMat;
                const Material& rMat = model.Materials[m];
                
                defaultMat.AlbedoColor = { (unsigned char)(rMat.AlbedoColor.r * 255), (unsigned char)(rMat.AlbedoColor.g * 255), (unsigned char)(rMat.AlbedoColor.b * 255), (unsigned char)(rMat.AlbedoColor.a * 255) };
                defaultMat.AlbedoPath = GetTexturePathFromID(rMat.AlbedoMap, modelTextures);
                defaultMat.OverrideAlbedo = !defaultMat.AlbedoPath.empty();
                
                defaultMat.NormalMapPath = GetTexturePathFromID(rMat.NormalMap, modelTextures);
                defaultMat.MetallicRoughnessPath = GetTexturePathFromID(rMat.MetallicRoughnessMap, modelTextures);
                defaultMat.OcclusionMapPath = GetTexturePathFromID(rMat.OcclusionMap, modelTextures);
                defaultMat.EmissivePath = GetTexturePathFromID(rMat.EmissiveMap, modelTextures);
                defaultMat.EmissiveColor = { (unsigned char)(rMat.EmissiveColor.r * 255), (unsigned char)(rMat.EmissiveColor.g * 255), (unsigned char)(rMat.EmissiveColor.b * 255), (unsigned char)(rMat.EmissiveColor.a * 255) };
                defaultMat.EmissiveIntensity = rMat.EmissiveIntensity;
                defaultMat.Metalness = rMat.Metalness;
                defaultMat.Roughness = rMat.Roughness;

                DrawMaterialInstance(defaultMat, m, false);
            }
        }
    }

}

void PropertyEditor::DrawAddComponentPopup(CHEngine::Entity entity)
{
    if (ImGui::BeginPopup("AddComponent"))
    {
        bool isUIEntity = entity.HasComponent<ControlComponent>();
        auto* scene = entity.GetRegistry().ctx().find<Scene*>();
        bool is3DScene = scene && (*scene)->GetSettings().Mode == BackgroundMode::Environment3D;

        for (auto& [id, metadata] : s_ComponentRegistry)
        {
            if (!metadata.AllowAdd)
            {
                continue;
            }

            // Filtering: Only show widgets if the entity is a UI entity
            // (or if it's the ControlComponent itself which can be added to any transform)
            if (metadata.IsWidget && !isUIEntity)
            {
                continue;
            }

            // [FIX] Constraint: Do not allow adding UI components/widgets in 3D (Environment3D) scenes
            if (is3DScene)
            {
                if (metadata.IsWidget || id == entt::type_hash<ControlComponent>::value())
                {
                    continue;
                }
            }

            auto& registry = entity.GetRegistry();
            auto* storage = registry.storage(id);
            if (storage && storage->contains(entity))
            {
                continue;
            }

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
