#include "component_serializer.h"
#include "scene.h"
#include "components.h"
#include "engine/core/yaml.h"
#include "script_registry.h"

namespace CHEngine
{
    std::vector<ComponentSerializerEntry> ComponentSerializer::s_Registry;

#define WRITE_PROP(...) out << YAML::Key << #__VA_ARGS__ << YAML::Value << c.__VA_ARGS__
#define READ_PROP(prop, type) if (n[#prop]) c.prop = n[#prop].template as<type>()

    // === Reflective Serialization Helpers ===

    template<typename TValue>
    static void SerializeProperty(YAML::Emitter& out, const PropertyInfo& prop, const void* componentPtr)
    {
        out << YAML::Key << prop.Name << YAML::Value << *(TValue*)((char*)componentPtr + prop.Offset);
    }

    template<typename TValue>
    static void DeserializeProperty(const YAML::Node& node, const PropertyInfo& prop, void* componentPtr)
    {
        if (node[prop.Name])
            *(TValue*)((char*)componentPtr + prop.Offset) = node[prop.Name].as<TValue>();
    }

    template<typename T>
    static void ReflectiveSerialize(YAML::Emitter& out, T& comp)
    {
        for (const auto& prop : ReflectData<T>::GetProperties())
        {
            std::string type = prop.TypeName;
            if (type == "float") SerializeProperty<float>(out, prop, &comp);
            else if (type == "int") SerializeProperty<int>(out, prop, &comp);
            else if (type == "bool") SerializeProperty<bool>(out, prop, &comp);
            else if (type == "std::string") SerializeProperty<std::string>(out, prop, &comp);
            else if (type == "Vector2") SerializeProperty<Vector2>(out, prop, &comp);
            else if (type == "Vector3") SerializeProperty<Vector3>(out, prop, &comp);
            else if (type == "Vector4") SerializeProperty<Vector4>(out, prop, &comp);
            else if (type == "Quaternion") SerializeProperty<Quaternion>(out, prop, &comp);
            else if (type == "Color") SerializeProperty<Color>(out, prop, &comp);
            else if (type == "glm::vec2") SerializeProperty<glm::vec2>(out, prop, &comp);
            else if (type == "glm::vec3") SerializeProperty<glm::vec3>(out, prop, &comp);
        }
    }

    template<typename T>
    static void ReflectiveDeserialize(T& comp, YAML::Node node)
    {
        for (const auto& prop : ReflectData<T>::GetProperties())
        {
            std::string type = prop.TypeName;
            if (type == "float") DeserializeProperty<float>(node, prop, &comp);
            else if (type == "int") DeserializeProperty<int>(node, prop, &comp);
            else if (type == "bool") DeserializeProperty<bool>(node, prop, &comp);
            else if (type == "std::string") DeserializeProperty<std::string>(node, prop, &comp);
            else if (type == "Vector2") DeserializeProperty<Vector2>(node, prop, &comp);
            else if (type == "Vector3") DeserializeProperty<Vector3>(node, prop, &comp);
            else if (type == "Vector4") DeserializeProperty<Vector4>(node, prop, &comp);
            else if (type == "Quaternion") DeserializeProperty<Quaternion>(node, prop, &comp);
            else if (type == "Color") DeserializeProperty<Color>(node, prop, &comp);
            else if (type == "glm::vec2") DeserializeProperty<glm::vec2>(node, prop, &comp);
            else if (type == "glm::vec3") DeserializeProperty<glm::vec3>(node, prop, &comp);
        }
    }

    template<typename T>
    static void RegisterReflective(const std::string& key)
    {
        ComponentSerializer::Register<T>(key,
            [](YAML::Emitter& out, T& comp) { 
                out << YAML::BeginMap;
                ReflectiveSerialize<T>(out, comp); 
                out << YAML::EndMap;
            },
            [](T& comp, YAML::Node node) { 
                if (node) ReflectiveDeserialize<T>(comp, node); 
            }
        );
    }

    // === Manual Serializers for complex types using reflection hooks ===

    static void SerializeModelHelper(YAML::Emitter& out, Entity entity)
    {
        if (entity.HasComponent<ModelComponent>())
        {
            out << YAML::Key << "ModelComponent";
            auto &mc = entity.GetComponent<ModelComponent>();
            out << YAML::BeginMap;
            ReflectiveSerialize<ModelComponent>(out, mc);
            
            if (!mc.Materials.empty())
            {
                out << YAML::Key << "Materials" << YAML::BeginSeq;
                for (const auto &slot : mc.Materials)
                {
                    out << YAML::BeginMap;
                    out << YAML::Key << "Name" << YAML::Value << slot.Name;
                    out << YAML::Key << "Index" << YAML::Value << slot.Index;
                    out << YAML::Key << "Target" << YAML::Value << (int)slot.Target;
                    out << YAML::Key << "Material" << YAML::BeginMap;
                    ReflectiveSerialize<MaterialAsset>(out, slot.Material);
                    out << YAML::EndMap;
                    out << YAML::EndMap;
                }
                out << YAML::EndSeq;
            }
            out << YAML::EndMap;
        }
    }

