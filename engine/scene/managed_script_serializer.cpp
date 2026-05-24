#include "managed_script_serializer.h"
#include "components/scripting_components.h"
#include "engine/scene/component_registry.h"
#include "engine/scene/scripting_serialization.h"

namespace CHEngine
{

void RegisterManagedScriptComponentMetadata()
{
    ComponentMetadata metadata;
    metadata.Name = "Managed Script";
    metadata.SerializationKey = "ManagedScriptComponent";
    metadata.Category = "Scripting";
    metadata.Serialize = [](YAML::Emitter& out, Entity entity) {
        if (!entity.HasComponent<ManagedScriptComponent>())
        {
            return;
        }

        auto& comp = entity.GetComponent<ManagedScriptComponent>();
        out << YAML::Key << "ManagedScriptComponent" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "Scripts" << YAML::Value << YAML::BeginSeq;
        for (const auto& s : comp.Scripts)
        {
            out << s;
        }
        out << YAML::EndSeq;
        out << YAML::EndMap;
    };
    metadata.Deserialize = [](Entity entity, YAML::Node node) {
        if (!node["ManagedScriptComponent"])
        {
            return;
        }

        if (!entity.HasComponent<ManagedScriptComponent>())
        {
            entity.AddComponent<ManagedScriptComponent>();
        }

        auto& comp = entity.GetComponent<ManagedScriptComponent>();
        comp.Scripts.clear();
        auto scriptsNode = node["ManagedScriptComponent"]["Scripts"];
        if (scriptsNode && scriptsNode.IsSequence())
        {
            for (auto s : scriptsNode)
            {
                CHEngine::ManagedScriptInstance inst;
                if (s.IsScalar())
                {
                    inst.ClassName = s.as<std::string>();
                }
                else
                {
                    YAML::convert<CHEngine::ManagedScriptInstance>::decode(s, inst);
                }
                comp.Scripts.push_back(std::move(inst));
            }
        }
    };
    metadata.Copy = [](Entity src, Entity dst) {
        if (src.HasComponent<ManagedScriptComponent>())
        {
            dst.AddOrReplaceComponent<ManagedScriptComponent>(src.GetComponent<ManagedScriptComponent>().ClonePersistent());
        }
    };
    metadata.Has = [](Entity e) { return e.HasComponent<ManagedScriptComponent>(); };
    metadata.GetAll = [](class Scene* s) {
        std::vector<uint64_t> ids;
        for (auto ent : s->GetRegistry().view<ManagedScriptComponent>())
        {
            ids.push_back((uint64_t)(uint32_t)ent);
        }
        return ids;
    };
    metadata.Add = [](Entity e) {
        if (!e.HasComponent<ManagedScriptComponent>())
        {
            e.AddComponent<ManagedScriptComponent>();
        }
    };
    metadata.Remove = [](Entity e) {
        if (e.HasComponent<ManagedScriptComponent>())
        {
            e.RemoveComponent<ManagedScriptComponent>();
        }
    };

    ComponentRegistry::Register(entt::type_hash<ManagedScriptComponent>::value(), metadata);
}

} // namespace CHEngine