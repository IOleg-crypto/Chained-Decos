#ifndef CH_COMPONENT_SERIALIZER_H
#define CH_COMPONENT_SERIALIZER_H

#include "engine/scene/entity.h"
#include <yaml-cpp/yaml.h>
#include <vector>

namespace CHEngine
{
    struct HierarchyTask
    {
        Entity entity;
        uint64_t parent;
        std::vector<uint64_t> children;
    };

    class ComponentSerializer
    {
    public:
        // Serialization
        static void SerializeID(YAML::Emitter& out, Entity entity);
        static void SerializeTag(YAML::Emitter& out, Entity entity);
        static void SerializeTransform(YAML::Emitter& out, Entity entity);
        static void SerializeModel(YAML::Emitter& out, Entity entity);
        static void SerializeSpawn(YAML::Emitter& out, Entity entity);
        static void SerializeCollider(YAML::Emitter& out, Entity entity);
        static void SerializePointLight(YAML::Emitter& out, Entity entity);
        static void SerializeSpotLight(YAML::Emitter& out, Entity entity);
        static void SerializeRigidBody(YAML::Emitter& out, Entity entity);
        static void SerializePlayer(YAML::Emitter& out, Entity entity);
        static void SerializeSceneTransition(YAML::Emitter& out, Entity entity);
        static void SerializeBillboard(YAML::Emitter& out, Entity entity);
        static void SerializeNavigation(YAML::Emitter& out, Entity entity);
        static void SerializeShader(YAML::Emitter& out, Entity entity);
        static void SerializeAnimation(YAML::Emitter& out, Entity entity);
        static void SerializeHierarchy(YAML::Emitter& out, Entity entity);
        static void SerializeAudio(YAML::Emitter& out, Entity entity);
        static void SerializeCamera(YAML::Emitter& out, Entity entity);
        static void SerializeNativeScript(YAML::Emitter& out, Entity entity);
        static void SerializeUI(YAML::Emitter& out, Entity entity);

        // Deserialization
        static void DeserializeTag(Entity entity, YAML::Node node);
        static void DeserializeTransform(Entity entity, YAML::Node node);
        static void DeserializeModel(Entity entity, YAML::Node node);
        static void DeserializeSpawn(Entity entity, YAML::Node node);
        static void DeserializeCollider(Entity entity, YAML::Node node);
        static void DeserializePointLight(Entity entity, YAML::Node node);
        static void DeserializeSpotLight(Entity entity, YAML::Node node);
        static void DeserializeRigidBody(Entity entity, YAML::Node node);
        static void DeserializePlayer(Entity entity, YAML::Node node);
        static void DeserializeSceneTransition(Entity entity, YAML::Node node);
        static void DeserializeBillboard(Entity entity, YAML::Node node);
        static void DeserializeNavigation(Entity entity, YAML::Node node);
        static void DeserializeShader(Entity entity, YAML::Node node);
        static void DeserializeAnimation(Entity entity, YAML::Node node);
        static void DeserializeHierarchyTask(Entity entity, YAML::Node node, HierarchyTask& outTask);
        static void DeserializeAudio(Entity entity, YAML::Node node);
        static void DeserializeCamera(Entity entity, YAML::Node node);
        static void DeserializeNativeScript(Entity entity, YAML::Node node);
        static void DeserializeUI(Entity entity, YAML::Node node);
    };
}

#endif // CH_COMPONENT_SERIALIZER_H
