#include "component_serializer.h"
#include "scene.h"
#include "components.h"
#include "engine/core/yaml.h"
#include "script_registry.h"
#include "components/hierarchy_component.h"
#include "components/id_component.h"
#include "engine/scene/serialization_utils.h"

namespace CHEngine
{
    using namespace SerializationUtils;

    std::vector<ComponentSerializerEntry> ComponentSerializer::s_Registry;

    // --- Special Serialization Helpers ---

    void ComponentSerializer::SerializeID(YAML::Emitter& out, Entity entity)
    {
        if (entity.HasComponent<IDComponent>())
            out << YAML::Key << "Entity" << YAML::Value << (uint64_t)entity.GetComponent<IDComponent>().ID;
        else
            out << YAML::Key << "Entity" << YAML::Value << 0;
    }

    void ComponentSerializer::SerializeHierarchy(YAML::Emitter& out, Entity entity)
    {
        if (entity.HasComponent<HierarchyComponent>())
        {
            auto& hc = entity.GetComponent<HierarchyComponent>();
            out << YAML::Key << "Hierarchy";
            out << YAML::BeginMap;

            uint64_t parentUUID = 0;
            if (hc.Parent != entt::null)
            {
                Entity parent{hc.Parent, entity.GetScene()};
                if (parent.HasComponent<IDComponent>())
                    parentUUID = (uint64_t)parent.GetComponent<IDComponent>().ID;
            }
            out << YAML::Key << "Parent" << YAML::Value << parentUUID;

            out << YAML::Key << "Children" << YAML::BeginSeq;
            for (auto childHandle : hc.Children)
            {
                Entity child{childHandle, entity.GetScene()};
                if (child.HasComponent<IDComponent>())
                    out << (uint64_t)child.GetComponent<IDComponent>().ID;
            }
            out << YAML::EndSeq;

            out << YAML::EndMap;
        }
    }

    void ComponentSerializer::DeserializeHierarchyTask(Entity entity, YAML::Node node, HierarchyTask& outTask)
    {
        auto hierarchyNode = node["Hierarchy"];
        if (hierarchyNode)
        {
            outTask.entity = entity;
            outTask.parent = hierarchyNode["Parent"] ? hierarchyNode["Parent"].as<uint64_t>() : 0;
            if (hierarchyNode["Children"])
            {
                for (auto child : hierarchyNode["Children"])
                    outTask.children.push_back(child.as<uint64_t>());
            }
        }
    }

    // --- Main Initialization ---

