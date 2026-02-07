#include "engine/scene/component_serializer.h"
#include "engine/scene/scene.h"
#include "engine/scene/components.h"
#include "engine/scene/serialization_utils.h"

namespace CHEngine
{
    using namespace SerializationUtils;

    void ComponentSerializer::RegisterGraphicsComponents()
    {
        // Tag Component
        Register<TagComponent>("TagComponent",
            [](auto& emitter, auto& component) {
                SerializeProperty(emitter, "Tag", component.Tag);
            },
            [](auto& component, auto yamlNode) {
                DeserializeProperty(yamlNode, "Tag", component.Tag);
            }
        );

        // Transform Component
        Register<TransformComponent>("TransformComponent",
            [](auto& emitter, auto& component) {
                SerializeProperty(emitter, "Translation", component.Translation);
                SerializeProperty(emitter, "Rotation", component.Rotation);
                SerializeProperty(emitter, "Scale", component.Scale);
            },
            [](auto& component, auto yamlNode) {
                DeserializeProperty(yamlNode, "Translation", component.Translation);
                DeserializeProperty(yamlNode, "Rotation", component.Rotation);
                DeserializeProperty(yamlNode, "Scale", component.Scale);
            }
        );

        // Point Light Component
        Register<PointLightComponent>("PointLightComponent",
            [](auto& emitter, auto& component) {
                SerializeProperty(emitter, "Color", component.LightColor);
                SerializeProperty(emitter, "Intensity", component.Intensity);
                SerializeProperty(emitter, "Radius", component.Radius);
            },
            [](auto& component, auto yamlNode) {
                DeserializeProperty(yamlNode, "Color", component.LightColor);
                DeserializeProperty(yamlNode, "Intensity", component.Intensity);
                DeserializeProperty(yamlNode, "Radius", component.Radius);
            }
        );

        // Camera Component
        Register<CameraComponent>("CameraComponent",
            [](auto& emitter, auto& component) {
                auto& camera = component.Camera;
                SerializeProperty(emitter, "ProjectionType", (int)camera.GetProjectionType());
                
                SerializeProperty(emitter, "PerspectiveFOV", camera.GetPerspectiveVerticalFOV());
                SerializeProperty(emitter, "PerspectiveNear", camera.GetPerspectiveNearClip());
                SerializeProperty(emitter, "PerspectiveFar", camera.GetPerspectiveFarClip());
                
                SerializeProperty(emitter, "OrthographicSize", camera.GetOrthographicSize());
                SerializeProperty(emitter, "OrthographicNear", camera.GetOrthographicNearClip());
                SerializeProperty(emitter, "OrthographicFar", camera.GetOrthographicFarClip());
                
                SerializeProperty(emitter, "Primary", component.Primary);
                SerializeProperty(emitter, "FixedAspectRatio", component.FixedAspectRatio);
                
                // Orbit camera settings
                SerializeProperty(emitter, "IsOrbitCamera", component.IsOrbitCamera);
                SerializeProperty(emitter, "TargetEntityTag", component.TargetEntityTag);
                SerializeProperty(emitter, "OrbitDistance", component.OrbitDistance);
                SerializeProperty(emitter, "OrbitYaw", component.OrbitYaw);
                SerializeProperty(emitter, "OrbitPitch", component.OrbitPitch);
                SerializeProperty(emitter, "LookSensitivity", component.LookSensitivity);
            },
            [](auto& component, auto yamlNode) {
                auto& camera = component.Camera;
                
                if (yamlNode["ProjectionType"])
                {
                    int projType;
                    DeserializeProperty(yamlNode, "ProjectionType", projType);
                    camera.SetProjectionType(static_cast<CHEngine::ProjectionType>(projType));
                }
                
                if (yamlNode["PerspectiveFOV"]) { float fov; DeserializeProperty(yamlNode, "PerspectiveFOV", fov); camera.SetPerspectiveVerticalFOV(fov); }
                if (yamlNode["PerspectiveNear"]) { float n; DeserializeProperty(yamlNode, "PerspectiveNear", n); camera.SetPerspectiveNearClip(n); }
                if (yamlNode["PerspectiveFar"]) { float f; DeserializeProperty(yamlNode, "PerspectiveFar", f); camera.SetPerspectiveFarClip(f); }
                
                if (yamlNode["OrthographicSize"]) { float s; DeserializeProperty(yamlNode, "OrthographicSize", s); camera.SetOrthographicSize(s); }
                if (yamlNode["OrthographicNear"]) { float n; DeserializeProperty(yamlNode, "OrthographicNear", n); camera.SetOrthographicNearClip(n); }
                if (yamlNode["OrthographicFar"]) { float f; DeserializeProperty(yamlNode, "OrthographicFar", f); camera.SetOrthographicFarClip(f); }
                
                DeserializeProperty(yamlNode, "Primary", component.Primary);
                DeserializeProperty(yamlNode, "FixedAspectRatio", component.FixedAspectRatio);

                // --- BACKWARD COMPATIBILITY ---
                if (yamlNode["Fov"]) { float fov; DeserializeProperty(yamlNode, "Fov", fov); camera.SetPerspectiveVerticalFOV(fov * DEG2RAD); }
                if (yamlNode["IsPrimary"]) { bool b; DeserializeProperty(yamlNode, "IsPrimary", b); component.Primary = b; }
                if (yamlNode["NearPlane"]) { float n; DeserializeProperty(yamlNode, "NearPlane", n); camera.SetPerspectiveNearClip(n); }
                if (yamlNode["FarPlane"]) { float f; DeserializeProperty(yamlNode, "FarPlane", f); camera.SetPerspectiveFarClip(f); }
                
                if (yamlNode["Projection"])
                {
                    int projValue;
                    DeserializeProperty(yamlNode, "Projection", projValue);
                    camera.SetProjectionType(static_cast<CHEngine::ProjectionType>(projValue));
                }
                
                // Orbit camera settings
                DeserializeProperty(yamlNode, "IsOrbitCamera", component.IsOrbitCamera);
                DeserializeProperty(yamlNode, "TargetEntityTag", component.TargetEntityTag);
                DeserializeProperty(yamlNode, "OrbitDistance", component.OrbitDistance);
                DeserializeProperty(yamlNode, "OrbitYaw", component.OrbitYaw);
                DeserializeProperty(yamlNode, "OrbitPitch", component.OrbitPitch);
                DeserializeProperty(yamlNode, "LookSensitivity", component.LookSensitivity);
            }
        );

        // Spot Light Component
        Register<SpotLightComponent>("SpotLightComponent",
            [](auto& emitter, auto& component) {
                SerializeProperty(emitter, "Color", component.LightColor);
                SerializeProperty(emitter, "Intensity", component.Intensity);
                SerializeProperty(emitter, "Range", component.Range);
                SerializeProperty(emitter, "InnerCutoff", component.InnerCutoff);
                SerializeProperty(emitter, "OuterCutoff", component.OuterCutoff);
            },
            [](auto& component, auto yamlNode) {
                DeserializeProperty(yamlNode, "Color", component.LightColor);
                DeserializeProperty(yamlNode, "Intensity", component.Intensity);
                DeserializeProperty(yamlNode, "Range", component.Range);
                DeserializeProperty(yamlNode, "InnerCutoff", component.InnerCutoff);
                DeserializeProperty(yamlNode, "OuterCutoff", component.OuterCutoff);
            }
        );

        // Shader Component
        Register<ShaderComponent>("ShaderComponent",
            [](auto& emitter, auto& component) {
                SerializePath(emitter, "ShaderPath", component.ShaderPath);
                
                if (!component.Uniforms.empty())
                {
                    emitter << YAML::Key << "Uniforms" << YAML::BeginSeq;
                    for (const auto& uniform : component.Uniforms)
                    {
                        emitter << YAML::BeginMap;
                        SerializeProperty(emitter, "Name", uniform.Name);
                        SerializeProperty(emitter, "Type", (int)uniform.Type);
                        // SerializeProperty(emitter, "Value", uniform.Value); // Needs special handling for variant
                        emitter << YAML::EndMap;
                    }
                    emitter << YAML::EndSeq;
                }
            },
            [](auto& component, auto yamlNode) {
                DeserializePath(yamlNode, "ShaderPath", component.ShaderPath);
                // Uniform deserialization logic here
            }
        );
    }
}
