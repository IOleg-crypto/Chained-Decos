#include "engine/scene/component_serializer.h"
#include "engine/scene/scene.h"
#include "engine/scene/components.h"
#include "engine/scene/serialization_utils.h"
#include "engine/scene/script_registry.h"

namespace CHEngine
{
    using namespace SerializationUtils;

    void ComponentSerializer::RegisterAudioComponents()
    {
        // Audio Component
        Register<AudioComponent>("AudioComponent",
            [](auto& emitter, auto& component) {
                SerializePath(emitter, "SoundPath", component.SoundPath);
                SerializeProperty(emitter, "Volume", component.Volume);
                SerializeProperty(emitter, "Pitch", component.Pitch);
                SerializeProperty(emitter, "Loop", component.Loop);
                SerializeProperty(emitter, "PlayOnStart", component.PlayOnStart);
            },
            [](auto& component, auto yamlNode) {
                DeserializePath(yamlNode, "SoundPath", component.SoundPath);
                DeserializeProperty(yamlNode, "Volume", component.Volume);
                DeserializeProperty(yamlNode, "Pitch", component.Pitch);
                DeserializeProperty(yamlNode, "Loop", component.Loop);
                DeserializeProperty(yamlNode, "PlayOnStart", component.PlayOnStart);
            }
        );
    }

