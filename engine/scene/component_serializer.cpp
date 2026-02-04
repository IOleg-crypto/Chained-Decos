#include "component_serializer.h"
#include "scene.h"
#include "components.h"
#include "engine/core/yaml.h"
#include "script_registry.h"

namespace CHEngine
{
    std::vector<ComponentSerializerEntry> ComponentSerializer::s_Registry;

    // === Manual Serializers for complex types ===

    static void SerializeModelHelper(YAML::Emitter& out, Entity entity)
    {
        if (entity.HasComponent<ModelComponent>())
        {
            out << YAML::Key << "ModelComponent";
            auto &mc = entity.GetComponent<ModelComponent>();
            out << YAML::BeginMap;
            out << YAML::Key << "ModelPath" << YAML::Value << mc.ModelPath;
            
            if (!mc.Materials.empty())
            {
                out << YAML::Key << "Materials" << YAML::BeginSeq;
                for (const auto &slot : mc.Materials)
                {
                    out << YAML::BeginMap;
                    out << YAML::Key << "Name" << YAML::Value << slot.Name;
                    out << YAML::Key << "Index" << YAML::Value << slot.Index;
                    out << YAML::Key << "AlbedoColor" << YAML::Value << slot.Material.AlbedoColor;
                    out << YAML::Key << "Metalness" << YAML::Value << slot.Material.Metalness;
                    out << YAML::Key << "Roughness" << YAML::Value << slot.Material.Roughness;
                    out << YAML::EndMap;
                }
                out << YAML::EndSeq;
            }
            out << YAML::EndMap;
        }
    }

    static void DeserializeModelHelper(Entity entity, YAML::Node node)
    {
        auto mcNode = node["ModelComponent"];
        if (mcNode)
        {
            auto &comp = entity.AddComponent<ModelComponent>();
            if (mcNode["ModelPath"]) comp.ModelPath = mcNode["ModelPath"].template as<std::string>();

            auto materials = mcNode["Materials"];
            if (materials && materials.IsSequence())
            {
                for (auto matNode : materials)
                {
                    MaterialSlot slot;
                    if (matNode["Name"]) slot.Name = matNode["Name"].template as<std::string>();
                    if (matNode["Index"]) slot.Index = matNode["Index"].template as<int>();
                    if (matNode["AlbedoColor"]) slot.Material.AlbedoColor = matNode["AlbedoColor"].template as<Color>();
                    if (matNode["Metalness"]) slot.Material.Metalness = matNode["Metalness"].template as<float>();
                    if (matNode["Roughness"]) slot.Material.Roughness = matNode["Roughness"].template as<float>();
                    comp.Materials.push_back(slot);
                }
            }
        }
    }

    static void SerializeNativeScriptHelper(YAML::Emitter& out, Entity entity)
    {
        if (entity.HasComponent<NativeScriptComponent>())
        {
            out << YAML::Key << "NativeScriptComponent";
            auto &nsc = entity.GetComponent<NativeScriptComponent>();
            out << YAML::BeginMap;
            out << YAML::Key << "Scripts" << YAML::Value << YAML::BeginSeq;
            for (const auto &script : nsc.Scripts)
            {
                out << YAML::BeginMap;
                out << YAML::Key << "ScriptName" << YAML::Value << script.ScriptName;
                out << YAML::EndMap;
            }
            out << YAML::EndSeq;
            out << YAML::EndMap;
        }
    }

    static void DeserializeNativeScriptHelper(Entity entity, YAML::Node node)
    {
        auto nsc = node["NativeScriptComponent"];
        if (nsc)
        {
            auto &comp = entity.AddComponent<NativeScriptComponent>();
            auto scripts = nsc["Scripts"];
            if (scripts && scripts.IsSequence())
            {
                for (auto scriptNode : scripts)
                {
                    std::string name = scriptNode["ScriptName"].as<std::string>();
                    ScriptRegistry::AddScript(name, comp);
                }
            }
        }
    }

    void ComponentSerializer::SerializeHierarchy(YAML::Emitter& out, Entity entity)
    {
        if (entity.HasComponent<HierarchyComponent>())
        {
             auto& hc = entity.GetComponent<HierarchyComponent>();
             if (hc.Parent != entt::null)
             {
                 Entity parent = { hc.Parent, entity.GetScene() };
                 if (parent.HasComponent<IDComponent>())
                 {
                     out << YAML::Key << "Parent" << YAML::Value << (uint64_t)parent.GetComponent<IDComponent>().ID;
                 }
             }

             if (!hc.Children.empty())
             {
                 out << YAML::Key << "Children" << YAML::Value << YAML::BeginSeq;
                 for (auto childID : hc.Children)
                 {
                     Entity child = { childID, entity.GetScene() };
                     if (child.HasComponent<IDComponent>())
                         out << (uint64_t)child.GetComponent<IDComponent>().ID;
                 }
                 out << YAML::EndSeq;
             }
        }
    }

    void ComponentSerializer::DeserializeHierarchyTask(Entity entity, YAML::Node node, HierarchyTask& outTask)
    {
        auto hierarchyNode = node["Hierarchy"]; // Or maybe it's just in the entity map directly?
        // Actually, hierarchy metadata (Parent/Children) is usually stored in the root entity map in this engine.
        
        outTask.entity = entity;
        if (node["Parent"])
            outTask.parent = node["Parent"].template as<uint64_t>();
        
        if (node["Children"] && node["Children"].IsSequence())
        {
            for (auto childNode : node["Children"])
                outTask.children.push_back(childNode.template as<uint64_t>());
        }
    }

