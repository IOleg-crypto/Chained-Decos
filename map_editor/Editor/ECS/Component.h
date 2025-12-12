#pragma once

#include <memory>
#include <nlohmann/json.hpp>
#include <string>


class GameObject;

class Component
{
public:
    Component(GameObject *owner);
    virtual ~Component() = default;

    virtual void OnStart()
    {
    }
    virtual void OnUpdate(float deltaTime)
    {
    }
    virtual void OnRender()
    {
    }
    virtual void OnInspectorGUI()
    {
    } // Draw properties in the Inspector

    // Serialization
    virtual void Serialize(nlohmann::json &json) const = 0;
    virtual void Deserialize(const nlohmann::json &json) = 0;

    void SetEnabled(bool enabled);
    bool IsEnabled() const;

protected:
    GameObject *m_owner;
    bool m_enabled;
};