    void ComponentSerializer::RegisterGameplayComponents()
    {
        // Billboard Component
        Register<BillboardComponent>("BillboardComponent",
            [](auto& emitter, auto& component) {
                SerializePath(emitter, "TexturePath", component.TexturePath);
                SerializeProperty(emitter, "Size", component.Size);
            },
            [](auto& component, auto yamlNode) {
                DeserializePath(yamlNode, "TexturePath", component.TexturePath);
                DeserializeProperty(yamlNode, "Size", component.Size);
            }
        );

        // Scene Transition Component
        Register<SceneTransitionComponent>("SceneTransitionComponent",
            [](auto& emitter, auto& component) {
                SerializePath(emitter, "TargetScenePath", component.TargetScenePath);
            },
            [](auto& component, auto yamlNode) {
                DeserializePath(yamlNode, "TargetScenePath", component.TargetScenePath);
            }
        );

        // Control Component (UI Base)
        Register<ControlComponent>("ControlComponent",
            [](auto& emitter, auto& component) {
                SerializeProperty(emitter, "ZOrder", component.ZOrder);
                SerializeProperty(emitter, "IsActive", component.IsActive);
                
                emitter << YAML::Key << "RectTransform" << YAML::BeginMap;
                SerializeProperty(emitter, "AnchorMin", component.Transform.AnchorMin);
                SerializeProperty(emitter, "AnchorMax", component.Transform.AnchorMax);
                SerializeProperty(emitter, "OffsetMin", component.Transform.OffsetMin);
                SerializeProperty(emitter, "OffsetMax", component.Transform.OffsetMax);
                SerializeProperty(emitter, "Pivot", component.Transform.Pivot);
                SerializeProperty(emitter, "Rotation", component.Transform.Rotation);
                SerializeProperty(emitter, "Scale", component.Transform.Scale);
                emitter << YAML::EndMap;
            },
            [](auto& component, auto yamlNode) {
                DeserializeProperty(yamlNode, "ZOrder", component.ZOrder);
                DeserializeProperty(yamlNode, "IsActive", component.IsActive);
                
                auto rectTransform = yamlNode["RectTransform"];
                if (rectTransform) {
                    DeserializeProperty(rectTransform, "AnchorMin", component.Transform.AnchorMin);
                    DeserializeProperty(rectTransform, "AnchorMax", component.Transform.AnchorMax);
                    DeserializeProperty(rectTransform, "OffsetMin", component.Transform.OffsetMin);
                    DeserializeProperty(rectTransform, "OffsetMax", component.Transform.OffsetMax);
                    DeserializeProperty(rectTransform, "Pivot", component.Transform.Pivot);
                    DeserializeProperty(rectTransform, "Rotation", component.Transform.Rotation);
                    DeserializeProperty(rectTransform, "Scale", component.Transform.Scale);
                }
            }
        );

        // Native Script Component
        Register<NativeScriptComponent>("NativeScriptComponent",
            [](auto& emitter, auto& nativeScriptComponent) {
                emitter << YAML::Key << "Scripts" << YAML::Value << YAML::BeginSeq;
                for (const auto& script : nativeScriptComponent.Scripts)
                {
                    emitter << YAML::BeginMap;
                    SerializeProperty(emitter, "ScriptName", script.ScriptName);
                    emitter << YAML::EndMap;
                }
                emitter << YAML::EndSeq;
            },
            [](auto& component, auto yamlNode) {
                auto scripts = yamlNode["Scripts"];
                if (scripts && scripts.IsSequence())
                {
                    for (auto scriptNode : scripts)
                    {
                        std::string scriptName;
                        DeserializeProperty(scriptNode, "ScriptName", scriptName);
                        ScriptRegistry::AddScript(scriptName, component);
                    }
                }
            }
        );

        // Animation Component
        Register<AnimationComponent>("AnimationComponent",
            [](auto& emitter, auto& component) {
                SerializePath(emitter, "AnimationPath", component.AnimationPath);
                SerializeProperty(emitter, "CurrentAnimationIndex", component.CurrentAnimationIndex);
                SerializeProperty(emitter, "IsLooping", component.IsLooping);
                SerializeProperty(emitter, "IsPlaying", component.IsPlaying);
            },
            [](auto& component, auto yamlNode) {
                DeserializePath(yamlNode, "AnimationPath", component.AnimationPath);
                DeserializeProperty(yamlNode, "CurrentAnimationIndex", component.CurrentAnimationIndex);
                DeserializeProperty(yamlNode, "IsLooping", component.IsLooping);
                DeserializeProperty(yamlNode, "IsPlaying", component.IsPlaying);
            }
        );

        // Navigation Component
        Register<NavigationComponent>("NavigationComponent",
            [](auto& emitter, auto& component) {
                SerializeProperty(emitter, "IsDefaultFocus", component.IsDefaultFocus);
                // We don't serialize entity handles (Up/Down/etc) here yet 
                // as they need UUID translation similar to Hierarchy
            },
            [](auto& component, auto yamlNode) {
                DeserializeProperty(yamlNode, "IsDefaultFocus", component.IsDefaultFocus);
            }
        );
        // Spawn Component
        Register<SpawnComponent>("SpawnComponent",
            [](auto& emitter, auto& component) {
                SerializeProperty(emitter, "SpawnZoneSize", component.ZoneSize);
                SerializePath(emitter, "SpawnTexturePath", component.TexturePath);
                SerializeProperty(emitter, "RenderSpawnZoneInScene", component.RenderSpawnZoneInScene);
            },
            [](auto& component, auto yamlNode) {
                DeserializeProperty(yamlNode, "SpawnZoneSize", component.ZoneSize);
                DeserializePath(yamlNode, "SpawnTexturePath", component.TexturePath);
                DeserializeProperty(yamlNode, "RenderSpawnZoneInScene", component.RenderSpawnZoneInScene);
            }
        );

        // Player Component
        Register<PlayerComponent>("PlayerComponent",
            [](auto& emitter, auto& component) {
                SerializeProperty(emitter, "MovementSpeed", component.MovementSpeed);
                SerializeProperty(emitter, "LookSensitivity", component.LookSensitivity);
                SerializeProperty(emitter, "JumpForce", component.JumpForce);
            },
            [](auto& component, auto yamlNode) {
                DeserializeProperty(yamlNode, "MovementSpeed", component.MovementSpeed);
                DeserializeProperty(yamlNode, "LookSensitivity", component.LookSensitivity);
                DeserializeProperty(yamlNode, "JumpForce", component.JumpForce);
            }
        );

        // Model Component
        Register<ModelComponent>("ModelComponent",
            [](auto& emitter, auto& modelComponent) {
                SerializePath(emitter, "ModelPath", modelComponent.ModelPath);
                
                if (!modelComponent.Materials.empty())
                {
                    emitter << YAML::Key << "Materials" << YAML::BeginSeq;
                    for (const auto& materialSlot : modelComponent.Materials)
                    {
                        emitter << YAML::BeginMap;
                        SerializeProperty(emitter, "Name", materialSlot.Name);
                        SerializeProperty(emitter, "Index", materialSlot.Index);
                        SerializeProperty(emitter, "AlbedoColor", materialSlot.Material.AlbedoColor);
                        SerializeProperty(emitter, "Metalness", materialSlot.Material.Metalness);
                        SerializeProperty(emitter, "Roughness", materialSlot.Material.Roughness);
                        
                        // Advanced Properties
                        SerializeProperty(emitter, "DoubleSided", materialSlot.Material.DoubleSided);
                        SerializeProperty(emitter, "Transparent", materialSlot.Material.Transparent);
                        SerializeProperty(emitter, "Alpha", materialSlot.Material.Alpha);
                        emitter << YAML::EndMap;
                    }
                    emitter << YAML::EndSeq;
                }
            },
            [](auto& component, auto yamlNode) {
                DeserializePath(yamlNode, "ModelPath", component.ModelPath);

                auto materials = yamlNode["Materials"];
                if (materials && materials.IsSequence())
                {
                    component.Materials.clear();
                    for (auto materialNode : materials)
                    {
                        MaterialSlot materialSlot;
                        DeserializeProperty(materialNode, "Name", materialSlot.Name);
                        DeserializeProperty(materialNode, "Index", materialSlot.Index);
                        DeserializeProperty(materialNode, "AlbedoColor", materialSlot.Material.AlbedoColor);
                        DeserializeProperty(materialNode, "Metalness", materialSlot.Material.Metalness);
                        DeserializeProperty(materialNode, "Roughness", materialSlot.Material.Roughness);

                        // Advanced Properties
                        DeserializeProperty(materialNode, "DoubleSided", materialSlot.Material.DoubleSided);
                        DeserializeProperty(materialNode, "Transparent", materialSlot.Material.Transparent);
                        DeserializeProperty(materialNode, "Alpha", materialSlot.Material.Alpha);
                        component.Materials.push_back(materialSlot);
                    }
                    component.MaterialsInitialized = true;
                }
            }
        );
    }
}