    void ComponentSerializer::Initialize()
    {
        s_Registry.clear();

        // ==========================================
        // GRAPHICS COMPONENTS
        // ==========================================
        
        Register<TagComponent>("TagComponent", [](auto& archive, auto& component) {
            archive.Property("Tag", component.Tag);
        });

        Register<TransformComponent>("TransformComponent", [](auto& archive, auto& component) {
            archive.Property("Translation", component.Translation)
                   .Property("Rotation", component.Rotation)
                   .Property("Scale", component.Scale);
            
            if (archive.GetMode() == PropertyArchive::Deserialize)
            {
                component.RotationQuat = QuaternionFromEuler(
                    component.Rotation.x * DEG2RAD, 
                    component.Rotation.y * DEG2RAD, 
                    component.Rotation.z * DEG2RAD
                );
            }
        });

        Register<PointLightComponent>("PointLightComponent", [](auto& archive, auto& component) {
            archive.Property("Color", component.LightColor)
                   .Property("Intensity", component.Intensity)
                   .Property("Radius", component.Radius);
        });

        Register<SpotLightComponent>("SpotLightComponent", [](auto& archive, auto& component) {
            archive.Property("Color", component.LightColor)
                   .Property("Intensity", component.Intensity)
                   .Property("Range", component.Range)
                   .Property("InnerCutoff", component.InnerCutoff)
                   .Property("OuterCutoff", component.OuterCutoff);
        });

        Register<CameraComponent>("CameraComponent", [](auto& archive, auto& component) {
            auto& camera = component.Camera;
            
            // Helper for ProjectionType enum
            int projType = (int)camera.GetProjectionType();
            if (archive.GetMode() == PropertyArchive::Deserialize)
            {
                 if (archive.HasProperty("ProjectionType")) 
                    archive.Property("ProjectionType", projType);
                 // Backwards compatibility
                 else if (archive.HasProperty("Projection"))
                    archive.Property("Projection", projType);

                 camera.SetProjectionType((ProjectionType)projType);
            }
            else
            {
                archive.Property("ProjectionType", projType);
            }

            // Properties that require setters/getters on Camera object
            // We use local variables as intermediates
            float pFov = camera.GetPerspectiveVerticalFOV();
            float pNear = camera.GetPerspectiveNearClip();
            float pFar = camera.GetPerspectiveFarClip();
            float oSize = camera.GetOrthographicSize();
            float oNear = camera.GetOrthographicNearClip();
            float oFar = camera.GetOrthographicFarClip();

            archive.Property("PerspectiveFOV", pFov)
                   .Property("PerspectiveNear", pNear)
                   .Property("PerspectiveFar", pFar)
                   .Property("OrthographicSize", oSize)
                   .Property("OrthographicNear", oNear)
                   .Property("OrthographicFar", oFar);

            if (archive.GetMode() == PropertyArchive::Deserialize)
            {
                camera.SetPerspectiveVerticalFOV(pFov);
                camera.SetPerspectiveNearClip(pNear);
                camera.SetPerspectiveFarClip(pFar);
                camera.SetOrthographicSize(oSize);
                camera.SetOrthographicNearClip(oNear);
                camera.SetOrthographicFarClip(oFar);
                
                // Backwards compatibility
                if (archive.HasProperty("Fov")) { float f; archive.Property("Fov", f); camera.SetPerspectiveVerticalFOV(f * DEG2RAD); }
                if (archive.HasProperty("NearPlane")) { float n; archive.Property("NearPlane", n); camera.SetPerspectiveNearClip(n); }
                if (archive.HasProperty("FarPlane")) { float f; archive.Property("FarPlane", f); camera.SetPerspectiveFarClip(f); }
                if (archive.HasProperty("IsPrimary")) { bool b; archive.Property("IsPrimary", b); component.Primary = b; }
            }

            archive.Property("Primary", component.Primary)
                   .Property("FixedAspectRatio", component.FixedAspectRatio)
                   .Property("IsOrbitCamera", component.IsOrbitCamera)
                   .Property("TargetEntityTag", component.TargetEntityTag)
                   .Property("OrbitDistance", component.OrbitDistance)
                   .Property("OrbitYaw", component.OrbitYaw)
                   .Property("OrbitPitch", component.OrbitPitch)
                   .Property("LookSensitivity", component.LookSensitivity);
        });

        Register<ShaderComponent>("ShaderComponent", [](auto& archive, auto& component) {
            archive.Path("ShaderPath", component.ShaderPath);
            // Uniforms are complex, handle carefully
            // Keeping simple for now, full implementation would require custom nested logic similar to Register overload
        });
        
        Register<ModelComponent>("ModelComponent", [](auto& archive, auto& component) {
            archive.Path("ModelPath", component.ModelPath);
            // Material slots logic (simplified or full)
             if (archive.GetMode() == PropertyArchive::Serialize)
            {
                 if (!component.Materials.empty())
                 {
                     // Custom sequence handling for complex objects
                     // (PropertyArchive currently supports simple types in Sequence)
                     // Falling back to raw yaml for complex sequence
                 }
            }
        });

        // ==========================================
        // PHYSICS COMPONENTS
        // ==========================================

        Register<RigidBodyComponent>("RigidBodyComponent", [](auto& archive, auto& component) {
            archive.Property("Mass", component.Mass)
                   .Property("UseGravity", component.UseGravity)
                   .Property("IsKinematic", component.IsKinematic)
                   .Property("Velocity", component.Velocity);
        });

        Register<ColliderComponent>("ColliderComponent", [](auto& archive, auto& component) {
            int type = (int)component.Type;
            archive.Property("Type", type)
                   .Property("Enabled", component.Enabled)
                   .Property("Offset", component.Offset)
                   .Property("Size", component.Size)
                   .Property("Radius", component.Radius)
                   .Property("Height", component.Height)
                   .Property("AutoCalculate", component.AutoCalculate)
                   .Property("ModelPath", component.ModelPath);
            
            if (archive.GetMode() == PropertyArchive::Deserialize)
                component.Type = (ColliderType)type;
        });

        // ==========================================
        // AUDIO & GAMEPLAY COMPONENTS
        // ==========================================

        Register<AudioComponent>("AudioComponent", [](auto& archive, auto& component) {
            archive.Path("SoundPath", component.SoundPath)
                   .Property("Volume", component.Volume)
                   .Property("Pitch", component.Pitch)
                   .Property("Loop", component.Loop)
                   .Property("PlayOnStart", component.PlayOnStart);
        });

        Register<BillboardComponent>("BillboardComponent", [](auto& archive, auto& component) {
            archive.Path("TexturePath", component.TexturePath)
                   .Property("Size", component.Size);
        });

        Register<SceneTransitionComponent>("SceneTransitionComponent", [](auto& archive, auto& component) {
            archive.Path("TargetScenePath", component.TargetScenePath);
        });

        Register<AnimationComponent>("AnimationComponent", [](auto& archive, auto& component) {
            archive.Path("AnimationPath", component.AnimationPath)
                   .Property("CurrentAnimationIndex", component.CurrentAnimationIndex)
                   .Property("IsLooping", component.IsLooping)
                   .Property("IsPlaying", component.IsPlaying);
        });
        
        Register<PlayerComponent>("PlayerComponent", [](auto& archive, auto& component) {
            archive.Property("MovementSpeed", component.MovementSpeed)
                   .Property("LookSensitivity", component.LookSensitivity)
                   .Property("JumpForce", component.JumpForce);
        });

        Register<NavigationComponent>("NavigationComponent", [](auto& archive, auto& component) {
            archive.Property("IsDefaultFocus", component.IsDefaultFocus);
        });

        Register<SpawnComponent>("SpawnComponent", [](auto& archive, auto& component) {
            archive.Property("SpawnZoneSize", component.ZoneSize)
                   .Path("SpawnTexturePath", component.TexturePath)
                   .Property("RenderSpawnZoneInScene", component.RenderSpawnZoneInScene);
        });

        // Manual registration for NativeScriptComponent to access Entity during deserialization
        {
            ComponentSerializerEntry entry;
            entry.YamlKey = "NativeScriptComponent";
            entry.Serialize = [](YAML::Emitter& out, Entity entity) {
                if (entity.HasComponent<NativeScriptComponent>()) {
                    out << YAML::Key << "NativeScriptComponent";
                    out << YAML::BeginMap;
                    
                    auto& component = entity.GetComponent<NativeScriptComponent>();
                    std::vector<std::string> scriptNames;
                    for (const auto& script : component.Scripts)
                        scriptNames.push_back(script.ScriptName);
                    
                    out << YAML::Key << "Scripts" << YAML::Value << YAML::BeginSeq;
                    for (const auto& item : scriptNames) out << item;
                    out << YAML::EndSeq;

                    out << YAML::EndMap;
                }
            };
            entry.Deserialize = [](Entity entity, YAML::Node node) {
                 auto componentNode = node["NativeScriptComponent"];
                 if (componentNode) {
                    if (!entity.HasComponent<NativeScriptComponent>())
                        entity.AddComponent<NativeScriptComponent>();
                    
                    auto& component = entity.GetComponent<NativeScriptComponent>();
                    
                    std::vector<std::string> scriptNames;
                    if (componentNode["Scripts"]) {
                        for (auto item : componentNode["Scripts"])
                            scriptNames.push_back(item.as<std::string>());
                    }

                    component.Scripts.clear();
                    if (entity.GetScene()) {
                        for (const auto& name : scriptNames)
                            entity.GetScene()->GetScriptRegistry().AddScript(name, component);
                    }
                 }
            };
            entry.Copy = [](Entity source, Entity destination) {
                if (source.HasComponent<NativeScriptComponent>()) {
                     destination.AddOrReplaceComponent<NativeScriptComponent>(source.GetComponent<NativeScriptComponent>());
                }
            };
            s_Registry.push_back(entry);
        }
        
        RegisterUIComponents(); // Defined in component_serializer_ui.cpp (or moved here later)
    }

    void ComponentSerializer::SerializeAll(YAML::Emitter& out, Entity entity)
    {
        for (auto& entry : s_Registry)
        {
            entry.Serialize(out, entity);
        }
        SerializeHierarchy(out, entity);
    }

    void ComponentSerializer::DeserializeAll(Entity entity, YAML::Node node)
    {
        for (const auto& entry : s_Registry)
        {
            if (entry.Deserialize)
            {
                entry.Deserialize(entity, node);
            }
        }
    }

    void ComponentSerializer::CopyAll(Entity source, Entity destination)
    {
        for (const auto& entry : s_Registry)
        {
            if (entry.Copy)
            {
                entry.Copy(source, destination);
            }
        }
    }

} // namespace CHEngine
