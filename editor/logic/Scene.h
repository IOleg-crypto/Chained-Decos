#ifndef SCENE_H
#define SCENE_H

#include <entt/entt.hpp>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace ChainedEngine
{

class Scene
{
public:
    Scene();
    ~Scene();

    const std::string &GetName() const
    {
        return m_name;
    }
    void SetName(const std::string &name)
    {
        m_name = name;
    }

    entt::entity CreateEntity(const std::string &name = "Entity");
    void DestroyEntity(entt::entity entity);
    void SetParent(entt::entity entity, entt::entity parent);

    // Serialization
    nlohmann::json ToJson() const;
    void FromJson(const nlohmann::json &j);

    // Get root entities
    std::vector<entt::entity> GetRootEntities() const;

private:
    nlohmann::json SerializeEntity(entt::entity entity) const;
    entt::entity DeserializeEntity(const nlohmann::json &j, entt::entity parent = entt::null);

private:
    std::string m_name;
};

} // namespace ChainedEngine

#endif // SCENE_H