    void ComponentSerializer::Init()
    {
        s_Registry.clear();

        Register<TagComponent>("TagComponent",
            [](auto& out, auto& c) { 
                out << YAML::BeginMap; out << YAML::Key << "Tag" << YAML::Value << c.Tag; out << YAML::EndMap; 
            },
            [](auto& c, auto node) { if (node["Tag"]) c.Tag = node["Tag"].template as<std::string>(); }
        );

        Register<TransformComponent>("TransformComponent",
            [](auto& out, auto& c) {
                out << YAML::BeginMap;
                out << YAML::Key << "Translation" << YAML::Value << c.Translation;
                out << YAML::Key << "Rotation" << YAML::Value << c.Rotation;
                out << YAML::Key << "Scale" << YAML::Value << c.Scale;
                out << YAML::EndMap;
            },
            [](auto& c, auto node) {
                if (node["Translation"]) c.Translation = node["Translation"].template as<Vector3>();
                if (node["Rotation"]) c.Rotation = node["Rotation"].template as<Vector3>();
                if (node["Scale"]) c.Scale = node["Scale"].template as<Vector3>();
            }
        );

        Register<PointLightComponent>("PointLightComponent",
            [](auto& out, auto& c) {
                out << YAML::BeginMap;
                out << YAML::Key << "Color" << YAML::Value << c.LightColor;
                out << YAML::Key << "Intensity" << YAML::Value << c.Intensity;
                out << YAML::Key << "Radius" << YAML::Value << c.Radius;
                out << YAML::EndMap;
            },
            [](auto& c, auto node) {
                if (node["Color"]) c.LightColor = node["Color"].template as<Color>();
                if (node["Intensity"]) c.Intensity = node["Intensity"].template as<float>();
                if (node["Radius"]) c.Radius = node["Radius"].template as<float>();
            }
        );

        Register<AudioComponent>("AudioComponent",
            [](auto& out, auto& c) {
                out << YAML::BeginMap;
                out << YAML::Key << "SoundPath" << YAML::Value << c.SoundPath;
                out << YAML::Key << "Volume" << YAML::Value << c.Volume;
                out << YAML::Key << "Pitch" << YAML::Value << c.Pitch;
                out << YAML::Key << "Loop" << YAML::Value << c.Loop;
                out << YAML::Key << "PlayOnStart" << YAML::Value << c.PlayOnStart;
                out << YAML::EndMap;
            },
            [](auto& c, auto node) {
                if (node["SoundPath"]) c.SoundPath = node["SoundPath"].template as<std::string>();
                if (node["Volume"]) c.Volume = node["Volume"].template as<float>();
                if (node["Pitch"]) c.Pitch = node["Pitch"].template as<float>();
                if (node["Loop"]) c.Loop = node["Loop"].template as<bool>();
                if (node["PlayOnStart"]) c.PlayOnStart = node["PlayOnStart"].template as<bool>();
            }
        );

        Register<RigidBodyComponent>("RigidBodyComponent",
            [](auto& out, auto& c) {
                out << YAML::BeginMap;
                out << YAML::Key << "Mass" << YAML::Value << c.Mass;
                out << YAML::Key << "UseGravity" << YAML::Value << c.UseGravity;
                out << YAML::EndMap;
            },
            [](auto& c, auto node) {
                if (node["Mass"]) c.Mass = node["Mass"].template as<float>();
                if (node["UseGravity"]) c.UseGravity = node["UseGravity"].template as<bool>();
            }
        );

        Register<CameraComponent>("CameraComponent",
            [](auto& out, auto& c) {
                out << YAML::BeginMap;
                out << YAML::Key << "Fov" << YAML::Value << c.Fov;
                out << YAML::Key << "IsPrimary" << YAML::Value << c.IsPrimary;
                out << YAML::Key << "NearPlane" << YAML::Value << c.NearPlane;
                out << YAML::Key << "FarPlane" << YAML::Value << c.FarPlane;
                out << YAML::EndMap;
            },
            [](auto& c, auto node) {
                if (node["Fov"]) c.Fov = node["Fov"].template as<float>();
                if (node["IsPrimary"]) c.IsPrimary = node["IsPrimary"].template as<bool>();
                if (node["NearPlane"]) c.NearPlane = node["NearPlane"].template as<float>();
                if (node["FarPlane"]) c.FarPlane = node["FarPlane"].template as<float>();
            }
        );

        Register<BillboardComponent>("BillboardComponent",
            [](auto& out, auto& c) {
                out << YAML::BeginMap;
                out << YAML::Key << "TexturePath" << YAML::Value << c.TexturePath;
                out << YAML::Key << "Size" << YAML::Value << c.Size;
                out << YAML::EndMap;
            },
            [](auto& c, auto node) {
                if (node["TexturePath"]) c.TexturePath = node["TexturePath"].template as<std::string>();
                if (node["Size"]) c.Size = node["Size"].template as<float>();
            }
        );

        Register<SceneTransitionComponent>("SceneTransitionComponent",
            [](auto& out, auto& c) {
                out << YAML::BeginMap;
                out << YAML::Key << "TargetScenePath" << YAML::Value << c.TargetScenePath;
                out << YAML::EndMap;
            },
            [](auto& c, auto node) {
                if (node["TargetScenePath"]) c.TargetScenePath = node["TargetScenePath"].template as<std::string>();
            }
        );

        // UI Components
        Register<ControlComponent>("ControlComponent",
            [](auto& out, auto& c) {
                out << YAML::BeginMap;
                out << YAML::Key << "ZOrder" << YAML::Value << c.ZOrder;
                out << YAML::Key << "IsActive" << YAML::Value << c.IsActive;
                
                out << YAML::Key << "RectTransform" << YAML::BeginMap;
                out << YAML::Key << "AnchorMin" << YAML::Value << c.Transform.AnchorMin;
                out << YAML::Key << "AnchorMax" << YAML::Value << c.Transform.AnchorMax;
                out << YAML::Key << "OffsetMin" << YAML::Value << c.Transform.OffsetMin;
                out << YAML::Key << "OffsetMax" << YAML::Value << c.Transform.OffsetMax;
                out << YAML::Key << "Pivot" << YAML::Value << c.Transform.Pivot;
                out << YAML::Key << "Rotation" << YAML::Value << c.Transform.Rotation;
                out << YAML::Key << "Scale" << YAML::Value << c.Transform.Scale;
                out << YAML::EndMap;

                out << YAML::EndMap;
            },
            [](auto& c, auto node) {
                if (node["ZOrder"]) c.ZOrder = node["ZOrder"].template as<int>();
                if (node["IsActive"]) c.IsActive = node["IsActive"].template as<bool>();
                
                auto rt = node["RectTransform"];
                if (rt) {
                    if (rt["AnchorMin"]) c.Transform.AnchorMin = rt["AnchorMin"].template as<glm::vec2>();
                    if (rt["AnchorMax"]) c.Transform.AnchorMax = rt["AnchorMax"].template as<glm::vec2>();
                    if (rt["OffsetMin"]) c.Transform.OffsetMin = rt["OffsetMin"].template as<glm::vec2>();
                    if (rt["OffsetMax"]) c.Transform.OffsetMax = rt["OffsetMax"].template as<glm::vec2>();
                    if (rt["Pivot"]) c.Transform.Pivot = rt["Pivot"].template as<glm::vec2>();
                    if (rt["Rotation"]) c.Transform.Rotation = rt["Rotation"].template as<float>();
                    if (rt["Scale"]) c.Transform.Scale = rt["Scale"].template as<glm::vec2>();
                }
            }
        );

        auto serializeTextStyle = [](YAML::Emitter& out, const TextStyle& style) {
            out << YAML::BeginMap;
            out << YAML::Key << "FontName" << YAML::Value << style.FontName;
            out << YAML::Key << "FontSize" << YAML::Value << style.FontSize;
            out << YAML::Key << "TextColor" << YAML::Value << style.TextColor;
            out << YAML::Key << "Shadow" << YAML::Value << style.Shadow;
            out << YAML::Key << "ShadowOffset" << YAML::Value << style.ShadowOffset;
            out << YAML::Key << "ShadowColor" << YAML::Value << style.ShadowColor;
            out << YAML::Key << "LetterSpacing" << YAML::Value << style.LetterSpacing;
            out << YAML::Key << "LineHeight" << YAML::Value << style.LineHeight;
            out << YAML::Key << "HorizontalAlignment" << YAML::Value << (int)style.HorizontalAlignment;
            out << YAML::Key << "VerticalAlignment" << YAML::Value << (int)style.VerticalAlignment;
            out << YAML::EndMap;
        };

        auto deserializeTextStyle = [](TextStyle& style, YAML::Node node) {
            if (node["FontName"]) style.FontName = node["FontName"].template as<std::string>();
            if (node["FontSize"]) style.FontSize = node["FontSize"].template as<float>();
            if (node["TextColor"]) style.TextColor = node["TextColor"].template as<Color>();
            if (node["Shadow"]) style.Shadow = node["Shadow"].template as<bool>();
            if (node["ShadowOffset"]) style.ShadowOffset = node["ShadowOffset"].template as<float>();
            if (node["ShadowColor"]) style.ShadowColor = node["ShadowColor"].template as<Color>();
            if (node["LetterSpacing"]) style.LetterSpacing = node["LetterSpacing"].template as<float>();
            if (node["LineHeight"]) style.LineHeight = node["LineHeight"].template as<float>();
            
            if (node["HorizontalAlignment"]) style.HorizontalAlignment = (TextAlignment)node["HorizontalAlignment"].template as<int>();
            else if (node["HAlign"]) style.HorizontalAlignment = (TextAlignment)node["HAlign"].template as<int>(); // Legacy

            if (node["VerticalAlignment"]) style.VerticalAlignment = (TextAlignment)node["VerticalAlignment"].template as<int>();
            else if (node["VAlign"]) style.VerticalAlignment = (TextAlignment)node["VAlign"].template as<int>(); // Legacy
        };

        auto serializeUIStyle = [](YAML::Emitter& out, const UIStyle& style) {
            out << YAML::BeginMap;
            out << YAML::Key << "BackgroundColor" << YAML::Value << style.BackgroundColor;
            out << YAML::Key << "HoverColor" << YAML::Value << style.HoverColor;
            out << YAML::Key << "PressedColor" << YAML::Value << style.PressedColor;
            out << YAML::Key << "Rounding" << YAML::Value << style.Rounding;
            out << YAML::Key << "BorderSize" << YAML::Value << style.BorderSize;
            out << YAML::Key << "BorderColor" << YAML::Value << style.BorderColor;
            out << YAML::Key << "Padding" << YAML::Value << style.Padding;
            out << YAML::EndMap;
        };

        auto deserializeUIStyle = [](UIStyle& style, YAML::Node node) {
            if (node["BackgroundColor"]) style.BackgroundColor = node["BackgroundColor"].template as<Color>();
            else if (node["BGColor"]) style.BackgroundColor = node["BGColor"].template as<Color>(); // Legacy

            if (node["HoverColor"]) style.HoverColor = node["HoverColor"].template as<Color>();
            if (node["PressedColor"]) style.PressedColor = node["PressedColor"].template as<Color>();
            if (node["Rounding"]) style.Rounding = node["Rounding"].template as<float>();
            if (node["BorderSize"]) style.BorderSize = node["BorderSize"].template as<float>();
            if (node["BorderColor"]) style.BorderColor = node["BorderColor"].template as<Color>();
            if (node["Padding"]) style.Padding = node["Padding"].template as<float>();
        };

        Register<ButtonControl>("ButtonControl",
            [=](auto& out, auto& c) {
                out << YAML::BeginMap;
                out << YAML::Key << "Label" << YAML::Value << c.Label;
                out << YAML::Key << "Interactable" << YAML::Value << c.IsInteractable;
                out << YAML::Key << "Style"; serializeUIStyle(out, c.Style);
                out << YAML::Key << "Text"; serializeTextStyle(out, c.Text);
                out << YAML::EndMap;
            },
            [=](auto& c, auto node) {
                if (node["Label"]) c.Label = node["Label"].template as<std::string>();
                if (node["Interactable"]) c.IsInteractable = node["Interactable"].template as<bool>();
                
                if (node["Style"]) deserializeUIStyle(c.Style, node["Style"]);
                else if (node["UIStyle"]) deserializeUIStyle(c.Style, node["UIStyle"]); // Legacy support

                if (node["Text"]) deserializeTextStyle(c.Text, node["Text"]);
                else if (node["TextStyle"]) deserializeTextStyle(c.Text, node["TextStyle"]); // Legacy support
            }
        );

        Register<PanelControl>("PanelControl",
            [=](auto& out, auto& c) {
                out << YAML::BeginMap;
                out << YAML::Key << "TexturePath" << YAML::Value << c.TexturePath;
                out << YAML::Key << "UIStyle"; serializeUIStyle(out, c.Style);
                out << YAML::EndMap;
            },
            [=](auto& c, auto node) {
                if (node["TexturePath"]) c.TexturePath = node["TexturePath"].template as<std::string>();
                if (node["UIStyle"]) deserializeUIStyle(c.Style, node["UIStyle"]);
            }
        );

        Register<LabelControl>("LabelControl",
            [=](auto& out, auto& c) {
                out << YAML::BeginMap;
                out << YAML::Key << "Text" << YAML::Value << c.Text;
                out << YAML::Key << "Style"; serializeTextStyle(out, c.Style);
                out << YAML::EndMap;
            },
            [=](auto& c, auto node) {
                if (node["Text"]) c.Text = node["Text"].template as<std::string>();
                
                if (node["Style"]) deserializeTextStyle(c.Style, node["Style"]);
                else if (node["TextStyle"]) deserializeTextStyle(c.Style, node["TextStyle"]); // Legacy support
            }
        );

        Register<SliderControl>("SliderControl",
            [=](auto& out, auto& c) {
                out << YAML::BeginMap;
                out << YAML::Key << "Label" << YAML::Value << c.Label;
                out << YAML::Key << "Value" << YAML::Value << c.Value;
                out << YAML::Key << "Min" << YAML::Value << c.Min;
                out << YAML::Key << "Max" << YAML::Value << c.Max;
                out << YAML::Key << "Text"; serializeTextStyle(out, c.Text);
                out << YAML::Key << "Style"; serializeUIStyle(out, c.Style);
                out << YAML::EndMap;
            },
            [=](auto& c, auto node) {
                if (node["Label"]) c.Label = node["Label"].template as<std::string>();
                if (node["Value"]) c.Value = node["Value"].template as<float>();
                if (node["Min"]) c.Min = node["Min"].template as<float>();
                if (node["Max"]) c.Max = node["Max"].template as<float>();
                
                if (node["Text"]) deserializeTextStyle(c.Text, node["Text"]);
                else if (node["TextStyle"]) deserializeTextStyle(c.Text, node["TextStyle"]);

                if (node["Style"]) deserializeUIStyle(c.Style, node["Style"]);
                else if (node["UIStyle"]) deserializeUIStyle(c.Style, node["UIStyle"]);
            }
        );

        Register<CheckboxControl>("CheckboxControl",
            [=](auto& out, auto& c) {
                out << YAML::BeginMap;
                out << YAML::Key << "Label" << YAML::Value << c.Label;
                out << YAML::Key << "Checked" << YAML::Value << c.Checked;
                out << YAML::Key << "Text"; serializeTextStyle(out, c.Text);
                out << YAML::Key << "Style"; serializeUIStyle(out, c.Style);
                out << YAML::EndMap;
            },
            [=](auto& c, auto node) {
                if (node["Label"]) c.Label = node["Label"].template as<std::string>();
                if (node["Checked"]) c.Checked = node["Checked"].template as<bool>();
                
                if (node["Text"]) deserializeTextStyle(c.Text, node["Text"]);
                else if (node["TextStyle"]) deserializeTextStyle(c.Text, node["TextStyle"]);

                if (node["Style"]) deserializeUIStyle(c.Style, node["Style"]);
                else if (node["UIStyle"]) deserializeUIStyle(c.Style, node["UIStyle"]);
            }
        );

        Register<InputTextControl>("InputTextControl",
            [=](auto& out, auto& c) {
                out << YAML::BeginMap;
                out << YAML::Key << "Label" << YAML::Value << c.Label;
                out << YAML::Key << "Text" << YAML::Value << c.Text;
                out << YAML::Key << "Placeholder" << YAML::Value << c.Placeholder;
                out << YAML::Key << "MaxLength" << YAML::Value << c.MaxLength;
                out << YAML::Key << "Multiline" << YAML::Value << c.Multiline;
                out << YAML::Key << "ReadOnly" << YAML::Value << c.ReadOnly;
                out << YAML::Key << "Password" << YAML::Value << c.Password;
                out << YAML::Key << "Text"; serializeTextStyle(out, c.Style);
                out << YAML::Key << "Style"; serializeUIStyle(out, c.BoxStyle);
                out << YAML::EndMap;
            },
            [=](auto& c, auto node) {
                if (node["Label"]) c.Label = node["Label"].template as<std::string>();
                if (node["Text"]) c.Text = node["Text"].template as<std::string>();
                if (node["Placeholder"]) c.Placeholder = node["Placeholder"].template as<std::string>();
                if (node["MaxLength"]) c.MaxLength = node["MaxLength"].template as<int>();
                if (node["Multiline"]) c.Multiline = node["Multiline"].template as<bool>();
                if (node["ReadOnly"]) c.ReadOnly = node["ReadOnly"].template as<bool>();
                if (node["Password"]) c.Password = node["Password"].template as<bool>();
                
                if (node["Text"]) deserializeTextStyle(c.Style, node["Text"]);
                else if (node["TextStyle"]) deserializeTextStyle(c.Style, node["TextStyle"]);

                if (node["Style"]) deserializeUIStyle(c.BoxStyle, node["Style"]);
                else if (node["BoxStyle"]) deserializeUIStyle(c.BoxStyle, node["BoxStyle"]);
            }
        );

        Register<ComboBoxControl>("ComboBoxControl",
            [=](auto& out, auto& c) {
                out << YAML::BeginMap;
                out << YAML::Key << "Label" << YAML::Value << c.Label;
                out << YAML::Key << "SelectedIndex" << YAML::Value << c.SelectedIndex;
                out << YAML::Key << "Items" << YAML::Value << YAML::BeginSeq;
                for (const auto& item : c.Items)
                    out << item;
                out << YAML::EndSeq;
                out << YAML::Key << "Text"; serializeTextStyle(out, c.Style);
                out << YAML::Key << "Style"; serializeUIStyle(out, c.BoxStyle);
                out << YAML::EndMap;
            },
            [=](auto& c, auto node) {
                if (node["Label"]) c.Label = node["Label"].template as<std::string>();
                if (node["SelectedIndex"]) c.SelectedIndex = node["SelectedIndex"].template as<int>();
                if (node["Items"] && node["Items"].IsSequence()) {
                    c.Items.clear();
                    for (auto item : node["Items"])
                        c.Items.push_back(item.template as<std::string>());
                }
                
                if (node["Text"]) deserializeTextStyle(c.Style, node["Text"]);
                else if (node["TextStyle"]) deserializeTextStyle(c.Style, node["TextStyle"]);

                if (node["Style"]) deserializeUIStyle(c.BoxStyle, node["Style"]);
                else if (node["BoxStyle"]) deserializeUIStyle(c.BoxStyle, node["BoxStyle"]);
            }
        );

        Register<ProgressBarControl>("ProgressBarControl",
            [=](auto& out, auto& c) {
                out << YAML::BeginMap;
                out << YAML::Key << "Progress" << YAML::Value << c.Progress;
                out << YAML::Key << "OverlayText" << YAML::Value << c.OverlayText;
                out << YAML::Key << "ShowPercentage" << YAML::Value << c.ShowPercentage;
                out << YAML::Key << "Text"; serializeTextStyle(out, c.Style);
                out << YAML::Key << "Style"; serializeUIStyle(out, c.BarStyle);
                out << YAML::EndMap;
            },
            [=](auto& c, auto node) {
                if (node["Progress"]) c.Progress = node["Progress"].template as<float>();
                if (node["OverlayText"]) c.OverlayText = node["OverlayText"].template as<std::string>();
                if (node["ShowPercentage"]) c.ShowPercentage = node["ShowPercentage"].template as<bool>();
                
                if (node["Text"]) deserializeTextStyle(c.Style, node["Text"]);
                else if (node["TextStyle"]) deserializeTextStyle(c.Style, node["TextStyle"]);

                if (node["Style"]) deserializeUIStyle(c.BarStyle, node["Style"]);
                else if (node["BarStyle"]) deserializeUIStyle(c.BarStyle, node["BarStyle"]);
            }
        );

        Register<ImageControl>("ImageControl",
            [=](auto& out, auto& c) {
                out << YAML::BeginMap;
                out << YAML::Key << "TexturePath" << YAML::Value << c.TexturePath;
                out << YAML::Key << "Size" << YAML::Value << c.Size;
                out << YAML::Key << "TintColor" << YAML::Value << c.TintColor;
                out << YAML::Key << "BorderColor" << YAML::Value << c.BorderColor;
                out << YAML::Key << "Style"; serializeUIStyle(out, c.Style);
                out << YAML::EndMap;
            },
            [=](auto& c, auto node) {
                if (node["TexturePath"]) c.TexturePath = node["TexturePath"].template as<std::string>();
                if (node["Size"]) c.Size = node["Size"].template as<Vector2>();
                if (node["TintColor"]) c.TintColor = node["TintColor"].template as<Color>();
                if (node["BorderColor"]) c.BorderColor = node["BorderColor"].template as<Color>();
                
                if (node["Style"]) deserializeUIStyle(c.Style, node["Style"]);
                else if (node["UIStyle"]) deserializeUIStyle(c.Style, node["UIStyle"]); // Legacy
            }
        );

        Register<ImageButtonControl>("ImageButtonControl",
            [=](auto& out, auto& c) {
                out << YAML::BeginMap;
                out << YAML::Key << "Label" << YAML::Value << c.Label;
                out << YAML::Key << "TexturePath" << YAML::Value << c.TexturePath;
                out << YAML::Key << "Size" << YAML::Value << c.Size;
                out << YAML::Key << "TintColor" << YAML::Value << c.TintColor;
                out << YAML::Key << "BackgroundColor" << YAML::Value << c.BackgroundColor;
                out << YAML::Key << "FramePadding" << YAML::Value << c.FramePadding;
                out << YAML::Key << "Style"; serializeUIStyle(out, c.Style);
                out << YAML::EndMap;
            },
            [=](auto& c, auto node) {
                if (node["Label"]) c.Label = node["Label"].template as<std::string>();
                if (node["TexturePath"]) c.TexturePath = node["TexturePath"].template as<std::string>();
                if (node["Size"]) c.Size = node["Size"].template as<Vector2>();
                if (node["TintColor"]) c.TintColor = node["TintColor"].template as<Color>();
                if (node["BackgroundColor"]) c.BackgroundColor = node["BackgroundColor"].template as<Color>();
                if (node["FramePadding"]) c.FramePadding = node["FramePadding"].template as<int>();
                
                if (node["Style"]) deserializeUIStyle(c.Style, node["Style"]);
                else if (node["UIStyle"]) deserializeUIStyle(c.Style, node["UIStyle"]);
            }
        );

        Register<SeparatorControl>("SeparatorControl",
            [=](auto& out, auto& c) {
                out << YAML::BeginMap;
                out << YAML::Key << "Thickness" << YAML::Value << c.Thickness;
                out << YAML::Key << "LineColor" << YAML::Value << c.LineColor;
                out << YAML::EndMap;
            },
            [=](auto& c, auto node) {
                if (node["Thickness"]) c.Thickness = node["Thickness"].template as<float>();
                if (node["LineColor"]) c.LineColor = node["LineColor"].template as<Color>();
            }
        );

        Register<RadioButtonControl>("RadioButtonControl",
            [=](auto& out, auto& c) {
                out << YAML::BeginMap;
                out << YAML::Key << "Label" << YAML::Value << c.Label;
                out << YAML::Key << "SelectedIndex" << YAML::Value << c.SelectedIndex;
                out << YAML::Key << "Horizontal" << YAML::Value << c.Horizontal;
                out << YAML::Key << "Options" << YAML::Value << YAML::BeginSeq;
                for (const auto& opt : c.Options) out << opt;
                out << YAML::EndSeq;
                out << YAML::Key << "Text"; serializeTextStyle(out, c.Style);
                out << YAML::EndMap;
            },
            [=](auto& c, auto node) {
                if (node["Label"]) c.Label = node["Label"].template as<std::string>();
                if (node["SelectedIndex"]) c.SelectedIndex = node["SelectedIndex"].template as<int>();
                if (node["Horizontal"]) c.Horizontal = node["Horizontal"].template as<bool>();
                if (node["Options"] && node["Options"].IsSequence()) {
                    c.Options.clear();
                    for (auto opt : node["Options"]) c.Options.push_back(opt.template as<std::string>());
                }
                
                if (node["Text"]) deserializeTextStyle(c.Style, node["Text"]);
                else if (node["TextStyle"]) deserializeTextStyle(c.Style, node["TextStyle"]);
            }
        );

        Register<ColorPickerControl>("ColorPickerControl",
            [=](auto& out, auto& c) {
                out << YAML::BeginMap;
                out << YAML::Key << "Label" << YAML::Value << c.Label;
                out << YAML::Key << "SelectedColor" << YAML::Value << c.SelectedColor;
                out << YAML::Key << "ShowAlpha" << YAML::Value << c.ShowAlpha;
                out << YAML::Key << "ShowPicker" << YAML::Value << c.ShowPicker;
                out << YAML::Key << "Style"; serializeUIStyle(out, c.Style);
                out << YAML::EndMap;
            },
            [=](auto& c, auto node) {
                if (node["Label"]) c.Label = node["Label"].template as<std::string>();
                if (node["SelectedColor"]) c.SelectedColor = node["SelectedColor"].template as<Color>();
                if (node["ShowAlpha"]) c.ShowAlpha = node["ShowAlpha"].template as<bool>();
                if (node["ShowPicker"]) c.ShowPicker = node["ShowPicker"].template as<bool>();
                
                if (node["Style"]) deserializeUIStyle(c.Style, node["Style"]);
                else if (node["UIStyle"]) deserializeUIStyle(c.Style, node["UIStyle"]);
            }
        );

        Register<DragFloatControl>("DragFloatControl",
            [=](auto& out, auto& c) {
                out << YAML::BeginMap;
                out << YAML::Key << "Label" << YAML::Value << c.Label;
                out << YAML::Key << "Value" << YAML::Value << c.Value;
                out << YAML::Key << "Speed" << YAML::Value << c.Speed;
                out << YAML::Key << "Min" << YAML::Value << c.Min;
                out << YAML::Key << "Max" << YAML::Value << c.Max;
                out << YAML::Key << "Format" << YAML::Value << c.Format;
                out << YAML::Key << "Text"; serializeTextStyle(out, c.Style);
                out << YAML::Key << "Style"; serializeUIStyle(out, c.BoxStyle);
                out << YAML::EndMap;
            },
            [=](auto& c, auto node) {
                if (node["Label"]) c.Label = node["Label"].template as<std::string>();
                if (node["Value"]) c.Value = node["Value"].template as<float>();
                if (node["Speed"]) c.Speed = node["Speed"].template as<float>();
                if (node["Min"]) c.Min = node["Min"].template as<float>();
                if (node["Max"]) c.Max = node["Max"].template as<float>();
                if (node["Format"]) c.Format = node["Format"].template as<std::string>();
                
                if (node["Text"]) deserializeTextStyle(c.Style, node["Text"]);
                else if (node["TextStyle"]) deserializeTextStyle(c.Style, node["TextStyle"]);

                if (node["Style"]) deserializeUIStyle(c.BoxStyle, node["Style"]);
                else if (node["BoxStyle"]) deserializeUIStyle(c.BoxStyle, node["BoxStyle"]);
            }
        );

        Register<DragIntControl>("DragIntControl",
            [=](auto& out, auto& c) {
                out << YAML::BeginMap;
                out << YAML::Key << "Label" << YAML::Value << c.Label;
                out << YAML::Key << "Value" << YAML::Value << c.Value;
                out << YAML::Key << "Speed" << YAML::Value << c.Speed;
                out << YAML::Key << "Min" << YAML::Value << c.Min;
                out << YAML::Key << "Max" << YAML::Value << c.Max;
                out << YAML::Key << "Format" << YAML::Value << c.Format;
                out << YAML::Key << "Text"; serializeTextStyle(out, c.Style);
                out << YAML::Key << "Style"; serializeUIStyle(out, c.BoxStyle);
                out << YAML::EndMap;
            },
            [=](auto& c, auto node) {
                if (node["Label"]) c.Label = node["Label"].template as<std::string>();
                if (node["Value"]) c.Value = node["Value"].template as<int>();
                if (node["Speed"]) c.Speed = node["Speed"].template as<float>();
                if (node["Min"]) c.Min = node["Min"].template as<int>();
                if (node["Max"]) c.Max = node["Max"].template as<int>();
                if (node["Format"]) c.Format = node["Format"].template as<std::string>();
                
                if (node["Text"]) deserializeTextStyle(c.Style, node["Text"]);
                else if (node["TextStyle"]) deserializeTextStyle(c.Style, node["TextStyle"]);

                if (node["Style"]) deserializeUIStyle(c.BoxStyle, node["Style"]);
                else if (node["BoxStyle"]) deserializeUIStyle(c.BoxStyle, node["BoxStyle"]);
            }
        );

        Register<TreeNodeControl>("TreeNodeControl",
            [=](auto& out, auto& c) {
                out << YAML::BeginMap;
                out << YAML::Key << "Label" << YAML::Value << c.Label;
                out << YAML::Key << "DefaultOpen" << YAML::Value << c.DefaultOpen;
                out << YAML::Key << "IsLeaf" << YAML::Value << c.IsLeaf;
                out << YAML::Key << "Text"; serializeTextStyle(out, c.Style);
                out << YAML::EndMap;
            },
            [=](auto& c, auto node) {
                if (node["Label"]) c.Label = node["Label"].template as<std::string>();
                if (node["DefaultOpen"]) c.DefaultOpen = node["DefaultOpen"].template as<bool>();
                if (node["IsLeaf"]) c.IsLeaf = node["IsLeaf"].template as<bool>();
                
                if (node["Text"]) deserializeTextStyle(c.Style, node["Text"]);
                else if (node["TextStyle"]) deserializeTextStyle(c.Style, node["TextStyle"]);
            }
        );

        Register<TabBarControl>("TabBarControl",
            [=](auto& out, auto& c) {
                out << YAML::BeginMap;
                out << YAML::Key << "Label" << YAML::Value << c.Label;
                out << YAML::Key << "Reorderable" << YAML::Value << c.Reorderable;
                out << YAML::Key << "AutoSelectNewTabs" << YAML::Value << c.AutoSelectNewTabs;
                out << YAML::Key << "Style"; serializeUIStyle(out, c.Style);
                out << YAML::EndMap;
            },
            [=](auto& c, auto node) {
                if (node["Label"]) c.Label = node["Label"].template as<std::string>();
                if (node["Reorderable"]) c.Reorderable = node["Reorderable"].template as<bool>();
                if (node["AutoSelectNewTabs"]) c.AutoSelectNewTabs = node["AutoSelectNewTabs"].template as<bool>();
                
                if (node["Style"]) deserializeUIStyle(c.Style, node["Style"]);
                else if (node["UIStyle"]) deserializeUIStyle(c.Style, node["UIStyle"]);
            }
        );

        Register<TabItemControl>("TabItemControl",
            [=](auto& out, auto& c) {
                out << YAML::BeginMap;
                out << YAML::Key << "Label" << YAML::Value << c.Label;
                out << YAML::Key << "IsOpen" << YAML::Value << c.IsOpen;
                out << YAML::Key << "Text"; serializeTextStyle(out, c.Style);
                out << YAML::EndMap;
            },
            [=](auto& c, auto node) {
                if (node["Label"]) c.Label = node["Label"].template as<std::string>();
                if (node["IsOpen"]) c.IsOpen = node["IsOpen"].template as<bool>();
                
                if (node["Text"]) deserializeTextStyle(c.Style, node["Text"]);
                else if (node["TextStyle"]) deserializeTextStyle(c.Style, node["TextStyle"]);
            }
        );

        Register<CollapsingHeaderControl>("CollapsingHeaderControl",
            [=](auto& out, auto& c) {
                out << YAML::BeginMap;
                out << YAML::Key << "Label" << YAML::Value << c.Label;
                out << YAML::Key << "DefaultOpen" << YAML::Value << c.DefaultOpen;
                out << YAML::Key << "Text"; serializeTextStyle(out, c.Style);
                out << YAML::EndMap;
            },
            [=](auto& c, auto node) {
                if (node["Label"]) c.Label = node["Label"].template as<std::string>();
                if (node["DefaultOpen"]) c.DefaultOpen = node["DefaultOpen"].template as<bool>();
                
                if (node["Text"]) deserializeTextStyle(c.Style, node["Text"]);
                else if (node["TextStyle"]) deserializeTextStyle(c.Style, node["TextStyle"]);
            }
        );

        Register<PlotLinesControl>("PlotLinesControl",
            [=](auto& out, auto& c) {
                out << YAML::BeginMap;
                out << YAML::Key << "Label" << YAML::Value << c.Label;
                out << YAML::Key << "OverlayText" << YAML::Value << c.OverlayText;
                out << YAML::Key << "ScaleMin" << YAML::Value << c.ScaleMin;
                out << YAML::Key << "ScaleMax" << YAML::Value << c.ScaleMax;
                out << YAML::Key << "GraphSize" << YAML::Value << c.GraphSize;
                out << YAML::Key << "Values" << YAML::Value << YAML::BeginSeq;
                for (auto v : c.Values) out << v;
                out << YAML::EndSeq;
                out << YAML::Key << "Text"; serializeTextStyle(out, c.Style);
                out << YAML::Key << "Style"; serializeUIStyle(out, c.BoxStyle);
                out << YAML::EndMap;
            },
            [=](auto& c, auto node) {
                if (node["Label"]) c.Label = node["Label"].template as<std::string>();
                if (node["OverlayText"]) c.OverlayText = node["OverlayText"].template as<std::string>();
                if (node["ScaleMin"]) c.ScaleMin = node["ScaleMin"].template as<float>();
                if (node["ScaleMax"]) c.ScaleMax = node["ScaleMax"].template as<float>();
                if (node["GraphSize"]) c.GraphSize = node["GraphSize"].template as<Vector2>();
                if (node["Values"] && node["Values"].IsSequence()) {
                    c.Values.clear();
                    for (auto v : node["Values"]) c.Values.push_back(v.template as<float>());
                }
                
                if (node["Text"]) deserializeTextStyle(c.Style, node["Text"]);
                else if (node["TextStyle"]) deserializeTextStyle(c.Style, node["TextStyle"]);

                if (node["Style"]) deserializeUIStyle(c.BoxStyle, node["Style"]);
                else if (node["BoxStyle"]) deserializeUIStyle(c.BoxStyle, node["BoxStyle"]);
            }
        );

        Register<PlotHistogramControl>("PlotHistogramControl",
            [=](auto& out, auto& c) {
                out << YAML::BeginMap;
                out << YAML::Key << "Label" << YAML::Value << c.Label;
                out << YAML::Key << "OverlayText" << YAML::Value << c.OverlayText;
                out << YAML::Key << "ScaleMin" << YAML::Value << c.ScaleMin;
                out << YAML::Key << "ScaleMax" << YAML::Value << c.ScaleMax;
                out << YAML::Key << "GraphSize" << YAML::Value << c.GraphSize;
                out << YAML::Key << "Values" << YAML::Value << YAML::BeginSeq;
                for (auto v : c.Values) out << v;
                out << YAML::EndSeq;
                out << YAML::Key << "Text"; serializeTextStyle(out, c.Style);
                out << YAML::Key << "Style"; serializeUIStyle(out, c.BoxStyle);
                out << YAML::EndMap;
            },
            [=](auto& c, auto node) {
                if (node["Label"]) c.Label = node["Label"].template as<std::string>();
                if (node["OverlayText"]) c.OverlayText = node["OverlayText"].template as<std::string>();
                if (node["ScaleMin"]) c.ScaleMin = node["ScaleMin"].template as<float>();
                if (node["ScaleMax"]) c.ScaleMax = node["ScaleMax"].template as<float>();
                if (node["GraphSize"]) c.GraphSize = node["GraphSize"].template as<Vector2>();
                if (node["Values"] && node["Values"].IsSequence()) {
                    c.Values.clear();
                    for (auto v : node["Values"]) c.Values.push_back(v.template as<float>());
                }
                
                if (node["Text"]) deserializeTextStyle(c.Style, node["Text"]);
                else if (node["TextStyle"]) deserializeTextStyle(c.Style, node["TextStyle"]);

                if (node["Style"]) deserializeUIStyle(c.BoxStyle, node["Style"]);
                else if (node["BoxStyle"]) deserializeUIStyle(c.BoxStyle, node["BoxStyle"]);
            }
        );
    }

    void ComponentSerializer::SerializeAll(YAML::Emitter& out, Entity entity)
    {
        for (auto& entry : s_Registry)
        {
            entry.Serialize(out, entity);
        }
        
        SerializeModelHelper(out, entity);
        SerializeNativeScriptHelper(out, entity);
        SerializeHierarchy(out, entity);
    }

    void ComponentSerializer::DeserializeAll(Entity entity, YAML::Node node)
    {
        for (auto& entry : s_Registry)
        {
            entry.Deserialize(entity, node);
        }
        
        DeserializeModelHelper(entity, node);
        DeserializeNativeScriptHelper(entity, node);
    }

    void ComponentSerializer::SerializeID(YAML::Emitter& out, Entity entity)
    {
        if (entity.HasComponent<IDComponent>())
            out << YAML::Key << "Entity" << YAML::Value << (uint64_t)entity.GetComponent<IDComponent>().ID;
        else
            out << YAML::Key << "Entity" << YAML::Value << 0;
    }

} // namespace CHEngine
