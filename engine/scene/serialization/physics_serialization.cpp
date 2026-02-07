#include "engine/scene/component_serializer.h"
#include "engine/scene/scene.h"
#include "engine/scene/components.h"
#include "engine/scene/serialization_utils.h"

namespace CHEngine
{
    using namespace SerializationUtils;

    void ComponentSerializer::RegisterPhysicsComponents()
    {
        // RigidBody Component
        Register<RigidBodyComponent>("RigidBodyComponent",
            [](auto& emitter, auto& component) {
                SerializeProperty(emitter, "Mass", component.Mass);
                SerializeProperty(emitter, "UseGravity", component.UseGravity);
                SerializeProperty(emitter, "IsKinematic", component.IsKinematic);
                SerializeProperty(emitter, "Velocity", component.Velocity);
            },
            [](auto& component, auto yamlNode) {
                DeserializeProperty(yamlNode, "Mass", component.Mass);
                DeserializeProperty(yamlNode, "UseGravity", component.UseGravity);
                DeserializeProperty(yamlNode, "IsKinematic", component.IsKinematic);
                DeserializeProperty(yamlNode, "Velocity", component.Velocity);
            }
        );

        // Collider Component
        Register<ColliderComponent>("ColliderComponent",
            [](auto& emitter, auto& component) {
                SerializeProperty(emitter, "Type", (int)component.Type);
                SerializeProperty(emitter, "Enabled", component.Enabled);
                SerializeProperty(emitter, "Offset", component.Offset);
                SerializeProperty(emitter, "Size", component.Size);
                SerializeProperty(emitter, "Radius", component.Radius);
                SerializeProperty(emitter, "Height", component.Height);
                SerializeProperty(emitter, "AutoCalculate", component.AutoCalculate);
                SerializeProperty(emitter, "ModelPath", component.ModelPath);
            },
            [](auto& component, auto yamlNode) {
                int colliderType = 0;
                DeserializeProperty(yamlNode, "Type", colliderType);
                component.Type = (ColliderType)colliderType;
                
                DeserializeProperty(yamlNode, "Enabled", component.Enabled);
                DeserializeProperty(yamlNode, "Offset", component.Offset);
                DeserializeProperty(yamlNode, "Size", component.Size);
                DeserializeProperty(yamlNode, "Radius", component.Radius);
                DeserializeProperty(yamlNode, "Height", component.Height);
                DeserializeProperty(yamlNode, "AutoCalculate", component.AutoCalculate);
                DeserializeProperty(yamlNode, "ModelPath", component.ModelPath);
            }
        );
    }
}