    static void DeserializeModelHelper(Entity entity, YAML::Node node)
    {
        auto modelComponent = node["ModelComponent"];
        if (modelComponent)
        {
            auto &mc = entity.AddComponent<ModelComponent>();
            ReflectiveDeserialize<ModelComponent>(mc, modelComponent);

            auto materials = modelComponent["Materials"];
            if (materials && materials.IsSequence())
            {
                for (auto slotNode : materials)
                {
                    MaterialSlot slot;
                    slot.Name = slotNode["Name"].as<std::string>();
                    slot.Index = slotNode["Index"].as<int>();
                    if (slotNode["Target"]) slot.Target = (MaterialSlotTarget)slotNode["Target"].as<int>();

                    if (slotNode["Material"])
                        ReflectiveDeserialize<MaterialAsset>(slot.Material, slotNode["Material"]);
                    
                    mc.Materials.push_back(slot);
                }
                mc.MaterialsInitialized = true;
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

    void ComponentSerializer::Init()
    {
        s_Registry.clear();

        RegisterReflective<TagComponent>("TagComponent");
        RegisterReflective<TransformComponent>("TransformComponent");
        RegisterReflective<SpawnComponent>("SpawnComponent");

        Register<ColliderComponent>("ColliderComponent",
            [](auto& out, auto& c) { 
                out << YAML::BeginMap;
                out << YAML::Key << "Type" << YAML::Value << (int)c.Type; 
                ReflectiveSerialize<ColliderComponent>(out, c); 
                out << YAML::EndMap;
            },
            [](auto& c, auto node) { 
                if (node["Type"]) c.Type = (ColliderType)node["Type"].template as<int>(); 
                ReflectiveDeserialize<ColliderComponent>(c, node); 
            }
        );

        RegisterReflective<PointLightComponent>("PointLightComponent");
        RegisterReflective<SpotLightComponent>("SpotLightComponent");
        RegisterReflective<RigidBodyComponent>("RigidBodyComponent");
        RegisterReflective<PlayerComponent>("PlayerComponent");
        RegisterReflective<SceneTransitionComponent>("SceneTransitionComponent");
        RegisterReflective<BillboardComponent>("BillboardComponent");
        RegisterReflective<NavigationComponent>("NavigationComponent");
        RegisterReflective<ShaderComponent>("ShaderComponent");
        RegisterReflective<AnimationComponent>("AnimationComponent");
        RegisterReflective<AudioComponent>("AudioComponent");
        RegisterReflective<CameraComponent>("CameraComponent");
        
        // UI Components
        RegisterReflective<ControlComponent>("ControlComponent");
        RegisterReflective<LabelControl>("LabelControl");
        RegisterReflective<ButtonControl>("ButtonControl");
    }

    void ComponentSerializer::SerializeAll(YAML::Emitter& out, Entity entity)
    {
        for (auto& entry : s_Registry)
        {
            entry.Serialize(out, entity);
        }
        
        // Special complex components or those needing specific nesting
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
        
        // Special complex components  
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

    void ComponentSerializer::SerializeHierarchy(YAML::Emitter& out, Entity entity)
    {
        if (entity.HasComponent<HierarchyComponent>())
        {
            out << YAML::Key << "HierarchyComponent";
            auto &hc = entity.GetComponent<HierarchyComponent>();
            out << YAML::BeginMap;
            
            uint64_t parentUUID = 0;
            if (hc.Parent != ::entt::null)
            {
                Entity parentEntity(hc.Parent, entity.GetScene());
                if (parentEntity && parentEntity.HasComponent<IDComponent>())
                    parentUUID = (uint64_t)parentEntity.GetComponent<IDComponent>().ID;
            }
            out << YAML::Key << "Parent" << YAML::Value << parentUUID;
            
            out << YAML::Key << "Children" << YAML::BeginSeq;
            for (auto childHandle : hc.Children)
            {
                if (childHandle != ::entt::null)
                {
                    Entity childEntity(childHandle, entity.GetScene());
                    if (childEntity && childEntity.HasComponent<IDComponent>())
                        out << (uint64_t)childEntity.GetComponent<IDComponent>().ID;
                }
            }
            out << YAML::EndSeq;
            out << YAML::EndMap;
        }
    }

    void ComponentSerializer::DeserializeHierarchyTask(Entity entity, YAML::Node node, HierarchyTask& outTask)
    {
        outTask.entity = entity;
        outTask.parent = 0;
        outTask.children.clear();
        
        auto hc = node["HierarchyComponent"];
        if (hc)
        {
            if (hc["Parent"]) outTask.parent = hc["Parent"].as<uint64_t>();
            auto children = hc["Children"];
            if (children && children.IsSequence())
            {
                for (auto child : children)
                    outTask.children.push_back(child.as<uint64_t>());
            }
        }
    }

} // namespace CHEngine
